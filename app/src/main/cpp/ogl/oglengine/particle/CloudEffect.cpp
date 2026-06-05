#include "CloudEffect.h"
#include <cmath>

// -----------------------------------------------------------------------
// 构造：云预设
// -----------------------------------------------------------------------
CloudEffect::CloudEffect() {
    // 多粒子密集叠加才有云团感，提高发射率
    emitRate     = 15.0f;

    minLife      = 20.0f;
    maxLife      = 35.0f;

    // 粒子要足够大才能互相重叠融合
    minSize      = 28.0f;
    maxSize      = 55.0f;

    minVelocity  = { 0.0f, 0.0f, 0.0f };
    maxVelocity  = { 0.0f, 0.0f, 0.0f };

    // 单粒子透明度低，叠加后自然产生厚度
    colorStart   = { 1.0f,  1.0f,  1.0f,  0.18f };
    colorEnd     = { 0.85f, 0.85f, 0.9f,  0.0f  };

    blendSrc     = GL_SRC_ALPHA;
    blendDst     = GL_ONE_MINUS_SRC_ALPHA;

    sizeScale    = 200.0f;
}

// -----------------------------------------------------------------------
// onEmit：在水平面随机散布，高度微变
// -----------------------------------------------------------------------
void CloudEffect::onEmit(Particle& p, const glm::vec3& emitPos) {
    p.position = glm::vec3(
        emitPos.x + randF(-spawnWidth * 0.5f, spawnWidth * 0.5f),
        emitPos.y + randF(-heightVar,  heightVar),
        emitPos.z + randF(-spawnDepth * 0.5f, spawnDepth * 0.5f));

    p.velocity = glm::vec3(driftSpeed + randF(-0.03f, 0.03f), 0.0f, randF(-0.02f, 0.02f));

    p.maxLife  = randF(minLife, maxLife);
    p.life     = p.maxLife;
    p.size     = randF(minSize, maxSize);
    // 出生时完全透明，交由 onUpdate 淡入逻辑控制，避免第一帧突然闪现
    p.color    = glm::vec4(glm::vec3(colorStart), 0.0f);

    // custom0: 浮动/湍流相位（每粒子随机偏移，避免整齐感）
    p.custom0  = randF(0.0f, 6.2832f);
    // custom1: 累计时间
    p.custom1  = 0.0f;
}

// -----------------------------------------------------------------------
// onUpdate：横向漂移 + 垂直浮动 + 淡入淡出（出生淡入，消亡淡出）
// -----------------------------------------------------------------------
void CloudEffect::onUpdate(Particle& p, float dt) {
    p.custom1 += dt;

    float t     = p.custom1;
    float phase = p.custom0;

    // 湍流扰动：两层不同频率 sin 波叠加，每粒子相位不同
    // 产生差异化漂移路径，形成流动感
    float turbX = sinf(phase + t * 0.37f) * 0.06f
                + sinf(phase * 1.7f + t * 0.71f) * 0.03f;
    float turbZ = cosf(phase + t * 0.29f) * 0.05f
                + cosf(phase * 2.1f + t * 0.53f) * 0.025f;

    // 横向漂移 + 湍流
    p.position.x += (p.velocity.x + turbX) * dt;
    p.position.z += (p.velocity.z + turbZ) * dt;

    // 垂直浮动（bobbing）
    p.position.y += sinf(phase + t * bobFreq * 6.2832f) * bobAmp * dt;

    // 淡入淡出：生命前 20% 淡入，后 30% 淡出
    float lifeFrac = p.life / p.maxLife;  // 1.0→0.0
    float age      = 1.0f - lifeFrac;     // 0.0→1.0

    float alpha;
    if (age < 0.2f) {
        alpha = age / 0.2f;               // 淡入
    } else if (lifeFrac < 0.3f) {
        alpha = lifeFrac / 0.3f;          // 淡出
    } else {
        alpha = 1.0f;
    }

    p.color = glm::vec4(
        glm::mix(glm::vec3(colorEnd),   glm::vec3(colorStart),   alpha),
        colorStart.a * alpha);
}

// -----------------------------------------------------------------------
// buildTexture：蓬松软圆（多重高斯叠加，模拟积云质感）
// -----------------------------------------------------------------------
void CloudEffect::buildTexture() {
    const int SZ = 128;
    float texData[SZ * SZ];   // 先用 float 累加
    for (int i = 0; i < SZ * SZ; i++) texData[i] = 0.0f;

    float c = (SZ - 1) * 0.5f;

    // 主圆（大而柔）
    for (int y = 0; y < SZ; y++) {
        for (int x = 0; x < SZ; x++) {
            float dx = (x - c) / c;
            float dy = (y - c) / c;
            float d  = sqrtf(dx*dx + dy*dy);
            float v  = expf(-d * d * 2.5f);   // 高斯衰减
            texData[y * SZ + x] += v * 0.6f;
        }
    }

    // 叠加几个偏心小圆（模拟积云凸起）
    struct Blob { float ox, oy, r, w; };
    Blob blobs[] = {
        {  0.25f,  0.15f, 1.8f, 0.35f },
        { -0.20f,  0.20f, 2.2f, 0.30f },
        {  0.10f, -0.15f, 2.0f, 0.25f },
        { -0.30f, -0.10f, 1.6f, 0.20f },
        {  0.35f, -0.20f, 1.5f, 0.18f },
    };
    for (auto& b : blobs) {
        float bcx = c + b.ox * c;
        float bcy = c + b.oy * c;
        for (int y = 0; y < SZ; y++) {
            for (int x = 0; x < SZ; x++) {
                float dx = (x - bcx) / c;
                float dy = (y - bcy) / c;
                float d  = sqrtf(dx*dx + dy*dy);
                float v  = expf(-d * d * b.r);
                texData[y * SZ + x] += v * b.w;
            }
        }
    }

    // 归一化并转 u8，同时乘以径向蒙版让边缘平滑归零
    unsigned char u8[SZ * SZ];
    float mx = 0.0f;
    for (int i = 0; i < SZ * SZ; i++) mx = glm::max(mx, texData[i]);
    for (int y = 0; y < SZ; y++) {
        for (int x = 0; x < SZ; x++) {
            float dx = (x - c) / c;
            float dy = (y - c) / c;
            float d  = sqrtf(dx*dx + dy*dy);
            // 径向平滑蒙版：中心=1，r=0.85 开始降，r>=1 为 0
            // 用 smoothstep 让过渡柔和，彻底消除边缘割裂
            float mask = 1.0f - glm::smoothstep(0.75f, 1.0f, d);
            float v = glm::clamp(texData[y * SZ + x] / mx, 0.0f, 1.0f) * mask;
            u8[y * SZ + x] = (unsigned char)(v * 255);
        }
    }

    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, SZ, SZ, 0, GL_RED, GL_UNSIGNED_BYTE, u8);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}
