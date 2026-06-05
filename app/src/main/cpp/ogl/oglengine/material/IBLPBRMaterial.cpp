#include "../include/IBLPBRMaterial.h"
#include "../include/Shader.h"
#include "../include/Common.h"
#include <cstdio>
#include <cmath>
#include <map>
#include <utility>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

IBLPBRMaterial::IBLPBRMaterial() : Material() {
    frameIndex = 0;
    baseColor[0] = baseColor[1] = baseColor[2] = 0.0f;
    metallic = 0.0f;
    roughness = 0.0f;
    ao = 1.0f;
    clearCoat = 0.0f;
    clearCoatRoughness = 0.1f;
    camPos[0] = camPos[1] = camPos[2] = 0.0f;
    for (int i = 0; i < 16; i++) {
        modelMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        projectionMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        viewMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        lightSpaceMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
    for (int i = 0; i < 4; i++) {
        lightPositions[i][0] = lightPositions[i][1] = lightPositions[i][2] = 0.0f;
        lightColors[i][0] = lightColors[i][1] = lightColors[i][2] = 0.0f;
    }
    shadowMap = 0;
    shadowEnabled = false;
    initialized = false;
}

IBLPBRMaterial::~IBLPBRMaterial() {
    release();
}

void IBLPBRMaterial::init() {
    if (initialized) return;
    initialized = true;
    (void)getOrCreateVariant(0);
    LOGI("IBLPBRMaterial init done, %zu variants", variants.size());
}

void IBLPBRMaterial::release() {
    for (auto& kv : variants) {
        if (kv.second->shader) {
            delete kv.second->shader;
            kv.second->shader = nullptr;
        }
        delete kv.second;
    }
    variants.clear();
    initialized = false;
}

IBLPBRMaterial::ShaderVariant* IBLPBRMaterial::getOrCreateVariant(int flag) {
    auto it = variants.find(flag);
    if (it != variants.end()) {
        return it->second;
    }
    std::string vs = buildVS(flag);
    std::string fs = buildFS(flag);
    Shader* s = new Shader();
    bool initSuccess = s->init(vs.c_str(), fs.c_str());
    bool linked = false;
    if (!initSuccess) {
        LOGE("IBLPBRMaterial variant 0x%x shader init failed", flag);
    } else {
        while (glGetError() != GL_NO_ERROR) {}
        GLint linkStatus = 0;
        GLuint prog = s->getProgram();
        glGetProgramiv(prog, GL_LINK_STATUS, &linkStatus);
        linked = (linkStatus != 0);
        GLint e = glGetError();
        LOGI("IBLPBRMaterial variant 0x%x: program=%d linked=%d err=0x%x",
             flag, prog, linked ? 1 : 0, e);
        if (!linked) {
            // Get log
            GLint logLen = 0;
            glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
            if (logLen > 0) {
                std::vector<char> log(logLen);
                glGetProgramInfoLog(prog, logLen, nullptr, log.data());
                LOGE("IBLPBRMaterial variant 0x%x link log: %s", flag, log.data());
            }
        }
    }
    ShaderVariant* v = new ShaderVariant();
    v->shader = s;
    v->linked = initSuccess && linked;
    v->vsSource = vs;
    v->fsSource = fs;
    if ((flag & FLAG_SKINNED) && v->linked) {
        v->bonesLoc = s->getUniformLocation("uBones[0]");
    }
    variants[flag] = v;
    return v;
}

void IBLPBRMaterial::setTransform(const float* model, const float* view, const float* projection) {
    setModelMatrix(model);
    setViewMatrix(view);
    setProjectionMatrix(projection);
}

void IBLPBRMaterial::use() {
    // Enable blending for transparent materials
    if (opacity_ < 1.0f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
    }

    ShaderVariant* v = getOrCreateVariant(shaderFlag);
    if (!v || !v->shader || !v->linked) {
        LOGE("IBLPBRMaterial::use() - shader variant not linked, flag=0x%x", shaderFlag);
        return;
    }
    Shader* sh = v->shader;
    sh->use();
    glm::mat4 mm = glm::make_mat4(modelMatrix);
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(mm)));
    sh->setMat3("normalMatrix", &normalMatrix[0][0]);
    sh->setMat4("projection", projectionMatrix);
    sh->setMat4("view", viewMatrix);
    sh->setMat4("model", modelMatrix);
    sh->setVec2("jitterOffset", jitterOffset);
    sh->setMat4("prevModel", prevModelMatrix);
    sh->setMat4("prevProjection", prevProjMatrix);
    sh->setMat4("prevView", prevViewMatrix);
    sh->setVec3("albedo", baseColor);
    sh->setFloat("metallic", metallic);
    sh->setFloat("roughness", roughness);
    sh->setFloat("ao", ao);
    sh->setVec3("camPos", camPos);
    sh->setFloat("uOpacity", opacity_);
    sh->setFloat("uScreenWidth", (float)screenWidth);
    sh->setFloat("uScreenHeight", (float)screenHeight);
    sh->setInt("uFrameIndex", frameIndex);

    for (int i = 0; i < 4; i++) {
        char name[32];
        snprintf(name, sizeof(name), "lightPositions[%d]", i);
        sh->setVec3(name, lightPositions[i]);
        snprintf(name, sizeof(name), "lightColors[%d]", i);
        sh->setVec3(name, lightColors[i]);
    }

    sh->setInt("irradianceMap", 0);
    sh->setInt("prefilterMap", 1);
    sh->setInt("brdfLUT", 2);

    sh->setInt("albedoMap", 3);
    sh->setInt("metallicMap", 4);
    sh->setInt("roughnessMap", 5);
    sh->setInt("aoMap", 6);
    sh->setInt("normalMap", 7);

    // Shadow uniforms — only set when this variant has shadows compiled in
    if (shaderFlag & FLAG_SHADOW) {
        sh->setInt("shadowMap", 8);
        sh->setMat4("lightSpaceMatrix", lightSpaceMatrix);
        sh->setFloat("shadowBias", shadowBias);
        sh->setInt("shadowMapSize", shadowMapSize);
    }

    if (albedoTex != 0) {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, albedoTex);
    }
    if (metallicTex != 0) {
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, metallicTex);
    }
    if (roughnessTex != 0) {
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, roughnessTex);
    }
    if (aoTex != 0) {
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, aoTex);
    }
    if (normalTex != 0) {
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, normalTex);
    }
    
    // Bind shadow map if available
    if ((shaderFlag & FLAG_SHADOW) && shadowMap != 0) {
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, shadowMap);
    }

    if (shaderFlag & FLAG_CLEARCOAT) {
        sh->setFloat("clearCoat", clearCoat);
        sh->setFloat("clearCoatRoughness", clearCoatRoughness);
    }

    if (shaderFlag & FLAG_SKINNED) {
        uploadBonePalette(v->bonesLoc);
    }
}

const Material::VertexAttrib* IBLPBRMaterial::getVertexAttribs(int& count) {
    static const VertexAttrib attribs[] = {
        {"aPos", 3, 0, 0},
        {"aNormal", 3, 1, 3 * sizeof(float)},
        {"aTexCoords", 2, 2, 6 * sizeof(float)}
    };
    count = 3;
    return attribs;
}

void IBLPBRMaterial::setBaseColor(float r, float g, float b) {
    baseColor[0] = r;
    baseColor[1] = g;
    baseColor[2] = b;
}

void IBLPBRMaterial::setMetallic(float m) { metallic = m; }

void IBLPBRMaterial::setRoughness(float r) { roughness = r; }

void IBLPBRMaterial::setAO(float a) { ao = a; }

void IBLPBRMaterial::setClearCoat(float intensity, float roughnessVal) {
    clearCoat = intensity;
    clearCoatRoughness = roughnessVal;
    if (intensity > 0.0f) shaderFlag |= FLAG_CLEARCOAT;
}

void IBLPBRMaterial::setAlbedoMap(GLuint tex) {
    albedoTex = tex;
    if (tex != 0) shaderFlag |= FLAG_ALBEDO_MAP;
}
void IBLPBRMaterial::setMetallicMap(GLuint tex) {
    metallicTex = tex;
    if (tex != 0) shaderFlag |= FLAG_METALLIC_MAP;
}
void IBLPBRMaterial::setRoughnessMap(GLuint tex) {
    roughnessTex = tex;
    if (tex != 0) shaderFlag |= FLAG_ROUGHNESS_MAP;
}
void IBLPBRMaterial::setAOMap(GLuint tex) {
    aoTex = tex;
    if (tex != 0) shaderFlag |= FLAG_AO_MAP;
}
void IBLPBRMaterial::setNormalMap(GLuint tex) {
    normalTex = tex;
    if (tex != 0) shaderFlag |= FLAG_NORMAL_MAP;
}

void IBLPBRMaterial::setCamPos(float x, float y, float z) {
    camPos[0] = x; camPos[1] = y; camPos[2] = z;
}

void IBLPBRMaterial::setModelMatrix(const float* m) {
    for (int i = 0; i < 16; i++) modelMatrix[i] = m[i];
}

void IBLPBRMaterial::setProjectionMatrix(const float* p) {
    for (int i = 0; i < 16; i++) projectionMatrix[i] = p[i];
}

void IBLPBRMaterial::setViewMatrix(const float* v) {
    for (int i = 0; i < 16; i++) viewMatrix[i] = v[i];
}

void IBLPBRMaterial::setLight(int idx, const float* pos, const float* color) {
    if (idx < 0 || idx >= 4) return;
    lightPositions[idx][0] = pos[0]; lightPositions[idx][1] = pos[1]; lightPositions[idx][2] = pos[2];
    lightColors[idx][0] = color[0]; lightColors[idx][1] = color[1]; lightColors[idx][2] = color[2];
}

void IBLPBRMaterial::setPrevViewMatrix(const float* v) {
    for (int i = 0; i < 16; i++) prevViewMatrix[i] = v[i];
}

void IBLPBRMaterial::setPrevProjectionMatrix(const float* p) {
    for (int i = 0; i < 16; i++) prevProjMatrix[i] = p[i];
}

void IBLPBRMaterial::setPrevModelMatrix(const float* m) {
    for (int i = 0; i < 16; i++) prevModelMatrix[i] = m[i];
}

void IBLPBRMaterial::setScreenSize(int w, int h) {
    screenWidth = w; screenHeight = h;
}

void IBLPBRMaterial::setFrameCount(int count) {
    frameIndex = count;
}

void IBLPBRMaterial::setOpacity(float opacity) {
    opacity_ = opacity;
    if (opacity < 1.0f) shaderFlag |= FLAG_TRANSPARENT;
    else                shaderFlag &= ~FLAG_TRANSPARENT;
}

void IBLPBRMaterial::onPostRender() {
    if (opacity_ < 1.0f) {
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
    }
}

const char* IBLPBRMaterial::getBaseVS() {
    // Kept for reference; buildVS() now generates the VS source so it
    // can splice in the optional skinning chunk based on FLAG_SKINNED.
    return "";
}

const char* IBLPBRMaterial::getBaseFS() {
    return R"(
#version 300 es
precision highp float;
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 PrevWorldPos;
in vec3 Normal;
in vec2 vNowNDC;
in vec2 vPrevNDC;
layout(location=0) out vec4 FragColor;
layout(location=1) out vec2 Velocity;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;
uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];
uniform vec3 camPos;
uniform mat4 projection;
uniform mat4 view;
uniform float uOpacity;
uniform sampler2D albedoMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform sampler2D normalMap;
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
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
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
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
float SchlickFresnel(float u) {
    float m = clamp(1.0 - u, 0.0, 1.0);
    float m2 = m * m;
    return m2 * m2 * m;
}
)";
}

const char* IBLPBRMaterial::getShadowChunk() {
    return R"(
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;
uniform float shadowBias;
uniform int shadowMapSize;

float calculateShadow(vec3 worldPos) {
    vec4 lightSpacePos = lightSpaceMatrix * vec4(worldPos, 1.0);
    if (abs(lightSpacePos.w) < 0.0001) return 1.0;
    lightSpacePos.xyz /= lightSpacePos.w;
    vec3 projCoords = lightSpacePos.xyz * 0.5 + 0.5;
    if (projCoords.z < 0.0 || projCoords.z > 1.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;
    }
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z - shadowBias;
    float shadow = (currentDepth > closestDepth) ? 0.0 : 1.0;
    return mix(1.0, shadow, 0.7);
}
)";
}

const char* IBLPBRMaterial::getNormalMapChunk() {
    return R"(
vec3 getNormalFromMap() {
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;
    vec3 Q1 = dFdx(WorldPos);
    vec3 Q2 = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);
    vec3 N = normalize(Normal);
    vec3 T = normalize(Q1 * st2.y - Q2 * st1.y);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * tangentNormal);
}
)";
}

const char* IBLPBRMaterial::getAlbedoMapChunk() {
    return "";
}

const char* IBLPBRMaterial::getMetallicMapChunk() {
    return "";
}

const char* IBLPBRMaterial::getRoughnessMapChunk() {
    return "";
}

const char* IBLPBRMaterial::getAOMapChunk() {
    return "";
}

const char* IBLPBRMaterial::getClearCoatChunk() {
    return R"(
uniform float clearCoat;
uniform float clearCoatRoughness;
)";
}

std::string IBLPBRMaterial::buildVS(int flag) {
    std::string vs;
    vs.reserve(2048);
    vs = R"(#version 300 es
precision highp float;
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
)";
    vs += Material::skinningVSChunk((flag & FLAG_SKINNED) != 0);
    vs += R"(
out vec2 TexCoords;
out vec3 WorldPos;
out vec3 PrevWorldPos;
out vec3 Normal;
out vec2 vNowNDC;
out vec2 vPrevNDC;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 prevProjection;
uniform mat4 prevView;
uniform mat4 prevModel;
uniform mat3 normalMatrix;
uniform vec2 jitterOffset;

void main() {
    vec4 skinned = skinPos(vec4(aPos, 1.0));
    vec3 skinnedN = skinNormal(aNormal);

    vec4 worldPos = model * skinned;
    WorldPos = worldPos.xyz;
    Normal = normalize(normalMatrix * skinnedN);
    TexCoords = aTexCoords;

    vec4 prevWorld = prevModel * skinned;
    PrevWorldPos = prevWorld.xyz;

    vec4 nowClip  = projection * view * worldPos;
    vec4 prevClip = prevProjection * prevView * prevWorld;
    vNowNDC  = nowClip.xy  / nowClip.w;
    vPrevNDC = prevClip.xy / prevClip.w;

    mat4 jp = projection;
    jp[2][0] += jitterOffset.x;
    jp[2][1] += jitterOffset.y;
    gl_Position = jp * view * worldPos;
}
)";
    return vs;
}

std::string IBLPBRMaterial::buildFS(int flag) {
    std::string fs;
    fs.reserve(4096);
    fs = getBaseFS();

    if (flag & FLAG_NORMAL_MAP) {
        fs += getNormalMapChunk();
    }
    if (flag & FLAG_CLEARCOAT) {
        fs += getClearCoatChunk();
    }
    if (flag & FLAG_SHADOW) {
        fs += getShadowChunk();
    }

    fs += R"(
void main() {
    vec3 N = normalize(Normal);
)";

    if (flag & FLAG_NORMAL_MAP) {
        fs += "    N = getNormalFromMap();\n";
    }

    fs += R"(
    vec3 V = normalize(camPos - WorldPos);
    vec3 R = reflect(-V, N);
    float NdotV = max(dot(N, V), 0.0001);

    float mapMetallic = metallic;
    float mapRoughness = roughness;
    float mapAO = ao;
    vec3 mapAlbedo = albedo;
)";

    if (flag & FLAG_ALBEDO_MAP) {
        fs += "    mapAlbedo = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));\n";
    }
    if (flag & FLAG_METALLIC_MAP) {
        fs += "    mapMetallic = texture(metallicMap, TexCoords).r;\n";
    }
    if (flag & FLAG_ROUGHNESS_MAP) {
        fs += "    mapRoughness = texture(roughnessMap, TexCoords).r;\n";
    }
    if (flag & FLAG_AO_MAP) {
        fs += "    mapAO = texture(aoMap, TexCoords).r;\n";
    }

    fs += R"(
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, mapAlbedo, mapMetallic);

    vec3 Lo = vec3(0.0);
    
    // 4 个 IBL 光源用于 PBR 照明计算（点光源）
    // 阴影来自独立的定向光（太阳），在最终结果上应用
    for (int i = 0; i < 4; ++i) {
        vec3 L = normalize(lightPositions[i] - WorldPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;

        float NDF = DistributionGGX(N, H, mapRoughness);
        float G = GeometrySmith(N, V, L, mapRoughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - mapMetallic;

        float NdotL = max(dot(N, L), 0.0);
        
        // 不对 IBL 光源应用阴影（它们是点光源，不是阴影光源）
        Lo += (kD * mapAlbedo / PI + specular) * radiance * NdotL;
    }

    vec3 F = fresnelSchlickRoughness(NdotV, F0, mapRoughness);
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - mapMetallic;

    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * mapAlbedo;

    const float MAX_REFLECTION_LOD = 4.0;
    float prefilterLod = mapRoughness * MAX_REFLECTION_LOD;
    vec3 prefilteredColor = textureLod(prefilterMap, R, prefilterLod).rgb;
    vec2 brdf2 = texture(brdfLUT, vec2(NdotV, mapRoughness)).xy;

    vec3 specular = prefilteredColor * (F * brdf2.x + brdf2.y);

    vec3 ambient = (kD * diffuse + specular) * mapAO;

    vec3 color = ambient + Lo;
)";

    if (flag & FLAG_SHADOW) {
        fs += R"(
    // Apply shadow ONCE to final color (shadow is from separate sun light)
    float shadow = calculateShadow(WorldPos);
    color *= shadow;
)";
    }

    if (flag & FLAG_CLEARCOAT) {
        fs += R"(
    float ccR = max(clearCoatRoughness, 0.04);
    float ccF = mix(0.04, 1.0, SchlickFresnel(NdotV));
    float ccLod = ccR * MAX_REFLECTION_LOD;
    vec3 ccRefl = textureLod(prefilterMap, R, ccLod).rgb;
    vec2 ccBrdf = texture(brdfLUT, vec2(NdotV, ccR)).xy;
    color += vec3(ccF) * (ccRefl * ccBrdf.x + ccBrdf.y) * clearCoat;

    for (int i = 0; i < 4; ++i) {
        vec3 L = normalize(lightPositions[i] - WorldPos);
        float NdotL = max(dot(N, L), 0.0);
        float dist = length(lightPositions[i] - WorldPos);
        float atten = 1.0 / (dist * dist);
        vec3 radiance = lightColors[i] * atten;
        
        color += vec3(ccF) * radiance * NdotL * clearCoat;
    }
)";
    }

    fs += R"(
    // No tonemap/gamma here — HDR linear output goes to TonemapEffect post-pass.

    FragColor = vec4(color, uOpacity);

    // Transparent pixels must not contribute to the velocity buffer
    // (would cause TAA ghosting).
    if (uOpacity < 1.0) {
        Velocity = vec2(0.0);
    } else {
        // vNowNDC / vPrevNDC are already perspective-divided in the VS.
        // NDC is in [-1,1]; convert difference to UV-space [0,1] by * 0.5.
        Velocity = (vNowNDC - vPrevNDC) * 0.5;
    }
}
)";

    return fs;
}
