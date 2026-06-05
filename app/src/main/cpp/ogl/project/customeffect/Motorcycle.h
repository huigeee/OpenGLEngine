#pragma once
#include <GLES3/gl3.h>
#include <glm/glm.hpp>

// -----------------------------------------------------------------------
// Motorcycle — 程序化生成的摩托车几何体
//   使用基本图元（立方体、圆柱体、圆环）拼装。
//   轮子支持旋转动画。
// -----------------------------------------------------------------------
class Motorcycle {
public:
    Motorcycle();
    ~Motorcycle();

    void init();
    void update(float dt);
    void render(const float* viewMat, const float* projMat);
    void release();

    // 位置 / 朝向
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    float     heading  = 0.0f;      // Y 轴旋转（弧度）
    float     scale    = 1.0f;

    // 轮子转速（弧度/秒）
    float wheelSpeed = 4.0f;

public:
    // ---- 几何体基元 ----
    struct Mesh {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ebo = 0;
        int    count = 0;   // index count
    };

    // ---- 部件清单 ----
    Mesh bodyMesh_;     // 车身
    Mesh fuelTank_;     // 油箱
    Mesh seatMesh_;     // 坐垫
    Mesh headMesh_;     // 车头
    Mesh handleMesh_;   // 把手
    Mesh forkMesh_;     // 前叉
    Mesh exhaustMesh_;  // 排气管
    Mesh wheelMesh_;    // 轮圈
    Mesh wheelMesh2_;   // 后轮独立mesh
    Mesh spokeMesh_;    // 辐条

    // 共享 shader
    GLuint program_ = 0;
    GLint  locVP_ = -1;
    GLint  locModel_ = -1;
    GLint  locColor_ = -1;

    float wheelAngle_ = 0.0f;

    // 构建辅助函数
    Mesh buildBox(float sx, float sy, float sz);
    Mesh buildCylinder(float radius, float height, int seg);
    Mesh buildTorus(float majorR, float minorR, int seg, int rings);
    Mesh buildSpokes(float radius, int count);
    void buildSharedShader();
};
