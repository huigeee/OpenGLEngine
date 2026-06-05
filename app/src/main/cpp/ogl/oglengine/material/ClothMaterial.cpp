#include "../include/ClothMaterial.h"
#include "../include/Shader.h"
#include "../include/Common.h"
#include <cstdio>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

ClothMaterial::ClothMaterial() : Material() {
    for (int i = 0; i < 16; i++) {
        modelMatrix[i]      = (i % 5 == 0) ? 1.0f : 0.0f;
        projectionMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        viewMatrix[i]       = (i % 5 == 0) ? 1.0f : 0.0f;
        lightSpaceMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
    for (int i = 0; i < 4; i++) {
        lightPositions[i][0] = lightPositions[i][1] = lightPositions[i][2] = 0.0f;
        lightColors[i][0]    = lightColors[i][1]    = lightColors[i][2]    = 0.0f;
    }
}

ClothMaterial::~ClothMaterial() { release(); }

void ClothMaterial::init() {
    if (initialized) return;
    initialized = true;
    (void)getOrCreateVariant(0);
    LOGI("ClothMaterial init done, %zu variants", variants.size());
}

void ClothMaterial::release() {
    for (auto& kv : variants) {
        if (kv.second->shader) delete kv.second->shader;
        delete kv.second;
    }
    variants.clear();
    initialized = false;
}

ClothMaterial::ShaderVariant* ClothMaterial::getOrCreateVariant(int flag) {
    auto it = variants.find(flag);
    if (it != variants.end()) return it->second;

    std::string vs = buildVS(flag);
    std::string fs = buildFS(flag);
    Shader* s = new Shader();
    bool ok = s->init(vs.c_str(), fs.c_str());
    bool linked = false;
    if (ok) {
        GLint linkStatus = 0;
        glGetProgramiv(s->getProgram(), GL_LINK_STATUS, &linkStatus);
        linked = (linkStatus != 0);
        if (!linked) {
            GLint logLen = 0;
            glGetProgramiv(s->getProgram(), GL_INFO_LOG_LENGTH, &logLen);
            if (logLen > 0) {
                std::vector<char> log(logLen);
                glGetProgramInfoLog(s->getProgram(), logLen, nullptr, log.data());
                LOGE("ClothMaterial variant 0x%x link log: %s", flag, log.data());
            }
        }
    }
    ShaderVariant* v = new ShaderVariant();
    v->shader = s;
    v->linked = ok && linked;
    v->vsSource = vs;
    v->fsSource = fs;
    if ((flag & FLAG_SKINNED) && v->linked) {
        v->bonesLoc = s->getUniformLocation("uBones[0]");
    }
    variants[flag] = v;
    LOGI("ClothMaterial variant 0x%x: linked=%d", flag, v->linked ? 1 : 0);
    return v;
}

void ClothMaterial::setTransform(const float* model, const float* view, const float* projection) {
    setModelMatrix(model);
    setViewMatrix(view);
    setProjectionMatrix(projection);
}

void ClothMaterial::use() {
    if (opacity_ < 1.0f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
    }

    ShaderVariant* v = getOrCreateVariant(shaderFlag);
    if (!v || !v->linked) {
        LOGE("ClothMaterial::use() variant not linked, flag=0x%x", shaderFlag);
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

    sh->setVec3("baseColor", baseColor);
    sh->setFloat("roughness", roughness);
    sh->setFloat("ao", ao);
    sh->setVec3("sheenColor", sheenColor);
    sh->setVec3("subsurfaceColor", subsurfaceColor);
    sh->setVec3("camPos", camPos);
    sh->setFloat("uOpacity", opacity_);

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
    sh->setInt("normalMap", 4);
    sh->setInt("roughnessMap", 5);
    sh->setInt("aoMap", 6);
    sh->setInt("sheenColorMap", 7);

    if (shaderFlag & FLAG_SHADOW) {
        sh->setInt("shadowMap", 8);
        sh->setMat4("lightSpaceMatrix", lightSpaceMatrix);
        sh->setFloat("shadowBias", shadowBias);
        sh->setInt("shadowMapSize", shadowMapSize);
    }

    if (albedoTex)     { glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, albedoTex); }
    if (normalTex)     { glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, normalTex); }
    if (roughnessTex)  { glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, roughnessTex); }
    if (aoTex)         { glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, aoTex); }
    if (sheenColorTex) { glActiveTexture(GL_TEXTURE7); glBindTexture(GL_TEXTURE_2D, sheenColorTex); }
    if ((shaderFlag & FLAG_SHADOW) && shadowMap) {
        glActiveTexture(GL_TEXTURE8); glBindTexture(GL_TEXTURE_2D, shadowMap);
    }

    if (shaderFlag & FLAG_SKINNED) uploadBonePalette(v->bonesLoc);
}

const Material::VertexAttrib* ClothMaterial::getVertexAttribs(int& count) {
    static const VertexAttrib attribs[] = {
        {"aPos", 3, 0, 0},
        {"aNormal", 3, 1, 3 * sizeof(float)},
        {"aTexCoords", 2, 2, 6 * sizeof(float)}
    };
    count = 3;
    return attribs;
}

// ---------- setters ----------
void ClothMaterial::setBaseColor(float r, float g, float b)        { baseColor[0]=r; baseColor[1]=g; baseColor[2]=b; }
void ClothMaterial::setRoughness(float r)                          { roughness = r; }
void ClothMaterial::setAO(float a)                                 { ao = a; }
void ClothMaterial::setSheenColor(float r, float g, float b)       { sheenColor[0]=r; sheenColor[1]=g; sheenColor[2]=b; }
void ClothMaterial::setSubsurfaceColor(float r, float g, float b)  {
    subsurfaceColor[0]=r; subsurfaceColor[1]=g; subsurfaceColor[2]=b;
    if (r>0.0f||g>0.0f||b>0.0f) shaderFlag |= FLAG_SUBSURFACE;
    else                        shaderFlag &= ~FLAG_SUBSURFACE;
}
void ClothMaterial::setAlbedoMap(GLuint t)     { albedoTex=t;     if (t) shaderFlag|=FLAG_ALBEDO_MAP; }
void ClothMaterial::setNormalMap(GLuint t)     { normalTex=t;     if (t) shaderFlag|=FLAG_NORMAL_MAP; }
void ClothMaterial::setRoughnessMap(GLuint t)  { roughnessTex=t;  if (t) shaderFlag|=FLAG_ROUGHNESS_MAP; }
void ClothMaterial::setAOMap(GLuint t)         { aoTex=t;         if (t) shaderFlag|=FLAG_AO_MAP; }
void ClothMaterial::setSheenColorMap(GLuint t) { sheenColorTex=t; if (t) shaderFlag|=FLAG_SHEEN_COLOR_MAP; }
void ClothMaterial::setCamPos(float x, float y, float z)           { camPos[0]=x; camPos[1]=y; camPos[2]=z; }
void ClothMaterial::setModelMatrix(const float* m)      { for (int i=0;i<16;i++) modelMatrix[i]      = m[i]; }
void ClothMaterial::setProjectionMatrix(const float* p) { for (int i=0;i<16;i++) projectionMatrix[i] = p[i]; }
void ClothMaterial::setViewMatrix(const float* v)       { for (int i=0;i<16;i++) viewMatrix[i]       = v[i]; }
void ClothMaterial::setLight(int idx, const float* pos, const float* color) {
    if (idx<0||idx>=4) return;
    lightPositions[idx][0]=pos[0]; lightPositions[idx][1]=pos[1]; lightPositions[idx][2]=pos[2];
    lightColors[idx][0]=color[0];  lightColors[idx][1]=color[1];  lightColors[idx][2]=color[2];
}
void ClothMaterial::setPrevViewMatrix(const float* v)       { for (int i=0;i<16;i++) prevViewMatrix[i] = v[i]; }
void ClothMaterial::setPrevProjectionMatrix(const float* p) { for (int i=0;i<16;i++) prevProjMatrix[i] = p[i]; }
void ClothMaterial::setPrevModelMatrix(const float* m)      { for (int i=0;i<16;i++) prevModelMatrix[i] = m[i]; }
void ClothMaterial::setScreenSize(int w, int h) { screenWidth = w; screenHeight = h; }
void ClothMaterial::setFrameCount(int count)    { frameIndex = count; }
void ClothMaterial::setOpacity(float opacity) {
    opacity_ = opacity;
    if (opacity < 1.0f) shaderFlag |= FLAG_TRANSPARENT;
    else                shaderFlag &= ~FLAG_TRANSPARENT;
}
void ClothMaterial::onPostRender() {
    if (opacity_ < 1.0f) { glDisable(GL_BLEND); glDepthMask(GL_TRUE); }
}

// ============================================================================
// Vertex shader — identical layout/outputs to IBLPBRMaterial so the engine
// pipeline (TAA velocity buffer, jitter, skinning) works unchanged.
// ============================================================================
std::string ClothMaterial::buildVS(int flag) {
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

// ============================================================================
// Fragment shader — Charlie sheen + Neubelt V + wrap diffuse.
// Charlie: D(h) = (2 + 1/r) * pow(sin^2(theta_h), 1/(2r)) / (2 PI)
// Neubelt: V(NoL,NoV) = 1 / (4 * (NoL + NoV - NoL*NoV))
// Wrap diffuse: ((NoL + w) / (1 + w)^2) for soft thin-fabric look.
// IBL specular reuses the engine's GGX prefilter map; for cloth we drop the
// fresnel term and just modulate by sheenColor (Filament's documented
// approximation), which gives the correct rim glow without needing a
// dedicated cloth LUT.
// ============================================================================
std::string ClothMaterial::buildFS(int flag) {
    std::string fs;
    fs.reserve(4096);
    fs = R"(#version 300 es
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

uniform vec3 baseColor;
uniform float roughness;
uniform float ao;
uniform vec3 sheenColor;
uniform vec3 subsurfaceColor;
uniform vec3 camPos;
uniform float uOpacity;

uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform sampler2D sheenColorMap;

const float PI = 3.14159265359;

float D_Charlie(float r, float NoH) {
    float invR = 1.0 / max(r, 0.001);
    float cos2h = NoH * NoH;
    float sin2h = max(1.0 - cos2h, 0.0078125);
    return (2.0 + invR) * pow(sin2h, invR * 0.5) / (2.0 * PI);
}

float V_Neubelt(float NoV, float NoL) {
    return clamp(1.0 / (4.0 * (NoL + NoV - NoL * NoV)), 0.0, 1.0);
}

vec3 Fd_Wrap(vec3 albedo, vec3 ssTint, float NoL) {
    // wrap term softens N.L so the unlit side is not pitch-black.
    float w = 0.5;
    float wrap = max((NoL + w) / ((1.0 + w) * (1.0 + w)), 0.0);
    vec3 d = albedo / PI * wrap;
    // cheap subsurface: mix in tint based on inverted N.L
    float backLight = clamp(1.0 - NoL, 0.0, 1.0);
    return d + ssTint * backLight * 0.3183;
}
)";

    if (flag & FLAG_NORMAL_MAP) {
        fs += R"(
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

    if (flag & FLAG_SHADOW) {
        fs += R"(
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
        projCoords.y < 0.0 || projCoords.y > 1.0) return 1.0;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z - shadowBias;
    float shadow = (currentDepth > closestDepth) ? 0.0 : 1.0;
    return mix(1.0, shadow, 0.7);
}
)";
    }

    fs += R"(
void main() {
    vec3 N = normalize(Normal);
)";
    if (flag & FLAG_NORMAL_MAP) fs += "    N = getNormalFromMap();\n";

    fs += R"(
    vec3 V = normalize(camPos - WorldPos);
    float NoV = max(dot(N, V), 0.0001);

    vec3 mapAlbedo = baseColor;
    float mapRough = clamp(roughness, 0.07, 1.0);
    float mapAO    = ao;
    vec3 mapSheen  = sheenColor;
)";
    if (flag & FLAG_ALBEDO_MAP)
        fs += "    mapAlbedo = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));\n";
    if (flag & FLAG_ROUGHNESS_MAP)
        fs += "    mapRough = clamp(texture(roughnessMap, TexCoords).r, 0.07, 1.0);\n";
    if (flag & FLAG_AO_MAP)
        fs += "    mapAO = texture(aoMap, TexCoords).r;\n";
    if (flag & FLAG_SHEEN_COLOR_MAP)
        fs += "    mapSheen *= texture(sheenColorMap, TexCoords).rgb;\n";

    fs += R"(
    // ---- Direct lighting ----
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < 4; ++i) {
        vec3 L = normalize(lightPositions[i] - WorldPos);
        vec3 H = normalize(V + L);
        float dist = length(lightPositions[i] - WorldPos);
        float atten = 1.0 / (dist * dist);
        vec3 radiance = lightColors[i] * atten;

        float NoL = max(dot(N, L), 0.0);
        float NoH = max(dot(N, H), 0.0);

        // Cloth specular: Charlie D + Neubelt V, modulated by sheenColor.
        float D = D_Charlie(mapRough, NoH);
        float Vt = V_Neubelt(NoV, NoL);
        vec3 Fr = mapSheen * (D * Vt);

        // Wrap diffuse + cheap subsurface.
        vec3 Fd = Fd_Wrap(mapAlbedo, subsurfaceColor, NoL);

        Lo += (Fd + Fr) * radiance * NoL;
    }

    // ---- IBL ----
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuseIBL = irradiance * mapAlbedo;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 R = reflect(-V, N);
    vec3 prefilteredColor = textureLod(prefilterMap, R, mapRough * MAX_REFLECTION_LOD).rgb;
    vec2 brdf2 = texture(brdfLUT, vec2(NoV, mapRough)).xy;
    // Filament cloth IBL approximation: drop fresnel, modulate by sheen tint.
    vec3 specIBL = prefilteredColor * mapSheen * (brdf2.x + brdf2.y);

    // Cloth AO: brighten facing parts, darken grazing crevices a bit less
    // than standard PBR — gives the soft fabric look.
    float clothAO = mix(mapAO, 1.0, pow(NoV, 0.5));
    vec3 ambient = (diffuseIBL + specIBL) * clothAO;

    vec3 color = ambient + Lo;
)";

    if (flag & FLAG_SHADOW) {
        fs += R"(
    float shadow = calculateShadow(WorldPos);
    color *= shadow;
)";
    }

    fs += R"(
    FragColor = vec4(color, uOpacity);
    if (uOpacity < 1.0) {
        Velocity = vec2(0.0);
    } else {
        Velocity = (vNowNDC - vPrevNDC) * 0.5;
    }
}
)";
    return fs;
}
