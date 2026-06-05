#pragma once

#include <GLES3/gl3.h>

class DebugQuad {
public:
    void init();
    void render(GLuint texture, float x, float y, float w, float h, int mode);
    void release();

private:
    GLuint prog = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLint loc_tex = -1;
    GLint loc_pos = -1;
    GLint loc_mode = -1;
};
