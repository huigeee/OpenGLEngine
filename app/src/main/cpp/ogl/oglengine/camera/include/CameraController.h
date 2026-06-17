#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

/**
 * CameraController — 相机控制器基类。
 *
 * Camera 可以持有（或不持有）一个 CameraController。
 *   没有 controller → Camera 原有逻辑（yaw/pitch/distance）不变
 *   有 controller  → Camera 的参数由 controller 产生
 *
 * 子类示例：CameraBrain（Cinemachine 风格多相机管理）
 */
class CameraController {
public:
    virtual ~CameraController() = default;

    /** 每帧更新，将结果写入 Camera（由 Camera::update 内部调用） */
    virtual void update(float dt) = 0;

    /**
     * 将控制器的参数应用到 Camera。
     * 子类应调用 camera->setPositionDirect/setTargetDirect。
     */
    virtual void applyToCamera(class Camera* camera) const = 0;

    /**
     * 处理 touch 事件。
     * @return true 表示消费了事件（不传给场景）
     */
    virtual bool handleTouch(int actionMasked, int pointerCount,
                             const float* xs, const float* ys, const int* ids) = 0;

    /** 获取当前活跃的 VirtualCamera（如果有）— 用于外部面板设置参数 */
    virtual class VirtualCamera* getActiveVirtualCamera() const { return nullptr; }

    /** 更新相机参数接口（由外部面板或 AIDL 调用） */
    struct CameraParams {
        float yaw = 0, pitch = 15, distance = 5;
        float targetX = 0, targetY = 0, targetZ = 0;
    };
    virtual void updateCameraParams(const CameraParams& params) {}

    /**
     * 从 position/target 更新相机参数。
     * Scene::updateCameraParams 调用此方法，controller 自行决定如何处理。
     */
    virtual void updateFromPosTarget(float posX, float posY, float posZ,
                                     float targetX, float targetY, float targetZ) {}
};

#endif // CAMERA_CONTROLLER_H
