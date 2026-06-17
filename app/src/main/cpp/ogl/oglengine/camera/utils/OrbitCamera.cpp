#include "../include/OrbitCamera.h"
#include <algorithm>
#include <cmath>

OrbitCamera::OrbitCamera() {
    currentYaw_ = yaw_;
    currentPitch_ = pitch_;
    currentDistance_ = distance_;
}

void OrbitCamera::update(float dt) {
    // 平滑插值
    float smoothFactor = 1.0f - expf(-smoothSpeed_ * dt);
    currentYaw_ += (yaw_ - currentYaw_) * smoothFactor;
    currentPitch_ += (pitch_ - currentPitch_) * smoothFactor;
    currentDistance_ += (distance_ - currentDistance_) * smoothFactor;

    // 限制俯仰
    currentPitch_ = std::max(pitchMin_, std::min(pitchMax_, currentPitch_));

    // position 在 getPosition 中惰性计算
}

void OrbitCamera::getPosition(float& x, float& y, float& z) const {
    float yawRad = currentYaw_ * 3.14159f / 180.0f;
    float pitchRad = currentPitch_ * 3.14159f / 180.0f;
    // 使用基类的 targetX_/targetY_/targetZ_（protected，可访问）
    x = this->targetX_ + currentDistance_ * cosf(pitchRad) * sinf(yawRad);
    y = this->targetY_ + currentDistance_ * sinf(pitchRad);
    z = this->targetZ_ + currentDistance_ * cosf(pitchRad) * cosf(yawRad);
}
