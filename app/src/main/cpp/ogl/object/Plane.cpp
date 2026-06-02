#include "../include/Plane.h"
#include "../include/Material.h"
#include "../include/Matrix.h"
#include "../include/Camera.h"
#include <cstring>

#define LOG_TAG "Plane"

Plane::Plane() : camera(nullptr), projectionMatrix(nullptr),
    vao(0), vbo(0), texture(0),
    mvpLoc(0), texLoc(0), posLoc(0), uvLoc(0),
    shaderReady(false) {
}

void Plane::init() {
    float vertices[] = {
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
    };

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void Plane::setCamera(Camera* cam) { this->camera = cam; }
void Plane::setProjectionMatrix(float* proj) { this->projectionMatrix = proj; }
void Plane::setTexture(GLuint tex) { this->texture = tex; }

void Plane::render() {
    if (material == nullptr || camera == nullptr || projectionMatrix == nullptr) {
        return;
    }

    GLboolean depthMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    glDepthMask(GL_FALSE);

    float modelMatrix[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        position[0], position[1], position[2], 1
    };

    float* viewMatrix = camera->getViewMatrix();
    float vpMatrix[16];
    Matrix::multiply(vpMatrix, viewMatrix, projectionMatrix);
    Matrix::multiply(mvpMatrix, modelMatrix, vpMatrix);

    material->setTransform(modelMatrix, viewMatrix, projectionMatrix);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);

    glDepthMask(depthMask);
}

void Plane::release() {
    if (vao != 0) { glDeleteVertexArrays(1, &vao); vao = 0; }
    if (vbo != 0) { glDeleteBuffers(1, &vbo); vbo = 0; }
}
