#pragma once

#include "Object3D.h"
#include "Camera.h"
#include "Matrix.h"

class Cube : public Object3D {
public:
    Cube();
    void init() override;
    void setCamera(Camera* camera);
    void setProjectionMatrix(float* projection);
    void render() override;
    void release() override;

private:
    Camera* camera;
    float* projectionMatrix;
    GLuint vertexBuffer;
    GLuint indexBuffer;
    int vertexCount;
};