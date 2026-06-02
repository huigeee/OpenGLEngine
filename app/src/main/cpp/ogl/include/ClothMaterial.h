#pragma once

#include <GLES3/gl3.h>
#include "Material.h"
#include <unordered_map>
#include <string>
#include <cstring>

// ---------------------------------------------------------------------------
// ClothMaterial — energy-conserving cloth BRDF for fabric / clothing.
//
// Specular lobe replaces standard GGX with the Charlie distribution and
// Neubelt visibility (Filament / UE4-style), producing the soft rim
// sheen typical of cotton, silk, velvet, etc. Diffuse keeps Lambert with
// optional wrap subsurface for thin fabrics.
//
// Texture slots / behaviour mirror IBLPBRMaterial so the rest of the
// engine can drop in a ClothMaterial wherever it expects a PBR material.
// Scene::renderObjects() binds irradiance/prefilter/brdfLUT for it the
// same way it does for IBLPBRMaterial.
// ---------------------------------------------------------------------------

class ClothMaterial : public Material {
public:
    ClothMaterial();
    ~ClothMaterial() override;

    void init() override;
    void setTransform(const float* model, const float* view, const float* projection) override;
    void release() override;
    void use() override;
    bool isIBL() const override { return true; }

    const VertexAttrib* getVertexAttribs(int& count) override;

    // -------- material parameters --------
    void setBaseColor(float r, float g, float b);
    void setRoughness(float r);                // 0.04..1, recommend >= 0.3
    void setAO(float ao);
    void setSheenColor(float r, float g, float b);        // tint of the rim/sheen lobe
    void setSubsurfaceColor(float r, float g, float b);   // wrap-diffuse tint for thin fabric

    // -------- texture maps --------
    void setAlbedoMap(GLuint tex);
    void setNormalMap(GLuint tex);
    void setRoughnessMap(GLuint tex);
    void setAOMap(GLuint tex);
    void setSheenColorMap(GLuint tex);

    void setCamPos(float x, float y, float z);

    // Opacity / transparency
    void setOpacity(float opacity);
    float getOpacity() const override { return opacity_; }
    bool isTransparent() const override { return opacity_ < 1.0f; }
    void onPostRender() override;

    // Transforms
    void setModelMatrix(const float* m);
    void setProjectionMatrix(const float* p);
    void setViewMatrix(const float* v);
    void setLight(int idx, const float* pos, const float* color);
    void setPrevViewMatrix(const float* v);
    void setPrevProjectionMatrix(const float* p);
    void setPrevModelMatrix(const float* m);
    void setScreenSize(int w, int h);
    void setFrameCount(int count);

    // Shadows
    void setShadowMapTexture(GLuint tex) { shadowMap = tex; }
    void setLightSpaceMatrix(const float* mat) { memcpy(lightSpaceMatrix, mat, 16 * sizeof(float)); }
    void setShadowEnabled(bool enabled) {
        shadowEnabled = enabled;
        if (enabled) shaderFlag |= FLAG_SHADOW;
        else         shaderFlag &= ~FLAG_SHADOW;
    }
    void setShadowBias(float bias) { shadowBias = bias; }
    void setShadowMapSize(int size) { shadowMapSize = size; }

    // Skinning — same contract as IBLPBRMaterial / LitMaterial.
    void setSkinned(bool enabled) {
        if (enabled) shaderFlag |= FLAG_SKINNED;
        else         shaderFlag &= ~FLAG_SKINNED;
    }
    bool isSkinned() const { return (shaderFlag & FLAG_SKINNED) != 0; }

    enum ShaderFlag {
        FLAG_ALBEDO_MAP        = 1 << 0,
        FLAG_NORMAL_MAP        = 1 << 1,
        FLAG_ROUGHNESS_MAP     = 1 << 2,
        FLAG_AO_MAP            = 1 << 3,
        FLAG_SHEEN_COLOR_MAP   = 1 << 4,
        FLAG_SUBSURFACE        = 1 << 5,
        FLAG_TRANSPARENT       = 1 << 6,
        FLAG_SHADOW            = 1 << 7,
        FLAG_SKINNED           = 1 << 8,
    };

    int getShaderFlag() const { return shaderFlag; }

private:
    int shaderFlag = 0;
    float opacity_ = 1.0f;

    struct ShaderVariant {
        Shader* shader = nullptr;
        bool linked = false;
        std::string vsSource;
        std::string fsSource;
        GLint bonesLoc = -1;
    };
    ShaderVariant* getOrCreateVariant(int flag);
    std::unordered_map<int, ShaderVariant*> variants;

    float baseColor[3]        = {0.5f, 0.5f, 0.5f};
    float roughness           = 0.5f;
    float ao                  = 1.0f;
    float sheenColor[3]       = {1.0f, 1.0f, 1.0f};
    float subsurfaceColor[3]  = {0.0f, 0.0f, 0.0f};
    float camPos[3]           = {0.0f, 0.0f, 0.0f};

    float modelMatrix[16];
    float projectionMatrix[16];
    float viewMatrix[16];
    float lightPositions[4][3];
    float lightColors[4][3];

    GLuint albedoTex     = 0;
    GLuint normalTex     = 0;
    GLuint roughnessTex  = 0;
    GLuint aoTex         = 0;
    GLuint sheenColorTex = 0;

    GLuint shadowMap     = 0;
    float lightSpaceMatrix[16];
    bool  shadowEnabled  = false;
    float shadowBias     = 0.05f;
    int   shadowMapSize  = 2048;

    static std::string buildVS(int flag);
    static std::string buildFS(int flag);
};
