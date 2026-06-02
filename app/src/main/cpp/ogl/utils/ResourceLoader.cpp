#include "../include/ResourceLoader.h"
#include <GLES3/gl3.h>
#include "../include/Common.h"
#include <stb_image.h>
#include <math.h>
#include <string>

void HDRImage::clear() {
    if (data != nullptr) {
        stbi_image_free(data);
        data = nullptr;
    }
    width = 0;
    height = 0;
    channels = 0;
}

void TextureData::clear() {
    if (data != nullptr) {
        stbi_image_free(data);
        data = nullptr;
    }
    if (data16 != nullptr) {
        stbi_image_free(data16);
        data16 = nullptr;
    }
    width = 0;
    height = 0;
    channels = 0;
    is16bit = false;
}

void BRDFLUTData::clear() {
    if (data != nullptr) {
        delete[] data;
        data = nullptr;
    }
    width = 0;
    height = 0;
}

ResourceLoader::ResourceLoader() {
}

ResourceLoader::~ResourceLoader() {
}

void ResourceLoader::setFilesDir(const char* dir) {
    filesDir = dir ? dir : "";
}

HDRImage ResourceLoader::loadHDR(const char* relativePath) {
    HDRImage img;
    if (filesDir.empty()) {
        LOGE("filesDir not set");
        return img;
    }
    std::string fullPath = filesDir + "/" + relativePath;
    img.data = stbi_loadf(fullPath.c_str(), &img.width, &img.height, &img.channels, 4);
    if (!img.data) {
        LOGE("stbi_loadf failed: %s", fullPath.c_str());
        return img;
    }
    img.channels = 4;
    LOGI("HDR loaded: %s %dx%d ch=%d", relativePath, img.width, img.height, img.channels);
    return img;
}

GLuint ResourceLoader::loadTexture1(const char* path) {
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        LOGI("loadTexture: tex=%d path=%s %dx%d ch=%d", textureID, path, width, height, nrComponents);
        stbi_image_free(data);
    } else {
        LOGE("Texture failed to load: %s", path);
        stbi_image_free(data);
    }
    return textureID;
}

TextureData ResourceLoader::loadTexture(const char* relativePath, bool flipUv) {
    TextureData tex;
    if (filesDir.empty()) {
        LOGE("loadTexture: filesDir not set");
        return tex;
    }
    std::string fullPath = filesDir + "/" + relativePath;

    stbi_set_flip_vertically_on_load(flipUv);

    if (stbi_is_16_bit(fullPath.c_str())) {
        tex.data16 = stbi_load_16(fullPath.c_str(), &tex.width, &tex.height, &tex.channels, 0);
        if (!tex.data16) {
            LOGE("stbi_load_16 failed: %s", fullPath.c_str());
            return tex;
        }
        tex.is16bit = true;
        LOGI("Texture loaded (16-bit): %s %dx%d ch=%d", relativePath, tex.width, tex.height, tex.channels);
    } else {
        tex.data = stbi_load(fullPath.c_str(), &tex.width, &tex.height, &tex.channels, 0);
        if (!tex.data) {
            LOGE("stbi_load failed: %s", fullPath.c_str());
            return tex;
        }
        tex.is16bit = false;
        LOGI("Texture loaded (8-bit): %s %dx%d ch=%d", relativePath, tex.width, tex.height, tex.channels);
    }
    return tex;
}

BRDFLUTData ResourceLoader::generateBRDFLUT(int width, int height) {
    BRDFLUTData lut;
    if (width <= 0 || height <= 0) {
        return lut;
    }
    lut.width = width;
    lut.height = height;
    lut.data = new float[width * height * 2];

    const float PI = 3.14159265359f;
    const int SAMPLE_COUNT = 256;

    auto radicalInverse_VdC = [&](uint32_t bits) -> float {
        bits = (bits << 16u) | (bits >> 16u);
        bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
        bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
        bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
        bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
        return (float)(bits) * 2.3283064365386963e-10f;
    };

    for (int y = 0; y < height; y++) {
        float roughness = (float(y) + 0.5f) / float(height);
        float a = roughness * roughness;

        for (int x = 0; x < width; x++) {
            float NdotV = (float(x) + 0.5f) / float(width);
            float Vx = sqrtf(1.0f - NdotV * NdotV);
            float Vz = NdotV;

            float A = 0.0f;
            float B = 0.0f;

            for (int i = 0; i < SAMPLE_COUNT; i++) {
                float Xi0 = (float)i / (float)SAMPLE_COUNT;
                float Xi1 = radicalInverse_VdC(i);
                float phi = 2.0f * PI * Xi0;
                float cosTheta = sqrtf((1.0f - Xi1) / (1.0f + (a * a - 1.0f) * Xi1));
                float sinTheta = sqrtf(1.0f - cosTheta * cosTheta);

                float Hx = cosf(phi) * sinTheta;
                float Hy = sinf(phi) * sinTheta;
                float Hz = cosTheta;

                float VdotH = Vx * Hx + Vz * Hz;
                float NdotH = Hz;
                float NdotL = fmaxf(Vx * Hx + Vz * Hz, 0.0f);

                if (NdotL <= 0.0f) continue;

                float G = NdotV * NdotL / (VdotH * NdotH + 1e-5f);
                float Fc = powf(1.0f - VdotH, 5.0f);

                A += (1.0f - Fc) * G * VdotH / (NdotH * NdotV + 1e-5f);
                B += Fc * G * VdotH / (NdotH * NdotV + 1e-5f);
            }

            A /= (float)SAMPLE_COUNT;
            B /= (float)SAMPLE_COUNT;

            int idx = (y * width + x) * 2;
            lut.data[idx + 0] = A;
            lut.data[idx + 1] = B;
        }
    }
    LOGI("BRDF LUT generated: %dx%d", width, height);
    return lut;
}

void ResourceLoader::freeHDR(HDRImage& img) {
    img.clear();
}

void ResourceLoader::freeTexture(TextureData& tex) {
    tex.clear();
}

void ResourceLoader::freeBRDFLUT(BRDFLUTData& lut) {
    lut.clear();
}
