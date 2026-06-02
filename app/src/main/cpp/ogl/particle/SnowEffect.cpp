#include "SnowEffect.h"
#include <cmath>

// -----------------------------------------------------------------------
// 构造：雪预设
// -----------------------------------------------------------------------
SnowEffect::SnowEffect() {
    emitRate     = 80.0f;

    minLife      = 4.0f;
    maxLife      = 7.0f;

    // 再缩小一点
    minSize      = 2.0f;
    maxSize      = 5.0f;

    minVelocity  = { -0.15f, -0.8f, -0.15f };
    maxVelocity  = {  0.15f, -0.3f,  0.15f };

    colorStart   = { 1.0f, 1.0f, 1.0f, 0.85f };
    colorEnd     = { 0.9f, 0.9f, 1.0f, 0.0f  };

    blendSrc     = GL_SRC_ALPHA;
    blendDst     = GL_ONE_MINUS_SRC_ALPHA;

    // 较小的 sizeScale，避免近距离粒子过大
    sizeScale    = 80.0f;
}

// -----------------------------------------------------------------------
// onEmit
// -----------------------------------------------------------------------
void SnowEffect::onEmit(Particle& p, const glm::vec3& emitPos) {
    p.position = glm::vec3(
        emitPos.x + randF(-spawnWidth  * 0.5f, spawnWidth  * 0.5f),
        emitPos.y + spawnHeight,
        emitPos.z + randF(-spawnDepth  * 0.5f, spawnDepth  * 0.5f));

    p.velocity = glm::vec3(
        randF(minVelocity.x, maxVelocity.x),
        randF(minVelocity.y, maxVelocity.y),
        randF(minVelocity.z, maxVelocity.z));

    p.maxLife  = randF(minLife, maxLife);
    p.life     = p.maxLife;
    p.size     = randF(minSize, maxSize);
    p.color    = colorStart;

    // custom0: 摆动相位随机偏移（使每片雪花摆动不同步）
    p.custom0  = randF(0.0f, 6.2832f);
    // custom1: 累计时间（用于计算 sin 摆动）
    p.custom1  = 0.0f;
}

// -----------------------------------------------------------------------
// onUpdate：慢速下落 + 左右摆动 + 生命末淡出
// -----------------------------------------------------------------------
void SnowEffect::onUpdate(Particle& p, float dt) {
    p.custom1 += dt;

    // 左右摆动：sin 模拟微风摇摆
    float swing = sinf(p.custom0 + p.custom1 * swingFreq * 6.2832f) * swingAmp;
    p.velocity.x = swing;
    p.velocity.z = cosf(p.custom0 + p.custom1 * swingFreq * 5.0f) * swingAmp * 0.5f;

    p.position += p.velocity * dt;
    // 下落分量单独处理（不受摆动影响）
    float fallSpeed = randF(minVelocity.y, maxVelocity.y);
    p.position.y += fallSpeed * dt;

    // 落地回收
    if (p.position.y < groundY) {
        p.active = false;
        return;
    }

    // 淡出
    float t = 1.0f - (p.life / p.maxLife);
    p.color = glm::mix(colorStart, colorEnd, t);
}

// -----------------------------------------------------------------------
// buildTexture：柔软高斯圆点，边缘平滑淡出
// 小尺寸下六角星会显得很假，用柔和圆形更贴近真实雪景中的光点感
// -----------------------------------------------------------------------
void SnowEffect::buildTexture() {
    const int SZ = 32;
    unsigned char texData[SZ * SZ];
    float c = (SZ - 1) * 0.5f;
    for (int y = 0; y < SZ; y++) {
        for (int x = 0; x < SZ; x++) {
            float dx = (x - c) / c;
            float dy = (y - c) / c;
            float d2 = dx*dx + dy*dy;
            // 高斯衰减，sigma=0.38，边缘柔和
            float v = expf(-d2 / (2.0f * 0.38f * 0.38f));
            // 超出单位圆直接截断，避免方形伪影
            if (sqrtf(d2) > 1.0f) v = 0.0f;
            texData[y * SZ + x] = (unsigned char)(glm::clamp(v, 0.0f, 1.0f) * 255);
        }
    }
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, SZ, SZ, 0, GL_RED, GL_UNSIGNED_BYTE, texData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}
