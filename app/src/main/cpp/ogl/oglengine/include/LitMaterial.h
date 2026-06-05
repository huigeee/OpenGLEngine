#pragma once

#include "Material.h"
#include <cstring>
#include <string>
#include <unordered_map>

class Shader;

class LitMaterial : public Material {
public:
    LitMaterial();
    ~LitMaterial() override;

    void init() override;
    void setTransform(const float* model, const float* view, const float* projection) override;
    void release() override;
    void use() override;

    void setBaseColor(float r, float g, float b, float a);
    void setLightPos(float x, float y, float z);
    void setLightColor(float r, float g, float b);
    float* getBaseColor() { return baseColor; }
    float* getLightPos() { return lightPos; }

    // Base color map (optional). Setting a non-zero texture enables
    // FLAG_BASE_COLOR_MAP; the per-vertex UV at location 2 is already
    // supplied by LitMaterial's vertex layout.
    void setBaseColorMap(GLuint tex) {
        baseColorTex = tex;
        if (tex != 0) shaderFlag |= FLAG_BASE_COLOR_MAP;
        else          shaderFlag &= ~FLAG_BASE_COLOR_MAP;
    }
    GLuint getBaseColorMap() const { return baseColorTex; }

    // Shadow support (mirrors UnlitMaterial/IBLPBRMaterial)
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

    // Enable GPU skinning (see Material::skinningVSChunk for the contract).
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
        GLint modelLoc, viewLoc, projLoc;
        GLint prevModelLoc, prevViewLoc, prevProjLoc;
        GLint jitterLoc;
        GLint baseColorLoc, lightPosLoc, lightColorLoc, camPosLoc;
        GLint baseColorMapLoc;
        GLint shadowMapLoc, lightSpaceMatrixLoc;
        GLint shadowBiasLoc, shadowIntensityLoc, shadowMapTexelLoc;
        GLint bonesLoc;
    };
    ShaderVariant* getOrCreateVariant(int flag);
    std::unordered_map<int, ShaderVariant*> variants;

    static std::string buildVS(int flag);
    static std::string buildFS(int flag);

    float baseColor[4];
    float lightPos[3];
    float lightColor[3];

    GLuint baseColorTex = 0;

    GLuint shadowMap = 0;
    float lightSpaceMatrix[16];
    bool shadowEnabled = false;
    float shadowBias = 0.005f;
    bool shadowPCFEnabled = true;
    float shadowIntensity = 0.7f;
    int shadowMapSize = 2048;
};
