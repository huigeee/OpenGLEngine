#pragma once

#include <string>
#include <vector>
#include <functional>

class MeshNode;

// ---------------------------------------------------------------------------
// SceneNode — a transform-tree group node.
//
// Purpose:
//   • Reintroduce the parent/child hierarchy that ModelLoader currently
//     flattens. SceneNode does NOT participate in Scene::objects[]; it is
//     purely a transform container.
//   • A SceneNode owns:
//       - a local TRS (relative to parent),
//       - a pivot offset (the rotation/scale center in *its own local space*,
//         used so a door rotates around its hinge instead of the world origin),
//       - an optional bind matrix (e.g. coming from aiNode->mTransformation),
//       - 0..N child SceneNodes,
//       - 0..N MeshNode references (non-owning — Scene owns them).
//   • Each frame the Scene calls root->updateWorld(identity); this propagates
//     a world matrix down the tree and pushes it into every attached MeshNode
//     via MeshNode::setExternalMatrix(...).
//
// World matrix formula:
//     local = bindMatrix * Translate(pivot) * Tx,Ty,Tz * Rx,Ry,Rz * S * Translate(-pivot)
//     world = parentWorld * local
//
//   bindMatrix carries the original DCC transform (so the assembled model
//   keeps its rest pose). The user-facing TRS edits are applied *around the
//   pivot* on top of that bind pose, which is exactly the "open the door"
//   semantics we want.
// ---------------------------------------------------------------------------
class SceneNode {
public:
    explicit SceneNode(const std::string& name = "");
    ~SceneNode();

    // ---- tree ops ----
    void  setParent(SceneNode* p) { parent = p; }
    void  addChild(SceneNode* c);
    void  attachMesh(MeshNode* m);
    bool  detachMesh(MeshNode* m);   // remove m from this node's mesh list (non-recursive)
    SceneNode* findByName(const std::string& n);  // depth-first
    SceneNode* getParent() const { return parent; }
    const std::vector<SceneNode*>& getChildren() const { return children; }
    const std::vector<MeshNode*>&  getMeshes()    const { return meshes;   }

    // ---- transform ----
    void setBindMatrix(const float* m16);     // 16 floats, column-major
    void setPivot(float x, float y, float z);
    void setPosition(float x, float y, float z);
    void setRotation(float xDeg, float yDeg, float zDeg);
    void setRotationY(float yDeg);
    void setScale(float s);
    float getRotationY() const { return rotation[1]; }

    // ---- visibility (recursive into MeshNodes & children) ----
    void setVisibleRecursive(bool v);

    // ---- ownership of children ----
    // If true (default), destructor deletes child SceneNodes. MeshNodes are
    // never deleted by SceneNode (Scene owns them).
    void setOwnsChildren(bool b) { ownsChildren = b; }

    // ---- per-frame: walk tree, compute world matrices, push to MeshNodes ----
    // Pruning rules (P4 dirty-flags):
    //   • subtreeDirty=false → entire subtree skipped, zero work.
    //   • localDirty         → rebuild local matrix, then clear.
    //   • worldDirty         → recompute world = parent*local, push to meshes,
    //                          then clear.
    // Setters mark this node localDirty and recursively mark every descendant
    // worldDirty (their world depends on ours), and propagate subtreeDirty
    // up the parent chain so updateWorld() can find them.
    void updateWorld(const float* parentWorld);

    // explicit dirty markers (mostly internal but exposed for advanced cases)
    void markLocalDirty();           // local TRS/bind/pivot changed
    void markWorldDirtySubtree();    // me + all descendants need world recompute
    void propagateSubtreeDirtyUp();  // walk parent chain setting subtreeDirty

    const std::string& getName() const { return name; }
    void getWorldMatrix(float* out16) const;

private:
    void buildLocalMatrix(float* out16) const;

    std::string             name;
    SceneNode*              parent  = nullptr;
    std::vector<SceneNode*> children;
    std::vector<MeshNode*>  meshes;
    bool                    ownsChildren = true;

    // local TRS
    float position[3] = {0, 0, 0};
    float rotation[3] = {0, 0, 0};   // degrees, applied as Ry * Rx * Rz
    float scale       = 1.0f;
    float pivot[3]    = {0, 0, 0};

    float bindMatrix [16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    float localMatrix[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    float worldMatrix[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};

    // P4 dirty flags. Constructed true so the first frame produces a full
    // matrix tree and pushes externalMatrix into every attached MeshNode.
    bool localDirty   = true;   // rebuild localMatrix from TRS+bind+pivot
    bool worldDirty   = true;   // recompute world = parent*local, push to meshes
    bool subtreeDirty = true;   // descend into me; somewhere below something is dirty
};
