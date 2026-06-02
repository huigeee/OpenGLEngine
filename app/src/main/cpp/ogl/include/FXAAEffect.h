#pragma once

#include "PostEffectBase.h"

class FXAAEffect : public PostEffectBase {
public:
    FXAAEffect(float contrastThreshold = 0.0312f, float relativeThreshold = 0.063f);
    ~FXAAEffect() override;

    void init(int w, int h) override;
    RenderContext render(const RenderContext& input) override;
    void release() override;
    PostEffectType getType() const override { return PostEffectType::FXAA; }

    void setContrastThreshold(float v) { contrastThreshold = v; }
    void setRelativeThreshold(float v) { relativeThreshold = v; }

private:
    void initFBO(int w, int h);

    static const char* VERTEX_SHADER;
    static const char* FRAGMENT_SHADER;

    float contrastThreshold;
    float relativeThreshold;

    GLuint program = 0;
    GLint loc_color = -1;
    GLint loc_texelSize = -1;
    GLint loc_contrastThreshold = -1;
    GLint loc_relativeThreshold = -1;

    GLuint outputTex = 0;
    GLuint outputFBO = 0;
    GLuint quadVAO = 0;
    GLuint quadVBO = 0;
};
