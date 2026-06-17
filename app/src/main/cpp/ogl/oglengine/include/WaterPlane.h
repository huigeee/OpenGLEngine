#ifndef WATER_PLANE_H
#define WATER_PLANE_H

#include <GLES3/gl3.h>

/**
 * WaterPlane — 动态水面效果。
 *
 * 一个平面网格 + 自定义 shader，实现：
 *   - 顶点波浪（正弦波叠加）
 *   - 半透明颜色 + 深度混合
 *   - 视角相关的菲涅尔效果
 *   - 时间驱动的波浪动画
 *
 * 在 MainScene::onRenderExtra() 中调用 render()，
 * 在所有不透物体渲染完成后绘制。
 */
class WaterPlane {
public:
    WaterPlane();
    ~WaterPlane();

    /** 初始化 shader、VAO、VBO */
    void init();

    /** 释放 OpenGL 资源 */
    void release();

    /** 每帧更新波浪时间 */
    void update(float dt);

    /**
     * 渲染水面。
     * @param viewMat     视图矩阵 4x4
     * @param projMat     投影矩阵 4x4
     * @param screenW     屏幕宽度
     * @param screenH     屏幕高度
     */
    void render(const float* viewMat, const float* projMat,
                int screenW, int screenH);

    // --- 水面参数 ---
    void setPosition(float x, float y, float z) { posX_ = x; posY_ = y; posZ_ = z; }
    void setSize(float w, float h) { sizeW_ = w; sizeH_ = h; }
    void setColor(float r, float g, float b, float a) { colorR_ = r; colorG_ = g; colorB_ = b; colorA_ = a; }
    void setWaveSpeed(float s) { waveSpeed_ = s; }
    void setWaveHeight(float h) { waveHeight_ = h; }

private:
    /** 编译 shader */
    GLuint compileShader(GLenum type, const char* source);
    GLuint createProgram(const char* vs, const char* fs);

    // VAO / VBO
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ibo_ = 0;
    int indexCount_ = 0;
    int gridDivisions_ = 64;  // 网格细分（64x64）

    // Shader program
    GLuint program_ = 0;

    // Uniform locations
    GLint loc_uMVP_ = -1;
    GLint loc_uModel_ = -1;
    GLint loc_uView_ = -1;
    GLint loc_uProj_ = -1;
    GLint loc_uTime_ = -1;
    GLint loc_uColor_ = -1;
    GLint loc_uAlpha_ = -1;
    GLint loc_uWaveSpeed_ = -1;
    GLint loc_uWaveHeight_ = -1;
    GLint loc_uCameraPos_ = -1;
    GLint loc_uScreenSize_ = -1;

    // 参数
    float posX_ = 0.0f, posY_ = 0.0f, posZ_ = -2.0f;
    float sizeW_ = 6.0f, sizeH_ = 6.0f;
    float colorR_ = 0.1f, colorG_ = 0.4f, colorB_ = 0.7f, colorA_ = 0.7f;
    float waveSpeed_ = 0.8f;
    float waveHeight_ = 0.15f;
    float time_ = 0.0f;
};

#endif // WATER_PLANE_H
