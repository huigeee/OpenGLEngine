#ifndef ORBIT_CAMERA_H
#define ORBIT_CAMERA_H

#include "VirtualCamera.h"
#include <glm/glm.hpp>

/**
 * OrbitCamera — 轨道环绕虚拟相机。
 *
 * 围绕 target 旋转，输出 position + target + fov 到 CameraBrain。
 * 由 CameraBrain 将数据写入真正的 Camera。
 */
class OrbitCamera : public VirtualCamera {
public:
    OrbitCamera();
    virtual ~OrbitCamera() = default;

    virtual void update(float dt) override;

    // 输出：orbit 模式下 position 由 yaw/pitch/distance 计算
    virtual void getPosition(float& x, float& y, float& z) const override;
    virtual void getTarget(float& x, float& y, float& z) const override {
        x = targetX_; y = targetY_; z = targetZ_;
    }

    // --- 轨道参数 ---
    void setDistance(float d) { distance_ = d; }
    float getDistance() const { return distance_; }
    void addDistance(float delta) { distance_ += delta; }

    void setYaw(float y) { yaw_ = y; }
    float getYaw() const { return yaw_; }
    void addYaw(float delta) { yaw_ += delta; }

    void setPitch(float p) { pitch_ = p; }
    float getPitch() const { return pitch_; }
    void addPitch(float delta) { pitch_ += delta; }

    void setPitchRange(float min, float max) { pitchMin_ = min; pitchMax_ = max; }
    void setSmoothSpeed(float s) { smoothSpeed_ = s; }

private:
    float distance_ = 5.0f;
    float yaw_ = 0.0f;
    float pitch_ = 15.0f;
    float pitchMin_ = -80.0f;
    float pitchMax_ = 80.0f;

    float smoothSpeed_ = 5.0f;
    float currentYaw_ = 0.0f;
    float currentPitch_ = 15.0f;
    float currentDistance_ = 5.0f;
};

#endif // ORBIT_CAMERA_H
