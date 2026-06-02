#pragma once

#include "PostEffectBase.h"
#include <vector>

class TAAEffect : public PostEffectBase {
public:
    TAAEffect();
    ~TAAEffect();

    void init(int w, int h) override;
    RenderContext render(const RenderContext& input) override;
    void release() override;
    PostEffectType getType() const override { return PostEffectType::TAA; }

private:
    void initFBO(int w, int h);

    struct Program {
        GLuint prog = 0;
        GLint loc_screenWidth = -1;
        GLint loc_screenHeight = -1;
        GLint loc_frameCount = -1;
        GLint loc_currentColor = -1;
        GLint loc_previousColor = -1;
        GLint loc_velocityTexture = -1;
        GLint loc_currentDepth = -1;
    };

    Program program;

    std::vector<GLuint> historyFBO;
    std::vector<GLuint> historyTex;
    int historyIndex = 0;
    int frameCount = 0;

    GLuint displayTex = 0;
    GLuint displayFBO = 0;



    GLuint quadVAO = 0;
    GLuint quadVBO = 0;

    static const char* VERTEX_SHADER;
    static const char* FRAGMENT_SHADER;
};
