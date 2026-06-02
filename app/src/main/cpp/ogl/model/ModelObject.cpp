#include "ModelObject.h"
#include <android/log.h>
#include "../include/Common.h"

ModelObject::ModelObject() = default;

ModelObject::~ModelObject() {
    release();
    // Only delete root if we still own it; releaseHierarchy() transfers
    // ownership to the caller while leaving `root` cached for getRoot()/
    // findByName() to keep functioning.
    if (root && ownsRoot) { delete root; }
    root = nullptr;
}

// ---------------------------------------------------------------------------
// Recursively map NodeDesc -> SceneNode, creating MeshNodes as we go.
// `flatMeshes` is consumed (moved-from) so each mesh ends up exactly once
// in the resulting MeshNode list.
// ---------------------------------------------------------------------------
static SceneNode* buildSceneTree(const NodeDesc* nd,
                                 std::vector<Mesh>& flatMeshes,
                                 std::vector<MeshNode*>& outMeshNodes,
                                 const MeshInitCallback& cb,
                                 int& meshCounter) {
    if (!nd) return nullptr;

    SceneNode* sn = new SceneNode(nd->name);
    sn->setBindMatrix(nd->localMatrix);

    // Attach meshes belonging to this node.
    for (unsigned int mi : nd->meshIndices) {
        if (mi >= flatMeshes.size()) continue;
        // Mesh may have been moved already in pathological cases; guard.
        if (flatMeshes[mi].vertices.empty() && flatMeshes[mi].indices.empty()
            && flatMeshes[mi].VAO == 0) {
            // already taken — skip
            continue;
        }
        std::string meshName = flatMeshes[mi].name;
        // Prefer the mesh's own name for the MeshNode (callers identify by it).
        // If the aiNode has a sane name and the mesh name is empty, fall back.
        if (meshName.empty()) meshName = nd->name;

        MeshNode* m = new MeshNode(std::move(flatMeshes[mi]), meshName);
        if (cb) cb(m, meshCounter);
        ++meshCounter;
        outMeshNodes.push_back(m);
        sn->attachMesh(m);
    }

    for (NodeDesc* child : nd->children) {
        SceneNode* cs = buildSceneTree(child, flatMeshes, outMeshNodes, cb, meshCounter);
        if (cs) sn->addChild(cs);
    }
    return sn;
}

bool ModelObject::load(const std::string& path, MeshInitCallback callback) {
    ModelLoader loader;
    if (!loader.load(path)) {
        LOGE("ModelObject: failed to load %s", path.c_str());
        return false;
    }

    std::vector<Mesh> meshes = loader.takeMeshes();
    NodeDesc*         tree   = loader.takeRootNode();
    meshNodes.reserve(meshes.size());

    if (tree) {
        int counter = 0;
        root = buildSceneTree(tree, meshes, meshNodes, callback, counter);
        delete tree;   // descriptors copied into SceneNode tree
    } else {
        // Fallback: flat list (shouldn't happen with the new loader).
        for (int i = 0; i < (int)meshes.size(); ++i) {
            std::string meshName = meshes[i].name;
            MeshNode* node = new MeshNode(std::move(meshes[i]), meshName);
            if (callback) callback(node, i);
            meshNodes.push_back(node);
        }
    }

    LOGI("ModelObject: loaded %s  meshes=%d  hierarchyRoot=%s",
         path.c_str(), (int)meshNodes.size(),
         root ? root->getName().c_str() : "(null)");
    return true;
}

MeshNode* ModelObject::getMeshNode(int index) const {
    if (index < 0 || index >= (int)meshNodes.size()) return nullptr;
    return meshNodes[index];
}

MeshNode* ModelObject::findMeshByName(const std::string& name) const {
    for (MeshNode* m : meshNodes) {
        if (m && m->getName() == name) return m;
    }
    return nullptr;
}

void ModelObject::release() {
    if (ownsNodes) {
        for (MeshNode* node : meshNodes) {
            if (node) { node->release(); delete node; }
        }
    }
    meshNodes.clear();
}

// ---------------------------------------------------------------------------
// Hierarchy API
// ---------------------------------------------------------------------------

SceneNode* ModelObject::getOrCreateRoot(const std::string& name) {
    if (!root) root = new SceneNode(name);
    return root;
}

SceneNode* ModelObject::findByName(const std::string& name) const {
    if (!root) return nullptr;
    return root->findByName(name);
}

// ---------------------------------------------------------------------------
// createGroup — for parts that don't have their own pivot in the FBX.
//
// Walks all SceneNodes under `parent`, finds those whose attached MeshNodes
// match `filter`, and reparents those MeshNodes onto a new group SceneNode
// with explicit pivot. The original SceneNodes are left in place but their
// meshes are detached.
//
// NOTE: this is intentionally simple — meshes can only belong to one
// SceneNode at a time (so the SceneNode that previously owned them must
// stop pushing externalMatrix). We achieve that by detaching from the old
// owner in-place.
// ---------------------------------------------------------------------------
static void detachMeshFromTree(SceneNode* node, MeshNode* m) {
    if (!node) return;
    if (node->detachMesh(m)) return;
    for (SceneNode* c : node->getChildren()) detachMeshFromTree(c, m);
}

SceneNode* ModelObject::createGroup(SceneNode* parent,
                                    const std::string& name,
                                    const MeshFilter& filter,
                                    float px, float py, float pz) {
    if (!parent) parent = getOrCreateRoot();
    SceneNode* group = new SceneNode(name);
    group->setPivot(px, py, pz);
    parent->addChild(group);

    int matched = 0;
    for (MeshNode* m : meshNodes) {
        if (m && filter && filter(m)) {
            // Detach from whichever SceneNode currently owns this mesh.
            if (root) detachMeshFromTree(root, m);
            // Reparent under the new group.
            group->attachMesh(m);
            ++matched;
        }
    }
    LOGI("ModelObject::createGroup '%s' matched %d meshes", name.c_str(), matched);
    return group;
}

SceneNode* ModelObject::createGroupByNames(SceneNode* parent,
                                           const std::string& name,
                                           const std::vector<std::string>& meshNames,
                                           float px, float py, float pz) {
    return createGroup(parent, name,
        [&meshNames](MeshNode* m) -> bool {
            const std::string& n = m->getName();
            for (const auto& s : meshNames) if (s == n) return true;
            return false;
        },
        px, py, pz);
}
