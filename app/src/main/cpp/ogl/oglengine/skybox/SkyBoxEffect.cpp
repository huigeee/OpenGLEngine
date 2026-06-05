/**
 * Copyright @ 2020 - 2027 iAuto Software(Shanghai) Co., Ltd.
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are NOT permitted except as agreed by
 * iAuto Software(Shanghai) Co., Ltd.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */

#include "../include/SkyBoxEffect.h"
#include "png.h"
#include "../stb_image/stb_image.h"
#include "../include/Common.h"

static SkyBoxEffect* instance;

SkyBoxEffect *SkyBoxEffect::getInstance() {
    if (instance == nullptr) {
        instance = new SkyBoxEffect();
    }
    return instance;
}

SkyBoxEffect::SkyBoxEffect() {
    isCreated = false;
    hdrInfos.clear();
    isPathChanged = false;
    iMaxMipLevels = 5;

    backgroundVertexShader1 =(unsigned char *)"#version 300 es\n"
                                              "layout(location = 0) in vec3 aPos;\n"
                                              "out vec3 TexCoord;\n"
                                              "uniform mat4 projection;\n"
                                              "uniform mat4 view;\n"
                                              "uniform mat4 model;\n"
                                              "void main()\n"
                                              "{\n"
//                                              "    gl_Position =  (projection*view*model*vec4(aPos,1.0)).xyww;\n"
                                              "    gl_Position =  (projection*mat4(mat3(view))*vec4(aPos,1.0)).xyww;\n"
                                              "    TexCoord = aPos;\n"
                                              "}";
    backgroundFragmentShader1 = (unsigned char *)"#version 300 es\n"
                                                 "precision highp float;\n"
                                                 "in vec3 TexCoord;\n"
                                                 "out vec4 FragColor;\n"
                                                 "uniform samplerCube texture_background;\n"
                                                 "void main()\n"
                                                 "{\n"
//                                                "    FragColor = texture(texture_background, TexCoord);\n"
                                                 "    vec3 envColor = textureLod(texture_background, TexCoord, 0.0).rgb;\n"
                                                 "    \n"
                                                 "    // HDR linear output — tonemap/gamma handled by TonemapEffect post-pass\n"
                                                 "    \n"
                                                 "    FragColor = vec4(envColor, 1.0);"
                                                 "}\n";

    backgroundVertexShader =(unsigned char *)"#version 300 es\n"
                                             "layout(location = 0) in vec3 aPos;\n"
                                             "out vec3 TexCoord;\n"
                                             "uniform mat4 projection;\n"
                                             "uniform mat4 view;\n"
                                             "void main()\n"
                                             "{\n"
                                             "    gl_Position =  projection*view*vec4(aPos,1.0);\n"
                                             "    TexCoord = aPos;\n"
                                             "}";
    backgroundFragmentShader = (unsigned char *)"#version 300 es\n"
                                                "precision highp float;\n"
                                                "in vec3 TexCoord;\n"
                                                "out vec4 FragColor;\n"
                                                "uniform sampler2D texture_background;\n"
                                                "const vec2 invAtan = vec2(0.1591, 0.3183);\n"
                                                "vec2 SampleSphericalMap(vec3 v)\n"
                                                "{\n"
                                                "    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));\n"
                                                "    uv *= invAtan;\n"
                                                "    uv += 0.5;\n"
                                                "    return uv;\n"
                                                "}\n"
                                                "\n"
                                                "void main()\n"
                                                "{\t\t\n"
                                                "    vec2 uv = SampleSphericalMap(normalize(TexCoord));\n"
                                                "    vec3 color = texture(texture_background, uv).rgb;\n"
                                                "    \n"
                                                "    FragColor = vec4(color, 1.0);\n"
                                                "}";

    backgroundVertexShader2 =(unsigned char *)"#version 300 es\n"
                                              "layout(location = 0) in vec3 aPos;\n"
                                              "out vec3 TexCoord;\n"
                                              "uniform mat4 projection;\n"
                                              "uniform mat4 view;\n"
                                              "void main()\n"
                                              "{\n"
                                              "    gl_Position =  projection*view*vec4(aPos,1.0);\n"
                                              "    TexCoord = aPos;\n"
                                              "}";
    backgroundFragmentShader2 = (unsigned char *)"#version 300 es\n"
                                                 "precision highp float;\n"
                                                 "in vec3 TexCoord;\n"
                                                 "out vec4 FragColor;\n"
                                                 "uniform samplerCube texture_background;\n"
                                                 "\n"
                                                 "const float PI = 3.14159265359;\n"
                                                 "\n"
                                                 "void main()\n"
                                                 "{\t\t\n"
                                                 "    vec3 N = normalize(TexCoord);\n"
                                                 "\n"
                                                 "    vec3 irradiance = vec3(0.0);   \n"
                                                 "    \n"
                                                 "    // tangent space calculation from origin point\n"
                                                 "    vec3 up    = vec3(0.0, 1.0, 0.0);\n"
                                                 "    vec3 right = normalize(cross(up, N));\n"
                                                 "    up         = normalize(cross(N, right));\n"
                                                 "       \n"
                                                 "    float sampleDelta = 0.025;\n"
                                                 "    float nrSamples = 0.0;\n"
                                                 "    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)\n"
                                                 "    {\n"
                                                 "        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)\n"
                                                 "        {\n"
                                                 "            // spherical to cartesian (in tangent space)\n"
                                                 "            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));\n"
                                                 "            // tangent space to world\n"
                                                 "            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N; \n"
                                                 "\n"
                                                 "            irradiance += texture(texture_background, sampleVec).rgb * cos(theta) * sin(theta);\n"
                                                 "            nrSamples++;\n"
                                                 "        }\n"
                                                 "    }\n"
                                                 "    irradiance = PI * irradiance * (1.0 / float(nrSamples));\n"
                                                 "    \n"
                                                 "    FragColor = vec4(irradiance, 1.0);\n"
                                                 "}";
    prefilterVertexShader = (unsigned char *)"#version 300 es\n"
                                             "layout(location = 0) in vec3 aPos;\n"
                                             "out vec3 WorldPos;\n"
                                             "uniform mat4 projection;\n"
                                             "uniform mat4 view;\n"
                                             "void main()\n"
                                             "{\n"
                                             "    gl_Position =  projection*view*vec4(aPos,1.0);\n"
                                             "    WorldPos = aPos;\n"
                                             "}";
    prefilterFramentShader = (unsigned char *)"#version 300 es\n"
                                              "precision highp float;\n"
                                              "precision highp int;\n"
                                              "out vec4 FragColor;\n"
                                              "in vec3 WorldPos;\n"
                                              "\n"
                                              "uniform samplerCube environmentMap;\n"
                                              "uniform float roughness;\n"
                                              "\n"
                                              "const float PI = 3.14159265359;\n"
                                              "// ----------------------------------------------------------------------------\n"
                                              "float DistributionGGX(vec3 N, vec3 H, float roughness)\n"
                                              "{\n"
                                              "    float a = roughness*roughness;\n"
                                              "    float a2 = a*a;\n"
                                              "    float NdotH = max(dot(N, H), 0.0);\n"
                                              "    float NdotH2 = NdotH*NdotH;\n"
                                              "\n"
                                              "    float nom   = a2;\n"
                                              "    float denom = (NdotH2 * (a2 - 1.0) + 1.0);\n"
                                              "    denom = PI * denom * denom;\n"
                                              "\n"
                                              "    return nom / denom;\n"
                                              "}\n"
                                              "// ----------------------------------------------------------------------------\n"
                                              "// efficient VanDerCorpus calculation.\n"
                                              "float RadicalInverse_VdC(uint bits) \n"
                                              "{\n"
                                              "     bits = (bits << 16u) | (bits >> 16u);\n"
                                              "     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);\n"
                                              "     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);\n"
                                              "     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);\n"
                                              "     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);\n"
                                              "     return float(bits) * 2.3283064365386963e-10; // / 0x100000000\n"
                                              "}\n"
                                              "// ----------------------------------------------------------------------------\n"
                                              "vec2 Hammersley(uint i, uint N)\n"
                                              "{\n"
                                              "\treturn vec2(float(i)/float(N), RadicalInverse_VdC(i));\n"
                                              "}\n"
                                              "float VanDerCorpus(uint n, uint base)\n"
                                              "{\n"
                                              "    float invBase = 1.0 / float(base);\n"
                                              "    float denom = 1.0;\n"
                                              "    float result = 0.0;\n"
                                              "\n"
                                              "    for(uint i = 0u; i < 32u; ++i)\n"
                                              "    {\n"
                                              "        if(n > 0u)\n"
                                              "        {\n"
                                              "            denom   = mod(float(n), 2.0);\n"
                                              "            result += denom * invBase;\n"
                                              "            invBase = invBase / 2.0;\n"
                                              "            n       = uint(float(n) / 2.0);\n"
                                              "        }\n"
                                              "    }\n"
                                              "\n"
                                              "    return result;\n"
                                              "}\n"
                                              "vec2 HammersleyNoBitOps(uint i, uint N)\n"
                                              "{\n"
                                              "    return vec2(float(i)/float(N), VanDerCorpus(i, 2u));\n"
                                              "}"
                                              "// ----------------------------------------------------------------------------\n"
                                              "vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)\n"
                                              "{\n"
                                              "\tfloat a = roughness*roughness;\n"
                                              "\t\n"
                                              "\tfloat phi = 2.0 * PI * Xi.x;\n"
                                              "\tfloat cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));\n"
                                              "\tfloat sinTheta = sqrt(1.0 - cosTheta*cosTheta);\n"
                                              "\t\n"
                                              "\t// from spherical coordinates to cartesian coordinates - halfway vector\n"
                                              "\tvec3 H;\n"
                                              "\tH.x = cos(phi) * sinTheta;\n"
                                              "\tH.y = sin(phi) * sinTheta;\n"
                                              "\tH.z = cosTheta;\n"
                                              "\t\n"
                                              "\t// from tangent-space H vector to world-space sample vector\n"
                                              "\tvec3 up          = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);\n"
                                              "\tvec3 tangent   = normalize(cross(up, N));\n"
                                              "\tvec3 bitangent = cross(N, tangent);\n"
                                              "\t\n"
                                              "\tvec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;\n"
                                              "\treturn normalize(sampleVec);\n"
                                              "}\n"
                                              "// ----------------------------------------------------------------------------\n"
                                              "void main()\n"
                                              "{\t\t\n"
                                              "    vec3 N = normalize(WorldPos);\n"
                                              "    \n"
                                              "    // make the simplyfying assumption that V equals R equals the normal \n"
                                              "    vec3 R = N;\n"
                                              "    vec3 V = R;\n"
                                              "\n"
                                              "    const uint SAMPLE_COUNT = 1024u;\n"
                                              "    vec3 prefilteredColor = vec3(0.0);\n"
                                              "    float totalWeight = 0.0;\n"
                                              "    \n"
                                              "    for(uint i = 0u; i < SAMPLE_COUNT; ++i)\n"
                                              "    {\n"
                                              "        // generates a sample vector that's biased towards the preferred alignment direction (importance sampling).\n"
                                              "        vec2 Xi = HammersleyNoBitOps(i, SAMPLE_COUNT);\n"
                                              "        vec3 H = ImportanceSampleGGX(Xi, N, roughness);\n"
                                              "        vec3 L  = normalize(2.0 * dot(V, H) * H - V);\n"
                                              "\n"
                                              "        float NdotL = max(dot(N, L), 0.0);\n"
                                              "        if(NdotL > 0.0)\n"
                                              "        {\n"
                                              "            // sample from the environment's mip level based on roughness/pdf\n"
                                              "            float D   = DistributionGGX(N, H, roughness);\n"
                                              "            float NdotH = max(dot(N, H), 0.0);\n"
                                              "            float HdotV = max(dot(H, V), 0.0);\n"
                                              "            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001; \n"
                                              "\n"
                                              "            float resolution = 512.0; // resolution of source cubemap (per face)\n"
                                              "            float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);\n"
                                              "            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);\n"
                                              "\n"
                                              "            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); \n"
                                              "            \n"
                                              "            prefilteredColor += textureLod(environmentMap, L, mipLevel).rgb * NdotL;\n"
                                              "            totalWeight      += NdotL;\n"
                                              "        }\n"
                                              "    }\n"
                                              "\n"
                                              "    prefilteredColor = prefilteredColor / totalWeight;\n"
                                              "\n"
                                              "    FragColor = vec4(prefilteredColor, 1.0);\n"
                                              "}";

    brdfVertexShader =(unsigned char *)"#version 300 es\n"
                                       "precision highp float;\n"
                                       "layout(location = 0) in vec3 aPos;\n"
                                       "layout(location = 1) in vec2 aTexCoords;\n"
                                       "\n"
                                       "out vec2 TexCoords;\n"
                                       "\n"
                                       "void main()\n"
                                       "{\n"
                                       "    TexCoords = aTexCoords;\n"
                                       "    gl_Position = vec4(aPos, 1.0);\n"
                                       "}";
    brdfFramentShader =(unsigned char *) "#version 300 es\n"
                                         "precision highp float;\n"
                                         "out vec2 FragColor;\n"
                                         "in vec2 TexCoords;\n"
                                         "\n"
                                         "const float PI = 3.14159265359;\n"
                                         "// ----------------------------------------------------------------------------\n"
                                         "// efficient VanDerCorpus calculation.\n"
                                         "float RadicalInverse_VdC(uint bits) \n"
                                         "{\n"
                                         "     bits = (bits << 16u) | (bits >> 16u);\n"
                                         "     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);\n"
                                         "     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);\n"
                                         "     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);\n"
                                         "     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);\n"
                                         "     return float(bits) * 2.3283064365386963e-10; // / 0x100000000\n"
                                         "}\n"
                                         "// ----------------------------------------------------------------------------\n"
                                         "vec2 Hammersley(uint i, uint N)\n"
                                         "{\n"
                                         "\treturn vec2(float(i)/float(N), RadicalInverse_VdC(i));\n"
                                         "}\n"
                                         "float VanDerCorpus(uint n, uint base)\n"
                                         "{\n"
                                         "    float invBase = 1.0 / float(base);\n"
                                         "    float denom = 1.0;\n"
                                         "    float result = 0.0;\n"
                                         "\n"
                                         "    for(uint i = 0u; i < 32u; ++i)\n"
                                         "    {\n"
                                         "        if(n > 0u)\n"
                                         "        {\n"
                                         "            denom   = mod(float(n), 2.0);\n"
                                         "            result += denom * invBase;\n"
                                         "            invBase = invBase / 2.0;\n"
                                         "            n       = uint(float(n) / 2.0);\n"
                                         "        }\n"
                                         "    }\n"
                                         "\n"
                                         "    return result;\n"
                                         "}\n"
                                         "vec2 HammersleyNoBitOps(uint i, uint N)\n"
                                         "{\n"
                                         "    return vec2(float(i)/float(N), VanDerCorpus(i, 2u));\n"
                                         "}"
                                         "// ----------------------------------------------------------------------------\n"
                                         "vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)\n"
                                         "{\n"
                                         "\tfloat a = roughness*roughness;\n"
                                         "\t\n"
                                         "\tfloat phi = 2.0 * PI * Xi.x;\n"
                                         "\tfloat cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));\n"
                                         "\tfloat sinTheta = sqrt(1.0 - cosTheta*cosTheta);\n"
                                         "\t\n"
                                         "\t// from spherical coordinates to cartesian coordinates - halfway vector\n"
                                         "\tvec3 H;\n"
                                         "\tH.x = cos(phi) * sinTheta;\n"
                                         "\tH.y = sin(phi) * sinTheta;\n"
                                         "\tH.z = cosTheta;\n"
                                         "\t\n"
                                         "\t// from tangent-space H vector to world-space sample vector\n"
                                         "\tvec3 up          = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);\n"
                                         "\tvec3 tangent   = normalize(cross(up, N));\n"
                                         "\tvec3 bitangent = cross(N, tangent);\n"
                                         "\t\n"
                                         "\tvec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;\n"
                                         "\treturn normalize(sampleVec);\n"
                                         "}\n"
                                         "// ----------------------------------------------------------------------------\n"
                                         "float GeometrySchlickGGX(float NdotV, float roughness)\n"
                                         "{\n"
                                         "    // note that we use a different k for IBL\n"
                                         "    float a = roughness;\n"
                                         "    float k = (a * a) / 2.0;\n"
                                         "\n"
                                         "    float nom   = NdotV;\n"
                                         "    float denom = NdotV * (1.0 - k) + k;\n"
                                         "\n"
                                         "    return nom / denom;\n"
                                         "}\n"
                                         "// ----------------------------------------------------------------------------\n"
                                         "float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)\n"
                                         "{\n"
                                         "    float NdotV = max(dot(N, V), 0.0);\n"
                                         "    float NdotL = max(dot(N, L), 0.0);\n"
                                         "    float ggx2 = GeometrySchlickGGX(NdotV, roughness);\n"
                                         "    float ggx1 = GeometrySchlickGGX(NdotL, roughness);\n"
                                         "\n"
                                         "    return ggx1 * ggx2;\n"
                                         "}\n"
                                         "// ----------------------------------------------------------------------------\n"
                                         "vec2 IntegrateBRDF(float NdotV, float roughness)\n"
                                         "{\n"
                                         "    vec3 V;\n"
                                         "    V.x = sqrt(1.0 - NdotV*NdotV);\n"
                                         "    V.y = 0.0;\n"
                                         "    V.z = NdotV;\n"
                                         "\n"
                                         "    float A = 0.0;\n"
                                         "    float B = 0.0; \n"
                                         "\n"
                                         "    vec3 N = vec3(0.0, 0.0, 1.0);\n"
                                         "    \n"
                                         "    const uint SAMPLE_COUNT = 1024u;\n"
                                         "    for(uint i = 0u; i < SAMPLE_COUNT; ++i)\n"
                                         "    {\n"
                                         "        // generates a sample vector that's biased towards the\n"
                                         "        // preferred alignment direction (importance sampling).\n"
                                         "        vec2 Xi = HammersleyNoBitOps(i, SAMPLE_COUNT);\n"
                                         "        vec3 H = ImportanceSampleGGX(Xi, N, roughness);\n"
                                         "        vec3 L = normalize(2.0 * dot(V, H) * H - V);\n"
                                         "\n"
                                         "        float NdotL = max(L.z, 0.0);\n"
                                         "        float NdotH = max(H.z, 0.0);\n"
                                         "        float VdotH = max(dot(V, H), 0.0);\n"
                                         "\n"
                                         "        if(NdotL > 0.0)\n"
                                         "        {\n"
                                         "            float G = GeometrySmith(N, V, L, roughness);\n"
                                         "            float G_Vis = (G * VdotH) / (NdotH * NdotV);\n"
                                         "            float Fc = pow(1.0 - VdotH, 5.0);\n"
                                         "\n"
                                         "            A += (1.0 - Fc) * G_Vis;\n"
                                         "            B += Fc * G_Vis;\n"
                                         "        }\n"
                                         "    }\n"
                                         "    A /= float(SAMPLE_COUNT);\n"
                                         "    B /= float(SAMPLE_COUNT);\n"
                                         "    return vec2(A, B);\n"
                                         "}\n"
                                         "// ----------------------------------------------------------------------------\n"
                                         "void main() \n"
                                         "{\n"
                                         "    vec2 integratedBRDF = IntegrateBRDF(TexCoords.x, TexCoords.y);\n"
                                         "    FragColor = integratedBRDF;\n"
                                         "}";
}

SkyBoxEffect::~SkyBoxEffect() {
    delete camera;
    camera = NULL;
//    delete backgroundVertexShader;
//    backgroundVertexShader = NULL;
//    delete backgroundFragmentShader;
//    backgroundFragmentShader = NULL;
}

void SkyBoxEffect::InitProgram() {
    LOGI("InitProgram called, isCreated=%d", isCreated);
    if (isCreated) {
        return;
    }
    tBackgroundProgram = SkyBoxEffect::createProgram(reinterpret_cast<const char *>(backgroundVertexShader), reinterpret_cast<const char *>(backgroundFragmentShader));
    tBackgroundProgram1 = SkyBoxEffect::createProgram(reinterpret_cast<const char *>(backgroundVertexShader1), reinterpret_cast<const char *>(backgroundFragmentShader1));
    tBackgroundProgram2 = SkyBoxEffect::createProgram(reinterpret_cast<const char *>(backgroundVertexShader2), reinterpret_cast<const char *>(backgroundFragmentShader2));
    prefilterProgram = SkyBoxEffect::createProgram(reinterpret_cast<const char *>(prefilterVertexShader), reinterpret_cast<const char *>(prefilterFramentShader));
    brdfProgram = SkyBoxEffect::createProgram(reinterpret_cast<const char *>(brdfVertexShader), reinterpret_cast<const char *>(brdfFramentShader));
    LOGI("InitProgram: prog0=%d, prog1=%d, prog2=%d, prefilter=%d, brdf=%d",
         tBackgroundProgram, tBackgroundProgram1, tBackgroundProgram2, prefilterProgram, brdfProgram);

//    camera = new Camera(glm::vec3(COMMON_RENDER_DATA->getCameraData().x
//                                ,COMMON_RENDER_DATA->getCameraData().y
//                                ,COMMON_RENDER_DATA->getCameraData().z)
//            ,glm::vec3(0.0f, 1.0f, 0.0f)
//            ,COMMON_RENDER_DATA->getCameraData().yaw
//            ,COMMON_RENDER_DATA->getCameraData().pitch);
}

void SkyBoxEffect::generatePBRData(GLuint* ir,GLuint* pr,GLuint* br) {
    LOGI("generatePBRData isCreated=%d, isPathChanged=%d", isCreated, isPathChanged);
    if (isCreated && !isPathChanged) {
        *ir = irradianceMap;
        *pr = prefilterMap;
        *br = brdfLUTTexture;
        return;
    }
    GLint savedViewport[4];
    glGetIntegerv(GL_VIEWPORT, savedViewport);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
//    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    glUseProgram(tBackgroundProgram1);
    glUniform1i(glGetUniformLocation(tBackgroundProgram1, "texture_background"), 0);

    if (!isCreated) {

        // Background
        glGenVertexArrays(1, &VAO_Background);
        glGenBuffers(1, &VBO_Background);
        glBindVertexArray(VAO_Background);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_Background);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verticesAll), verticesAll, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // plane
        glGenVertexArrays(1, &VAO_Background1);
        glGenBuffers(1, &VBO_Background1);
        glBindVertexArray(VAO_Background1);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_Background1);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verticesAll1), &verticesAll1, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glBindVertexArray(0);

        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    }

    captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
//    ogl_print(" generatePBRData1 error = %d",glGetError());
    if(generateEnvCubeMap()) {
        generateIrradianceMap();
        generatePrefilterMap();
        generateBRDFLUTTexture();
        isCreated = true;
        isPathChanged = false;
        for (int i = 0; i < hdrInfos.size(); i++) {
            HdrInfo & info = hdrInfos[i];
            if (hdr_path == info.hdr_path) {
                info.hdrTexture = hdrTexture;
                info.envCubemap = envCubemap;
                info.irradianceMap = irradianceMap;
                info.prefilterMap = prefilterMap;
                info.brdfLUTTexture = brdfLUTTexture;
                info.isCreate = true;
                break;
            }
        }
//        DataCenter::getInstance()->is_need_draw = true;
    } else {
        isCreated = false;
    }
    *ir = irradianceMap;
    *pr = prefilterMap;
    *br = brdfLUTTexture;
    LOGI(" isCreated irradianceMap = %d, prefilterMap = %d, brdfLUTTexture = %d",irradianceMap,prefilterMap,brdfLUTTexture);
    glViewport(savedViewport[0], savedViewport[1], savedViewport[2], savedViewport[3]);
    LOGI(" isCreated error = %d",glGetError());
}

void SkyBoxEffect::SurfaceChange(int width, int height) {

}

void SkyBoxEffect::DrawFrame(float* projection, float* view) {
    // Save OpenGL state
    GLboolean prevCullFace = glIsEnabled(GL_CULL_FACE);
    GLint prevDepthFunc;
    glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);
    GLboolean prevDepthMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);
    GLint prevProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    GLint prevVAO;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
    GLint prevTex;
    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &prevTex);

    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glUseProgram(tBackgroundProgram1);
    glUniformMatrix4fv(glGetUniformLocation(tBackgroundProgram1, "projection"), 1, GL_FALSE, projection);
    glUniformMatrix4fv(glGetUniformLocation(tBackgroundProgram1, "view"), 1, GL_FALSE, view);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glUniform1i(glGetUniformLocation(tBackgroundProgram1, "texture_background"), 0);

    glBindVertexArray(VAO_Background);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    // Restore OpenGL state
    if (prevCullFace) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    glDepthFunc(prevDepthFunc);
    glDepthMask(prevDepthMask);
    glUseProgram(prevProgram);
    glBindVertexArray(prevVAO);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prevTex);
}

bool SkyBoxEffect::generateEnvCubeMap() {
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
//    std::string hdr_path = MODEL_DATA->getRootDir()+"exterior/res/skybox/environment_info.hdr";
    LOGI("generateEnvCubeMap: loading HDR from path: %s", hdr_path.c_str());
    float *data = stbi_loadf(hdr_path.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        LOGI("generateEnvCubeMap: HDR loaded successfully: %dx%d, %d components", width, height, nrComponents);
        glGenTextures(1, &hdrTexture);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        LOGI("environment path = %s", hdr_path.c_str());
    } else {
        LOGI("environment_info load failed, path = %s", hdr_path.c_str());
        return false;
    }
    LOGI(" generateEnvCubeMap stbi_loadf error = %d",glGetError());

    glGenTextures(1, &envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, 512, 512, 0, GL_RGBA, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    ogl_print(" generateEnvCubeMap glGenTextures error = %d",glGetError());
    glUseProgram(tBackgroundProgram);

    glUniformMatrix4fv(glGetUniformLocation(tBackgroundProgram, "projection"), 1, GL_FALSE, &captureProjection[0][0]);

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(tBackgroundProgram, "texture_background"), 0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    glViewport(0, 0, 512, 512);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glUniformMatrix4fv(glGetUniformLocation(tBackgroundProgram, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(VAO_Background);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
//        ogl_print(" generateEnvCubeMap glDrawArrays error = %d",glGetError());
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
//    ogl_print(" generateEnvCubeMap2 error = %d",glGetError());
    LOGI("generateEnvCubeMap framebuffer state = %0x",glCheckFramebufferStatus(GL_FRAMEBUFFER));
    return true;
}

void SkyBoxEffect::generateIrradianceMap() {
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

//    ogl_print(" generateIrradianceMap1 error = %d",glGetError());
    glUseProgram(tBackgroundProgram2);
    glUniform1i(glGetUniformLocation(tBackgroundProgram2, "texture_background"), 0);
    glUniformMatrix4fv(glGetUniformLocation(tBackgroundProgram2, "projection"), 1, GL_FALSE, &captureProjection[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
//    ogl_print(" generateIrradianceMap2 error = %d",glGetError());

    glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glUniformMatrix4fv(glGetUniformLocation(tBackgroundProgram2, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(VAO_Background);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
//        ogl_print(" generateIrradianceMap3 error = %d",glGetError());
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    LOGI("generateIrradianceMap framebuffer state = %0x",glCheckFramebufferStatus(GL_FRAMEBUFFER));
}

void SkyBoxEffect::generatePrefilterMap() {
    int mapWidth = 128;
    int mapHeight = 128;

    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, mapWidth, mapHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
//    ogl_print(" generatePrefilterMap1 error = %d",glGetError());

    glUseProgram(prefilterProgram);
    glUniformMatrix4fv(glGetUniformLocation(prefilterProgram, "projection"), 1, GL_FALSE, &captureProjection[0][0]);
    glUniform1i(glGetUniformLocation(prefilterProgram, "environmentMap"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
//    ogl_print(" generatePrefilterMap2 error = %d",glGetError());

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    unsigned int maxMipLevels = iMaxMipLevels;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {
        unsigned int mipWidth = static_cast<unsigned int>(mapWidth * std::pow(0.5, mip));
        unsigned int mipHeight = static_cast<unsigned int>(mapHeight * std::pow(0.5, mip));
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);
//        ogl_print(" generatePrefilterMap3 error = %d",glGetError());

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        glUniform1f(glGetUniformLocation(prefilterProgram, "roughness"), roughness);
        for (unsigned int i = 0; i < 6; ++i)
        {
            glUniformMatrix4fv(glGetUniformLocation(prefilterProgram, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glBindVertexArray(VAO_Background);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
//            ogl_print(" generatePrefilterMap4 error = %d",glGetError());
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    LOGI("generatePrefilterMap framebuffer state = %0x",glCheckFramebufferStatus(GL_FRAMEBUFFER));
}

void SkyBoxEffect::generateBRDFLUTTexture() {
    glGenTextures(1, &brdfLUTTexture);
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 512, 512, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

    glViewport(0, 0, 512, 512);
    glUseProgram(brdfProgram);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(VAO_Background1);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
//    ogl_print(" generateBRDFLUTTexture1 error = %d",glGetError());

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    LOGI("generateBRDFLUTTexture framebuffer state = %0x",glCheckFramebufferStatus(GL_FRAMEBUFFER));
}

void SkyBoxEffect::setHdrPath(const std::string& path) {
    LOGI("SkyBoxEffect::setHdrPath %s", path.c_str());
    hdr_path = path;
    isPathChanged = true;

    // find texture if exist
    bool bFind = false;
    for (int i = 0; i < hdrInfos.size(); i++) {
        HdrInfo & info = hdrInfos[i];
        if (hdr_path == info.hdr_path) {
            hdrTexture = info.hdrTexture;
            envCubemap = info.envCubemap;
            irradianceMap = info.irradianceMap;
            prefilterMap = info.prefilterMap;
            brdfLUTTexture = info.brdfLUTTexture;
            isPathChanged = !info.isCreate;
            bFind = true;
            break;
        }
    }
//    ogl_print("SkyBoxEffect::setHdrPath bFind %d", bFind);
//    ogl_print("SkyBoxEffect::setHdrPath isPathChanged %d", isPathChanged);

    // can not find, need to re-create
    if (!bFind) {
        HdrInfo newInfo;
        newInfo.isCreate = false;
        newInfo.hdr_path = hdr_path;
        hdrInfos.push_back(newInfo);
    }
}

void SkyBoxEffect::setMaxMipLevels(unsigned int lod) { // 3,4,5
//    ogl_print("SkyBoxEffect::setMaxMipLevels %d", lod);
    if (lod < 3 || lod > 5) {
        return;
    }
    iMaxMipLevels = lod;
}

void SkyBoxEffect::clear() {
    LOGI("SkyBoxEffect::clear");
    for (int i = 0; i < hdrInfos.size(); i++) {
        HdrInfo & info = hdrInfos[i];
        if (info.hdrTexture != 0) {
            glDeleteTextures(1, &info.hdrTexture);
        }
        if (info.envCubemap != 0) {
            glDeleteTextures(1, &info.envCubemap);
        }
        if (info.irradianceMap != 0) {
            glDeleteTextures(1, &info.irradianceMap);
        }
        if (info.prefilterMap != 0) {
            glDeleteTextures(1, &info.prefilterMap);
        }
        if (info.brdfLUTTexture != 0) {
            glDeleteTextures(1, &info.brdfLUTTexture);
        }
    }

    hdrInfos.clear();
}

GLuint SkyBoxEffect::loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        LOGI("type = %d, compiled=%d", shaderType, compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGI("type = %d, error =%s", shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint SkyBoxEffect::createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (vertexShader == 0) {
        LOGE("Vertex shader compilation failed");
        return 0;
    }
    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        LOGE("Fragment shader compilation failed");
        return 0;
    }
    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOGE("Program link error: %s", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        } else {
            glDetachShader(program, vertexShader);
            glDeleteShader(vertexShader);
            glDetachShader(program,fragmentShader);
            glDeleteShader(fragmentShader);
            LOGI("Shader program %d created successfully", program);
        }
    }
    return program;
}
