#include "CollisionPrimitives.h"
#include <algorithm>
#include <cmath>

void CollisionWorld::addSphere(const glm::vec3& center, float radius) {
    spheres_.push_back({center, radius});
}

void CollisionWorld::addPlane(const glm::vec3& normal, float d) {
    planes_.push_back({glm::normalize(normal), d});
}

void CollisionWorld::addPlaneFromPoint(const glm::vec3& point, const glm::vec3& normal) {
    glm::vec3 n = glm::normalize(normal);
    planes_.push_back({n, -glm::dot(n, point)});
}

bool CollisionWorld::resolveVertex(glm::vec3& pos, float radius) const {
    bool pushed = false;

    for (const auto& sphere : spheres_) {
        glm::vec3 delta = pos - sphere.center;
        float dist = glm::length(delta);
        float minDist = sphere.radius + radius;
        if (dist < minDist && dist > 1e-8f) {
            pos = sphere.center + (delta / dist) * minDist;
            pushed = true;
        }
    }

    for (const auto& plane : planes_) {
        float signedDist = glm::dot(pos, plane.normal) + plane.d;
        float minDist = radius;
        if (signedDist < minDist) {
            pos += plane.normal * (minDist - signedDist);
            pushed = true;
        }
    }

    return pushed;
}

void CollisionWorld::clear() {
    spheres_.clear();
    planes_.clear();
}
