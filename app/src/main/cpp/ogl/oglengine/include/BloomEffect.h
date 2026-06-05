#pragma once

#include "PostEffectBase.h"
#include <vector>

class BloomEffect : public PostEffectBase {
public:
    BloomEffect(float threshold = 1.0f, float intensity = 1.0f, int iterations = 4);
    ~BloomEffect() override;

    void init(int w, int h) override;
    RenderContext render(const RenderContext& input) override;
    void release() override;
    PostEffectType getType() const override { return PostEffectType::Bloom; }

    void setThreshold(float t) { threshold = t; }
    void setIntensity(float i) { intensity = i; }
    void setIterations(int n) { iterations = n; }

private:
    void initFBOs(int w, int h);
    static const char* getQuadVS();

    float threshold;
    float intensity;
    int iterations;

    GLuint brightProgram = 0;
    GLuint blurProgram = 0;
    GLuint compositeProgram = 0;

    GLint loc_brightThreshold = -1;
    GLint loc_brightColor = -1;
    GLint loc_blurColor = -1;
    GLint loc_blurDir = -1;
    GLint loc_compositeColor = -1;
    GLint loc_compositeBloom = -1;
    GLint loc_compositeIntensity = -1;

    GLuint quadVAO = 0;
    GLuint quadVBO = 0;

    std::vector<GLuint> blurFBO;
    std::vector<GLuint> blurTex;
    int blurW = 0;
    int blurH = 0;

    GLuint compositeFBO = 0;
    GLuint compositeTex = 0;
};
