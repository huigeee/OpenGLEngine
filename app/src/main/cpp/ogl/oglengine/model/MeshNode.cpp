#include "MeshNode.h"
#include "../include/Common.h"

MeshNode::MeshNode(Mesh&& m, const std::string& n)
    : mesh(std::move(m)), name(n) {}

MeshNode::~MeshNode() {
    release();
}

void MeshNode::render() {
    if (mesh.VAO == 0) return;
    glBindVertexArray(mesh.VAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indices.size(), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void MeshNode::release() {
    mesh.release();
}

void MeshNode::addTexture(GLuint id, const std::string& type, const std::string& path) {
    MeshTexture t;
    t.id   = id;
    t.type = type;
    t.path = path;
    mesh.textures.push_back(t);
}

GLuint MeshNode::getTextureId(const std::string& type) const {
    for (const auto& t : mesh.textures) {
        if (t.type == type) return t.id;
    }
    return 0;
}

void MeshNode::getModelMatrix(float* out) {
    if (externalMatrixEnabled) {
        for (int i = 0; i < 16; ++i) out[i] = externalMatrix[i];
        return;
    }
    Object3D::getModelMatrix(out);
}
