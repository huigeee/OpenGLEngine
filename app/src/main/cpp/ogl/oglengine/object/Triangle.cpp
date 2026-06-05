#include "../include/Triangle.h"
#include "../include/Common.h"

const char* Triangle::vertexShaderCode = R"(
    #version 300 es
    in vec4 vPosition;
    void main() {
        gl_Position = vPosition;
    }
)";

const char* Triangle::fragmentShaderCode = R"(
    #version 300 es
    precision mediump float;
    out vec4 fragColor;
    void main() {
        fragColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
)";

Triangle::Triangle() : shader(nullptr), vertexBuffer(0), positionHandle(0) {
}

void Triangle::init() {
    shader = new Shader();
    if (!shader->init(vertexShaderCode, fragmentShaderCode)) {
        LOGI("Failed to init triangle shader");
        return;
    }

    positionHandle = shader->getAttributeLocation("vPosition");

    GLfloat vertices[] = {
        0.0f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f
    };

    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    LOGI("Triangle initialized");
}

void Triangle::render() {
    if (shader == nullptr || vertexBuffer == 0 || positionHandle < 0) {
        return;
    }

    shader->use();

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(positionHandle, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(positionHandle);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(positionHandle);
}

void Triangle::release() {
    if (vertexBuffer != 0) {
        glDeleteBuffers(1, &vertexBuffer);
        vertexBuffer = 0;
    }
    if (shader != nullptr) {
        shader->release();
        delete shader;
        shader = nullptr;
    }
    LOGI("Triangle released");
}