#include "../include/Object3D.h"
#include "../include/Material.h"
#include "../include/Shader.h"
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Object3D::Object3D() : visible(true), material(nullptr) {
}

Object3D::~Object3D() {
    if (material != nullptr) {
        material->release();
        delete material;
        material = nullptr;
    }
}

void Object3D::beforeRender() {
    if (material != nullptr) {
        material->beforeRenderVirtual();
        if (!material->isIBL() && material->getShader() != nullptr) {
            material->getShader()->use();
        }
    }
}

void Object3D::afterRender() {
}

void Object3D::setVisible(bool visible) {
    this->visible = visible;
}

bool Object3D::isVisible() const {
    return visible;
}

void Object3D::setPosition(float x, float y, float z) {
    position[0] = x;
    position[1] = y;
    position[2] = z;
}

float* Object3D::getPosition() {
    return position;
}

float Object3D::getScale() {
    return scale;
}

void Object3D::setScale(float scale) {
    this->scale = scale;
}

void Object3D::setRotation(float xDeg, float yDeg, float zDeg) {
    rotation[0] = xDeg;
    rotation[1] = yDeg;
    rotation[2] = zDeg;
}

void Object3D::setRotationY(float yDeg) {
    rotation[1] = yDeg;
}

void Object3D::setRotationAxis(float ax, float ay, float az, float angleDeg) {
    axis[0] = ax; axis[1] = ay; axis[2] = az;
    axisAngle = angleDeg;
}

void Object3D::setPivot(float px, float py, float pz) {
    pivot[0] = px; pivot[1] = py; pivot[2] = pz;
}

float* Object3D::getRotation() {
    return rotation;
}

void Object3D::getModelMatrix(float* m) {
    // M = T(position) * T(pivot) * R_axis * R_euler * T(-pivot) * S(scale)
    //
    // - Euler order Ry * Rx * Rz preserves prior behaviour
    // - When pivot==(0,0,0) and axisAngle==0, this matches the original
    //   T * Ry*Rx*Rz * S exactly.
    const float deg2rad = 3.14159265358979323846f / 180.0f;

    glm::mat4 M(1.0f);
    M = glm::translate(M, glm::vec3(position[0], position[1], position[2]));
    M = glm::translate(M, glm::vec3(pivot[0], pivot[1], pivot[2]));

    // Arbitrary-axis self rotation (skip if angle ~0 or axis is zero)
    if (fabsf(axisAngle) > 1e-6f) {
        glm::vec3 a(axis[0], axis[1], axis[2]);
        if (glm::dot(a, a) > 1e-12f) {
            M = glm::rotate(M, axisAngle * deg2rad, glm::normalize(a));
        }
    }

    // Euler Ry * Rx * Rz
    M = glm::rotate(M, rotation[1] * deg2rad, glm::vec3(0.0f, 1.0f, 0.0f));
    M = glm::rotate(M, rotation[0] * deg2rad, glm::vec3(1.0f, 0.0f, 0.0f));
    M = glm::rotate(M, rotation[2] * deg2rad, glm::vec3(0.0f, 0.0f, 1.0f));

    M = glm::translate(M, glm::vec3(-pivot[0], -pivot[1], -pivot[2]));
    M = glm::scale(M, glm::vec3(scale));

    const float* p = glm::value_ptr(M);
    for (int i = 0; i < 16; ++i) m[i] = p[i];
}

void Object3D::setMaterial(Material* mat) {
    if (material != nullptr) {
        material->release();
        delete material;
    }
    material = mat;
}