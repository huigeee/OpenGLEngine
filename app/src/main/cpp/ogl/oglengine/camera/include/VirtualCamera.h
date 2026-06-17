#ifndef VIRTUAL_CAMERA_H
#define VIRTUAL_CAMERA_H

/**
 * VirtualCamera — 虚拟相机基类（纯数据源）。
 *
 * 类似 Unity Cinemachine VirtualCamera：
 *   不生成 view/proj 矩阵，只提供 position + target + FOV 参数。
 *   CameraBrain 选择活跃的 VirtualCamera，将其参数 blend 后写入真正的 Camera。
 */
class VirtualCamera {
public:
    VirtualCamera();
    virtual ~VirtualCamera();

    /** 每帧更新内部状态（如轨道相机的角度计算） */
    virtual void update(float dt) = 0;

    // --- 输出参数（被 CameraBrain 读取） ---
    virtual void getPosition(float& x, float& y, float& z) const { x = posX_; y = posY_; z = posZ_; }
    virtual void getTarget(float& x, float& y, float& z) const { x = targetX_; y = targetY_; z = targetZ_; }
    virtual float getFov() const { return fovDeg_; }
    virtual float getNear() const { return near_; }
    virtual float getFar() const { return far_; }

    // --- 设置参数（用于外部控制如 touch/面板） ---
    void setPosition(float x, float y, float z) { posX_ = x; posY_ = y; posZ_ = z; }
    void setTarget(float x, float y, float z) { targetX_ = x; targetY_ = y; targetZ_ = z; }
    void setFov(float f) { fovDeg_ = f; }
    void setNear(float n) { near_ = n; }
    void setFar(float f) { far_ = f; }

    // --- 优先级和过渡 ---
    void setPriority(int p) { priority_ = p; }
    int getPriority() const { return priority_; }

    void setBlendTime(float t) { blendTime_ = t; }
    float getBlendTime() const { return blendTime_; }

protected:
    float posX_ = 0, posY_ = 0, posZ_ = 5;
    float targetX_ = 0, targetY_ = 0, targetZ_ = 0;
    float fovDeg_ = 45.0f;
    float near_ = 0.1f;
    float far_ = 100.0f;

    int priority_ = 0;
    float blendTime_ = 0.5f;
};

#endif // VIRTUAL_CAMERA_H
