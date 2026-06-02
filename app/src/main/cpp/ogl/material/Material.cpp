#include "../include/Material.h"
#include "../include/Shader.h"
#include "../model/Animator.h"
#include <GLES3/gl3.h>
#include <glm/gtc/type_ptr.hpp>
#include <string.h>

Material::Material() : shader(nullptr), camera(nullptr), initialized(false) {
    for (int i = 0; i < 16; i++) {
        prevViewMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        prevProjMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        prevModelMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
    jitterOffset[0] = jitterOffset[1] = 0.0f;
}

Material::~Material() {
    if (shader != nullptr) {
        delete shader;
        shader = nullptr;
    }
}

void Material::setPrevViewMatrix(const float* v) {
    for (int i = 0; i < 16; i++) prevViewMatrix[i] = v[i];
}

void Material::setPrevProjectionMatrix(const float* p) {
    for (int i = 0; i < 16; i++) prevProjMatrix[i] = p[i];
}

void Material::setPrevModelMatrix(const float* m) {
    for (int i = 0; i < 16; i++) prevModelMatrix[i] = m[i];
}

void Material::setJitterOffset(const float* offset) {
    jitterOffset[0] = offset[0];
    jitterOffset[1] = offset[1];
}

void Material::setScreenSize(int w, int h) {
    screenWidth = w;
    screenHeight = h;
}

void Material::setFrameCount(int count) {
    frameIndex = count;
}

void Material::setPrevMVP(const float* mvp, const float* model) {
    for (int i = 0; i < 16; i++) {
        prevProjMatrix[i] = mvp[i];
        prevModelMatrix[i] = model[i];
    }
    for (int i = 0; i < 16; i++) prevViewMatrix[i] = 0.0f;
    prevViewMatrix[0] = prevViewMatrix[5] = prevViewMatrix[10] = prevViewMatrix[15] = 1.0f;
}

// ---------------------------------------------------------------------------
// Skeletal animation helpers (shared by every concrete Material that opts in
// via its own SKINNED shader-flag bit).
// ---------------------------------------------------------------------------
const char* Material::skinningVSChunk(bool enabled) {
    if (enabled) {
        // Declares bone attributes + 100-mat4 palette; provides skinPos /
        // skinNormal helpers. The mesh VAO must expose attributes 4 & 5
        // (Mesh::upload() sets them up automatically when bone data is
        // present).
        return R"(
layout(location=4) in ivec4 aBoneIds;
layout(location=5) in vec4  aBoneWeights;
uniform mat4 uBones[100];
mat4 _skinMatrix() {
    float ws = aBoneWeights.x + aBoneWeights.y + aBoneWeights.z + aBoneWeights.w;
    if (ws < 1e-4) return mat4(1.0);
    return aBoneWeights.x * uBones[aBoneIds.x]
         + aBoneWeights.y * uBones[aBoneIds.y]
         + aBoneWeights.z * uBones[aBoneIds.z]
         + aBoneWeights.w * uBones[aBoneIds.w];
}
vec4 skinPos(vec4 p)    { return _skinMatrix() * p; }
vec3 skinNormal(vec3 n) { return mat3(_skinMatrix()) * n; }
)";
    }
    // Identity stubs so the same VS body works without skinning enabled.
    return R"(
vec4 skinPos(vec4 p)    { return p; }
vec3 skinNormal(vec3 n) { return n; }
)";
}

void Material::uploadBonePalette(GLint locBones) const {
    if (!animator || locBones < 0) return;
    const auto& mats = animator->getBoneMatrices();
    int n = (int)mats.size();
    if (n <= 0) return;
    if (n > MAX_BONES_UNIFORM) n = MAX_BONES_UNIFORM;
    glUniformMatrix4fv(locBones, n, GL_FALSE, glm::value_ptr(mats[0]));
}
