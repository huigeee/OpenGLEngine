#ifndef FIXED_CAMERA_H
#define FIXED_CAMERA_H

#include "VirtualCamera.h"

/**
 * FixedCamera — 固定视角虚拟相机。
 *
 * 直接输出固定的 position + target，没有动态行为。
 */
class FixedCamera : public VirtualCamera {
public:
    FixedCamera() = default;
    ~FixedCamera() override = default;
    void update(float) override {}

    // 使用基类的 getPosition/getTarget（直接返回 posX_/targetX_ 等）
    // 基类的 setPosition/setTarget 可以直接设置
};

#endif // FIXED_CAMERA_H
