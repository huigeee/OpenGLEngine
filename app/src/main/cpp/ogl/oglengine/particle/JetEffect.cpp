#include "JetEffect.h"
#include <cmath>

JetEffect::JetEffect() {
    emitRate    = 2000.0f;       // 超高密度

    minLife     = 0.08f;
    maxLife     = 0.2f;

    minSize     = 6.0f;          // 大一点让粒子重叠
    maxSize     = 16.0f;

    colorStart  = { 1.0f, 0.95f, 0.7f, 1.0f };
    colorEnd    = { 0.1f, 0.3f, 0.9f, 0.0f };

    blendSrc    = GL_SRC_ALPHA;
    blendDst    = GL_ONE;

    sizeScale   = 80.0f;        // 放大让粒子重叠成连续流
}

void JetEffect::onEmit(Particle& p, const glm::vec3& emitPos) {
    glm::vec3 dir = jetDirection;

    glm::vec3 up = glm::abs(glm::dot(dir, glm::vec3(0,1,0))) < 0.99f
                   ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
    glm::vec3 right = glm::normalize(glm::cross(dir, up));
    glm::vec3 localUp = glm::normalize(glm::cross(right, dir));

    float a = randF(0, 6.2832f);
    float r = randF(0, emitRadius);
    p.position = emitPos + (right * cosf(a) + localUp * sinf(a)) * r;

    float theta = randF(-spreadAngle, spreadAngle);
    float phi   = randF(0, 6.2832f);
    glm::vec3 spread = right * sinf(theta) * cosf(phi)
                     + localUp * sinf(theta) * sinf(phi)
                     + dir * cosf(theta);
    p.velocity = glm::normalize(spread) * randF(minSpeed, maxSpeed);

    p.maxLife = randF(minLife, maxLife);
    p.life = p.maxLife;
    p.size = randF(minSize, maxSize);
    p.color = colorStart;
    p.custom0 = glm::length(p.velocity);
    p.custom1 = randF(0, 1.0f);
}

void JetEffect::onUpdate(Particle& p, float dt) {
    p.position += p.velocity * dt;
    p.velocity *= 0.92f;

    float t = 1.0f - (p.life / p.maxLife);

    // 三层颜色
    float blueShift = p.custom1 * 0.15f;
    if (t < 0.2f) {
        float s = t / 0.2f;
        p.color.r = 1.0f;
        p.color.g = glm::mix(0.95f, 0.4f, s);
        p.color.b = glm::mix(0.7f, 0.1f, s);
    } else if (t < 0.5f) {
        float s = (t - 0.2f) / 0.3f;
        p.color.r = 1.0f;
        p.color.g = glm::mix(0.4f, 0.2f, s);
        p.color.b = glm::mix(0.1f, 0.2f, s);
    } else {
        float s = (t - 0.5f) / 0.5f;
        p.color.r = glm::mix(1.0f, 0.1f + blueShift, s);
        p.color.g = glm::mix(0.2f, 0.2f + blueShift, s);
        p.color.b = glm::mix(0.2f, 0.9f, s);
    }

    // 柔和的 alpha 衰减（连续过渡）
    p.color.a = 1.0f - t * t;

    // 平滑缩小
    p.size = glm::mix(maxSize, minSize * 0.3f, t);
}

void JetEffect::buildTexture() {
    const int SZ = 32;
    unsigned char td[SZ * SZ];
    float ctr = (SZ - 1) * 0.5f;
    for (int y = 0; y < SZ; y++) {
        for (int x = 0; x < SZ; x++) {
            float d = sqrtf((x-ctr)*(x-ctr) + (y-ctr)*(y-ctr)) / ctr;
            float v = 1.0f - fminf(d, 1.0f);
            v = v * v;
            td[y * SZ + x] = (unsigned char)(v * 255);
        }
    }
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, SZ, SZ, 0, GL_RED, GL_UNSIGNED_BYTE, td);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}
