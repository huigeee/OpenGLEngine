#include "ClothSimulation.h"
#include <cmath>
#include <algorithm>
#include <cstdio>

ClothSimulation::ClothSimulation()
    : cols_(0), rows_(0), spacing_(0.0f) {
}

void ClothSimulation::initGrid(int cols, int rows, float spacing) {
    cols_ = cols;
    rows_ = rows;
    spacing_ = spacing;

    vertices_.clear();
    indices_.clear();
    springs_.clear();

    const int totalVerts = cols * rows;

    // --- vertices ---
    vertices_.reserve(totalVerts);
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            ClothVertex v;
            float x = (float)col * spacing - (float)(cols - 1) * spacing * 0.5f;
            float z = (float)row * spacing - (float)(rows - 1) * spacing * 0.5f;
            v.position = glm::vec3(x, 0.0f, z);
            v.oldPosition = v.position;
            v.velocity = glm::vec3(0.0f);
            v.mass = 0.5f;
            v.pinned = false;
            vertices_.push_back(v);
        }
    }

    // --- triangle indices ---
    indices_.reserve((cols - 1) * (rows - 1) * 6);
    for (int row = 0; row < rows - 1; ++row) {
        for (int col = 0; col < cols - 1; ++col) {
            int i0 = row * cols + col;
            int i1 = i0 + 1;
            int i2 = (row + 1) * cols + col;
            int i3 = i2 + 1;

            indices_.push_back(i0);
            indices_.push_back(i2);
            indices_.push_back(i1);
            indices_.push_back(i1);
            indices_.push_back(i2);
            indices_.push_back(i3);
        }
    }

    buildSprings();
}

void ClothSimulation::pinTopRow() {
    for (int col = 0; col < cols_; ++col) {
        vertices_[col].pinned = true;
    }
}

void ClothSimulation::pinCorners() {
    if (cols_ < 2 || rows_ < 2) return;
    vertices_[0].pinned = true;
    vertices_[cols_ - 1].pinned = true;
    vertices_[(rows_ - 1) * cols_].pinned = true;
    vertices_[(rows_ - 1) * cols_ + (cols_ - 1)].pinned = true;
}

void ClothSimulation::pinTopCenter(float fraction) {
    int pinCount = std::max(1, (int)(cols_ * fraction));
    int start = (cols_ - pinCount) / 2;
    for (int c = start; c < start + pinCount && c < cols_; ++c) {
        vertices_[c].pinned = true;
    }
}

void ClothSimulation::pinVertex(int col, int row) {
    if (col < 0 || col >= cols_ || row < 0 || row >= rows_) return;
    vertices_[row * cols_ + col].pinned = true;
}

void ClothSimulation::buildSprings() {
    // Structural
    for (int row = 0; row < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            int idx = row * cols_ + col;
            if (col < cols_ - 1) {
                int right = idx + 1;
                float len = glm::distance(vertices_[idx].position, vertices_[right].position);
                springs_.push_back({idx, right, len, SPRING_STRUCTURAL});
            }
            if (row < rows_ - 1) {
                int down = idx + cols_;
                float len = glm::distance(vertices_[idx].position, vertices_[down].position);
                springs_.push_back({idx, down, len, SPRING_STRUCTURAL});
            }
        }
    }
    // Shear
    for (int row = 0; row < rows_ - 1; ++row) {
        for (int col = 0; col < cols_ - 1; ++col) {
            int idx = row * cols_ + col;
            int idxRight = idx + 1;
            int idxDown = idx + cols_;
            int idxDiag = idxDown + 1;
            float len1 = glm::distance(vertices_[idx].position, vertices_[idxDiag].position);
            springs_.push_back({idx, idxDiag, len1, SPRING_SHEAR});
            float len2 = glm::distance(vertices_[idxRight].position, vertices_[idxDown].position);
            springs_.push_back({idxRight, idxDown, len2, SPRING_SHEAR});
        }
    }
    // Bend
    for (int row = 0; row < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            int idx = row * cols_ + col;
            if (col < cols_ - 2) {
                int skip2 = idx + 2;
                float len = glm::distance(vertices_[idx].position, vertices_[skip2].position);
                springs_.push_back({idx, skip2, len, SPRING_BEND});
            }
            if (row < rows_ - 2) {
                int skip2 = idx + 2 * cols_;
                float len = glm::distance(vertices_[idx].position, vertices_[skip2].position);
                springs_.push_back({idx, skip2, len, SPRING_BEND});
            }
        }
    }
}

void ClothSimulation::solveConstraints() {
    for (int iter = 0; iter < constraintIterations_; ++iter) {
        for (const auto& spring : springs_) {
            ClothVertex& a = vertices_[spring.v0];
            ClothVertex& b = vertices_[spring.v1];

            if (a.pinned && b.pinned) continue;

            glm::vec3 delta = b.position - a.position;
            float dist = glm::length(delta);
            if (dist < 1e-8f) continue;

            float invMassA = a.pinned ? 0.0f : (1.0f / a.mass);
            float invMassB = b.pinned ? 0.0f : (1.0f / b.mass);
            float totalInvMass = invMassA + invMassB;
            if (totalInvMass < 1e-10f) continue;

            float correction = (dist - spring.restLength) / dist;
            glm::vec3 correctionVec = delta * correction / totalInvMass;

            a.position += correctionVec * invMassA;
            b.position -= correctionVec * invMassB;
        }
    }
}

void ClothSimulation::update(float dt, const CollisionWorld& collisionWorld) {
    if (dt > 0.05f) dt = 0.05f;

    windTime_ += dt;

    // Animated wind: two overlapping sine waves for natural gusting / lulls
    float gust1 = sinf(windTime_ * 1.3f);
    float gust2 = sinf(windTime_ * 2.7f + 1.2f);
    float gust  = gust1 * 0.6f + gust2 * 0.4f;
    float windStrength = 0.5f + 0.5f * gust;
    glm::vec3 wind = baseWind_ * windStrength;

    // 1. Predict positions
    for (auto& v : vertices_) {
        if (v.pinned) continue;

        v.oldPosition = v.position;
        v.velocity += wind * dt;
        v.velocity += gravity_ * dt;
        v.velocity *= damping_;
        v.position += v.velocity * dt;
    }

    // 2. Solve spring constraints
    solveConstraints();

    // 3. Collision resolution
    for (auto& v : vertices_) {
        if (v.pinned) continue;
        collisionWorld.resolveVertex(v.position, 0.0f);
    }

    // 4. Extract velocity
    for (auto& v : vertices_) {
        if (v.pinned) continue;
        v.velocity = (v.position - v.oldPosition) / dt;
    }
}
