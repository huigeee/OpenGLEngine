#include "../include/IBLPipeline.h"
#include "../include/Common.h"
#include <math.h>
#include <string.h>

IBLPipeline::IBLPipeline(ResourceLoader* loader) : resourceLoader(loader),
    envTexture(0), prefilteredLevels(0), brdfLUTTexture(0), ready(false) {
    for (int i = 0; i < 9; i++) {
        shCoeffs[i][0] = shCoeffs[i][1] = shCoeffs[i][2] = 0.0f;
    }
    for (int i = 0; i < 8; i++) {
        prefilteredTextures[i] = 0;
    }
}

IBLPipeline::~IBLPipeline() {
    release();
}

uint32_t IBLPipeline::radicalInverse_VdC(uint32_t bits) const {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return bits;
}

IBLPipeline::V2 IBLPipeline::hammersley(uint32_t i, uint32_t N) const {
    return { (float)i / (float)N, (float)radicalInverse_VdC(i) * 2.3283064365386963e-10f };
}

IBLPipeline::V3 IBLPipeline::importanceSampleGGX(V2 Xi, V3 N, float roughness) const {
    float a = roughness * roughness;
    float phi = 2.0f * 3.14159265359f * Xi.x;
    float cosTheta = sqrtf((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
    float sinTheta = sqrtf(1.0f - cosTheta * cosTheta);
    V3 H;
    H.x = cosf(phi) * sinTheta;
    H.y = sinf(phi) * sinTheta;
    H.z = cosTheta;
    V3 up = fabsf(N.z) < 0.999f ? V3{0.0f, 0.0f, 1.0f} : V3{1.0f, 0.0f, 0.0f};
    V3 tangent;
    float tdotn = up.x * N.x + up.y * N.y + up.z * N.z;
    float len2 = up.x * up.x + up.y * up.y + N.z * N.z - tdotn * tdotn;
    float len = len2 > 0.0f ? sqrtf(len2) : 1e-6f;
    tangent.x = (up.x - N.x * tdotn) / len;
    tangent.y = (up.y - N.y * tdotn) / len;
    tangent.z = (up.z - N.z * tdotn) / len;
    V3 bitangent;
    bitangent.x = N.y * tangent.z - N.z * tangent.y;
    bitangent.y = N.z * tangent.x - N.x * tangent.z;
    bitangent.z = N.x * tangent.y - N.y * tangent.x;
    V3 sampleVec;
    sampleVec.x = tangent.x * H.x + bitangent.x * H.y + N.x * H.z;
    sampleVec.y = tangent.y * H.x + bitangent.y * H.y + N.y * H.z;
    sampleVec.z = tangent.z * H.x + bitangent.z * H.y + N.z * H.z;
    float len3 = sampleVec.x * sampleVec.x + sampleVec.y * sampleVec.y + sampleVec.z * sampleVec.z;
    float invLen = len3 > 0.0f ? 1.0f / sqrtf(len3) : 1.0f;
    return { sampleVec.x * invLen, sampleVec.y * invLen, sampleVec.z * invLen };
}

static int getSharedExponent(float r, float g, float b) {
    float maxRGB = fmaxf(fmaxf(fabsf(r), fabsf(g)), fabsf(b));
    if (maxRGB <= 0.0f) return 0;
    int exp = (int)floorf(log2f(maxRGB));
    if (exp < -15) exp = -15;
    if (exp > 15) exp = 15;
    return exp;
}

static uint32_t rgb9e5Encode(float r, float g, float b, int exp) {
    float div = exp2f((float)exp);
    uint32_t R = (uint32_t)fminf(r / div * 511.0f, 511.0f);
    uint32_t G = (uint32_t)fminf(g / div * 511.0f, 511.0f);
    uint32_t B = (uint32_t)fminf(b / div * 511.0f, 511.0f);
    uint32_t e = (uint32_t)(exp + 15);
    return (e << 27) | (B << 18) | (G << 9) | R;
}

void IBLPipeline::computeSH(float* pixels, int w, int h, float sh[9][3]) {
    const int SAMPLES = 128;
    float pi = 3.14159265359f;

    for (int c = 0; c < 3; c++) {
        float s[9] = {0};
        for (int i = 0; i < SAMPLES; i++) {
            float u1 = ((float)rand() / RAND_MAX);
            float u2 = ((float)rand() / RAND_MAX);
            float phi = 2.0f * pi * u1;
            float theta = acosf(1.0f - 2.0f * u2);
            float sinT = sinf(theta);

            float sx = sinT * cosf(phi);
            float sy = sinT * sinf(phi);
            float sz = cosf(theta);

            int x = (int)((phi / (2.0f * pi)) * w) % w;
            int y = (int)((theta / pi) * h) % h;
            if (x < 0) x += w; if (y < 0) y += h;
            if (x >= w) x = w - 1; if (y >= h) y = h - 1;

            float* p = pixels + (y * w + x) * 4;
            float env[3] = {p[0], p[1], p[2]};

            float cosTheta = sz;
            s[0] += env[c] * 0.5f/0.282095f * cosTheta;
            s[1] += env[c] * 0.94617469556f/0.488603f * cosTheta * sy;
            s[2] += env[c] * 0.94617469556f/0.488603f * cosTheta * sz;
            s[3] += env[c] * 0.94617469556f/0.488603f * cosTheta * sx;
            s[4] += env[c] * 0.5462742153f/1.092548f * cosTheta * sx * sy;
            s[5] += env[c] * 0.5462742153f/1.092548f * cosTheta * sy * sz;
            s[6] += env[c] * 0.5462742153f/1.092548f * cosTheta * (3.0f * sz * sz - 1.0f);
            s[7] += env[c] * 0.5462742153f/1.092548f * cosTheta * sx * sz;
            s[8] += env[c] * 0.891307f/0.315392f * cosTheta * (sx*sx - sy*sy);
        }

        s[0] *= 1.0f;
        s[1] *= 0.333333f;
        s[2] *= 0.333333f;
        s[3] *= 0.333333f;
        s[4] *= 0.2f;
        s[5] *= 0.2f;
        s[6] *= 0.2f;
        s[7] *= 0.2f;
        s[8] *= 0.2f;

        float norm = 4.0f * pi / SAMPLES;
        for (int j = 0; j < 9; j++) {
            sh[j][c] = s[j] * norm;
        }
    }
}

void IBLPipeline::prefilterEnv(float* pixels, int w, int h) {
    float pi = 3.14159265359f;
    const float MAX_REFLECTION_LOD = 4.0f;

    for (int level = 0; level < prefilteredLevels; level++) {
        int mipW = w >> level;
        int mipH = h >> level;
        if (mipW < 4) mipW = 4;
        if (mipH < 4) mipH = 4;

        float roughness = (float)level / (float)(prefilteredLevels - 1);
        roughness = roughness * roughness;
        LOGI("Prefilter level %d: %dx%d roughness=%.3f", level, mipW, mipH, roughness);

        float* buf = new float[mipW * mipH * 3];
        const uint32_t SAMPLE_COUNT = 256u;

        for (int py = 0; py < mipH; py++) {
            if (py % 32 == 0) LOGI("  row %d/%d", py, mipH);
            for (int px = 0; px < mipW; px++) {
                float u = (px + 0.5f) / mipW;
                float v = (py + 0.5f) / mipH;
                float phi = u * 2.0f * pi;
                float theta = v * pi;

                float sx = sinf(theta) * cosf(phi);
                float sy = sinf(theta) * sinf(phi);
                float sz = cosf(theta);

                V3 N = {sx, sy, sz};
                V3 R = N;
                V3 V = R;

                float r = 0.0f, g = 0.0f, b = 0.0f;
                float totalWeight = 0.0f;

                for (uint32_t i = 0; i < SAMPLE_COUNT; i++) {
                    V2 Xi = hammersley(i, SAMPLE_COUNT);
                    V3 H = importanceSampleGGX(Xi, N, roughness);
                    float dotVH = V.x * H.x + V.y * H.y + V.z * H.z;
                    V3 L;
                    L.x = 2.0f * dotVH * H.x - V.x;
                    L.y = 2.0f * dotVH * H.y - V.y;
                    L.z = 2.0f * dotVH * H.z - V.z;

                    float NdotL = fmaxf(L.z, 0.0f);
                    if (NdotL > 0.0f) {
                        int ex = (int)(((atan2f(L.y, L.x) / (2.0f * pi)) + 1.0f) * 0.5f * w) % w;
                        int ey = (int)((acosf(fmaxf(L.z, -1.0f)) / pi) * h) % h;
                        if (ex < 0) ex += w;
                        if (ey < 0) ey += h;
                        if (ex >= w) ex = w - 1;
                        if (ey >= h) ey = h - 1;

                        float* p = pixels + (ey * w + ex) * 4;
                        r += p[0] * NdotL;
                        g += p[1] * NdotL;
                        b += p[2] * NdotL;
                        totalWeight += NdotL;
                    }
                }

                if (totalWeight > 0.0f) {
                    r /= totalWeight;
                    g /= totalWeight;
                    b /= totalWeight;
                }
                buf[(py * mipW + px) * 3 + 0] = r;
                buf[(py * mipW + px) * 3 + 1] = g;
                buf[(py * mipW + px) * 3 + 2] = b;
            }
        }

        glBindTexture(GL_TEXTURE_2D, prefilteredTextures[level]);
        unsigned char* ubuf = new unsigned char[mipW * mipH * 3];
        float globalMax = 0.001f;
        for (int i = 0; i < mipW * mipH; i++) {
            float maxV = fmaxf(fmaxf(buf[i*3], buf[i*3+1]), buf[i*3+2]);
            if (maxV > globalMax) globalMax = maxV;
        }
        float s = (globalMax > 0.0f) ? (1.0f / globalMax) : 1.0f;
        for (int i = 0; i < mipW * mipH; i++) {
            ubuf[i*3+0] = (unsigned char)(fminf(buf[i*3] * s * 255.0f, 255.0f));
            ubuf[i*3+1] = (unsigned char)(fminf(buf[i*3+1] * s * 255.0f, 255.0f));
            ubuf[i*3+2] = (unsigned char)(fminf(buf[i*3+2] * s * 255.0f, 255.0f));
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mipW, mipH, 0, GL_RGB, GL_UNSIGNED_BYTE, ubuf);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        delete[] ubuf;
        delete[] buf;
    }
}

bool IBLPipeline::init(const char* hdrPath, int levels, int maxTexSize) {
    if (resourceLoader == nullptr) {
        LOGE("ResourceLoader not set");
        return false;
    }

    prefilteredLevels = levels;
    if (prefilteredLevels > 8) prefilteredLevels = 8;

    HDRImage hdr = resourceLoader->loadHDR(hdrPath);
    if (!hdr.isValid()) {
        LOGE("Failed to load HDR: %s", hdrPath);
        return false;
    }

    computeSH(hdr.data, hdr.width, hdr.height, shCoeffs);
    LOGI("SH computed: L00=(%.3f,%.3f,%.3f)", shCoeffs[0][0], shCoeffs[0][1], shCoeffs[0][2]);

    int texW = hdr.width > maxTexSize ? maxTexSize : hdr.width;
    int texH = hdr.height > maxTexSize ? maxTexSize : hdr.height;

    glGenTextures(1, &envTexture);
    glGenTextures(prefilteredLevels, prefilteredTextures);

    glBindTexture(GL_TEXTURE_2D, envTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    unsigned char* ubuf = new unsigned char[texW * texH * 3];
    float globalMax = 0.001f;
    for (int i = 0; i < texW * texH; i++) {
        float maxV = fmaxf(fmaxf(hdr.data[i*4], hdr.data[i*4+1]), hdr.data[i*4+2]);
        if (maxV > globalMax) globalMax = maxV;
    }
    float sc = (globalMax > 0.0f) ? (1.0f / globalMax) : 1.0f;
    for (int i = 0; i < texW * texH; i++) {
        ubuf[i*3+0] = (unsigned char)(fminf(hdr.data[i*4] * sc * 255.0f, 255.0f));
        ubuf[i*3+1] = (unsigned char)(fminf(hdr.data[i*4+1] * sc * 255.0f, 255.0f));
        ubuf[i*3+2] = (unsigned char)(fminf(hdr.data[i*4+2] * sc * 255.0f, 255.0f));
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texW, texH, 0, GL_RGB, GL_UNSIGNED_BYTE, ubuf);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    delete[] ubuf;

    prefilterEnv(hdr.data, texW, texH);

    TextureData brdfLUTTex = resourceLoader->loadTexture("texture/ibl_brdf_lut.png");
    if (!brdfLUTTex.isValid()) {
        LOGE("BRDF LUT load failed, trying without extension");
        brdfLUTTex = resourceLoader->loadTexture("texture/ibl_brdf_lut");
        if (!brdfLUTTex.isValid()) {
            LOGE("Failed to load BRDF LUT");
            resourceLoader->freeHDR(hdr);
            return false;
        }
    }

    glGenTextures(1, &brdfLUTTexture);
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GLenum fmt = brdfLUTTex.channels == 2 ? GL_RG : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, brdfLUTTex.width, brdfLUTTex.height, 0, fmt, GL_UNSIGNED_BYTE, brdfLUTTex.data);
    resourceLoader->freeTexture(brdfLUTTex);

    resourceLoader->freeHDR(hdr);
    ready = true;
    LOGI("IBLPipeline ready: env=%d, prefilter=%d, brdfLUT=%d", envTexture, prefilteredLevels, brdfLUTTexture);
    return true;
}

void IBLPipeline::release() {
    if (envTexture != 0) { glDeleteTextures(1, &envTexture); envTexture = 0; }
    if (prefilteredLevels > 0) { glDeleteTextures(prefilteredLevels, prefilteredTextures); prefilteredLevels = 0; }
    if (brdfLUTTexture != 0) { glDeleteTextures(1, &brdfLUTTexture); brdfLUTTexture = 0; }
    ready = false;
}
