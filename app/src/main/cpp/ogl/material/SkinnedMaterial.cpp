#include "../include/SkinnedMaterial.h"
#include "../include/Shader.h"
#include "../include/Common.h"
#include "../model/Animator.h"
#include "../model/ModelLoader.h"   // MAX_BONES
#include <GLES3/gl3.h>
#include <vector>
#include <glm/gtc/type_ptr.hpp>

SkinnedMaterial::SkinnedMaterial() = default;

SkinnedMaterial::~SkinnedMaterial() {
    release();
}

void SkinnedMaterial::init() {
    if (initialized) return;
    initialized = true;
    std::string vs = buildVS();
    std::string fs = buildFS();
    shaderProg = new Shader();
    bool ok = shaderProg->init(vs.c_str(), fs.c_str());
    GLint linkStatus = 0;
    if (ok) glGetProgramiv(shaderProg->getProgram(), GL_LINK_STATUS, &linkStatus);
    linked = ok && (linkStatus != 0);
    if (!linked) {
        GLint logLen = 0;
        glGetProgramiv(shaderProg->getProgram(), GL_INFO_LOG_LENGTH, &logLen);
        if (logLen > 0) {
            std::vector<char> log(logLen);
            glGetProgramInfoLog(shaderProg->getProgram(), logLen, nullptr, log.data());
            LOGE("SkinnedMaterial link log: %s", log.data());
        }
    }
    locModel       = shaderProg->getUniformLocation("uModelMatrix");
    locView        = shaderProg->getUniformLocation("uViewMatrix");
    locProj        = shaderProg->getUniformLocation("uProjectionMatrix");
    locPrevModel   = shaderProg->getUniformLocation("uPrevModelMatrix");
    locPrevView    = shaderProg->getUniformLocation("uPrevViewMatrix");
    locPrevProj    = shaderProg->getUniformLocation("uPrevProjectionMatrix");
    locJitter      = shaderProg->getUniformLocation("uJitterOffset");
    locColor       = shaderProg->getUniformLocation("uColor");
    locBaseColor   = shaderProg->getUniformLocation("uBaseColorMap");
    locHasSkeleton = shaderProg->getUniformLocation("uHasSkeleton");
    locBones       = shaderProg->getUniformLocation("uBones[0]");
    LOGI("SkinnedMaterial init: linked=%d locBones=%d", linked ? 1 : 0, locBones);
}

void SkinnedMaterial::release() {
    if (shaderProg) {
        shaderProg->release();
        delete shaderProg;
        shaderProg = nullptr;
    }
    initialized = false;
}

void SkinnedMaterial::setTransform(const float* model, const float* view, const float* projection) {
    if (!linked) return;
    shaderProg->use();
    glUniformMatrix4fv(locModel, 1, GL_FALSE, model);
    glUniformMatrix4fv(locView,  1, GL_FALSE, view);
    glUniformMatrix4fv(locProj,  1, GL_FALSE, projection);
    glUniformMatrix4fv(locPrevModel, 1, GL_FALSE, prevModelMatrix);
    glUniformMatrix4fv(locPrevView,  1, GL_FALSE, prevViewMatrix);
    glUniformMatrix4fv(locPrevProj,  1, GL_FALSE, prevProjMatrix);
    glUniform2fv(locJitter, 1, jitterOffset);
}

void SkinnedMaterial::use() {
    if (!linked) return;
    shaderProg->use();
    glUniform4f(locColor, color[0], color[1], color[2], color[3]);

    if (baseColorTex != 0) {
        glUniform1i(locBaseColor, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, baseColorTex);
    }

    if (animator && animator->hasSkeleton()) {
        const auto& mats = animator->getBoneMatrices();
        int n = (int)mats.size();
        if (n > MAX_BONES) n = MAX_BONES;
        if (n > 0 && locBones >= 0) {
            glUniformMatrix4fv(locBones, n, GL_FALSE, glm::value_ptr(mats[0]));
        }
        glUniform1i(locHasSkeleton, 1);
    } else {
        glUniform1i(locHasSkeleton, 0);
    }
}

const Material::VertexAttrib* SkinnedMaterial::getVertexAttribs(int& count) {
    static VertexAttrib attribs[6] = {
        {"aPosition", 3, 0, 0},
        {"aNormal",   3, 1, 12},
        {"aTexCoord", 2, 2, 24},
        {"aTangent",  3, 3, 32},
        {"aBoneIds",  4, 4, 0},
        {"aWeights",  4, 5, 0},
    };
    count = 6;
    return attribs;
}

// ---------------------------------------------------------------------------
// Shader source
// ---------------------------------------------------------------------------
std::string SkinnedMaterial::buildVS() {
    return R"(#version 300 es
precision highp float;
const int MAX_BONES = 100;
uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;
uniform mat4 uPrevModelMatrix;
uniform mat4 uPrevViewMatrix;
uniform mat4 uPrevProjectionMatrix;
uniform vec2 uJitterOffset;
uniform mat4 uBones[MAX_BONES];
uniform int  uHasSkeleton;

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;
layout(location=3) in vec3 aTangent;
layout(location=4) in ivec4 aBoneIds;
layout(location=5) in vec4 aWeights;

out vec2 vTexCoord;
out vec3 vNormal;
out vec3 vWorldPos;
flat out vec4 nowScreenPos;
flat out vec4 preScreenPos;

void main() {
    mat4 skin = mat4(1.0);
    if (uHasSkeleton == 1) {
        float ws = aWeights.x + aWeights.y + aWeights.z + aWeights.w;
        if (ws > 0.001) {
            skin =  aWeights.x * uBones[aBoneIds.x]
                  + aWeights.y * uBones[aBoneIds.y]
                  + aWeights.z * uBones[aBoneIds.z]
                  + aWeights.w * uBones[aBoneIds.w];
        }
    }

    vec4 skinned = skin * vec4(aPosition, 1.0);
    vec4 worldPos = uModelMatrix * skinned;
    vWorldPos = worldPos.xyz;
    vTexCoord = aTexCoord;
    vNormal = mat3(uModelMatrix) * mat3(skin) * aNormal;

    mat4 p = uProjectionMatrix;
    p[2][0] += uJitterOffset.x;
    p[2][1] += uJitterOffset.y;
    gl_Position = p * uViewMatrix * worldPos;

    nowScreenPos = uProjectionMatrix * uViewMatrix * worldPos;
    vec4 prevWorld = uPrevModelMatrix * skinned;
    preScreenPos = uPrevProjectionMatrix * uPrevViewMatrix * prevWorld;
}
)";
}

std::string SkinnedMaterial::buildFS() {
    return R"(#version 300 es
precision highp float;
uniform vec4 uColor;
uniform sampler2D uBaseColorMap;

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vWorldPos;
flat in vec4 nowScreenPos;
flat in vec4 preScreenPos;

layout(location=0) out vec4 fragColor;
layout(location=1) out vec2 velocity;

void main() {
    vec3 base = texture(uBaseColorMap, vTexCoord).rgb;
    vec3 N = normalize(vNormal);
    // Simple half-Lambert against an arbitrary light direction
    vec3 L = normalize(vec3(0.4, 0.8, 0.5));
    float ndl = max(dot(N, L), 0.0) * 0.5 + 0.5;
    vec3 finalColor = base * uColor.rgb * ndl;
    fragColor = vec4(finalColor, uColor.a);

    vec2 newPos = ((nowScreenPos.xy / nowScreenPos.w) * 0.5 + 0.5);
    vec2 prePos = ((preScreenPos.xy / preScreenPos.w) * 0.5 + 0.5);
    velocity = newPos - prePos;
}
)";
}
