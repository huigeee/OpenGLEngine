#pragma once

#include "PostEffectBase.h"

class DoFEffect : public PostEffectBase {
public:
    DoFEffect(float focalDist = 0.5f, float aperture = 0.5f, float maxBlur = 20.0f);
    ~DoFEffect() override;

    void init(int w, int h) override;
    RenderContext render(const RenderContext& input) override;
    void release() override;
    PostEffectType getType() const override { return PostEffectType::DoF; }

    void setFocalDistance(float d) { focalDistance = d; }
    void setAperture(float a) { aperture = a; }
    void setMaxBlur(float m) { maxBlur = m; }

private:
    void initFBO(int w, int h);

    static const char* VERTEX_SHADER;
    static const char* FRAGMENT_SHADER;

    float focalDistance;
    float aperture;
    float maxBlur;

    GLuint program = 0;
    GLint loc_color = -1;
    GLint loc_depth = -1;
    GLint loc_focalDist = -1;
    GLint loc_aperture = -1;
    GLint loc_maxBlur = -1;
    GLint loc_texelSize = -1;

    GLuint outputTex = 0;
    GLuint outputFBO = 0;

    GLuint quadVAO = 0;
    GLuint quadVBO = 0;
};
