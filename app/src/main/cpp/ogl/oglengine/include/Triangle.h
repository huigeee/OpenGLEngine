#pragma once

#include "Object3D.h"
#include "Shader.h"

class Triangle : public Object3D {
public:
    Triangle();
    void init() override;
    void render() override;
    void release() override;

private:
    Shader* shader;
    GLuint vertexBuffer;
    GLint positionHandle;

    static const char* vertexShaderCode;
    static const char* fragmentShaderCode;
};