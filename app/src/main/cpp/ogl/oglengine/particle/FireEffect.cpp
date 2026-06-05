#include "FireEffect.h"
#include <android/log.h>
#include <cmath>

#define LOG_TAG "FireEffect"

// -----------------------------------------------------------------------
// 构造：设置火焰预设参数
// -----------------------------------------------------------------------
FireEffect::FireEffect() {
    // 发射率
    emitRate    = 40.0f;

    // 粒子生命
    minLife     = 0.5f;
    maxLife     = 1.2f;

    // 点精灵大小（像素）
    minSize     = 10.0f;
    maxSize     = 36.0f;

    // 初速度：主要向上，少量水平散布
    minVelocity = { -0.12f, 0.5f, -0.12f };
    maxVelocity = {  0.12f, 1.4f,  0.12f };

    // 颜色：亮橙黄 → 暗红透明
    colorStart  = { 1.0f, 0.75f, 0.1f, 1.0f };
    colorEnd    = { 0.7f, 0.05f, 0.0f, 0.0f };

    // 加法混合（叠加发光）
    blendSrc    = GL_SRC_ALPHA;
    blendDst    = GL_ONE;

    // 点精灵大小系数
    sizeScale   = 220.0f;
}

// -----------------------------------------------------------------------
// onEmit：在发射点附近圆形范围内随机位置生成粒子
// -----------------------------------------------------------------------
void FireEffect::onEmit(Particle& p, const glm::vec3& emitPos) {
    // 随机圆盘散布
    float angle = randF(0.0f, 6.2832f);
    float r     = randF(0.0f, emitRadius);
    p.position  = emitPos + glm::vec3(cosf(angle) * r, 0.0f, sinf(angle) * r);

    p.velocity  = glm::vec3(
        randF(minVelocity.x, maxVelocity.x),
        randF(minVelocity.y, maxVelocity.y),
        randF(minVelocity.z, maxVelocity.z));

    p.maxLife   = randF(minLife, maxLife);
    p.life      = p.maxLife;
    p.size      = randF(minSize, maxSize);
    p.color     = colorStart;

    // custom0: 存储初始水平速度幅度（用于湍流相位）
    p.custom0   = randF(0.0f, 6.2832f);   // 随机相位偏移
    p.custom1   = 0.0f;
}

// -----------------------------------------------------------------------
// onUpdate：火焰物理
//   1. 上升加速（热气流）
//   2. 水平湍流（sin 扰动，营造跳动感）
//   3. 随生命衰减缩小粒子
//   4. 颜色从橙黄渐变为暗红透明
// -----------------------------------------------------------------------
void FireEffect::onUpdate(Particle& p, float dt) {
    // 上升加速
    p.velocity.y += riseAccel * dt;

    // 水平湍流：用 custom0 作相位，随时间推进
    p.custom0 += dt * 4.0f;
    p.velocity.x += sinf(p.custom0)          * turbulence * dt;
    p.velocity.z += sinf(p.custom0 + 1.571f) * turbulence * dt;

    // 位置更新
    p.position += p.velocity * dt;

    // 颜色插值（t=0 出生，t=1 消亡）
    float t = 1.0f - (p.life / p.maxLife);
    p.color = glm::mix(colorStart, colorEnd, t);

    // 随生命缩小（让尾端粒子更小）
    float sizeFactor = glm::clamp(p.life / p.maxLife, 0.0f, 1.0f);
    p.size = glm::mix(randF(minSize * 0.3f, minSize * 0.6f), maxSize, sizeFactor);
}

// -----------------------------------------------------------------------
// buildTexture：火焰纹理（中心极亮，四次方衰减，更锐利）
// -----------------------------------------------------------------------
void FireEffect::buildTexture() {
    const int SZ = 64;
    unsigned char texData[SZ * SZ];
    float center = (SZ - 1) * 0.5f;
    for (int y = 0; y < SZ; y++) {
        for (int x = 0; x < SZ; x++) {
            float dx = (x - center) / center;
            float dy = (y - center) / center;
            float d  = sqrtf(dx*dx + dy*dy);
            float v  = 1.0f - glm::clamp(d, 0.0f, 1.0f);
            v = v * v * v;   // 三次方：中心极亮，边缘锐利衰减
            texData[y * SZ + x] = (unsigned char)(v * 255);
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
