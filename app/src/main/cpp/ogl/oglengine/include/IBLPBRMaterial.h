#pragma once

#include <GLES3/gl3.h>
#include "Material.h"
#include <unordered_map>
#include <string>

class IBLPBRMaterial : public Material {
public:
    IBLPBRMaterial();
    ~IBLPBRMaterial();

    void init() override;
    void setTransform(const float* model, const float* view, const float* projection) override;
    void release() override;
    void use() override;
    bool isIBL() const override { return true; }

    const VertexAttrib* getVertexAttribs(int& count) override;

    void setBaseColor(float r, float g, float b);
    void setMetallic(float m);
    void setRoughness(float r);
    void setAO(float ao);
    void setClearCoat(float intensity, float roughness);
    void setAlbedoMap(GLuint tex);
    void setMetallicMap(GLuint tex);
    void setRoughnessMap(GLuint tex);
    void setAOMap(GLuint tex);
    void setNormalMap(GLuint tex);
    void setCamPos(float x, float y, float z);

    // Opacity / transparency support
    void setOpacity(float opacity);
    float getOpacity() const override { return opacity_; }
    bool isTransparent() const override { return opacity_ < 1.0f; }
    void onPostRender() override;

    void setModelMatrix(const float* m);
    void setProjectionMatrix(const float* p);
    void setViewMatrix(const float* v);
    void setLight(int idx, const float* pos, const float* color);
    void setPrevViewMatrix(const float* v);
    void setPrevProjectionMatrix(const float* p);
    void setPrevModelMatrix(const float* m);
    void setScreenSize(int w, int h);
    void setFrameCount(int count);
    int frameIndex = 0;
    
    // Shadow support
    void setShadowMapTexture(GLuint tex) { shadowMap = tex; }
    void setLightSpaceMatrix(const float* mat) { memcpy(lightSpaceMatrix, mat, 16 * sizeof(float)); }
    void setShadowEnabled(bool enabled) {
        shadowEnabled = enabled;
        if (enabled) shaderFlag |= FLAG_SHADOW;
        else         shaderFlag &= ~FLAG_SHADOW;
    }
    void setShadowBias(float bias) { shadowBias = bias; }
    void setShadowMapSize(int size) { shadowMapSize = size; }

    // Enable GPU skinning. See Material::skinningVSChunk for the contract.
    void setSkinned(bool enabled) {
        if (enabled) shaderFlag |= FLAG_SKINNED;
        else         shaderFlag &= ~FLAG_SKINNED;
    }
    bool isSkinned() const { return (shaderFlag & FLAG_SKINNED) != 0; }

    enum ShaderFlag {
        FLAG_CLEARCOAT     = 1 << 0,
        FLAG_ALBEDO_MAP    = 1 << 1,
        FLAG_METALLIC_MAP  = 1 << 2,
        FLAG_ROUGHNESS_MAP = 1 << 3,
        FLAG_AO_MAP        = 1 << 4,
        FLAG_NORMAL_MAP    = 1 << 5,
        FLAG_TRANSPARENT   = 1 << 6,
        FLAG_SHADOW        = 1 << 7,
        FLAG_SKINNED       = 1 << 8,
    };

    int getShaderFlag() const { return shaderFlag; }

private:
    int shaderFlag = 0;
    float opacity_ = 1.0f;

    struct ShaderVariant {
        Shader* shader;
        bool linked;
        std::string vsSource;
        std::string fsSource;
        GLint bonesLoc = -1;
    };
    ShaderVariant* getOrCreateVariant(int flag);
    std::unordered_map<int, ShaderVariant*> variants;

    float baseColor[3];
    float metallic;
    float roughness;
    float ao;
    float clearCoat;
    float clearCoatRoughness;
    float camPos[3];
    float modelMatrix[16];
    float projectionMatrix[16];
    float viewMatrix[16];
    float lightPositions[4][3];
    float lightColors[4][3];

    GLuint albedoTex = 0;
    GLuint metallicTex = 0;
    GLuint roughnessTex = 0;
    GLuint aoTex = 0;
    GLuint normalTex = 0;
    GLuint shadowMap = 0;
    float lightSpaceMatrix[16];
    bool shadowEnabled = false;
    float shadowBias = 0.05f;  // Increased significantly for visibility
    int shadowMapSize = 2048;

    static std::string buildVS(int flags);
    static std::string buildFS(int flags);
    static const char* getBaseVS();
    static const char* getBaseFS();
    static const char* getClearCoatChunk();
    static const char* getAlbedoMapChunk();
    static const char* getMetallicMapChunk();
    static const char* getRoughnessMapChunk();
    static const char* getAOMapChunk();
    static const char* getNormalMapChunk();
    static const char* getShadowChunk();
};
