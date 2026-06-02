#include "../include/DebugQuad.h"
#include "../include/Common.h"

static const char* VERT = R"(
#version 300 es
layout(location = 0) in vec2 aPos;
uniform vec4 uPos;
out vec2 vUV;
void main() {
    vec2 pos = uPos.xy + aPos * uPos.zw;
    vUV = aPos;
    gl_Position = vec4(pos, 0.0, 1.0);
}
)";

static const char* FRAG = R"(
#version 300 es
precision highp float;
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uTex;
uniform int uMode;
void main() {
    vec4 c = texture(uTex, vUV);
    if (uMode == 0) {
        vec2 v = c.rg * 50.0;
        FragColor = vec4(v * 0.5 + 0.5, 0.0, 1.0);
    } else if (uMode == 1) {
        float d = c.r;
        FragColor = vec4(d, d, d, 1.0);
    } else if (uMode == 2) {
        FragColor = vec4(c.rg * 0.5 + 0.5, 0.0, 1.0);
    } else if (uMode == 3) {
        vec2 v = c.rg * 500.0;
        FragColor = vec4(v * 0.5 + 0.5, 0.0, 1.0);
    }
}
)";

static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}

void DebugQuad::init() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, VERT);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, FRAG);
    prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    loc_tex = glGetUniformLocation(prog, "uTex");
    loc_pos = glGetUniformLocation(prog, "uPos");
    loc_mode = glGetUniformLocation(prog, "uMode");

    float quad[] = {0.0f,0.0f, 1.0f,0.0f, 0.0f,1.0f, 1.0f,1.0f};
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);

    LOGI("DebugQuad init: prog=%d", prog);
}

void DebugQuad::render(GLuint texture, float x, float y, float w, float h, int mode) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glUseProgram(prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(loc_tex, 0);
    glUniform4f(loc_pos, x, y, w, h);
    glUniform1i(loc_mode, mode);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void DebugQuad::release() {
    if (prog) { glDeleteProgram(prog); prog = 0; }
    if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
    if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
}
