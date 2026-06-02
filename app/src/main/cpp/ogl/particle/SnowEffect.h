#pragma once
#include "ParticleSystem.h"

// -----------------------------------------------------------------------
// SnowEffect — 雪粒子效果
//   特点：
//     - 慢速飘落，带左右摆动（pendulum 式）
//     - 雪花形软圆纹理，白色半透明
//     - 随机旋转（custom0 存旋转相位，造成视觉多样性）
//     - 支持矩形区域发射（与雨类似）
// -----------------------------------------------------------------------
class SnowEffect : public ParticleSystem {
public:
    SnowEffect();
    ~SnowEffect() override = default;

    // 降雪覆盖区域（以 emitPos 为中心）
    float spawnWidth  = 10.0f;
    float spawnDepth  = 10.0f;
    float spawnHeight = 6.0f;

    // 摆动频率（Hz）和幅度（m/s）
    float swingFreq   = 0.8f;
    float swingAmp    = 0.25f;

    // 落地 Y 坐标
    float groundY     = -2.0f;

protected:
    void onEmit  (Particle& p, const glm::vec3& emitPos) override;
    void onUpdate(Particle& p, float dt) override;
    void buildTexture() override;
};
