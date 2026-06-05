#include "RainEffect.h"
#include <cmath>

// -----------------------------------------------------------------------
// 构造：雨预设
// -----------------------------------------------------------------------
RainEffect::RainEffect() {
    emitRate     = 250.0f;           // 密集细雨

    minLife      = 1.2f;
    maxLife      = 2.5f;

    // 非常小的点精灵，让纹理中心细线只占 1~3px
    minSize      = 1.5f;
    maxSize      = 3.5f;

    // 快速向下
    minVelocity  = { -0.1f, -7.0f, -0.1f };
    maxVelocity  = {  0.1f, -4.0f,  0.1f };

    // 蓝白，适当透明
    colorStart   = { 0.80f, 0.90f, 1.0f, 0.55f };
    colorEnd     = { 0.65f, 0.80f, 1.0f, 0.0f  };

    blendSrc     = GL_SRC_ALPHA;
    blendDst     = GL_ONE_MINUS_SRC_ALPHA;

    // 小 sizeScale：粒子远离相机时不会变得很大
    sizeScale    = 60.0f;
}

// -----------------------------------------------------------------------
// onEmit：在矩形区域高空随机位置生成雨滴
// -----------------------------------------------------------------------
void RainEffect::onEmit(Particle& p, const glm::vec3& emitPos) {
    p.position = glm::vec3(
        emitPos.x + randF(-spawnWidth  * 0.5f, spawnWidth  * 0.5f),
        emitPos.y + spawnHeight,
        emitPos.z + randF(-spawnDepth  * 0.5f, spawnDepth  * 0.5f));

    p.velocity = glm::vec3(
        wind.x + randF(minVelocity.x, maxVelocity.x),
        randF(minVelocity.y, maxVelocity.y),
        wind.y + randF(minVelocity.z, maxVelocity.z));

    p.maxLife  = randF(minLife, maxLife);
    p.life     = p.maxLife;
    p.size     = randF(minSize, maxSize);
    p.color    = colorStart;
    p.custom0  = 0.0f;
    p.custom1  = 0.0f;
}

// -----------------------------------------------------------------------
// onUpdate：重力加速 + 风力 + 落地回收
// -----------------------------------------------------------------------
void RainEffect::onUpdate(Particle& p, float dt) {
    // 重力加速（雨滴快速加速向下）
    p.velocity.y -= 4.0f * dt;

    // 持续风力偏移
    p.velocity.x += wind.x * 0.05f * dt;
    p.velocity.z += wind.y * 0.05f * dt;

    p.position += p.velocity * dt;

    // 落地即回收（不等生命耗尽）
    if (p.position.y < groundY) {
        p.active = false;
        return;
    }

    // 颜色：生命末端淡出
    float t = 1.0f - (p.life / p.maxLife);
    p.color = glm::mix(colorStart, colorEnd, t);
}

// -----------------------------------------------------------------------
// buildTexture：32×32 正方形，横向极陡高斯（模拟细线），竖向柔和
// 点精灵会把纹理等比贴满正方形 sprite，所以靠纹理本身的极窄横向来
// 控制视觉宽度，而不是依赖非正方形纹理尺寸。
// -----------------------------------------------------------------------
void RainEffect::buildTexture() {
    const int SZ = 32;
    unsigned char texData[SZ * SZ];
    for (int y = 0; y < SZ; y++) {
        for (int x = 0; x < SZ; x++) {
            // 水平方向：超陡高斯（sigma ≈ 0.04），只有中心 1~2 列亮
            float fx = (x / (float)(SZ - 1)) * 2.0f - 1.0f;  // -1 .. 1
            float hGauss = expf(-(fx * fx) / (2.0f * 0.04f * 0.04f));

            // 竖向方向：宽高斯（sigma ≈ 0.45），两端轻微淡出
            float fy = (y / (float)(SZ - 1)) * 2.0f - 1.0f;
            float vGauss = expf(-(fy * fy) / (2.0f * 0.45f * 0.45f));

            float v = hGauss * vGauss;
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
