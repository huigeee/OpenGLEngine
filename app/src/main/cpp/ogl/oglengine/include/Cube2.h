#pragma once

#include "Object3D.h"

class Cube2 : public Object3D {
public:
    Cube2();
    void init() override;
    void release() override;
    void render() override;

    void setCamera(void* cam) { camera = cam; }
    void setProjectionMatrix(float* proj) { projectionMatrix = proj; }

private:
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    int indexCount = 0;
    void* camera = nullptr;
    float* projectionMatrix = nullptr;
};
