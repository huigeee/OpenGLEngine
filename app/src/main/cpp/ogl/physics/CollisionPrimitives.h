#pragma once

#include <glm/glm.hpp>
#include <vector>

// ---------------------------------------------------------------------------
// Minimal collision primitives for cloth vertex collision.
// ---------------------------------------------------------------------------

struct CollisionSphere {
    glm::vec3 center;
    float radius;
};

struct CollisionPlane {
    glm::vec3 normal;
    float d; // plane equation: normal · p + d = 0  (d = -dot(normal, point))
};

class CollisionWorld {
public:
    void addSphere(const glm::vec3& center, float radius);
    void addPlane(const glm::vec3& normal, float d);
    void addPlaneFromPoint(const glm::vec3& point, const glm::vec3& normal);

    // Resolve a single vertex against all primitives, returns true if pushed.
    bool resolveVertex(glm::vec3& pos, float radius = 0.0f) const;

    void clear();

    const std::vector<CollisionSphere>& getSpheres() const { return spheres_; }
    const std::vector<CollisionPlane>& getPlanes() const { return planes_; }

private:
    std::vector<CollisionSphere> spheres_;
    std::vector<CollisionPlane> planes_;
};
