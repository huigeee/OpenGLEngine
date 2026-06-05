#pragma once

#include <GLES3/gl3.h>

struct RenderContext {
    GLuint colorTex = 0;
    GLuint depthTex = 0;
    GLuint velocityTex = 0;
    GLuint fbo = 0;
    int width = 0;
    int height = 0;
    bool isScreen = true;
};
