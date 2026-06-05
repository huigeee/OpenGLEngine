#pragma once

#include "Object3D.h"
#include "Camera.h"
#include "Matrix.h"
#include <math.h>

class Sphere2 : public Object3D {
public:
    Sphere2();
    void init() override;
    void initGeometry(float radius, bool createDefaultVAO = true);
    void setCamera(Camera* camera);
    void setProjectionMatrix(float* projection);
    void render() override;
    void release() override;

private:
    Camera* camera;
    float* projectionMatrix;
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    unsigned int indexCount;
    unsigned int stride;
};
