#pragma once
#include "ParticleSystem.h"

// -----------------------------------------------------------------------
// CloudEffect — 云粒子效果
//   特点：
//     - 大尺寸、慢速横向漂移（无下落）
//     - 柔软蓬松纹理（多层叠加软圆）
//     - 白/灰色，低透明度叠加，普通 alpha 混合
//     - 粒子在水平面内随机分布，y 轴微弱浮动
//     - custom0: 漂移相位（各云团微差，避免整齐感）
//     - custom1: 垂直浮动相位
// -----------------------------------------------------------------------
class CloudEffect : public ParticleSystem {
public:
    CloudEffect();
    ~CloudEffect() override = default;

    // 云层覆盖区域（以 emitPos 为中心，XZ 平面）
    float spawnWidth  = 12.0f;
    float spawnDepth  = 12.0f;

    // 云层高度偏差（y = emitPos.y + randF(-heightVar, heightVar)）
    float heightVar   = 0.8f;

    // 横向漂移速度（世界 X 轴，单位 m/s）
    float driftSpeed  = 0.15f;

    // 垂直浮动幅度（m）和频率（Hz）
    float bobAmp      = 0.12f;
    float bobFreq     = 0.2f;

protected:
    void onEmit  (Particle& p, const glm::vec3& emitPos) override;
    void onUpdate(Particle& p, float dt) override;
    void buildTexture() override;
};
