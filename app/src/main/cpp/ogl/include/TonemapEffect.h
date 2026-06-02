#pragma once

#include "PostEffectBase.h"

// ---------------------------------------------------------------------------
// TonemapEffect — HDR → LDR tone mapping + gamma correction
//
// Operators available:
//   0 = Reinhard          (simple, low contrast)
//   1 = ACES Filmic       (default, cinematic S-curve)
//   2 = Uncharted2/Hejl   (warm, punchy highlights)
//
// Also applies gamma correction (sRGB, 2.2) so downstream passes (FXAA,
// blitToScreen) receive a proper LDR [0,1] sRGB image.
// ---------------------------------------------------------------------------
class TonemapEffect : public PostEffectBase {
public:
    explicit TonemapEffect(int op = 1, float exposure = 1.0f);
    ~TonemapEffect() override;

    void init(int w, int h) override;
    RenderContext render(const RenderContext& input) override;
    void release() override;
    PostEffectType getType() const override { return PostEffectType::Tonemap; }

    // Runtime tuning
    void setOperator(int op)        { tonemapOp = op; }
    void setExposure(float e)       { exposure  = e;  }

private:
    void initFBO(int w, int h);

    static const char* VERTEX_SHADER;
    static const char* FRAGMENT_SHADER;

    int   tonemapOp = 1;
    float exposure  = 1.0f;

    struct {
        GLuint prog         = 0;
        GLint  loc_color    = -1;
        GLint  loc_exposure = -1;
        GLint  loc_op       = -1;
    } program;

    GLuint outputTex = 0;
    GLuint outputFBO = 0;
    GLuint quadVAO   = 0;
    GLuint quadVBO   = 0;
};
