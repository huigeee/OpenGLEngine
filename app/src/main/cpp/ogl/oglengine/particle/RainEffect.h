#pragma once
#include "ParticleSystem.h"

// -----------------------------------------------------------------------
// RainEffect — 雨粒子效果
//   特点：
//     - 细长线段拉伸（用 custom0 存粒子倾斜角度）
//     - 快速向下运动，带微弱风力偏移
//     - 半透明蓝白色，普通 alpha 混合
//     - 发射区域为矩形面（模拟从高空落下）
// -----------------------------------------------------------------------
class RainEffect : public ParticleSystem {
public:
    RainEffect();
    ~RainEffect() override = default;

    // 降雨区域：以 emitPos 为中心的矩形范围（单位 m）
    float spawnWidth  = 8.0f;
    float spawnDepth  = 8.0f;
    // 粒子从 emitPos.y + spawnHeight 高度开始下落
    float spawnHeight = 5.0f;

    // 风力（世界空间 XZ，单位 m/s）
    glm::vec2 wind    = { 0.3f, 0.1f };

    // 落地 Y 坐标（低于此值则回收粒子）
    float groundY     = -2.0f;

protected:
    void onEmit  (Particle& p, const glm::vec3& emitPos) override;
    void onUpdate(Particle& p, float dt) override;
    void buildTexture() override;
};
