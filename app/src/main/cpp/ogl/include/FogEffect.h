#pragma once

#include "PostEffectBase.h"

class FogEffect : public PostEffectBase {
public:
    // mode: 0=linear  1=exponential  2=exponential squared
    FogEffect(int mode = 1,
              float density = 0.15f,
              float fogStart = 1.0f,
              float fogEnd   = 8.0f,
              float fogR = 0.7f, float fogG = 0.75f, float fogB = 0.8f);
    ~FogEffect() override;

    void init(int w, int h) override;
    RenderContext render(const RenderContext& input) override;
    void release() override;
    PostEffectType getType() const override { return PostEffectType::Fog; }

    void setMode(int m)    { mode = m; }
    void setDensity(float d) { density = d; }
    void setFogStart(float s) { fogStart = s; }
    void setFogEnd(float e)   { fogEnd = e; }
    void setFogColor(float r, float g, float b) { fogR=r; fogG=g; fogB=b; }
    void setInvViewProj(const float* m16) { for(int i=0;i<16;i++) invViewProj[i]=m16[i]; }

private:
    void initFBO(int w, int h);

    static const char* VERTEX_SHADER;
    static const char* FRAGMENT_SHADER;

    int   mode;
    float density;
    float fogStart, fogEnd;
    float fogR, fogG, fogB;
    float invViewProj[16] = {
        1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
    };
    float time = 0.0f;

    GLuint program        = 0;
    GLint  loc_color      = -1;
    GLint  loc_depth      = -1;
    GLint  loc_mode       = -1;
    GLint  loc_density    = -1;
    GLint  loc_fogStart   = -1;
    GLint  loc_fogEnd     = -1;
    GLint  loc_fogColor   = -1;
    GLint  loc_time       = -1;
    GLint  loc_invViewProj = -1;

    GLuint outputTex = 0;
    GLuint outputFBO = 0;
    GLuint quadVAO   = 0;
    GLuint quadVBO   = 0;
};
