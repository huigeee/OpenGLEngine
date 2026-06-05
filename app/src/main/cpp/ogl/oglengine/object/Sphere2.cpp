#include "../include/Sphere2.h"
#include "../include/Material.h"
#include "../include/Common.h"
#include <vector>

#define LOG_TAG "Sphere2"

Sphere2::Sphere2() : camera(nullptr), projectionMatrix(nullptr),
    vao(0), vbo(0), ebo(0), indexCount(0) {
}

void Sphere2::init() {
    initGeometry(1.0f);
}

void Sphere2::initGeometry(float radius, bool createDefaultVAO) {
    const unsigned int X_SEGMENTS = 64;
    const unsigned int Y_SEGMENTS = 64;
    const float PI = 3.14159265359f;

    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> uvs;
    std::vector<unsigned int> indices;

    for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            float xPos = cosf(xSegment * 2.0f * PI) * sinf(ySegment * PI);
            float yPos = cosf(ySegment * PI);
            float zPos = sinf(xSegment * 2.0f * PI) * sinf(ySegment * PI);

            positions.push_back(xPos * radius);
            positions.push_back(yPos * radius);
            positions.push_back(zPos * radius);

            normals.push_back(xPos);
            normals.push_back(yPos);
            normals.push_back(zPos);

            uvs.push_back(xSegment);
            uvs.push_back(ySegment);
        }
    }

    bool oddRow = false;
    for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
        if (!oddRow) {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
                indices.push_back(y * (X_SEGMENTS + 1) + x);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            }
        } else {
            for (int x = X_SEGMENTS; x >= 0; --x) {
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                indices.push_back(y * (X_SEGMENTS + 1) + x);
            }
        }
        oddRow = !oddRow;
    }
    indexCount = (unsigned int)indices.size();

    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    std::vector<float> interleaved;
    interleaved.reserve(positions.size() + normals.size() + uvs.size());
    for (size_t i = 0; i < positions.size() / 3; ++i) {
        interleaved.push_back(positions[i * 3]);
        interleaved.push_back(positions[i * 3 + 1]);
        interleaved.push_back(positions[i * 3 + 2]);
        interleaved.push_back(normals[i * 3]);
        interleaved.push_back(normals[i * 3 + 1]);
        interleaved.push_back(normals[i * 3 + 2]);
        interleaved.push_back(uvs[i * 2]);
        interleaved.push_back(uvs[i * 2 + 1]);
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizei)(interleaved.size() * sizeof(float)), interleaved.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizei)(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);

    vao = 0;
    stride = 8 * sizeof(float);
    if (createDefaultVAO) {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
        glBindVertexArray(0);
    }
}

void Sphere2::setCamera(Camera* camera) {
    this->camera = camera;
}

void Sphere2::setProjectionMatrix(float* projection) {
    this->projectionMatrix = projection;
}

void Sphere2::render() {
    if (vao == 0) {
        if (vbo == 0 || ebo == 0 || material == nullptr || !material->initialized) {
            return;
        }
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        int attribCount = 0;
        const Material::VertexAttrib* attribs = material->getVertexAttribs(attribCount);
        for (int i = 0; i < attribCount; i++) {
            glEnableVertexAttribArray(attribs[i].location);
            glVertexAttribPointer(attribs[i].location, attribs[i].size,
                GL_FLOAT, GL_FALSE, stride, (void*)(intptr_t)attribs[i].offset);
        }
        glBindVertexArray(0);
    }

    if (material != nullptr && material->isIBL()) {
    } else {
        if (material == nullptr) {
            return;
        }
        // NOTE: do NOT call material->setTransform() here.
        // Scene::renderObjects() already configures transform + uniforms +
        // glUseProgram on the material BEFORE invoking obj->render().
        // Doing it again here would re-bind the material's program, which
        // breaks the shadow pass (the shadow program is bound by
        // ShadowEffect::renderShadowPass before Scene::renderObjectsForShadow
        // calls obj->render(); re-binding the unlit/lit shader here would
        // cause subsequent geometry to write garbage into the shadow map).
    }

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Sphere2::release() {
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    if (ebo != 0) {
        glDeleteBuffers(1, &ebo);
        ebo = 0;
    }
}
