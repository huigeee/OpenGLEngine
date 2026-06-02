#include "../include/ClothMesh.h"
#include "../include/ClothMaterial.h"
#include "../include/Camera.h"
#include "../include/Common.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

ClothMesh::ClothMesh() : Object3D(), cols_(0), rows_(0), vertexCount_(0), indexCount_(0) {
}

ClothMesh::~ClothMesh() {
    release();
}

void ClothMesh::setCamera(Camera* cam) {
    camera_ = cam;
}

void ClothMesh::setProjectionMatrix(const float* proj) {
    projectionMatrix_ = proj;
}

void ClothMesh::setPositionWorld(float x, float y, float z) {
    // Store offset for sim-to-world
    position[0] = x;
    position[1] = y;
    position[2] = z;

    if (!worldOffsetApplied_) {
        // First call: shift sim vertices from local to world
        offsetSimToWorld();
        worldOffsetApplied_ = true;
    }
}

void ClothMesh::offsetSimToWorld() {
    auto& verts = const_cast<std::vector<ClothVertex>&>(sim_.getVertices());
    for (auto& v : verts) {
        // Don't offset pinned vertices (keep them in local coords relative to world)
        v.position.x += position[0];
        v.position.y += position[1];
        v.position.z += position[2];
        v.oldPosition = v.position;
    }
}

void ClothMesh::initCloth(int cols, int rows, float spacing) {
    release();

    cols_ = cols;
    rows_ = rows;
    worldOffsetApplied_ = false;

    // Simulation starts in local coords (centered at origin)
    sim_.initGrid(cols_, rows_, spacing);

    vertexCount_ = cols_ * rows_;
    const int floatsPerVert = 8;
    vertexData_.resize(vertexCount_ * floatsPerVert, 0.0f);

    // Fill VBO with initial sim positions (still local at this point)
    const auto& simVerts = sim_.getVertices();
    for (int i = 0; i < vertexCount_; ++i) {
        vertexData_[i * floatsPerVert + 0] = simVerts[i].position.x;
        vertexData_[i * floatsPerVert + 1] = simVerts[i].position.y;
        vertexData_[i * floatsPerVert + 2] = simVerts[i].position.z;
        vertexData_[i * floatsPerVert + 3] = 0.0f;
        vertexData_[i * floatsPerVert + 4] = 1.0f;
        vertexData_[i * floatsPerVert + 5] = 0.0f;
        int col = i % cols_;
        int row = i / cols_;
        vertexData_[i * floatsPerVert + 6] = (float)col / (float)(cols_ - 1);
        vertexData_[i * floatsPerVert + 7] = (float)row / (float)(rows_ - 1);
    }
    rebuildNormals();

    const auto& simIdx = sim_.getIndices();
    indexData_ = simIdx;
    indexCount_ = (int)simIdx.size();

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    uploadGeometry();

    LOGI("ClothMesh init: %dx%d = %d verts, %d indices",
         cols_, rows_, vertexCount_, indexCount_);
}

void ClothMesh::rebuildNormals() {
    const int floatsPerVert = 8;
    std::vector<glm::vec3> normals(vertexCount_, glm::vec3(0.0f));

    for (int i = 0; i < indexCount_; i += 3) {
        unsigned short i0 = indexData_[i];
        unsigned short i1 = indexData_[i + 1];
        unsigned short i2 = indexData_[i + 2];

        auto p0 = glm::make_vec3(&vertexData_[i0 * floatsPerVert]);
        auto p1 = glm::make_vec3(&vertexData_[i1 * floatsPerVert]);
        auto p2 = glm::make_vec3(&vertexData_[i2 * floatsPerVert]);

        glm::vec3 faceN = glm::cross(p1 - p0, p2 - p0);
        normals[i0] += faceN;
        normals[i1] += faceN;
        normals[i2] += faceN;
    }

    for (int i = 0; i < vertexCount_; ++i) {
        glm::vec3 n = glm::normalize(normals[i]);
        vertexData_[i * floatsPerVert + 3] = n.x;
        vertexData_[i * floatsPerVert + 4] = n.y;
        vertexData_[i * floatsPerVert + 5] = n.z;
    }
}

void ClothMesh::syncSimToVBO() {
    const auto& simVerts = sim_.getVertices();
    const int floatsPerVert = 8;

    for (int i = 0; i < vertexCount_; ++i) {
        vertexData_[i * floatsPerVert + 0] = simVerts[i].position.x;
        vertexData_[i * floatsPerVert + 1] = simVerts[i].position.y;
        vertexData_[i * floatsPerVert + 2] = simVerts[i].position.z;
    }
    rebuildNormals();
}

void ClothMesh::uploadGeometry() {
    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 vertexData_.size() * sizeof(float),
                 vertexData_.data(),
                 GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indexData_.size() * sizeof(unsigned short),
                 indexData_.data(),
                 GL_STATIC_DRAW);

    const int stride = 8 * sizeof(float);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void ClothMesh::onPreRender() {
    // Physics step — sim is in WORLD space, collision is in WORLD space
    const float dt = 0.016f;
    sim_.update(dt, collisionWorld_ ? *collisionWorld_ : CollisionWorld());

    // Copy world-space sim positions into VBO
    syncSimToVBO();

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    vertexData_.size() * sizeof(float),
                    vertexData_.data());
}

void ClothMesh::render() {
    if (!material || vao_ == 0) return;

    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
}

void ClothMesh::getModelMatrix(float* outMatrix) {
    // VBO is in world space — model matrix = identity
    for (int i = 0; i < 16; ++i) outMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
}

void ClothMesh::release() {
    if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
    if (ebo_) { glDeleteBuffers(1, &ebo_); ebo_ = 0; }
    if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
    vertexData_.clear();
    indexData_.clear();
    vertexCount_ = 0;
    indexCount_ = 0;
}
