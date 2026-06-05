#pragma once

#include "Object3D.h"
#include <GLES3/gl3.h>

class Plane : public Object3D {
public:
    Plane();
    void init() override;
    void render() override;
    void release() override;

    void setCamera(class Camera* cam);
    void setProjectionMatrix(float* projection);
    void setTexture(GLuint tex);

private:
    class Camera* camera;
    float* projectionMatrix;
    GLuint vao;
    GLuint vbo;
    GLuint texture;
    float mvpMatrix[16];
    float modelMatrix[16];
    GLint mvpLoc;
    GLint texLoc;
    GLint posLoc;
    GLint uvLoc;
    bool shaderReady;
};
