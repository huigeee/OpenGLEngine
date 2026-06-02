#include "../include/Cube.h"
#include "../include/UnlitMaterial.h"

#define LOG_TAG "Cube"

Cube::Cube() : vertexBuffer(0), indexBuffer(0),
         camera(nullptr), projectionMatrix(nullptr), vertexCount(0) {
}

void Cube::init() {
    UnlitMaterial* mat = new UnlitMaterial();
    mat->init();
    mat->setColor(1.0f, 1.0f, 1.0f, 1.0f);
    setMaterial(mat);

    float vertices[] = {
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f
    };

    unsigned short indices[] = {
        0, 1, 2, 0, 2, 3,
        4, 5, 6, 4, 6, 7,
        8, 9, 10, 8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23
    };

    vertexCount = 36;

    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

void Cube::setCamera(Camera* camera) {
    this->camera = camera;
}

void Cube::setProjectionMatrix(float* projection) {
    this->projectionMatrix = projection;
}

void Cube::render() {
    if (material == nullptr || vertexBuffer == 0 || camera == nullptr || projectionMatrix == nullptr) {
        return;
    }

    float modelMatrix[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        position[0], position[1], position[2], 1
    };

    float* viewMatrix = camera->getViewMatrix();

    float vpMatrix[16];
    Matrix::multiply(vpMatrix, viewMatrix, projectionMatrix);
    float mvpMatrix[16];
    Matrix::multiply(mvpMatrix, modelMatrix, vpMatrix);

    material->setTransform(modelMatrix, viewMatrix, projectionMatrix);

    int attribCount;
    auto attribs = material->getVertexAttribs(attribCount);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

    for (int i = 0; i < attribCount; i++) {
        glVertexAttribPointer(attribs[i].location, attribs[i].size, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(attribs[i].offset));
        glEnableVertexAttribArray(attribs[i].location);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_SHORT, 0);

    for (int i = 0; i < attribCount; i++) {
        glDisableVertexAttribArray(attribs[i].location);
    }
}

void Cube::release() {
    if (vertexBuffer != 0) {
        glDeleteBuffers(1, &vertexBuffer);
        vertexBuffer = 0;
    }
    if (indexBuffer != 0) {
        glDeleteBuffers(1, &indexBuffer);
        indexBuffer = 0;
    }
}