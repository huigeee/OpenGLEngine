#pragma once

class Matrix {
public:
    static void perspective(float* matrix, float fovY, float aspect, float nearZ, float farZ);
    static void ortho(float* matrix, float left, float right, float bottom, float top, float nearZ, float farZ);
    static void multiply(float* result, float* a, float* b);
    static void identity(float* matrix);
    static void translate(float* matrix, float x, float y, float z);
    static void rotate(float* matrix, float angle, float x, float y, float z);
    static void scale(float* matrix, float x, float y, float z);
    static void lookAt(float* matrix, float eyeX, float eyeY, float eyeZ, float targetX, float targetY, float targetZ, float upX, float upY, float upZ);
};