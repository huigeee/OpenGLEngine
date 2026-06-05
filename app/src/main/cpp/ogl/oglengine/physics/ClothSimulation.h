#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "CollisionPrimitives.h"

// ---------------------------------------------------------------------------
// ClothSimulation — PBD (Position Based Dynamics) cloth solver.
//
// Creates a grid of vertices connected by structural, shear, and bend
// springs. Each frame: predict positions * gravity, run constraint solver,
// collide against primitives, update velocity.
// ---------------------------------------------------------------------------

enum SpringType {
    SPRING_STRUCTURAL = 0,
    SPRING_SHEAR,
    SPRING_BEND
};

struct ClothVertex {
    glm::vec3 position;
    glm::vec3 oldPosition;   // for PBD velocity extraction
    glm::vec3 velocity;
    float mass;
    bool pinned;             // vertex does not move
};

struct Spring {
    int v0, v1;
    float restLength;
    SpringType type;
};

class ClothSimulation {
public:
    ClothSimulation();

    // Create a cols × rows grid with given spacing (world units).
    // Returns the number of triangles = (cols-1)*(rows-1)*2.
    void initGrid(int cols, int rows, float spacing);

    // Pin the top row (row 0) so the cloth hangs.
    void pinTopRow();

    // Pin all four corners of the grid.
    void pinCorners();

    // Pin the center fraction of the top row — e.g. 0.2 = middle 1/5.
    // Useful for a coat-hanger effect: the cloth is pinned at one point / short segment.
    void pinTopCenter(float fraction = 0.2f);

    // Pin a single vertex at (col, row) — for a single-point hanger hook.
    void pinVertex(int col, int row);

    // Advance simulation by dt seconds.
    void update(float dt, const CollisionWorld& collisionWorld);

    // Accessors for rendering.
    const std::vector<ClothVertex>& getVertices() const { return vertices_; }
    const std::vector<unsigned short>& getIndices() const { return indices_; }
    int getCols() const { return cols_; }
    int getRows() const { return rows_; }

    void setGravity(const glm::vec3& g) { gravity_ = g; }
    void setDamping(float d) { damping_ = d; }
    void setConstraintIterations(int iters) { constraintIterations_ = iters; }

    // Per-vertex wind force — simple directional drag.
    void setWind(const glm::vec3& wind) { baseWind_ = wind; }

private:
    int cols_, rows_;
    float spacing_;
    float windTime_ = 0.0f;  // accumulates dt for animated wind

    std::vector<ClothVertex> vertices_;
    std::vector<unsigned short> indices_;    // triangle strip indices
    std::vector<Spring> springs_;

    glm::vec3 gravity_ = glm::vec3(0.0f, -2.0f, 0.0f);  // very soft gravity so cloth rests on sphere
    float damping_ = 0.98f;
    int constraintIterations_ = 6;
    glm::vec3 baseWind_ = glm::vec3(0.0f);    // base wind direction (animated internally)

    void buildSprings();
    void solveConstraints();
};
