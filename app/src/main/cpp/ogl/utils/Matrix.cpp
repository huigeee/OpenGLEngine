#include "../include/Matrix.h"
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstring>
#include <cmath>

void Matrix::perspective(float* matrix, float fovY, float aspect, float nearZ, float farZ) {
    glm::mat4 mat = glm::perspective(fovY, aspect, nearZ, farZ);
    memcpy(matrix, &mat[0][0], 16 * sizeof(float));
}

void Matrix::ortho(float* matrix, float left, float right, float bottom, float top, float nearZ, float farZ) {
    glm::mat4 mat = glm::ortho(left, right, bottom, top, nearZ, farZ);
    memcpy(matrix, &mat[0][0], 16 * sizeof(float));
}

void Matrix::multiply(float* result, float* a, float* b) {
    glm::mat4 matA = glm::make_mat4(a);
    glm::mat4 matB = glm::make_mat4(b);
    glm::mat4 resultMat = matB * matA;
    memcpy(result, &resultMat[0][0], 16 * sizeof(float));
}

void Matrix::identity(float* matrix) {
    glm::mat4 mat(1.0f);
    memcpy(matrix, &mat[0][0], 16 * sizeof(float));
}

void Matrix::translate(float* matrix, float x, float y, float z) {
    glm::mat4 mat = glm::make_mat4(matrix);
    mat = glm::translate(mat, glm::vec3(x, y, z));
    memcpy(matrix, &mat[0][0], 16 * sizeof(float));
}

void Matrix::rotate(float* matrix, float angle, float x, float y, float z) {
    glm::mat4 mat = glm::make_mat4(matrix);
    mat = glm::rotate(mat, angle, glm::vec3(x, y, z));
    memcpy(matrix, &mat[0][0], 16 * sizeof(float));
}

void Matrix::scale(float* matrix, float x, float y, float z) {
    glm::mat4 mat = glm::make_mat4(matrix);
    mat = glm::scale(mat, glm::vec3(x, y, z));
    memcpy(matrix, &mat[0][0], 16 * sizeof(float));
}

void Matrix::lookAt(float* matrix, float eyeX, float eyeY, float eyeZ, float targetX, float targetY, float targetZ, float upX, float upY, float upZ) {
    glm::vec3 eye(eyeX, eyeY, eyeZ);
    glm::vec3 target(targetX, targetY, targetZ);
    glm::vec3 up(upX, upY, upZ);
    glm::mat4 m = glm::lookAt(eye, target, up);
    memcpy(matrix, &m[0][0], 16 * sizeof(float));
}