#include "../include/GroundPlane.h"
#include <GLES3/gl3.h>

GroundPlane::GroundPlane() = default;

void GroundPlane::init() {
    const float h = halfSize;
    const float uv = uvTile;

    // Two triangles covering the XZ plane at Y=0.
    // Vertex layout: pos(3) + normal(3) + uv(2) = 8 floats. Normal = +Y.
    float vertices[] = {
        -h, 0.0f, -h,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
         h, 0.0f, -h,   0.0f, 1.0f, 0.0f,   uv,   0.0f,
         h, 0.0f,  h,   0.0f, 1.0f, 0.0f,   uv,   uv,
        -h, 0.0f,  h,   0.0f, 1.0f, 0.0f,   0.0f, uv,
    };
    unsigned int indices[] = { 0, 2, 1,  0, 3, 2 };
    indexCount = 6;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // aPos(0), aNormal(1), aTexCoords(2)
    const unsigned int stride = 8 * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
}

void GroundPlane::render() {
    if (vao == 0) return;
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void GroundPlane::release() {
    if (vao != 0) { glDeleteVertexArrays(1, &vao); vao = 0; }
    if (vbo != 0) { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (ebo != 0) { glDeleteBuffers(1, &ebo); ebo = 0; }
}
