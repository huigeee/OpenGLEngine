#include "../include/Sphere.h"
#include "../include/UnlitMaterial.h"

#define LOG_TAG "Sphere"

Sphere::Sphere() : vertexBuffer(0), indexBuffer(0),
           camera(nullptr), projectionMatrix(nullptr), vertexCount(0), useNormals(false), vertexStride(6) {
}

void Sphere::init() {
    vertexStride = 6;
    UnlitMaterial* mat = new UnlitMaterial();
    mat->init();
    mat->setColor(1.0f, 0.5f, 0.0f, 1.0f);
    setMaterial(mat);

    int stacks = 20;
    int slices = 20;
    float radius = 0.4f;

    float* vertices = new float[stacks * slices * 4 * 6];
    unsigned short* indices = new unsigned short[stacks * slices * 4 * 6];

    int vertIndex = 0;
    int indexCount = 0;

    for (int i = 0; i < stacks; i++) {
        float lat = 3.14159f * i / stacks;
        float nextLat = 3.14159f * (i + 1) / stacks;

        for (int j = 0; j < slices; j++) {
            float lon = 2 * 3.14159f * j / slices;
            float nextLon = 2 * 3.14159f * (j + 1) / slices;

            float x1 = radius * sinf(lat) * cosf(lon);
            float y1 = radius * cosf(lat);
            float z1 = radius * sinf(lat) * sinf(lon);

            float x2 = radius * sinf(lat) * cosf(nextLon);
            float y2 = radius * cosf(lat);
            float z2 = radius * sinf(lat) * sinf(nextLon);

            float x3 = radius * sinf(nextLat) * cosf(nextLon);
            float y3 = radius * cosf(nextLat);
            float z3 = radius * sinf(nextLat) * sinf(nextLon);

            float x4 = radius * sinf(nextLat) * cosf(lon);
            float y4 = radius * cosf(nextLat);
            float z4 = radius * sinf(nextLat) * sinf(lon);

            int baseIndex = vertIndex / 6;

            vertices[vertIndex++] = x1; vertices[vertIndex++] = y1; vertices[vertIndex++] = z1;
            vertices[vertIndex++] = 1.0f; vertices[vertIndex++] = 0.5f; vertices[vertIndex++] = 0.0f;

            vertices[vertIndex++] = x2; vertices[vertIndex++] = y2; vertices[vertIndex++] = z2;
            vertices[vertIndex++] = 1.0f; vertices[vertIndex++] = 0.5f; vertices[vertIndex++] = 0.0f;

            vertices[vertIndex++] = x3; vertices[vertIndex++] = y3; vertices[vertIndex++] = z3;
            vertices[vertIndex++] = 1.0f; vertices[vertIndex++] = 0.5f; vertices[vertIndex++] = 0.0f;

            vertices[vertIndex++] = x4; vertices[vertIndex++] = y4; vertices[vertIndex++] = z4;
            vertices[vertIndex++] = 1.0f; vertices[vertIndex++] = 0.5f; vertices[vertIndex++] = 0.0f;

            indices[indexCount++] = baseIndex;
            indices[indexCount++] = baseIndex + 1;
            indices[indexCount++] = baseIndex + 2;

            indices[indexCount++] = baseIndex;
            indices[indexCount++] = baseIndex + 2;
            indices[indexCount++] = baseIndex + 3;
        }
    }

    vertexCount = indexCount;

    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertIndex * sizeof(float), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned short), indices, GL_STATIC_DRAW);

    delete[] vertices;
    delete[] indices;
}

void Sphere::initWithNormalsAndUV() {
    useNormals = true;
    vertexStride = 8;

    int stacks = 20;
    int slices = 20;
    float radius = 0.4f;

    float* vertices = new float[stacks * slices * 4 * 8];
    unsigned short* indices = new unsigned short[stacks * slices * 4 * 6];

    int vertIndex = 0;
    int indexCount = 0;

    for (int i = 0; i < stacks; i++) {
        float lat = 3.14159f * i / stacks;
        float nextLat = 3.14159f * (i + 1) / stacks;

        for (int j = 0; j < slices; j++) {
            float lon = 2 * 3.14159f * j / slices;
            float nextLon = 2 * 3.14159f * (j + 1) / slices;

            float x1 = radius * sinf(lat) * cosf(lon);
            float y1 = radius * cosf(lat);
            float z1 = radius * sinf(lat) * sinf(lon);

            float x2 = radius * sinf(lat) * cosf(nextLon);
            float y2 = radius * cosf(lat);
            float z2 = radius * sinf(lat) * sinf(nextLon);

            float x3 = radius * sinf(nextLat) * cosf(nextLon);
            float y3 = radius * cosf(nextLat);
            float z3 = radius * sinf(nextLat) * sinf(nextLon);

            float x4 = radius * sinf(nextLat) * cosf(lon);
            float y4 = radius * cosf(nextLat);
            float z4 = radius * sinf(nextLat) * sinf(lon);

            float len1 = sqrtf(x1*x1 + y1*y1 + z1*z1);
            float len2 = sqrtf(x2*x2 + y2*y2 + z2*z2);
            float len3 = sqrtf(x3*x3 + y3*y3 + z3*z3);
            float len4 = sqrtf(x4*x4 + y4*y4 + z4*z4);

            float u1 = (float)j / slices;
            float u2 = (float)(j + 1) / slices;
            float v1 = (float)i / stacks;
            float v2 = (float)(i + 1) / stacks;

            int baseIndex = vertIndex / 8;

            vertices[vertIndex++] = x1; vertices[vertIndex++] = y1; vertices[vertIndex++] = z1;
            vertices[vertIndex++] = x1/len1; vertices[vertIndex++] = y1/len1; vertices[vertIndex++] = z1/len1;
            vertices[vertIndex++] = u1; vertices[vertIndex++] = v1;

            vertices[vertIndex++] = x2; vertices[vertIndex++] = y2; vertices[vertIndex++] = z2;
            vertices[vertIndex++] = x2/len2; vertices[vertIndex++] = y2/len2; vertices[vertIndex++] = z2/len2;
            vertices[vertIndex++] = u2; vertices[vertIndex++] = v1;

            vertices[vertIndex++] = x3; vertices[vertIndex++] = y3; vertices[vertIndex++] = z3;
            vertices[vertIndex++] = x3/len3; vertices[vertIndex++] = y3/len3; vertices[vertIndex++] = z3/len3;
            vertices[vertIndex++] = u2; vertices[vertIndex++] = v2;

            vertices[vertIndex++] = x4; vertices[vertIndex++] = y4; vertices[vertIndex++] = z4;
            vertices[vertIndex++] = x4/len4; vertices[vertIndex++] = y4/len4; vertices[vertIndex++] = z4/len4;
            vertices[vertIndex++] = u1; vertices[vertIndex++] = v2;

            indices[indexCount++] = baseIndex;
            indices[indexCount++] = baseIndex + 1;
            indices[indexCount++] = baseIndex + 2;

            indices[indexCount++] = baseIndex;
            indices[indexCount++] = baseIndex + 2;
            indices[indexCount++] = baseIndex + 3;
        }
    }

    vertexCount = indexCount;

    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertIndex * sizeof(float), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned short), indices, GL_STATIC_DRAW);

    delete[] vertices;
    delete[] indices;
}

void Sphere::initWithNormals() {
    initWithNormalsAndUV();
}

void Sphere::setCamera(Camera* camera) {
    this->camera = camera;
}

void Sphere::setProjectionMatrix(float* projection) {
    this->projectionMatrix = projection;
}

void Sphere::render() {
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
        glVertexAttribPointer(attribs[i].location, attribs[i].size, GL_FLOAT, GL_FALSE, vertexStride * sizeof(float), (void*)(attribs[i].offset));
        glEnableVertexAttribArray(attribs[i].location);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_SHORT, 0);

    for (int i = 0; i < attribCount; i++) {
        glDisableVertexAttribArray(attribs[i].location);
    }
}

void Sphere::release() {
    if (vertexBuffer != 0) {
        glDeleteBuffers(1, &vertexBuffer);
        vertexBuffer = 0;
    }
    if (indexBuffer != 0) {
        glDeleteBuffers(1, &indexBuffer);
        indexBuffer = 0;
    }
}