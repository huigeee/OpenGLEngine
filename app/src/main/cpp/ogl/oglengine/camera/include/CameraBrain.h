#ifndef CAMERA_BRAIN_H
#define CAMERA_BRAIN_H

#include "VirtualCamera.h"
#include "OrbitCamera.h"
#include "CameraController.h"
#include "../../include/Camera.h"
#include <algorithm>
#include <cmath>
#include <vector>

/**
 * CameraBrain — 摄像机大脑控制器。
 *
 * 继承 CameraController，作为 Camera 的挂件。
 * 所有实现内联在头文件中，避免链接问题。
 */
class CameraBrain : public CameraController {
public:
    CameraBrain() = default;
    ~CameraBrain() override = default;

    // ====================================================================
    // CameraController 接口
    // ====================================================================
    void update(float dt) override;
    void applyToCamera(Camera* camera) const override;
    bool handleTouch(int actionMasked, int pointerCount,
                     const float* xs, const float* ys, const int* ids) override;
    VirtualCamera* getActiveVirtualCamera() const override { return activeCam_; }
    void updateCameraParams(const CameraParams& params) override;
    void updateFromPosTarget(float posX, float posY, float posZ,
                             float targetX, float targetY, float targetZ) override;

    // ====================================================================
    // VirtualCamera 管理
    // ====================================================================
    void addCamera(VirtualCamera* cam) { if (cam) cameras_.push_back(cam); }
    void removeCamera(VirtualCamera* cam);
    void clearCameras() { cameras_.clear(); }

    void cutTo(VirtualCamera* cam);
    void blendTo(VirtualCamera* cam, float duration);

    VirtualCamera* getActiveCamera() const { return activeCam_; }

    enum BlendCurve { LINEAR, EASE_IN_OUT, EASE_IN, EASE_OUT };
    void setBlendCurve(BlendCurve curve) { blendCurve_ = curve; }

private:
    float applyCurve(float t) const {
        switch (blendCurve_) {
            case LINEAR:      return t;
            case EASE_IN_OUT: return t * t / (t * t + (1.0f - t) * (1.0f - t));
            case EASE_IN:     return t * t;
            case EASE_OUT:    return 1.0f - (1.0f - t) * (1.0f - t);
            default:          return t;
        }
    }

    // blend 中间结果
    mutable float blendPosX_ = 0, blendPosY_ = 0, blendPosZ_ = 5;
    mutable float blendTargetX_ = 0, blendTargetY_ = 0, blendTargetZ_ = 0;
    mutable float blendFov_ = 45.0f;

    std::vector<VirtualCamera*> cameras_;
    VirtualCamera* activeCam_ = nullptr;
    VirtualCamera* fromCam_ = nullptr;
    VirtualCamera* toCam_ = nullptr;
    float blendTimer_ = 0.0f;
    float blendDuration_ = 0.0f;
    bool blending_ = false;
    BlendCurve blendCurve_ = EASE_IN_OUT;

    // Touch 控制
    float prevTouchX_ = 0, prevTouchY_ = 0, pinchDist_ = 0;
    bool touchDown_ = false, touchDown2_ = false;
    int touchCount_ = 0;

    // 非 OrbitCamera 的临时轨道参数（支持 fixedCam 拖拽）
    mutable float tmpOrbitYaw_ = 0;
    mutable float tmpOrbitPitch_ = 15;
    mutable float tmpOrbitDist_ = 5;
    mutable bool  tmpOrbitInit_ = false;
};

// ====================================================================
// 内联实现
// ====================================================================

inline void CameraBrain::removeCamera(VirtualCamera* cam) {
    auto it = std::find(cameras_.begin(), cameras_.end(), cam);
    if (it != cameras_.end()) cameras_.erase(it);
}

inline void CameraBrain::cutTo(VirtualCamera* cam) {
    if (!cam) return;
    activeCam_ = cam;
    fromCam_ = toCam_ = nullptr;
    blending_ = false;
    blendTimer_ = 0;
}

inline void CameraBrain::blendTo(VirtualCamera* cam, float duration) {
    if (!cam || activeCam_ == cam) return;
    fromCam_ = activeCam_;
    toCam_ = cam;
    blendDuration_ = std::max(duration, 0.001f);
    blendTimer_ = 0.0f;
    blending_ = true;
}

inline void CameraBrain::update(float dt) {
    if (cameras_.empty()) return;

    for (auto* cam : cameras_) cam->update(dt);

    if (blending_) {
        blendTimer_ += dt;
        float t = std::min(blendTimer_ / blendDuration_, 1.0f);
        float curveT = applyCurve(t);

        float fpx, fpy, fpz, ftx, fty, ftz, ffov;
        float tpx, tpy, tpz, ttx, tty, ttz, tfov;

        if (fromCam_) {
            fromCam_->getPosition(fpx, fpy, fpz);
            fromCam_->getTarget(ftx, fty, ftz);
            ffov = fromCam_->getFov();
        } else {
            fpx = blendPosX_; fpy = blendPosY_; fpz = blendPosZ_;
            ftx = blendTargetX_; fty = blendTargetY_; ftz = blendTargetZ_;
            ffov = blendFov_;
        }

        toCam_->getPosition(tpx, tpy, tpz);
        toCam_->getTarget(ttx, tty, ttz);
        tfov = toCam_->getFov();

        blendPosX_ = fpx + (tpx - fpx) * curveT;
        blendPosY_ = fpy + (tpy - fpy) * curveT;
        blendPosZ_ = fpz + (tpz - fpz) * curveT;
        blendTargetX_ = ftx + (ttx - ftx) * curveT;
        blendTargetY_ = fty + (tty - fty) * curveT;
        blendTargetZ_ = ftz + (ttz - ftz) * curveT;
        blendFov_ = ffov + (tfov - ffov) * curveT;

        if (t >= 1.0f) {
            activeCam_ = toCam_;
            fromCam_ = toCam_ = nullptr;
            blending_ = false;
        }
    } else if (activeCam_) {
        activeCam_->getPosition(blendPosX_, blendPosY_, blendPosZ_);
        activeCam_->getTarget(blendTargetX_, blendTargetY_, blendTargetZ_);
        blendFov_ = activeCam_->getFov();
    } else if (!cameras_.empty()) {
        activeCam_ = cameras_[0];
        activeCam_->getPosition(blendPosX_, blendPosY_, blendPosZ_);
        activeCam_->getTarget(blendTargetX_, blendTargetY_, blendTargetZ_);
        blendFov_ = activeCam_->getFov();
    }
}

inline void CameraBrain::applyToCamera(Camera* camera) const {
    if (!camera) return;
    camera->setPositionDirect(blendPosX_, blendPosY_, blendPosZ_);
    camera->setTargetDirect(blendTargetX_, blendTargetY_, blendTargetZ_);
}

inline bool CameraBrain::handleTouch(int actionMasked, int pointerCount,
                                      const float* xs, const float* ys,
                                      const int*) {
    if (!activeCam_) return false;

    // 对任何活跃相机都支持 touch 拖拽

    auto* orbit = dynamic_cast<OrbitCamera*>(activeCam_);
    touchCount_ = pointerCount;

    if (actionMasked == 0) {
        touchDown_ = true;
        prevTouchX_ = xs[0]; prevTouchY_ = ys[0];
        pinchDist_ = 0;

        // 非 OrbitCamera：从当前 position/target 初始化轨道参数
        if (!orbit) {
            float px, py, pz, tx, ty, tz;
            activeCam_->getPosition(px, py, pz);
            activeCam_->getTarget(tx, ty, tz);
            float dx = px - tx, dy = py - ty, dz = pz - tz;
            tmpOrbitDist_ = sqrtf(dx*dx + dy*dy + dz*dz);
            if (tmpOrbitDist_ > 0.001f) {
                tmpOrbitYaw_ = atan2f(dx, dz) * 180.0f / 3.14159f;
                tmpOrbitPitch_ = asinf(dy / tmpOrbitDist_) * 180.0f / 3.14159f;
            }
            tmpOrbitInit_ = true;
        }
    } else if (actionMasked == 5) {
        touchDown2_ = true;
        float dx = xs[1] - xs[0], dy = ys[1] - ys[0];
        pinchDist_ = sqrtf(dx * dx + dy * dy);
    } else if (actionMasked == 2) {
        if (touchCount_ >= 2 && touchDown2_) {
            float dx = xs[1] - xs[0], dy = ys[1] - ys[0];
            float dist = sqrtf(dx * dx + dy * dy);
            if (pinchDist_ > 0) {
                if (orbit) {
                    float nd = orbit->getDistance() - (dist - pinchDist_) * 0.05f;
                    orbit->setDistance(std::max(1.0f, std::min(20.0f, nd)));
                } else {
                    tmpOrbitDist_ -= (dist - pinchDist_) * 0.05f;
                    tmpOrbitDist_ = std::max(1.0f, std::min(20.0f, tmpOrbitDist_));
                }
            }
            pinchDist_ = dist;
        } else if (touchDown_) {
            float dx = xs[0] - prevTouchX_, dy = ys[0] - prevTouchY_;
            if (orbit) {
                orbit->addYaw(-dx * 0.3f);
                orbit->addPitch(dy * 0.3f);
            } else if (tmpOrbitInit_) {
                tmpOrbitYaw_ -= dx * 0.3f;
                tmpOrbitPitch_ += dy * 0.3f;
                // 直接更新 blendPos（非 OrbitCamera 时 brain 用 blendPos 输出）
                float yawRad = tmpOrbitYaw_ * 3.14159f / 180.0f;
                float pitchRad = tmpOrbitPitch_ * 3.14159f / 180.0f;
                float tx, ty, tz;
                activeCam_->getTarget(tx, ty, tz);
                blendPosX_ = tx + tmpOrbitDist_ * cosf(pitchRad) * sinf(yawRad);
                blendPosY_ = ty + tmpOrbitDist_ * sinf(pitchRad);
                blendPosZ_ = tz + tmpOrbitDist_ * cosf(pitchRad) * cosf(yawRad);
            }
            prevTouchX_ = xs[0]; prevTouchY_ = ys[0];
        }
    } else if (actionMasked == 6) {
        touchDown2_ = false; pinchDist_ = 0;
    } else if (actionMasked == 1 || actionMasked == 3) {
        touchDown_ = touchDown2_ = false;
        touchCount_ = 0; pinchDist_ = 0;
        tmpOrbitInit_ = false;
    }
    return true;
}

inline void CameraBrain::updateCameraParams(const CameraParams& params) {
    auto* orbit = dynamic_cast<OrbitCamera*>(activeCam_);
    if (!orbit) return;
    orbit->setYaw(params.yaw);
    orbit->setPitch(params.pitch);
    orbit->setDistance(params.distance);
    orbit->setTarget(params.targetX, params.targetY, params.targetZ);
}

inline void CameraBrain::updateFromPosTarget(float posX, float posY, float posZ,
                                              float targetX, float targetY, float targetZ) {
    auto* orbit = dynamic_cast<OrbitCamera*>(activeCam_);
    if (orbit) {
        float dx = posX - targetX;
        float dy = posY - targetY;
        float dz = posZ - targetZ;
        float dist = sqrtf(dx*dx + dy*dy + dz*dz);
        if (dist > 0.001f) {
            float yaw = atan2f(dx, dz) * 180.0f / 3.14159f;
            float pitch = asinf(dy / dist) * 180.0f / 3.14159f;
            orbit->setDistance(dist);
            orbit->setYaw(yaw);
            orbit->setPitch(pitch);
        }
        orbit->setTarget(targetX, targetY, targetZ);
    } else if (activeCam_) {
        activeCam_->setPosition(posX, posY, posZ);
        activeCam_->setTarget(targetX, targetY, targetZ);
    }
}

#endif // CAMERA_BRAIN_H
