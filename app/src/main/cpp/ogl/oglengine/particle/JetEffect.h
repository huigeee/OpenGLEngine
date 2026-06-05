#pragma once
#include "ParticleSystem.h"

// -----------------------------------------------------------------------
// JetEffect — 火箭/飞机引擎喷射尾焰
//   继承 ParticleSystem 用点精灵渲染，通过密集高速粒子模拟尾焰。
// -----------------------------------------------------------------------
class JetEffect : public ParticleSystem {
public:
    JetEffect();
    ~JetEffect() override = default;

    glm::vec3 jetDirection = {0.0f, 0.0f, 1.0f};
    float spreadAngle = 0.12f;
    float minSpeed = 8.0f;
    float maxSpeed = 14.0f;
    float emitRadius = 0.03f;

protected:
    void onEmit(Particle& p, const glm::vec3& emitPos) override;
    void onUpdate(Particle& p, float dt) override;
    void buildTexture() override;
};
