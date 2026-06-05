#pragma once
#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include <vector>

// -----------------------------------------------------------------------
// RippleEffect — 地面涟漪悬浮效果
//
// 在 XZ 平面上均匀分布点阵，周期性从中心向外扩散圆环波纹。
// 当波纹经过时，对应位置的点向上悬浮，形成水波纹般的起伏效果。
// -----------------------------------------------------------------------
class RippleEffect {
public:
    RippleEffect();
    ~RippleEffect();

    void init(int gridCols, int gridRows, float spacing);
    void update(float dt);
    void render(const float* viewMat, const float* projMat);
    void release();

    // --- 参数 ---
    glm::vec3 center    = {0.0f, 0.0f, 0.0f};   // 波纹中心
    float     groundY   = -0.7f;                 // 地面高度
    float     rippleSpeed   = 2.0f;              // 波纹扩散速度
    float     rippleInterval = 3.0f;             // 每次波纹间隔（秒）
    float     maxHeight     = 0.3f;              // 最大悬浮高度
    float     pointSize     = 8.0f;              // 点精灵大小

private:
    struct RipplePoint {
        float x, z;    // 平面位置
        float y;       // 当前高度
    };

    std::vector<RipplePoint> points_;
    int pointCount_ = 0;

    float rippleTime_ = 0.0f;       // 当前波纹传播时间
    float totalTime_  = 0.0f;       // 总时间
    float rippleRadius_ = 0.0f;     // 当前波纹半径
    bool  rippleActive_ = false;

    // GPU
    GLuint vao_ = 0;
    GLuint vbo_ = 0;       // 每帧更新: pos3 + color3 = 6 floats per point
    GLuint program_ = 0;
    GLint  locVP_ = -1;
    GLint  locSize_ = -1;
    GLuint texId_ = 0;

    void rebuildVBO();
};
