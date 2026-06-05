#include "SceneNode.h"
#include "MeshNode.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstring>

SceneNode::SceneNode(const std::string& n) : name(n) {}

SceneNode::~SceneNode() {
    if (ownsChildren) {
        for (SceneNode* c : children) delete c;
    }
    children.clear();
    meshes.clear();   // never owned
}

// ---------------------------------------------------------------------------
// Dirty-flag plumbing (P4)
// ---------------------------------------------------------------------------
//   localDirty   : my localMatrix needs rebuilding from TRS+bind+pivot.
//   worldDirty   : my worldMatrix needs recomputing as parent*local (and
//                  pushing into all my attached MeshNodes).
//   subtreeDirty : me OR somewhere below me is dirty; updateWorld() must
//                  descend into me. False means "skip this entire subtree".
//
//   Invariant: if any node X in a subtree has localDirty or worldDirty true,
//   then subtreeDirty is true on every ancestor up to (and including) any
//   root that updateWorld() will be invoked on. Setters establish this by
//   calling propagateSubtreeDirtyUp().
//
//   markLocalDirty(): only the node itself rebuilds its local; but its
//   world (and thus all descendants' worlds) must recompute, so we mark
//   the whole subtree worldDirty.
// ---------------------------------------------------------------------------

void SceneNode::markLocalDirty() {
    localDirty = true;
    markWorldDirtySubtree();   // also handles propagating subtreeDirty up
}

void SceneNode::markWorldDirtySubtree() {
    // Fast-out: if my world is already dirty AND my subtreeDirty is set,
    // then by induction every descendant is already worldDirty too — no
    // need to recurse again.
    if (worldDirty && subtreeDirty) {
        // still ensure ancestors know (cheap, walks until it sees a true)
        propagateSubtreeDirtyUp();
        return;
    }
    worldDirty   = true;
    subtreeDirty = true;
    for (SceneNode* c : children) {
        if (c) c->markWorldDirtySubtree();
    }
    propagateSubtreeDirtyUp();
}

void SceneNode::propagateSubtreeDirtyUp() {
    SceneNode* p = parent;
    while (p && !p->subtreeDirty) {
        p->subtreeDirty = true;
        p = p->parent;
    }
}

// ---------------------------------------------------------------------------
// Tree ops
// ---------------------------------------------------------------------------
void SceneNode::addChild(SceneNode* c) {
    if (!c) return;
    c->parent = this;
    children.push_back(c);
    // The child's world depends on our world, which it doesn't know yet —
    // force a recompute of its whole subtree on the next updateWorld pass.
    c->markWorldDirtySubtree();
}

void SceneNode::attachMesh(MeshNode* m) {
    if (!m) return;
    meshes.push_back(m);
    // Newly attached mesh has never received our worldMatrix — force a push
    // by marking ourselves worldDirty (children unaffected, but the helper
    // also sets subtreeDirty up the chain which is what we need).
    worldDirty   = true;
    subtreeDirty = true;
    propagateSubtreeDirtyUp();
}

bool SceneNode::detachMesh(MeshNode* m) {
    for (auto it = meshes.begin(); it != meshes.end(); ++it) {
        if (*it == m) { meshes.erase(it); return true; }
    }
    return false;
}

SceneNode* SceneNode::findByName(const std::string& n) {
    if (name == n) return this;
    for (SceneNode* c : children) {
        SceneNode* r = c->findByName(n);
        if (r) return r;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// TRS setters — every mutation funnels through markLocalDirty()
// ---------------------------------------------------------------------------
void SceneNode::setBindMatrix(const float* m16) {
    std::memcpy(bindMatrix, m16, sizeof(bindMatrix));
    markLocalDirty();
}
void SceneNode::setPivot(float x, float y, float z) {
    pivot[0]=x; pivot[1]=y; pivot[2]=z;
    markLocalDirty();
}
void SceneNode::setPosition(float x, float y, float z) {
    position[0]=x; position[1]=y; position[2]=z;
    markLocalDirty();
}
void SceneNode::setRotation(float xDeg, float yDeg, float zDeg) {
    rotation[0]=xDeg; rotation[1]=yDeg; rotation[2]=zDeg;
    markLocalDirty();
}
void SceneNode::setRotationY(float yDeg) {
    rotation[1]=yDeg;
    markLocalDirty();
}
void SceneNode::setScale(float s) {
    scale = s;
    markLocalDirty();
}

void SceneNode::setVisibleRecursive(bool v) {
    for (MeshNode* m : meshes) if (m) m->setVisible(v);
    for (SceneNode* c : children) c->setVisibleRecursive(v);
}

void SceneNode::getWorldMatrix(float* out16) const {
    std::memcpy(out16, worldMatrix, sizeof(worldMatrix));
}

// ---------------------------------------------------------------------------
// buildLocalMatrix:
//   local = bind * T(pos) * T(pivot) * Ry * Rx * Rz * S * T(-pivot)
// ---------------------------------------------------------------------------
void SceneNode::buildLocalMatrix(float* out) const {
    constexpr float D2R = 3.14159265358979323846f / 180.0f;
    glm::mat4 L = glm::make_mat4(bindMatrix);
    L = glm::translate(L, glm::vec3(position[0], position[1], position[2]));
    L = glm::translate(L, glm::vec3(pivot[0],    pivot[1],    pivot[2]));
    L = glm::rotate   (L, rotation[1] * D2R, glm::vec3(0, 1, 0));   // Ry
    L = glm::rotate   (L, rotation[0] * D2R, glm::vec3(1, 0, 0));   // Rx
    L = glm::rotate   (L, rotation[2] * D2R, glm::vec3(0, 0, 1));   // Rz
    L = glm::scale    (L, glm::vec3(scale, scale, scale));
    L = glm::translate(L, glm::vec3(-pivot[0], -pivot[1], -pivot[2]));
    std::memcpy(out, glm::value_ptr(L), 16 * sizeof(float));
}

// ---------------------------------------------------------------------------
// updateWorld — the per-frame traversal, now with pruning.
// ---------------------------------------------------------------------------
void SceneNode::updateWorld(const float* parentWorld) {
    // Whole subtree is clean → bail. This is the optimization: a fully
    // static branch costs us a single bool test per frame.
    if (!subtreeDirty) return;

    if (localDirty) {
        buildLocalMatrix(localMatrix);
        localDirty = false;
    }

    if (worldDirty) {
        glm::mat4 P = glm::make_mat4(parentWorld);
        glm::mat4 L = glm::make_mat4(localMatrix);
        glm::mat4 W = P * L;
        std::memcpy(worldMatrix, glm::value_ptr(W), sizeof(worldMatrix));

        // Push fresh world into every attached mesh.
        for (MeshNode* m : meshes) {
            if (m) m->setExternalMatrix(worldMatrix);
        }
        worldDirty = false;
    }

    // Recurse — children read our (now-current) worldMatrix.
    for (SceneNode* c : children) {
        if (c) c->updateWorld(worldMatrix);
    }

    subtreeDirty = false;
}
