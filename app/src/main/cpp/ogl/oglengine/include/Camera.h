#pragma once

#include <math.h>

class CameraController;

class Camera {
public:
    Camera();
    ~Camera();  // 负责删除 controller_

    void setPosition(float x, float y, float z);
    void setTarget(float x, float y, float z);
    void setUp(float x, float y, float z);
    void setRotation(float deltaX, float deltaY);
    float* getPosition();
    float* getTarget();
    float* getViewMatrix();
    void lookAt(float eyeX, float eyeY, float eyeZ, float targetX, float targetY, float targetZ, float upX, float upY, float upZ);
    void setDistance(float d);
    float getDistance() const { return distance; }
    float getPitch() const { return pitch; }
    void setYaw(float y) { yaw = y; }
    void setPitch(float p) { pitch = p; }
    
    // 直接设置位置和目标，不重置 yaw/pitch
    void setPositionDirect(float x, float y, float z);
    void setTargetDirect(float x, float y, float z);

    // --- CameraController 支持 ---
    /** 设置相机控制器（Camera 取得所有权，负责 delete） */
    void setController(CameraController* ctrl);

    /** 获取当前控制器 */
    CameraController* getController() const { return controller_; }

    /** 每帧更新：如果有 controller，由 controller 产生相机参数 */
    void update(float dt);

private:
    void updateViewMatrix();

    float position[3];
    float target[3];
    float up[3];
    float viewMatrix[16];
    float distance;
    float yaw;
    float pitch;
    bool directMode;  // true = 直接模式，updateViewMatrix 不重新计算位置

    // CameraController（可为 nullptr）
    CameraController* controller_ = nullptr;
};
