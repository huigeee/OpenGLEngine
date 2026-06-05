#include "../include/IBLPipeline2.h"
#include "../include/Common.h"
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

static const char* SHADER_CUBEMAP_VS = R"(
    #version 300 es
    layout(location = 0) in vec3 aPos;
    out vec3 TexCoord;
    uniform mat4 projection;
    uniform mat4 view;
    void main() {
        gl_Position = projection * view * vec4(aPos, 1.0);
        TexCoord = aPos;
    }
)";

static const char* SHADER_EQUI_TO_CUBEMAP_FS = R"(
    #version 300 es
    precision highp float;
    in vec3 TexCoord;
    out vec4 FragColor;
    uniform sampler2D texture_background;
    const vec2 invAtan = vec2(0.1591, 0.3183);
    vec2 SampleSphericalMap(vec3 v) {
        vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
        uv *= invAtan;
        uv += 0.5;
        return uv;
    }
    void main() {
        vec2 uv = SampleSphericalMap(normalize(TexCoord));
        vec3 color = texture(texture_background, uv).rgb;
        FragColor = vec4(color, 1.0);
    }
)";

static const char* SHADER_IRRADIANCE_FS = R"(
    #version 300 es
    precision highp float;
    in vec3 TexCoord;
    out vec4 FragColor;
    uniform samplerCube texture_background;
    const float PI = 3.14159265359;
    void main() {
        vec3 N = normalize(TexCoord);
        vec3 irradiance = vec3(0.0);
        vec3 up = vec3(0.0, 1.0, 0.0);
        vec3 right = normalize(cross(up, N));
        up = normalize(cross(N, right));
        float sampleDelta = 0.025;
        float nrSamples = 0.0;
        for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
            for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
                vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
                vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;
                irradiance += texture(texture_background, sampleVec).rgb * cos(theta) * sin(theta);
                nrSamples++;
            }
        }
        irradiance = PI * irradiance * (1.0 / float(nrSamples));
        FragColor = vec4(irradiance, 1.0);
    }
)";

static const char* SHADER_PREFILTER_VS = R"(
    #version 300 es
    layout(location = 0) in vec3 aPos;
    out vec3 WorldPos;
    uniform mat4 projection;
    uniform mat4 view;
    void main() {
        gl_Position = projection * view * vec4(aPos, 1.0);
        WorldPos = aPos;
    }
)";

static const char* SHADER_PREFILTER_FS = R"(
    #version 300 es
    precision highp float;
    precision highp int;
    out vec4 FragColor;
    in vec3 WorldPos;
    uniform samplerCube environmentMap;
    uniform float roughness;
    const float PI = 3.14159265359;

    float DistributionGGX(vec3 N, vec3 H, float roughness) {
        float a = roughness * roughness;
        float a2 = a * a;
        float NdotH = max(dot(N, H), 0.0);
        float NdotH2 = NdotH * NdotH;
        float nom = a2;
        float denom = (NdotH2 * (a2 - 1.0) + 1.0);
        denom = PI * denom * denom;
        return nom / denom;
    }

    float RadicalInverse_VdC(uint bits) {
        bits = (bits << 16u) | (bits >> 16u);
        bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
        bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
        bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
        bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
        return float(bits) * 2.3283064365386963e-10;
    }

    vec2 Hammersley(uint i, uint N) {
        return vec2(float(i) / float(N), RadicalInverse_VdC(i));
    }

    vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
        float a = roughness * roughness;
        float phi = 2.0 * PI * Xi.x;
        float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
        float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
        vec3 H;
        H.x = cos(phi) * sinTheta;
        H.y = sin(phi) * sinTheta;
        H.z = cosTheta;
        vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
        vec3 tangent = normalize(cross(up, N));
        vec3 bitangent = cross(N, tangent);
        vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
        return normalize(sampleVec);
    }

    void main() {
        vec3 N = normalize(WorldPos);
        vec3 R = N;
        vec3 V = R;
        const uint SAMPLE_COUNT = 1024u;
        vec3 prefilteredColor = vec3(0.0);
        float totalWeight = 0.0;
        for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
            vec2 Xi = Hammersley(i, SAMPLE_COUNT);
            vec3 H = ImportanceSampleGGX(Xi, N, roughness);
            vec3 L = normalize(2.0 * dot(V, H) * H - V);
            float NdotL = max(dot(N, L), 0.0);
            if (NdotL > 0.0) {
                float D = DistributionGGX(N, H, roughness);
                float NdotH = max(dot(N, H), 0.0);
                float HdotV = max(dot(H, V), 0.0);
                float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;
                float resolution = 512.0;
                float saTexel = 4.0 * PI / (6.0 * resolution * resolution);
                float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);
                float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);
                prefilteredColor += textureLod(environmentMap, L, mipLevel).rgb * NdotL;
                totalWeight += NdotL;
            }
        }
        prefilteredColor = prefilteredColor / totalWeight;
        FragColor = vec4(prefilteredColor, 1.0);
    }
)";

static const char* SHADER_BRDF_VS = R"(
    #version 300 es
    precision highp float;
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec2 aTexCoords;
    out vec2 TexCoords;
    void main() {
        TexCoords = aTexCoords;
        gl_Position = vec4(aPos, 1.0);
    }
)";

static const char* SHADER_BRDF_FS = R"(
    #version 300 es
    precision highp float;
    out vec4 FragColor;
    in vec2 TexCoords;
    const float PI = 3.14159265359;

    float RadicalInverse_VdC(uint bits) {
        bits = (bits << 16u) | (bits >> 16u);
        bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
        bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
        bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
        bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
        return float(bits) * 2.3283064365386963e-10;
    }

    vec2 Hammersley(uint i, uint N) {
        return vec2(float(i) / float(N), RadicalInverse_VdC(i));
    }

    vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
        float a = roughness * roughness;
        float phi = 2.0 * PI * Xi.x;
        float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
        float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
        vec3 H;
        H.x = cos(phi) * sinTheta;
        H.y = sin(phi) * sinTheta;
        H.z = cosTheta;
        vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
        vec3 tangent = normalize(cross(up, N));
        vec3 bitangent = cross(N, tangent);
        vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
        return normalize(sampleVec);
    }

    float GeometrySchlickGGX(float NdotV, float roughness) {
        float a = roughness;
        float k = (a * a) / 2.0;
        float nom = NdotV;
        float denom = NdotV * (1.0 - k) + k;
        return nom / denom;
    }

    float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
        float NdotV = max(dot(N, V), 0.0);
        float NdotL = max(dot(N, L), 0.0);
        float ggx2 = GeometrySchlickGGX(NdotV, roughness);
        float ggx1 = GeometrySchlickGGX(NdotL, roughness);
        return ggx1 * ggx2;
    }

    vec2 IntegrateBRDF(float NdotV, float roughness) {
        vec3 V;
        V.x = sqrt(1.0 - NdotV * NdotV);
        V.y = 0.0;
        V.z = NdotV;
        vec3 N = vec3(0.0, 0.0, 1.0);
        float A = 0.0;
        float B = 0.0;
        const uint SAMPLE_COUNT = 1024u;
        for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
            vec2 Xi = Hammersley(i, SAMPLE_COUNT);
            vec3 H = ImportanceSampleGGX(Xi, N, roughness);
            vec3 L = normalize(2.0 * dot(V, H) * H - V);
            float NdotL = max(L.z, 0.0);
            float NdotH = max(H.z, 0.0);
            float VdotH = max(dot(V, H), 0.0);
            if (NdotL > 0.0) {
                float G = GeometrySmith(N, V, L, roughness);
                float G_Vis = (G * VdotH) / (NdotH * NdotV);
                float Fc = pow(1.0 - VdotH, 5.0);
                A += (1.0 - Fc) * G_Vis;
                B += Fc * G_Vis;
            }
        }
        A /= float(SAMPLE_COUNT);
        B /= float(SAMPLE_COUNT);
        return vec2(A, B);
    }

    void main() {
        vec2 integratedBRDF = IntegrateBRDF(TexCoords.x, TexCoords.y);
        FragColor = vec4(integratedBRDF, 0.0, 1.0);
    }
)";

static const char* SHADER_BACKGROUND_VS = R"(
    #version 300 es
    layout(location = 0) in vec3 aPos;
    out vec3 TexCoord;
    uniform mat4 projection;
    uniform mat4 view;
    uniform mat4 model;
    void main() {
        gl_Position = (projection * mat4(mat3(view)) * vec4(aPos, 1.0)).xyww;
        TexCoord = aPos;
    }
)";

static const char* SHADER_BACKGROUND_FS = R"(
    #version 300 es
    precision highp float;
    in vec3 TexCoord;
    out vec4 FragColor;
    uniform samplerCube texture_background;
    void main() {
        vec3 envColor = textureLod(texture_background, TexCoord, 0.0).rgb;
        envColor = envColor / (envColor + vec3(1.0));
        envColor = pow(envColor, vec3(1.0 / 2.2));
        FragColor = vec4(1.0,0.0,0.0, 1.0);
    }
)";

void IBLPipeline2::rebuildBackgroundShader() {
    if (backgroundProgram) glDeleteProgram(backgroundProgram);
    backgroundProgram = createProgram(SHADER_BACKGROUND_VS, SHADER_BACKGROUND_FS);
    LOGI("rebuildBackgroundShader: program=%d", backgroundProgram);
}

bool IBLPipeline2::init(ResourceLoader* loader, const char* path) {
    this->loader = loader;
    hdrPath = path;
    if (isCreated) release();

    equiToCubemapProgram = createProgram(SHADER_CUBEMAP_VS, SHADER_EQUI_TO_CUBEMAP_FS);
    irradianceProgram = createProgram(SHADER_CUBEMAP_VS, SHADER_IRRADIANCE_FS);
    prefilterProgram = createProgram(SHADER_PREFILTER_VS, SHADER_PREFILTER_FS);
    brdfProgram = createProgram(SHADER_BRDF_VS, SHADER_BRDF_FS);
    backgroundProgram = createProgram(SHADER_BACKGROUND_VS, SHADER_BACKGROUND_FS);

    if (!equiToCubemapProgram) { LOGE("equiToCubemapProgram failed"); }
    if (!irradianceProgram) { LOGE("irradianceProgram failed"); }
    if (!prefilterProgram) { LOGE("prefilterProgram failed"); }
    if (!brdfProgram) { LOGE("brdfProgram failed"); }
    if (!backgroundProgram) { LOGE("backgroundProgram failed"); }
    if (!equiToCubemapProgram || !irradianceProgram || !prefilterProgram || !brdfProgram || !backgroundProgram) {
        LOGE("Failed to create shader programs");
        return false;
    }

    glGenVertexArrays(1, &VAO_Cubemap);
    glGenBuffers(1, &VBO_Cubemap);
    glBindVertexArray(VAO_Cubemap);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Cubemap);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVertices), kCubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenVertexArrays(1, &VAO_Quad);
    glGenBuffers(1, &VBO_Quad);
    glBindVertexArray(VAO_Quad);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Quad);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuadVertices), kQuadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    int w, h, ch;
    hdrTexture = loadHDR(path, w, h, ch);
    if (!hdrTexture) {
        LOGE("Failed to load HDR: %s", path);
        return false;
    }
    LOGI("HDR loaded: %dx%d ch=%d", w, h, ch);

    captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    if (!generateEnvCubemap()) {
        LOGE("Failed to generate env cubemap");
        return false;
    }

    generateIrradianceMap();
    generatePrefilterMap();
    generateBRDFLUTTexture();

    isCreated = true;
    LOGI("IBLPipeline2 ready: envCubemap=%d, irradiance=%d, prefilter=%d, brdfLUT=%d",
         envCubemap, irradianceMap, prefilterMap, brdfLUTTexture);
    return true;
}
