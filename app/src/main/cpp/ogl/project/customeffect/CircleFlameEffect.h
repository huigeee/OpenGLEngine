#pragma once
#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include <vector>

class CircleFlameEffect {
public:
    CircleFlameEffect();
    ~CircleFlameEffect();

    void init();
    void update(float dt);
    void render(const float* viewMat, const float* projMat);
    void release();

    glm::vec3 center     = {0.0f, 0.0f, 0.0f};
    float     groundY    = -0.72f;
    float     ringRadius = 0.6f;     // 圆环半径 = 火焰发射半径
    float     ringWidth  = 0.12f;    // 圆环宽度（宽一些）
    float     appearSpeed = 0.2f;    // 圆环出现速度
    float     burnSpeed  = 0.8f;     // 火焰沿圆环蔓延速度（弧度/秒）

private:
    // ---- 状态 ----
    float circleAlpha_ = 0.0f;
    float burnAngle_   = 0.0f;       // 已蔓延角度（弧度），从 0 到 2PI

    // ---- 火焰粒子 ----
    struct FlameParticle {
        glm::vec3 pos;
        glm::vec3 vel;
        float life, maxLife;
        float size;
        bool active = false;
    };
    std::vector<FlameParticle> flames_;
    int maxFlames_ = 500;
    float emitAccum_ = 0.0f;

    // ---- 萤火虫 ----
    struct Firefly {
        glm::vec3 pos;
        glm::vec3 vel;
        float life, maxLife;
        float size;
        float phase;
        bool active = false;
    };
    std::vector<Firefly> fireflies_;
    int maxFireflies_ = 40;
    float ffAccum_ = 0.0f;

    // ---- GPU ----
    GLuint ringVAO_ = 0;
    GLuint ringVBO_ = 0;
    int    ringSegments_ = 48;

    GLuint flameVAO_ = 0;
    GLuint flameVBO_ = 0;
    GLuint flameProg_ = 0;
    GLint  flameVP_ = -1, flameSize_ = -1;

    GLuint ffVAO_ = 0;
    GLuint ffVBO_ = 0;
    GLuint ffProg_ = 0;
    GLint  ffVP_ = -1, ffSize_ = -1;

    GLuint texId_ = 0;
    GLuint ringProg_ = 0;
    GLint  ringProj_ = -1, ringView_ = -1;

    void rebuildRingVBO();
    void rebuildFlameVBO();
    void rebuildFireflyVBO();
    float randF(float lo, float hi) {
        return lo + (hi - lo) * ((float)rand() / (float)RAND_MAX);
    }
};
