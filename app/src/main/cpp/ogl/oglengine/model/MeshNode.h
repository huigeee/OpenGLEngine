#pragma once

#include "../include/Object3D.h"
#include "../include/Material.h"
#include "ModelLoader.h"   // Mesh, MeshTexture
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// MeshNode — one mesh from a loaded model, exposed as an Object3D.
//
//   • Owns the GPU-side VAO/VBO/EBO (via the embedded Mesh struct).
//   • Owns and carries the per-mesh textures loaded by assimp.
//   • Owns its own Material* (set by the user via MeshInitCallback).
//   • Plugs into Scene's objects[] list just like Sphere2 / Cube2.
//   • visible, position, scale, rotation all inherited from Object3D.
//
// Usage (inside MeshInitCallback):
//   node->setVisible(false);
//   node->getMaterial<IBLPBRMaterial>()->setOpacity(0.5f);
//   node->loadExtraTexture("path/to/tex.jpg", "opacity");
// ---------------------------------------------------------------------------
class MeshNode : public Object3D {
public:
    // Take ownership of a Mesh (moves GPU handles)
    explicit MeshNode(Mesh&& mesh, const std::string& name = "");
    ~MeshNode() override;

    // Object3D interface
    void init()    override {}   // GPU data already uploaded by ModelLoader
    void render()  override;
    void release() override;

    // ----- Texture management -----
    // Access textures loaded from the model file
    const std::vector<MeshTexture>& getTextures() const { return mesh.textures; }

    // Add an extra texture not in the original model (user-supplied GL id)
    void addTexture(GLuint id, const std::string& type, const std::string& path = "");

    // Look up the first texture of a given type ("diffuse","normal",etc.)
    // Returns 0 if not found.
    GLuint getTextureId(const std::string& type) const;

    // ----- Material convenience -----
    // Typed getter — returns nullptr if the material is not of type T
    template<typename T>
    T* getMaterial() { return dynamic_cast<T*>(material); }

    // ----- Info -----
    const std::string& getName() const { return name; }
    int  getVertexCount() const { return (int)mesh.vertices.size(); }

    // ----- Hierarchy support -----
    // If a SceneNode owns this MeshNode, it pushes a world matrix every frame
    // here. When set, getModelMatrix() returns this matrix verbatim and ignores
    // the local position/rotation/scale fields. Set externalMatrixEnabled=false
    // to fall back to the standalone Object3D behavior.
    void setExternalMatrix(const float* m16) {
        for (int i = 0; i < 16; ++i) externalMatrix[i] = m16[i];
        externalMatrixEnabled = true;
    }
    void clearExternalMatrix() { externalMatrixEnabled = false; }
    bool hasExternalMatrix() const { return externalMatrixEnabled; }

    // Object3D override — picks externalMatrix if active.
    void getModelMatrix(float* outMatrix) override;

private:
    Mesh        mesh;
    std::string name;

    bool  externalMatrixEnabled = false;
    float externalMatrix[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
};
