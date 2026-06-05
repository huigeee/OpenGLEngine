#pragma once

#include "Object3D.h"
#include "Camera.h"
#include "Matrix.h"
#include <math.h>

class Sphere : public Object3D {
public:
    Sphere();
    void init() override;
    void initWithNormals();
    void initWithNormalsAndUV();
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
    bool useNormals;
    int vertexStride;
};