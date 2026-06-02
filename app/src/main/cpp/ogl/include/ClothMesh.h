#pragma once

#include "Object3D.h"
#include "../physics/ClothSimulation.h"
#include "../physics/CollisionPrimitives.h"

class Camera;
class ClothMaterial;

// ---------------------------------------------------------------------------
// ClothMesh — a dynamic cloth rectangle rendered via ClothMaterial.
//
// Simulation runs in WORLD space. VBO is in world coordinates.
// setPosition() sets both Objec3D position AND the sim offset.
// getModelMatrix() returns identity (VBO is already in world space).
// ---------------------------------------------------------------------------

class ClothMesh : public Object3D {
public:
    ClothMesh();
    ~ClothMesh() override;

    void init() override {}
    void initCloth(int cols, int rows, float spacing);

    void setCamera(Camera* cam);
    void setProjectionMatrix(const float* proj);

    // setPosition updates both Object3D position and offsets sim vertices.
    // Must call before first render.
    void setPositionWorld(float x, float y, float z);

    void render() override;
    void release() override;
    void onPreRender() override;
    void getModelMatrix(float* outMatrix) override;

    ClothSimulation& getSimulation() { return sim_; }
    void setCollisionWorld(CollisionWorld* world) { collisionWorld_ = world; }

private:
    void rebuildNormals();
    void syncSimToVBO();
    void uploadGeometry();
    void offsetSimToWorld();  // shift sim vertices from local to world

    ClothSimulation sim_;
    CollisionWorld* collisionWorld_ = nullptr;

    int cols_ = 0, rows_ = 0;
    int vertexCount_ = 0;
    int indexCount_ = 0;

    // Vertex layout: pos3 + normal3 + tex2 = 8 floats per vertex (WORLD coords)
    std::vector<float> vertexData_;
    std::vector<unsigned short> indexData_;

    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ebo_ = 0;

    Camera* camera_ = nullptr;
    const float* projectionMatrix_ = nullptr;
    bool worldOffsetApplied_ = false;  // true after first setPosition
};
