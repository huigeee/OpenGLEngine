#pragma once

#include "../include/Object3D.h"
#include "ModelLoader.h"
#include "MeshNode.h"
#include "SceneNode.h"
#include <string>
#include <vector>
#include <functional>

// ---------------------------------------------------------------------------
// ModelObject — loads a model file and exposes each mesh as a MeshNode.
//
// In addition, ModelObject can build a SceneNode hierarchy on top of the
// (currently flat) mesh list so users can group sub-parts (e.g. a car door)
// and animate them together. The SceneNode tree drives transforms; the
// MeshNodes are still added to the Scene's render list as before.
// ---------------------------------------------------------------------------

using MeshInitCallback = std::function<void(MeshNode*, int)>;
using MeshFilter       = std::function<bool(MeshNode*)>;

class ModelObject {
public:
    ModelObject();
    ~ModelObject();

    // Load model file.  For each mesh, creates a MeshNode (with the mesh name
    // set) and — if callback != nullptr — calls callback(node, index).
    // Returns false on failure.
    bool load(const std::string& path, MeshInitCallback callback = nullptr);

    // Number of mesh nodes created
    int meshCount() const { return (int)meshNodes.size(); }

    // Access individual mesh nodes (e.g. to addObject them into Scene)
    MeshNode* getMeshNode(int index) const;
    const std::vector<MeshNode*>& getMeshNodes() const { return meshNodes; }

    // Find a mesh by exact name (linear scan)
    MeshNode* findMeshByName(const std::string& name) const;

    // Release all MeshNodes (also frees GPU data of each node).
    // NOTE: if you have already transferred ownership to Scene via addObject,
    //       do NOT call this — Scene will handle deletion.
    void release();

    // Call this after you've added all MeshNodes to Scene via addObject().
    // Prevents this object's destructor from deleting the nodes.
    void releaseOwnership() { ownsNodes = false; }

    // -----------------------------------------------------------------------
    // Hierarchy (SceneNode) API
    //
    // After load(), ModelObject has built a SceneNode tree mirroring the
    // source aiNode hierarchy 1:1. Each MeshNode is attached to the
    // SceneNode it came from (so `findByName("exterior_wheel_tire_FL")`
    // returns a SceneNode whose pivot/transform matches the FBX wheel hub).
    //
    //   auto* root  = car->getRoot();
    //   auto* wheel = car->findByName("exterior_wheel_tire_FL");
    //   wheel->setRotationY(90);   // spins around its own hub
    //
    // For parts whose source FBX baked vertices in world space (no per-part
    // pivot), use createGroup() to add a virtual grouping SceneNode with
    // an explicit pivot — same as P1.
    //
    // The returned root must be registered with Scene via
    // Scene::addSceneNodeRoot(root) so updateWorld() runs each frame.
    // ModelObject owns the root by default; releaseHierarchy() to transfer
    // ownership to the caller.
    // -----------------------------------------------------------------------

    // Get the model's hierarchy root (built during load()).
    SceneNode* getOrCreateRoot(const std::string& name = "ModelRoot");
    SceneNode* getRoot() const { return root; }

    // Depth-first lookup by SceneNode name (== aiNode name == typically the
    // mesh name for leaf nodes). Returns nullptr if not found.
    SceneNode* findByName(const std::string& name) const;

    // Create a child group under `parent` and attach all meshes that match
    // `filter`. The group's pivot is set to (px, py, pz) in node-local space,
    // which is how you specify a hinge / rotation center.
    SceneNode* createGroup(SceneNode* parent,
                           const std::string& name,
                           const MeshFilter& filter,
                           float px = 0.0f, float py = 0.0f, float pz = 0.0f);

    // Convenience: create a group from an explicit list of mesh names.
    SceneNode* createGroupByNames(SceneNode* parent,
                                  const std::string& name,
                                  const std::vector<std::string>& meshNames,
                                  float px = 0.0f, float py = 0.0f, float pz = 0.0f);

    // Detach root ownership from this ModelObject (caller becomes the owner
    // and is responsible for `delete`). The pointer is still cached here so
    // getRoot() / findByName() keep working — useful right after release:
    //
    //     SceneNode* h = carLoader.releaseHierarchy();
    //     scene->addSceneNodeRoot(h);
    //     SceneNode* door = carLoader.getRoot()->findByName("exterior_doors_FL");
    //
    // Calling releaseHierarchy() twice on the same ModelObject returns the
    // same pointer but only the first caller "owns" it.
    SceneNode* releaseHierarchy() { ownsRoot = false; return root; }

private:
    std::vector<MeshNode*> meshNodes;
    SceneNode*             root      = nullptr;
    bool                   ownsNodes = true;
    bool                   ownsRoot  = true;
};
