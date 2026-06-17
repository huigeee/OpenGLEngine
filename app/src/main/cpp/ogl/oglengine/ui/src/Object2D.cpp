#include "../include/Object2D.h"
#include "../include/UIGroup.h"
#include "../include/UIRenderer.h"
#include "../../include/Common.h"
#include <GLES3/gl3.h>


// 2D 渲染使用的简单 shader
static const char* kQuadVertShader = R"(
#version 300 es
in vec2 aPos;
in vec2 aUV;
out vec2 vUV;
uniform mat4 uMVP;
void main() {
    gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
    vUV = aUV;
}
)";

static const char* kQuadFragShader = R"(
#version 300 es
precision mediump float;
in vec2 vUV;
uniform sampler2D uTex;
uniform vec4 uColor;
uniform bool uUseTexture;
layout(location = 0) out vec4 fragColor;
void main() {
    vec4 c = uColor;
    if (uUseTexture) {
        c *= texture(uTex, vUV);
    }
    fragColor = c;
}
)";

static GLuint g_quadProgram = 0;
static GLuint g_quadVAO = 0;
static GLuint g_quadVBO = 0;

static GLint g_locPos = -1;
static GLint g_locUV = -1;
static GLint g_locColor = -1;
static GLint g_locProj = -1;
static GLint g_locTex = -1;
static GLint g_locUseTex = -1;

static void ensureQuadProgram() {
    if (g_quadProgram) return;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &kQuadVertShader, nullptr);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &kQuadFragShader, nullptr);
    glCompileShader(fs);

    g_quadProgram = glCreateProgram();
    glAttachShader(g_quadProgram, vs);
    glAttachShader(g_quadProgram, fs);
    glLinkProgram(g_quadProgram);

    glDeleteShader(vs);
    glDeleteShader(fs);

    g_locPos = glGetAttribLocation(g_quadProgram, "aPos");
    g_locUV = glGetAttribLocation(g_quadProgram, "aUV");
    g_locColor = glGetUniformLocation(g_quadProgram, "uColor");
    g_locProj = glGetUniformLocation(g_quadProgram, "uMVP");
    g_locTex = glGetUniformLocation(g_quadProgram, "uTex");
    g_locUseTex = glGetUniformLocation(g_quadProgram, "uUseTexture");

    // VBO / VAO — flat unit quad
    float verts[] = {
        0, 0, 0, 0,
        1, 0, 1, 0,
        0, 1, 0, 1,
        1, 1, 1, 1,
    };
    glGenVertexArrays(1, &g_quadVAO);
    glGenBuffers(1, &g_quadVBO);
    glBindVertexArray(g_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray((GLuint)g_locPos);
    glVertexAttribPointer((GLuint)g_locPos, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray((GLuint)g_locUV);
    glVertexAttribPointer((GLuint)g_locUV, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
}

// ====================================================================
// 构造 / 析构
// ====================================================================
Object2D::Object2D(int id)
    : id_(id)
    , x_(0), y_(0)
    , width_(0), height_(0)
    , anchorX_(0), anchorY_(0)
    , rotation_(0)
    , scaleX_(1.0f), scaleY_(1.0f)
    , visible_(true)
    , enabled_(true)
    , alpha_(1.0f)
    , colorR_(1.0f), colorG_(1.0f), colorB_(1.0f), colorA_(1.0f)
    , zOrder_(0)
    , userData_(nullptr)
    , parent_(nullptr)
{}

Object2D::~Object2D() {}

void Object2D::setPosition(float x, float y) { x_ = x; y_ = y; onBoundsChanged(); }
void Object2D::setSize(float w, float h) {
    if (width_ != w || height_ != h) {
        width_ = w;
        height_ = h;
        onBoundsChanged();
    }
}
void Object2D::setAnchor(float ax, float ay) { anchorX_ = ax; anchorY_ = ay; onBoundsChanged(); }
void Object2D::setRotation(float degrees) { rotation_ = degrees; onBoundsChanged(); }
void Object2D::setScale(float sx, float sy) { scaleX_ = sx; scaleY_ = sy; onBoundsChanged(); }
void Object2D::setVisible(bool v) { visible_ = v; }
void Object2D::setAlpha(float a) { alpha_ = a; }
void Object2D::setColor(float r, float g, float b, float a) { colorR_ = r; colorG_ = g; colorB_ = b; colorA_ = a; }
void Object2D::setZOrder(int z) { zOrder_ = z; }

float Object2D::getAbsoluteX() const {
    return parent_ ? parent_->getAbsoluteX() + x_ : x_;
}

float Object2D::getAbsoluteY() const {
    return parent_ ? parent_->getAbsoluteY() + y_ : y_;
}

bool Object2D::hitTest(float screenX, float screenY) const {
    if (!visible_ || !enabled_) return false;
    float ax = getAbsoluteX() - anchorX_ * width_;
    float ay = getAbsoluteY() - anchorY_ * height_;
    return screenX >= ax && screenX <= ax + width_
        && screenY >= ay && screenY <= ay + height_;
}

bool Object2D::onTouch(float, float, int) {
    return false;
}

void Object2D::render() {
    if (!visible_ || width_ <= 0 || height_ <= 0) return;

    ensureQuadProgram();

    int scrW = UIRenderer::instance()->getScreenWidth();
    int scrH = UIRenderer::instance()->getScreenHeight();
    if (scrW <= 0 || scrH <= 0) return;

    float ax = getAbsoluteX();
    float ay = getAbsoluteY();
    float drawX = ax - anchorX_ * width_;
    float drawY = ay - anchorY_ * height_;

    glUseProgram(g_quadProgram);

    // MVP = ortho * model
    // 标准正交投影：left=0, right=scrW, top=0, bottom=scrH
    // 列主序矩阵。直接计算组合矩阵：
    // mvp[0]=2*w/scrW, mvp[5]=-2*h/scrH, mvp[12]=2*drawX/scrW-1, mvp[13]=1-2*drawY/scrH
    float mvp[16] = {
        2.0f*width_/scrW, 0, 0, 0,
        0, -2.0f*height_/scrH, 0, 0,
        0, 0, -1, 0,
        2.0f*drawX/scrW - 1.0f, 1.0f - 2.0f*drawY/scrH, 0, 1
    };
    glUniformMatrix4fv(g_locProj, 1, GL_FALSE, mvp);

    float color[4] = {colorR_ * alpha_, colorG_ * alpha_, colorB_ * alpha_, colorA_ * alpha_};
    glUniform4fv(g_locColor, 1, color);
    glUniform1i(g_locTex, 0);
    glUniform1i(g_locUseTex, 0);

    glBindVertexArray(g_quadVAO);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    drawContent();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
    glUseProgram(0);
}

void Object2D::drawTexturedQuad(float /*x*/, float /*y*/, float /*w*/, float /*h*/,
                                 float u0, float v0, float u1, float v1) {
    // 使用 unit-space (0~1) 坐标，模型矩阵已经在 Object2D::render() 中设置
    float verts[] = {
        0, 0, u0, v0,
        1, 0, u1, v0,
        0, 1, u0, v1,
        1, 1, u1, v1,
    };
    glBindBuffer(GL_ARRAY_BUFFER, g_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);

    glUniform1i(g_locUseTex, 1);
    glActiveTexture(GL_TEXTURE0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Object2D::drawSolidRect(float /*x*/, float /*y*/, float /*w*/, float /*h*/,
                              float r, float g, float b, float a) {
    // 使用 unit-space (0~1) 坐标
    float verts[] = {
        0, 0, 0, 0,
        1, 0, 1, 0,
        0, 1, 0, 1,
        1, 1, 1, 1,
    };
    glBindBuffer(GL_ARRAY_BUFFER, g_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);

    glUniform1i(g_locUseTex, 0);
    float col[4] = {r * alpha_, g * alpha_, b * alpha_, a * alpha_};
    glUniform4fv(g_locColor, 1, col);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Object2D::removeFromParent() {
    if (parent_) {
        parent_->removeChild(this);
    }
}
