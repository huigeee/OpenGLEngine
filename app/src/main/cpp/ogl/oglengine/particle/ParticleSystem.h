#pragma once
#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include <vector>

// -----------------------------------------------------------------------
// Particle data (POD)
// -----------------------------------------------------------------------
struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;      // rgb + alpha（生命衰减用）
    float     life;       // 剩余生命（秒）
    float     maxLife;    // 初始生命
    float     size;       // 点精灵像素大小
    bool      active = false;

    // 子类可存放额外自定义数据（如旋转角、拉伸量等）
    float     custom0 = 0.0f;
    float     custom1 = 0.0f;
};

// -----------------------------------------------------------------------
// ParticleSystem — 通用 CPU 粒子系统基类
//   子类只需重写虚函数即可定制行为，无需修改渲染管线。
// -----------------------------------------------------------------------
class ParticleSystem {
public:
    ParticleSystem() = default;
    virtual ~ParticleSystem() { release(); }

    // --- 生命周期 ---
    // maxParticles: 粒子池上限
    virtual void init(int maxParticles = 300);
    // dt: 帧间隔秒；emitPos: 发射源世界坐标
    void update(float dt, const glm::vec3& emitPos);
    // viewMat / projMat: 列主序 float[16]
    void render(const float* viewMat, const float* projMat);
    virtual void release();

    // ---------------------------------------------------------------
    // 公开调参属性（子类可在构造时设置默认值，也可外部直接赋值）
    // ---------------------------------------------------------------
    float     emitRate   = 30.0f;          // 每秒发射粒子数

    float     minLife    = 0.8f;           // 粒子最短生命（秒）
    float     maxLife    = 2.0f;           // 粒子最长生命（秒）

    float     minSize    = 8.0f;           // 点精灵最小尺寸（像素）
    float     maxSize    = 32.0f;          // 点精灵最大尺寸（像素）

    glm::vec3 minVelocity = {-0.2f,  0.3f, -0.2f};
    glm::vec3 maxVelocity = { 0.2f,  1.2f,  0.2f};

    glm::vec4 colorStart = {1.0f, 1.0f, 1.0f, 1.0f}; // 粒子出生颜色
    glm::vec4 colorEnd   = {1.0f, 1.0f, 1.0f, 0.0f}; // 粒子消亡颜色

    // 混合模式（在 setupBlend() 里使用）
    GLenum    blendSrc   = GL_SRC_ALPHA;
    GLenum    blendDst   = GL_ONE;         // 加法混合（火焰/魔法），按需改为 GL_ONE_MINUS_SRC_ALPHA

    // 点精灵大小计算系数（VS 里 gl_PointSize = size / w * sizeScale）
    float     sizeScale  = 200.0f;

protected:
    // ---------------------------------------------------------------
    // 虚钩子 — 子类选择性重写
    // ---------------------------------------------------------------

    // 初始化一个新粒子（子类可定制发射形状、初速度等）
    // 默认实现：随机速度/大小/生命，位置在 pos 附近小范围散布
    virtual void onEmit(Particle& p, const glm::vec3& emitPos);

    // 每帧更新单个粒子（子类可定制物理：重力/湍流/轨迹等）
    // 默认实现：匀速运动 + 颜色线性插值
    virtual void onUpdate(Particle& p, float dt);

    // 构建粒子贴图（子类可提供自定义纹理）
    // 默认实现：程序生成软圆形（高斯衰减）
    virtual void buildTexture();

    // 设置渲染前混合状态（子类可改为其他混合模式）
    virtual void setupBlend();

    // 恢复混合状态（在 render() 结束时调用）
    virtual void teardownBlend();

    // ---------------------------------------------------------------
    // 内部工具
    // ---------------------------------------------------------------
    static float randF(float lo, float hi);
    int  firstDead();

    // ---------------------------------------------------------------
    // GPU 资源
    // ---------------------------------------------------------------
    std::vector<Particle> particles;
    int   maxCount   = 300;
    float emitAccum  = 0.0f;

    GLuint vao       = 0;
    GLuint vbo       = 0;    // 动态 VBO: pos(3)+color(4)+size(1) = 8 floats
    GLuint program   = 0;
    GLuint texId     = 0;    // 粒子纹理

    GLint  locVP     = -1;
    GLint  locTex    = -1;

    // ---------------------------------------------------------------
    // 默认 Shader（子类可在 init() 之前替换）
    // ---------------------------------------------------------------
    const char* vertSrc = nullptr;
    const char* fragSrc = nullptr;

private:
    static const char* DEFAULT_VS;
    static const char* DEFAULT_FS;

    GLuint compileShader(GLenum type, const char* src);
    void   buildDefaultShaders();
};
