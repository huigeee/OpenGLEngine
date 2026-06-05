#pragma once
#include "ParticleSystem.h"

// -----------------------------------------------------------------------
// FireEffect — 火焰粒子效果
//   继承 ParticleSystem，定制：
//     - 橙红色渐变 + 上升湍流物理
//     - 程序生成"火焰形"软圆纹理（中心更亮）
//   外部可直接调整基类属性微调效果，也可子类化进一步定制。
// -----------------------------------------------------------------------
class FireEffect : public ParticleSystem {
public:
    FireEffect();
    ~FireEffect() override = default;

    // 湍流强度（水平随机扰动速度幅度，单位 m/s/s）
    float turbulence  = 0.3f;

    // 上升加速度（火焰热气上浮，单位 m/s/s）
    float riseAccel   = 0.6f;

    // 发射半径（在发射点周围的随机散布半径，单位 m）
    float emitRadius  = 0.08f;

protected:
    void onEmit(Particle& p, const glm::vec3& emitPos) override;
    void onUpdate(Particle& p, float dt) override;
    // 火焰用更"火苗"形的软圆纹理（中心极亮，边缘急衰减）
    void buildTexture() override;
};
