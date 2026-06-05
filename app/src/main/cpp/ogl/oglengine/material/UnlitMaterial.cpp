#include "../include/UnlitMaterial.h"
#include "../include/Shader.h"
#include "../include/Common.h"
#include <GLES3/gl3.h>
#include <string.h>
#include <vector>

UnlitMaterial::UnlitMaterial() {
    color[0] = 1.0f; color[1] = 1.0f; color[2] = 1.0f; color[3] = 1.0f;
    for (int i = 0; i < 16; i++) lightSpaceMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
}

UnlitMaterial::~UnlitMaterial() {
    release();
}

void UnlitMaterial::init() {
    if (initialized) return;
    initialized = true;
    (void)getOrCreateVariant(0);
    LOGI("UnlitMaterial init done, %zu variants", variants.size());
}

void UnlitMaterial::release() {
    for (auto& kv : variants) {
        if (kv.second->shader) {
            kv.second->shader->release();
            delete kv.second->shader;
        }
        delete kv.second;
    }
    variants.clear();
    initialized = false;
}

UnlitMaterial::ShaderVariant* UnlitMaterial::getOrCreateVariant(int flag) {
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
                LOGE("UnlitMaterial variant 0x%x link log: %s", flag, log.data());
            }
        }
    }
    ShaderVariant* v = new ShaderVariant();
    v->shader = s;
    v->linked = ok && linked;
    v->modelLoc       = s->getUniformLocation("uModelMatrix");
    v->viewLoc        = s->getUniformLocation("uViewMatrix");
    v->projLoc        = s->getUniformLocation("uProjectionMatrix");
    v->prevModelLoc   = s->getUniformLocation("uPrevModelMatrix");
    v->prevViewLoc    = s->getUniformLocation("uPrevViewMatrix");
    v->prevProjLoc    = s->getUniformLocation("uPrevProjectionMatrix");
    v->jitterLoc      = s->getUniformLocation("uJitterOffset");
    v->colorLoc       = s->getUniformLocation("uColor");
    v->baseColorMapLoc      = (flag & FLAG_BASE_COLOR_MAP) ? s->getUniformLocation("uBaseColorMap") : -1;
    v->shadowMapLoc         = (flag & FLAG_SHADOW) ? s->getUniformLocation("uShadowMap") : -1;
    v->lightSpaceMatrixLoc  = (flag & FLAG_SHADOW) ? s->getUniformLocation("uLightSpaceMatrix") : -1;
    v->shadowBiasLoc        = (flag & FLAG_SHADOW) ? s->getUniformLocation("uShadowBias") : -1;
    v->shadowIntensityLoc   = (flag & FLAG_SHADOW) ? s->getUniformLocation("uShadowIntensity") : -1;
    v->shadowMapTexelLoc    = (flag & FLAG_SHADOW_PCF) ? s->getUniformLocation("uShadowMapTexel") : -1;
    v->bonesLoc             = (flag & FLAG_SKINNED) ? s->getUniformLocation("uBones[0]") : -1;
    variants[flag] = v;
    LOGI("UnlitMaterial variant 0x%x: linked=%d", flag, v->linked ? 1 : 0);
    return v;
}

void UnlitMaterial::setTransform(const float* model, const float* view, const float* projection) {
    if (!initialized) return;
    ShaderVariant* v = getOrCreateVariant(shaderFlag);
    if (!v || !v->linked) return;
    v->shader->use();
    glUniformMatrix4fv(v->modelLoc,     1, GL_FALSE, model);
    glUniformMatrix4fv(v->viewLoc,      1, GL_FALSE, view);
    glUniformMatrix4fv(v->projLoc,      1, GL_FALSE, projection);
    glUniformMatrix4fv(v->prevModelLoc, 1, GL_FALSE, prevModelMatrix);
    glUniformMatrix4fv(v->prevViewLoc,  1, GL_FALSE, prevViewMatrix);
    glUniformMatrix4fv(v->prevProjLoc,  1, GL_FALSE, prevProjMatrix);
    glUniform2fv(v->jitterLoc, 1, jitterOffset);
}

void UnlitMaterial::use() {
    if (!initialized) return;
    ShaderVariant* v = getOrCreateVariant(shaderFlag);
    if (!v || !v->linked) {
        LOGE("UnlitMaterial::use() variant not linked flag=0x%x", shaderFlag);
        return;
    }
    v->shader->use();
    glUniform4f(v->colorLoc, color[0], color[1], color[2], color[3]);

    if (shaderFlag & FLAG_SKINNED) {
        uploadBonePalette(v->bonesLoc);
    }

    if (shaderFlag & FLAG_BASE_COLOR_MAP) {
        glUniform1i(v->baseColorMapLoc, 0);
        if (baseColorTex != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, baseColorTex);
        }
    }

    if (shaderFlag & FLAG_SHADOW) {
        glUniform1i(v->shadowMapLoc, 8);
        glUniformMatrix4fv(v->lightSpaceMatrixLoc, 1, GL_FALSE, lightSpaceMatrix);
        glUniform1f(v->shadowBiasLoc, shadowBias);
        glUniform1f(v->shadowIntensityLoc, shadowIntensity);
        if (shaderFlag & FLAG_SHADOW_PCF) {
            float texel = (shadowMapSize > 0) ? (1.0f / (float)shadowMapSize) : (1.0f / 2048.0f);
            glUniform1f(v->shadowMapTexelLoc, texel);
        }
        if (shadowMap != 0) {
            glActiveTexture(GL_TEXTURE8);
            glBindTexture(GL_TEXTURE_2D, shadowMap);
        }
    }
}

void UnlitMaterial::setColor(float r, float g, float b, float a) {
    color[0] = r; color[1] = g; color[2] = b; color[3] = a;
}

const Material::VertexAttrib* UnlitMaterial::getVertexAttribs(int& count) {
    // UnlitMaterial follows the canonical Mesh vertex layout:
    //   loc 0: position  (vec3)
    //   loc 1: normal    (vec3)  — declared by Mesh, ignored by Unlit shader
    //   loc 2: texCoord  (vec2)  — only used when FLAG_BASE_COLOR_MAP
    //   loc 3: tangent   (vec3)  — declared by Mesh, ignored
    //   loc 4: boneIds   (ivec4) — only used when FLAG_SKINNED
    //   loc 5: weights   (vec4)  — only used when FLAG_SKINNED
    //
    // For non-Mesh callers (Cube/Sphere) only the listed attribs are
    // bound; their tightly-packed VBOs supply position-only or
    // position+UV layouts. When FLAG_BASE_COLOR_MAP is on for those,
    // the caller MUST place UV at offset 12 of a 5-float stride.
    static VertexAttrib attribsNoMap[1] = {
        {"aPosition", 3, 0, 0}
    };
    static VertexAttrib attribsWithMap[2] = {
        {"aPosition", 3, 0, 0},
        {"aTexCoord", 2, 2, 12}   // location 2 to match Mesh layout
    };
    if (shaderFlag & FLAG_BASE_COLOR_MAP) {
        count = 2;
        return attribsWithMap;
    }
    count = 1;
    return attribsNoMap;
}

// ---------------------------------------------------------------------------
// Shader source generation
// ---------------------------------------------------------------------------

std::string UnlitMaterial::buildVS(int flag) {
    std::string vs;
    vs.reserve(1024);
    vs = R"(#version 300 es
precision highp float;
uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;
uniform mat4 uPrevModelMatrix;
uniform mat4 uPrevViewMatrix;
uniform mat4 uPrevProjectionMatrix;
uniform vec2 uJitterOffset;
layout(location = 0) in vec4 aPosition;
)";
    if (flag & FLAG_BASE_COLOR_MAP) {
        // NOTE: location=2 (not 1) so the shader matches the canonical
        // Mesh vertex layout used by ModelLoader / MeshNode:
        //   0=position, 1=normal, 2=texCoord, 3=tangent,
        //   4=boneIds (ivec4), 5=boneWeights (vec4).
        // Cube/Sphere never enable FLAG_BASE_COLOR_MAP, so their compact
        // VBOs continue to work via getVertexAttribs() reporting only loc 0.
        vs += "layout(location = 2) in vec2 aTexCoord;\n";
        vs += "out vec2 vTexCoord;\n";
    }
    vs += Material::skinningVSChunk((flag & FLAG_SKINNED) != 0);
    vs += R"(out vec3 vWorldPos;
flat out vec4 nowScreenPos;
flat out vec4 preScreenPos;
void main() {
    vec4 skinned = skinPos(aPosition);
    vec4 worldPos = uModelMatrix * skinned;
    vWorldPos = worldPos.xyz;
)";
    if (flag & FLAG_BASE_COLOR_MAP) {
        vs += "    vTexCoord = aTexCoord;\n";
    }
    vs += R"(    mat4 p = uProjectionMatrix;
    p[2][0] += uJitterOffset.x;
    p[2][1] += uJitterOffset.y;
    gl_Position = p * uViewMatrix * worldPos;
    nowScreenPos = uProjectionMatrix * uViewMatrix * worldPos;
    vec4 prevWorldPos = uPrevModelMatrix * skinned;
    preScreenPos = uPrevProjectionMatrix * uPrevViewMatrix * prevWorldPos;
}
)";
    return vs;
}

std::string UnlitMaterial::buildFS(int flag) {
    std::string fs;
    fs.reserve(2048);
    fs = R"(#version 300 es
precision highp float;
uniform vec4 uColor;
in vec3 vWorldPos;
flat in vec4 nowScreenPos;
flat in vec4 preScreenPos;
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 velocity;
)";
    if (flag & FLAG_BASE_COLOR_MAP) {
        fs += "uniform sampler2D uBaseColorMap;\n";
        fs += "in vec2 vTexCoord;\n";
    }

    if (flag & FLAG_SHADOW) {
        fs += R"(
uniform sampler2D uShadowMap;
uniform mat4 uLightSpaceMatrix;
uniform float uShadowBias;
uniform float uShadowIntensity;
)";
        if (flag & FLAG_SHADOW_PCF) {
            fs += "uniform float uShadowMapTexel;\n";
            // 5x5 PCF with Gaussian-like weights for smooth result (25 taps)
            fs += R"(
float calculateShadow(vec3 worldPos) {
    vec4 lightSpacePos = uLightSpaceMatrix * vec4(worldPos, 1.0);
    if (abs(lightSpacePos.w) < 0.0001) return 1.0;
    lightSpacePos.xyz /= lightSpacePos.w;
    vec3 projCoords = lightSpacePos.xyz * 0.5 + 0.5;
    if (projCoords.z < 0.0 || projCoords.z > 1.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;
    }
    float currentDepth = projCoords.z - uShadowBias;

    // 5x5 Gaussian-weighted PCF — produces a smooth gradient instead of
    // discrete steps. Weights approximate a sigma~1 Gaussian, normalised to 1.
    const float w0 = 0.0030;
    const float w1 = 0.0133;
    const float w2 = 0.0219;
    const float w3 = 0.0596;
    const float w4 = 0.0983;
    const float w5 = 0.1621;
    float weights[25];
    weights[ 0]=w0; weights[ 1]=w1; weights[ 2]=w2; weights[ 3]=w1; weights[ 4]=w0;
    weights[ 5]=w1; weights[ 6]=w3; weights[ 7]=w4; weights[ 8]=w3; weights[ 9]=w1;
    weights[10]=w2; weights[11]=w4; weights[12]=w5; weights[13]=w4; weights[14]=w2;
    weights[15]=w1; weights[16]=w3; weights[17]=w4; weights[18]=w3; weights[19]=w1;
    weights[20]=w0; weights[21]=w1; weights[22]=w2; weights[23]=w1; weights[24]=w0;

    float lit = 0.0;
    float totalW = 0.0;
    // Spread of 1.5 texels gives gentle penumbra without over-blurring
    float spread = 1.5;
    for (int y = -2; y <= 2; ++y) {
        for (int x = -2; x <= 2; ++x) {
            vec2 off = vec2(float(x), float(y)) * uShadowMapTexel * spread;
            float sampledDepth = texture(uShadowMap, projCoords.xy + off).r;
            float w = weights[(y + 2) * 5 + (x + 2)];
            lit += mix(0.0, w, step(currentDepth, sampledDepth));
            totalW += w;
        }
    }
    float shadow = lit / totalW;  // 0 = full shadow, 1 = fully lit
    return mix(1.0, shadow, uShadowIntensity);
}
)";
        } else {
            // Hard shadow — single tap
            fs += R"(
float calculateShadow(vec3 worldPos) {
    vec4 lightSpacePos = uLightSpaceMatrix * vec4(worldPos, 1.0);
    if (abs(lightSpacePos.w) < 0.0001) return 1.0;
    lightSpacePos.xyz /= lightSpacePos.w;
    vec3 projCoords = lightSpacePos.xyz * 0.5 + 0.5;
    if (projCoords.z < 0.0 || projCoords.z > 1.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;
    }
    float closestDepth = texture(uShadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z - uShadowBias;
    float shadow = (currentDepth > closestDepth) ? 0.0 : 1.0;
    return mix(1.0, shadow, uShadowIntensity);
}
)";
        }
    }

    fs += R"(
void main() {
    vec3 finalColor = uColor.rgb;
)";
    if (flag & FLAG_BASE_COLOR_MAP) {
        fs += "    finalColor *= texture(uBaseColorMap, vTexCoord).rgb;\n";
    }
    if (flag & FLAG_SHADOW) {
        fs += "    float shadow = calculateShadow(vWorldPos);\n";
        fs += "    finalColor *= shadow;\n";
    }
    fs += R"(
    fragColor = vec4(finalColor, uColor.a);
    vec2 newPos = ((nowScreenPos.xy / nowScreenPos.w) * 0.5 + 0.5);
    vec2 prePos = ((preScreenPos.xy / preScreenPos.w) * 0.5 + 0.5);
    velocity = newPos - prePos;
}
)";
    return fs;
}
