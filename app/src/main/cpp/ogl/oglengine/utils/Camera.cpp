#include "../include/Camera.h"
#include <math.h>
#include <algorithm>

Camera::Camera() : distance(3.0f), yaw(0.0f), pitch(0.0f), directMode(false) {
    position[0] = 0.0f; position[1] = 0.0f; position[2] = 3.0f;
    target[0] = 0.0f; target[1] = 0.0f; target[2] = 0.0f;
    up[0] = 0.0f; up[1] = 1.0f; up[2] = 0.0f;
    updateViewMatrix();
}

void Camera::setPosition(float x, float y, float z) {
    position[0] = x; position[1] = y; position[2] = z;
    distance = sqrtf((x-target[0])*(x-target[0]) + (y-target[1])*(y-target[1]) + (z-target[2])*(z-target[2]));
    yaw = atan2f(x - target[0], z - target[2]);
    pitch = asinf((y - target[1]) / (distance > 0.0f ? distance : 1.0f));
    updateViewMatrix();
}

void Camera::setTarget(float x, float y, float z) {
    target[0] = x; target[1] = y; target[2] = z;
    updateViewMatrix();
}

void Camera::setUp(float x, float y, float z) {
    up[0] = x; up[1] = y; up[2] = z;
}

void Camera::setDistance(float d) {
    distance = d;
    updateViewMatrix();
}

void Camera::setPositionDirect(float x, float y, float z) {
    // 直接设置位置，不重置 yaw/pitch，updateViewMatrix 不会覆盖
    position[0] = x; position[1] = y; position[2] = z;
    directMode = true;
    updateViewMatrix();
}

void Camera::setTargetDirect(float x, float y, float z) {
    // 直接设置目标点，不重置 yaw/pitch，updateViewMatrix 不会覆盖位置
    target[0] = x; target[1] = y; target[2] = z;
    directMode = true;
    updateViewMatrix();
}

void Camera::setRotation(float dx, float dy) {
    const float sensitivity = 0.01f;
    yaw -= dx * sensitivity;
    pitch += dy * sensitivity;
    pitch = std::max(-1.5f, std::min(1.5f, pitch));
    // 旋转时重置 directMode，让 updateViewMatrix 重新计算位置
    directMode = false;
    updateViewMatrix();
}

float* Camera::getPosition() { return position; }
float* Camera::getTarget() { return target; }
float* Camera::getViewMatrix() { return viewMatrix; }

void Camera::lookAt(float eyeX, float eyeY, float eyeZ, float targetX, float targetY, float targetZ, float upX, float upY, float upZ) {
    position[0] = eyeX; position[1] = eyeY; position[2] = eyeZ;
    target[0] = targetX; target[1] = targetY; target[2] = targetZ;
    up[0] = upX; up[1] = upY; up[2] = upZ;
    distance = sqrtf((eyeX-targetX)*(eyeX-targetX) + (eyeY-targetY)*(eyeY-targetY) + (eyeZ-targetZ)*(eyeZ-targetZ));
    yaw = atan2f(eyeX - targetX, eyeZ - targetZ);
    pitch = asinf((eyeY - targetY) / (distance > 0.0f ? distance : 1.0f));
    updateViewMatrix();
}

void Camera::updateViewMatrix() {
    // 在直接模式下，不重新计算位置，只更新 viewMatrix
    if (!directMode) {
        float camX = target[0] + distance * cosf(pitch) * sinf(yaw);
        float camY = target[1] + distance * sinf(pitch);
        float camZ = target[2] + distance * cosf(pitch) * cosf(yaw);
        position[0] = camX;
        position[1] = camY;
        position[2] = camZ;
    }

    float zAxis[3] = { position[0] - target[0], position[1] - target[1], position[2] - target[2] };
    float zLen = sqrtf(zAxis[0]*zAxis[0] + zAxis[1]*zAxis[1] + zAxis[2]*zAxis[2]);
    if (zLen > 0) { zAxis[0]/=zLen; zAxis[1]/=zLen; zAxis[2]/=zLen; }

    float upRef[3];
    if (fabsf(zAxis[1]) < 0.999f) {
        upRef[0] = 0.0f; upRef[1] = 1.0f; upRef[2] = 0.0f;
    } else {
        upRef[0] = 1.0f; upRef[1] = 0.0f; upRef[2] = 0.0f;
    }

    float xAxis[3] = { upRef[1]*zAxis[2] - upRef[2]*zAxis[1], upRef[2]*zAxis[0] - upRef[0]*zAxis[2], upRef[0]*zAxis[1] - upRef[1]*zAxis[0] };
    float xLen = sqrtf(xAxis[0]*xAxis[0] + xAxis[1]*xAxis[1] + xAxis[2]*xAxis[2]);
    if (xLen > 0) { xAxis[0]/=xLen; xAxis[1]/=xLen; xAxis[2]/=xLen; }

    float yAxis[3] = { zAxis[1]*xAxis[2] - zAxis[2]*xAxis[1], zAxis[2]*xAxis[0] - zAxis[0]*xAxis[2], zAxis[0]*xAxis[1] - zAxis[1]*xAxis[0] };

    viewMatrix[0] = xAxis[0]; viewMatrix[1] = yAxis[0]; viewMatrix[2] = zAxis[0]; viewMatrix[3] = 0;
    viewMatrix[4] = xAxis[1]; viewMatrix[5] = yAxis[1]; viewMatrix[6] = zAxis[1]; viewMatrix[7] = 0;
    viewMatrix[8] = xAxis[2]; viewMatrix[9] = yAxis[2]; viewMatrix[10] = zAxis[2]; viewMatrix[11] = 0;
    viewMatrix[12] = -(xAxis[0]*position[0] + xAxis[1]*position[1] + xAxis[2]*position[2]);
    viewMatrix[13] = -(yAxis[0]*position[0] + yAxis[1]*position[1] + yAxis[2]*position[2]);
    viewMatrix[14] = -(zAxis[0]*position[0] + zAxis[1]*position[1] + zAxis[2]*position[2]);
    viewMatrix[15] = 1;
}
