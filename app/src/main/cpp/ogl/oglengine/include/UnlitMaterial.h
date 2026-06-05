#pragma once

#include "Material.h"
#include <cstring>
#include <string>
#include <unordered_map>

class Shader;

class UnlitMaterial : public Material {
public:
    UnlitMaterial();
    ~UnlitMaterial() override;

    void init() override;
    void setTransform(const float* model, const float* view, const float* projection) override;
    void release() override;
    void use() override;

    void setColor(float r, float g, float b, float a);
    float* getColor() { return color; }

    // Base color map (optional). Setting a non-zero texture enables
    // FLAG_BASE_COLOR_MAP and requires the geometry to provide UVs at
    // attribute location 1 (vec2). Pass 0 to disable.
    void setBaseColorMap(GLuint tex) {
        baseColorTex = tex;
        if (tex != 0) shaderFlag |= FLAG_BASE_COLOR_MAP;
        else          shaderFlag &= ~FLAG_BASE_COLOR_MAP;
    }
    GLuint getBaseColorMap() const { return baseColorTex; }

    // Shadow support
    void setShadowMap(GLuint tex) { shadowMap = tex; }
    void setLightSpaceMatrix(const float* mat) {
        if (mat) memcpy(lightSpaceMatrix, mat, 16 * sizeof(float));
    }
    void setShadowEnabled(bool enabled) {
        shadowEnabled = enabled;
        if (enabled) shaderFlag |= FLAG_SHADOW;
        else         shaderFlag &= ~(FLAG_SHADOW | FLAG_SHADOW_PCF);
    }
    void setShadowBias(float bias) { shadowBias = bias; }
    void setShadowPCFEnabled(bool enabled) {
        shadowPCFEnabled = enabled;
        if (enabled) shaderFlag |= FLAG_SHADOW_PCF;
        else         shaderFlag &= ~FLAG_SHADOW_PCF;
    }
    void setShadowIntensity(float intensity) { shadowIntensity = intensity; }
    void setShadowMapSize(int size) { shadowMapSize = size; }
    void setDebugShadowMap(bool enabled) { debugShadowMap = enabled; }

    // Enable GPU skinning. Once enabled, the mesh VAO must expose
    // ivec4 aBoneIds @ loc 4 and vec4 aWeights @ loc 5 (Mesh::upload()
    // does this automatically when bone data is present), and an
    // Animator must be supplied via Material::setAnimator().
    void setSkinned(bool enabled) {
        if (enabled) shaderFlag |= FLAG_SKINNED;
        else         shaderFlag &= ~FLAG_SKINNED;
    }
    bool isSkinned() const { return (shaderFlag & FLAG_SKINNED) != 0; }

    const VertexAttrib* getVertexAttribs(int& count) override;

    enum ShaderFlag {
        FLAG_SHADOW          = 1 << 0,
        FLAG_SHADOW_PCF      = 1 << 1,
        FLAG_BASE_COLOR_MAP  = 1 << 2,
        FLAG_SKINNED         = 1 << 3,
    };

private:
    int shaderFlag = 0;

    struct ShaderVariant {
        Shader* shader;
        bool linked;
        // cached uniform locations
        GLint modelLoc, viewLoc, projLoc;
        GLint prevModelLoc, prevViewLoc, prevProjLoc;
        GLint jitterLoc;
        GLint colorLoc;
        GLint baseColorMapLoc;
        GLint shadowMapLoc, lightSpaceMatrixLoc;
        GLint shadowBiasLoc, shadowIntensityLoc, shadowMapTexelLoc;
        GLint bonesLoc;
    };
    ShaderVariant* getOrCreateVariant(int flag);
    std::unordered_map<int, ShaderVariant*> variants;

    static std::string buildVS(int flag);
    static std::string buildFS(int flag);

    float color[4];

    GLuint baseColorTex = 0;

    GLuint shadowMap = 0;
    float lightSpaceMatrix[16];
    bool shadowEnabled = false;
    float shadowBias = 0.005f;
    bool shadowPCFEnabled = true;
    float shadowIntensity = 0.7f;
    int shadowMapSize = 2048;
    bool debugShadowMap = false;
};
