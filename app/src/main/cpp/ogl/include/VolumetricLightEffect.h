#pragma once

#include "PostEffectBase.h"

// Volumetric Light (god rays) via screen-space ray-marching.
//
// Combines:
//   * scene depth (reconstructs world position per pixel)
//   * shadow map (decides whether each sample point is lit)
//   * Henyey-Greenstein phase function (directional scattering)
//
// Pipeline placement: AFTER fog/TAA/DoF/Bloom, BEFORE tonemap.
// (so the result is HDR and gets tonemapped naturally)
//
// Works WITHOUT a shadow map (uUseShadow = false): degrades into a directional
// fog that brightens toward the sun — still useful, just no light shafts.
class VolumetricLightEffect : public PostEffectBase {
public:
    VolumetricLightEffect();
    ~VolumetricLightEffect() override;

    void init(int w, int h) override;
    RenderContext render(const RenderContext& input) override;
    void release() override;
    PostEffectType getType() const override { return PostEffectType::Unknown; }

    // --- Per-frame inputs (set by Scene before render) ---
    void setInvViewProj(const float* m16)        { for (int i = 0; i < 16; i++) invViewProj[i] = m16[i]; }
    void setLightSpaceMatrix(const float* m16)   { for (int i = 0; i < 16; i++) lightSpaceMatrix[i] = m16[i]; }
    void setCameraPos(float x, float y, float z) { camPos[0] = x; camPos[1] = y; camPos[2] = z; }
    // lightDir = normalized direction the light TRAVELS (i.e. from sun toward scene).
    void setLightDir(float x, float y, float z)  { lightDir[0] = x; lightDir[1] = y; lightDir[2] = z; }
    void setLightColor(float r, float g, float b){ lightColor[0] = r; lightColor[1] = g; lightColor[2] = b; }
    void setShadowMap(GLuint tex)                { shadowMap = tex; }
    void setUseShadow(bool b)                    { useShadow = b; }

    // --- Quality / look knobs ---
    void setDensity(float d)        { density = d; }
    void setScattering(float g)     { scattering = g; }   // HG g, [0..0.95]
    void setSteps(int s)            { steps = s; }
    void setMaxDistance(float d)    { maxDistance = d; }
    void setIntensity(float i)      { intensity = i; }

private:
    void initFBO(int w, int h);

    static const char* VERTEX_SHADER;
    static const char* FRAGMENT_SHADER;

    // Uniforms / state
    float invViewProj[16]      = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    float lightSpaceMatrix[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    float camPos[3]    = {0.0f, 0.0f, 0.0f};
    float lightDir[3]  = {0.0f, -1.0f, 0.0f};
    float lightColor[3] = {1.0f, 0.95f, 0.85f};
    GLuint shadowMap = 0;
    bool   useShadow = true;

    float density     = 0.03f;
    float scattering  = 0.75f;
    int   steps       = 48;
    float maxDistance = 30.0f;
    float intensity   = 1.0f;

    // GL objects
    GLuint program       = 0;
    GLint  loc_color     = -1;
    GLint  loc_depth     = -1;
    GLint  loc_shadowMap = -1;
    GLint  loc_invVP     = -1;
    GLint  loc_lightSpace = -1;
    GLint  loc_camPos    = -1;
    GLint  loc_lightDir  = -1;
    GLint  loc_lightColor = -1;
    GLint  loc_useShadow = -1;
    GLint  loc_density   = -1;
    GLint  loc_scattering = -1;
    GLint  loc_steps     = -1;
    GLint  loc_maxDist   = -1;
    GLint  loc_intensity = -1;
    GLint  loc_frame     = -1;

    GLuint outputTex = 0;
    GLuint outputFBO = 0;
    GLuint quadVAO   = 0;
    GLuint quadVBO   = 0;

    int frameCounter = 0;
};
