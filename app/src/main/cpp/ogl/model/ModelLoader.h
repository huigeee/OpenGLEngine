#pragma once

#include <GLES3/gl3.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <assimp/matrix4x4.h>
#include <assimp/matrix3x3.h>
#include <assimp/scene.h>

// 单个顶点
struct Vertex {
    float position[3];
    float normal[3];
    float texCoord[2];
    float tangent[3];
};

// ---------------------------------------------------------------------------
// Skinning support
//
//   • MAX_BONES_PER_VERTEX is the number of bone influences blended per vertex
//     in the skinning shader. 4 is the universal practical cap.
//   • VertexBoneData lives in a *separate* VBO (attribute locations 4 & 5),
//     so non-skinned meshes / non-skinned materials are unaffected.
//   • BoneInfo per-bone offset matrix (= inverse bind-pose) lives on the
//     ModelLoader, keyed by bone index assigned during load().
// ---------------------------------------------------------------------------
static constexpr int MAX_BONES_PER_VERTEX = 4;
static constexpr int MAX_BONES = 100;   // shader uniform array size

struct VertexBoneData {
    int   boneIds[MAX_BONES_PER_VERTEX] = {0,0,0,0};
    float weights[MAX_BONES_PER_VERTEX] = {0,0,0,0};
};

struct BoneInfo {
    std::string name;
    float       offset[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};  // col-major
};

// 单个纹理（已上传到 GPU）
struct MeshTexture {
    GLuint id   = 0;
    std::string type;   // "diffuse" / "specular" / "normal" / "roughness" / "ao"
    std::string path;   // 原始文件路径（用于去重）
};

// 一个 Mesh（对应 assimp 的 aiMesh）
struct Mesh {
    std::string              name;      // aiMesh::mName
    std::vector<Vertex>      vertices;
    std::vector<unsigned int> indices;
    std::vector<MeshTexture> textures;

    // Skinning (empty when mesh has no bones)
    std::vector<VertexBoneData> boneData;   // size == vertices.size() if skinned

    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLuint boneVBO = 0;   // per-vertex bone IDs + weights (only if skinned)

    bool hasBones() const { return !boneData.empty(); }

    // 上传到 GPU
    void upload();
    // 绘制
    void draw(GLuint program) const;
    // 释放 GPU 资源
    void release();
};

// ----------------------------------------------------------------------
// NodeDesc — a lightweight description of one aiNode in the imported tree.
// ----------------------------------------------------------------------
struct NodeDesc {
    std::string                  name;
    float                        localMatrix[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    std::vector<unsigned int>    meshIndices;
    std::vector<NodeDesc*>       children;
    ~NodeDesc() { for (auto* c : children) delete c; }
};

// ---------------------------------------------------------------------------
// AnimChannel / AnimClip — assimp's aiAnimation transcribed into POD that
// the runtime Animator can sample without re-touching assimp.
// ---------------------------------------------------------------------------
struct VecKey   { double t; float v[3]; };
struct QuatKey  { double t; float v[4]; }; // x,y,z,w

struct AnimChannel {
    std::string nodeName;
    std::vector<VecKey>  positions;
    std::vector<QuatKey> rotations;
    std::vector<VecKey>  scales;
};

struct AnimClip {
    std::string              name;
    double                   duration       = 0.0;   // in ticks
    double                   ticksPerSecond = 25.0;
    std::vector<AnimChannel> channels;
};

// 模型加载器
class ModelLoader {
public:
    ModelLoader() = default;
    ~ModelLoader() { release(); if (rootNode) { delete rootNode; rootNode = nullptr; } }

    bool load(const std::string& path);

    const NodeDesc* getRootNode() const { return rootNode; }
    NodeDesc*       takeRootNode() { NodeDesc* r = rootNode; rootNode = nullptr; return r; }

    void draw(GLuint program) const;
    void release();

    const std::vector<Mesh>& getMeshes() const { return meshes; }
    std::vector<Mesh> takeMeshes() { return std::move(meshes); }
    bool isLoaded() const { return loaded; }

    // ----- Skeleton & animation accessors (valid after load()) -----
    const std::vector<BoneInfo>&                 getBones()      const { return bones; }
    const std::unordered_map<std::string, int>&  getBoneIndex()  const { return boneIndex; }
    const std::vector<AnimClip>&                 getAnimations() const { return animations; }
    bool                                         hasSkeleton()   const { return !bones.empty(); }
    const float*                                 getGlobalInverse() const { return globalInverse; }

    static void dumpAssimpHierarchy(const std::string& path);

private:
    std::vector<Mesh>        meshes;
    std::vector<MeshTexture> loadedTextures;
    std::string              directory;
    bool                     loaded = false;
    NodeDesc*                rootNode = nullptr;

    // Skeleton: a flat bone palette built across all meshes. Bone IDs in
    // VertexBoneData are indices into `bones`. boneIndex maps bone name →
    // index for fast lookup during animation sampling.
    std::vector<BoneInfo>                bones;
    std::unordered_map<std::string, int> boneIndex;
    std::vector<AnimClip>                animations;
    float                                globalInverse[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};

    NodeDesc* buildNodeDesc(const aiNode* node);
    void loadMeshes(const aiScene* scene);
    void extractBones(const aiMesh* aim, Mesh& out);
    void loadAnimations(const aiScene* scene);

    void processNode(const aiNode* node, const aiScene* scene,
                     const aiMatrix4x4& parentTransform);
    Mesh processMesh(const aiMesh* mesh, const aiScene* scene,
                     const aiMatrix4x4& transform);
    std::vector<MeshTexture> loadMaterialTextures(const aiMaterial* mat,
                                                   int type,
                                                   const std::string& typeName);
    GLuint loadTextureFromFile(const std::string& path);
};
