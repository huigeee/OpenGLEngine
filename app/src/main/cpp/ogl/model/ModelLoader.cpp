#include "ModelLoader.h"
#include <android/log.h>
#include <cmath>
#include <cstring>

// assimp headers
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// stb_image for texture loading
#include <stb_image.h>
#include "../include/Common.h"

// -----------------------------------------------------------------------
// Mesh
// -----------------------------------------------------------------------

void Mesh::upload() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(Vertex),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    // layout 0: position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, position));
    // layout 1: normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, normal));
    // layout 2: texCoord
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, texCoord));
    // layout 3: tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, tangent));

    // Skinning: separate VBO at attribute locations 4 (ivec4 boneIds) & 5 (vec4 weights)
    if (!boneData.empty()) {
        glGenBuffers(1, &boneVBO);
        glBindBuffer(GL_ARRAY_BUFFER, boneVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     boneData.size() * sizeof(VertexBoneData),
                     boneData.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(4);
        glVertexAttribIPointer(4, 4, GL_INT, sizeof(VertexBoneData),
                               (void*)offsetof(VertexBoneData, boneIds));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData),
                              (void*)offsetof(VertexBoneData, weights));
    }

    glBindVertexArray(0);
}

void Mesh::draw(GLuint program) const {
    if (VAO == 0) return;

    // 绑定纹理
    unsigned int diffuseIdx  = 0;
    unsigned int specularIdx = 0;
    unsigned int normalIdx   = 0;

    for (unsigned int i = 0; i < textures.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        std::string uniformName;
        if (textures[i].type == "diffuse") {
            uniformName = "uTexDiffuse" + std::to_string(diffuseIdx++);
        } else if (textures[i].type == "specular") {
            uniformName = "uTexSpecular" + std::to_string(specularIdx++);
        } else if (textures[i].type == "normal") {
            uniformName = "uTexNormal" + std::to_string(normalIdx++);
        } else {
            uniformName = "uTex_" + textures[i].type;
        }
        GLint loc = glGetUniformLocation(program, uniformName.c_str());
        if (loc >= 0) glUniform1i(loc, (int)i);
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE0);
}

void Mesh::release() {
    if (VAO) { glDeleteVertexArrays(1, &VAO); VAO = 0; }
    if (VBO) { glDeleteBuffers(1, &VBO);      VBO = 0; }
    if (EBO) { glDeleteBuffers(1, &EBO);      EBO = 0; }
    if (boneVBO) { glDeleteBuffers(1, &boneVBO); boneVBO = 0; }
}

// -----------------------------------------------------------------------
// ModelLoader
// -----------------------------------------------------------------------

bool ModelLoader::load(const std::string& path) {
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs     |
        aiProcess_CalcTangentSpace |
        aiProcess_GenSmoothNormals);


    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        LOGE("Assimp error: %s", importer.GetErrorString());
        return false;
    }

    // 目录用于贴图查找
    size_t lastSlash = path.find_last_of("/\\");
    directory = (lastSlash != std::string::npos) ? path.substr(0, lastSlash) : ".";
    LOGE("Assimp directory: %s", directory.c_str());

    // 1) Convert all meshes verbatim (no transform baking).
    loadMeshes(scene);

    // 1b) Per-mesh skeleton extraction (registers bones, fills VertexBoneData).
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        extractBones(scene->mMeshes[i], meshes[i]);
    }

    // 1c) Animations + global inverse (root inverse needed by animator).
    loadAnimations(scene);
    {
        aiMatrix4x4 gi = scene->mRootNode->mTransformation;
        gi.Inverse();
        float* o = globalInverse;
        o[0]=gi.a1; o[1]=gi.b1; o[2]=gi.c1; o[3]=gi.d1;
        o[4]=gi.a2; o[5]=gi.b2; o[6]=gi.c2; o[7]=gi.d2;
        o[8]=gi.a3; o[9]=gi.b3; o[10]=gi.c3; o[11]=gi.d3;
        o[12]=gi.a4; o[13]=gi.b4; o[14]=gi.c4; o[15]=gi.d4;
    }

    // 2) Build NodeDesc tree mirroring aiNode hierarchy.
    if (rootNode) { delete rootNode; rootNode = nullptr; }
    rootNode = buildNodeDesc(scene->mRootNode);

    // 3) Upload to GPU.
    for (auto& mesh : meshes) {
        mesh.upload();
    }

    loaded = true;
    LOGI("Model loaded: %s  meshes=%d  rootNodeChildren=%d  bones=%d  animations=%d",
         path.c_str(), (int)meshes.size(),
         rootNode ? (int)rootNode->children.size() : 0,
         (int)bones.size(), (int)animations.size());
    return true;
}

// Convert each aiMesh into our Mesh struct, preserving local vertex coords.
void ModelLoader::loadMeshes(const aiScene* scene) {
    meshes.clear();
    meshes.reserve(scene->mNumMeshes);
    aiMatrix4x4 ident;   // identity = no baking
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        meshes.push_back(processMesh(scene->mMeshes[i], scene, ident));
    }
}

// Recursively copy aiNode tree into NodeDesc tree.
NodeDesc* ModelLoader::buildNodeDesc(const aiNode* node) {
    if (!node) return nullptr;
    NodeDesc* nd = new NodeDesc();
    nd->name = node->mName.C_Str();

    // assimp's aiMatrix4x4 is row-major (m.a1 is first row). Convert to
    // column-major for OpenGL/glm.
    const aiMatrix4x4& m = node->mTransformation;
    float* o = nd->localMatrix;
    o[0]=m.a1; o[1]=m.b1; o[2]=m.c1; o[3]=m.d1;
    o[4]=m.a2; o[5]=m.b2; o[6]=m.c2; o[7]=m.d2;
    o[8]=m.a3; o[9]=m.b3; o[10]=m.c3; o[11]=m.d3;
    o[12]=m.a4; o[13]=m.b4; o[14]=m.c4; o[15]=m.d4;

    nd->meshIndices.assign(node->mMeshes, node->mMeshes + node->mNumMeshes);
    nd->children.reserve(node->mNumChildren);
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        nd->children.push_back(buildNodeDesc(node->mChildren[i]));
    }
    return nd;
}

// Legacy entry point — kept for compatibility but no longer called by load().
void ModelLoader::processNode(const aiNode* node, const aiScene* scene,
                              const aiMatrix4x4& parentTransform) {
    aiMatrix4x4 globalTransform = parentTransform * node->mTransformation;
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene, globalTransform));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, globalTransform);
    }
}

Mesh ModelLoader::processMesh(const aiMesh* mesh, const aiScene* scene,
                              const aiMatrix4x4& transform) {
    Mesh result;
    result.name = mesh->mName.C_Str();

    // 法线变换矩阵 = transpose(inverse(transform))，只取左上 3×3
    aiMatrix3x3 normalMat = aiMatrix3x3(transform);
    normalMat.Transpose();
    // aiMatrix3x3 没有 Inverse()，手动用 3×3 行列式求逆
    // 但 assimp 提供了 aiMatrix4x4::Inverse()，借用它
    aiMatrix4x4 invT = transform;
    invT.Inverse();
    aiMatrix3x3 normalMatrix = aiMatrix3x3(invT);
    normalMatrix.Transpose();   // transpose(inverse(M))

    // 顶点
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex v{};

        // 位置：用完整 4×4 变换
        aiVector3D pos = transform * mesh->mVertices[i];
        v.position[0] = pos.x;
        v.position[1] = pos.y;
        v.position[2] = pos.z;

        if (mesh->HasNormals()) {
            aiVector3D n = normalMatrix * mesh->mNormals[i];
            // 不在这里 normalize，保留长度供 GPU normalize（一致性）
            v.normal[0] = n.x;
            v.normal[1] = n.y;
            v.normal[2] = n.z;
        }

        if (mesh->mTextureCoords[0]) {
            v.texCoord[0] = mesh->mTextureCoords[0][i].x;
            v.texCoord[1] = mesh->mTextureCoords[0][i].y;
        }

        if (mesh->HasTangentsAndBitangents()) {
            aiVector3D t = normalMatrix * mesh->mTangents[i];
            v.tangent[0] = t.x;
            v.tangent[1] = t.y;
            v.tangent[2] = t.z;
        }

        result.vertices.push_back(v);
    }

    // 索引
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            result.indices.push_back(face.mIndices[j]);
        }
    }

    // 材质贴图
//    if (mesh->mMaterialIndex >= 0) {
//        aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
//        auto diffuse  = loadMaterialTextures(mat, aiTextureType_DIFFUSE,   "diffuse");
//        auto specular = loadMaterialTextures(mat, aiTextureType_SPECULAR,  "specular");
//        auto normals  = loadMaterialTextures(mat, aiTextureType_NORMALS,   "normal");
//        result.textures.insert(result.textures.end(), diffuse.begin(),  diffuse.end());
//        result.textures.insert(result.textures.end(), specular.begin(), specular.end());
//        result.textures.insert(result.textures.end(), normals.begin(),  normals.end());
//    }

    return result;
}

std::vector<MeshTexture> ModelLoader::loadMaterialTextures(const aiMaterial* mat,
                                                            int type,
                                                            const std::string& typeName) {
    std::vector<MeshTexture> result;
    unsigned int count = mat->GetTextureCount((aiTextureType)type);
    for (unsigned int i = 0; i < count; i++) {
        aiString str;
        mat->GetTexture((aiTextureType)type, i, &str);
        std::string fullPath = directory + "/" + str.C_Str();

        // 检查缓存，避免重复上传
        bool found = false;
        for (auto& loaded : loadedTextures) {
            if (loaded.path == fullPath) {
                result.push_back(loaded);
                found = true;
                break;
            }
        }
        if (!found) {
            MeshTexture tex;
            tex.id   = loadTextureFromFile(fullPath);
            tex.type = typeName;
            tex.path = fullPath;
            result.push_back(tex);
            loadedTextures.push_back(tex);
        }
    }
    return result;
}

GLuint ModelLoader::loadTextureFromFile(const std::string& path) {
    GLuint texID = 0;
    glGenTextures(1, &texID);

    int w, h, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 0);
    if (!data) {
        LOGE("Failed to load texture: %s", path.c_str());
        return texID;
    }

    GLenum fmt = GL_RGB;
    if (channels == 1)      fmt = GL_RED;
    else if (channels == 3) fmt = GL_RGB;
    else if (channels == 4) fmt = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    LOGI("Texture loaded: %s (%dx%d ch=%d)", path.c_str(), w, h, channels);
    return texID;
}

void ModelLoader::draw(GLuint program) const {
    for (const auto& mesh : meshes) {
        mesh.draw(program);
    }
}

void ModelLoader::release() {
    for (auto& mesh : meshes) {
        mesh.release();
    }
    meshes.clear();
    // 释放缓存纹理
    for (auto& tex : loadedTextures) {
        if (tex.id) glDeleteTextures(1, &tex.id);
    }
    loadedTextures.clear();
    bones.clear();
    boneIndex.clear();
    animations.clear();
    loaded = false;
}

// ---------------------------------------------------------------------------
// dumpAssimpHierarchy — diagnostic only. Re-parses the file (no caching),
// walks aiNode tree depth-first and logs each node.
//
// For each aiNode:
//   [depth] name  meshes=N  children=M  identity=Y/N
//     [mesh i] aiMeshIdx=K  name=...  verts=V
//
// Goal: confirm whether the source FBX carries usable hierarchy + per-node
// transforms (= worth a P3 rewrite that preserves the tree), or if every
// mesh sits under root with identity transform (= P3 won't help, model
// itself must be re-exported by the artist).
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Skeleton extraction
//
// For each aiBone in this mesh:
//   - register it in the global `bones` palette (assign next free index if new)
//   - record its offset matrix (= inverse bind pose)
//   - for every weight (vertexId, weight), append into out.boneData[vertexId]
//     until 4 slots filled. Excess influences are silently dropped.
// ---------------------------------------------------------------------------
static void aiMat4ToColMajor(const aiMatrix4x4& m, float* o) {
    o[0]=m.a1; o[1]=m.b1; o[2]=m.c1; o[3]=m.d1;
    o[4]=m.a2; o[5]=m.b2; o[6]=m.c2; o[7]=m.d2;
    o[8]=m.a3; o[9]=m.b3; o[10]=m.c3; o[11]=m.d3;
    o[12]=m.a4; o[13]=m.b4; o[14]=m.c4; o[15]=m.d4;
}

void ModelLoader::extractBones(const aiMesh* aim, Mesh& out) {
    if (aim->mNumBones == 0) return;
    out.boneData.assign(out.vertices.size(), VertexBoneData{});

    for (unsigned int b = 0; b < aim->mNumBones; ++b) {
        const aiBone* aiB = aim->mBones[b];
        std::string boneName = aiB->mName.C_Str();

        int boneId = -1;
        auto it = boneIndex.find(boneName);
        if (it == boneIndex.end()) {
            boneId = (int)bones.size();
            BoneInfo bi;
            bi.name = boneName;
            aiMat4ToColMajor(aiB->mOffsetMatrix, bi.offset);
            bones.push_back(bi);
            boneIndex[boneName] = boneId;
        } else {
            boneId = it->second;
        }
        if (boneId >= MAX_BONES) {
            LOGE("ModelLoader: bone count exceeds MAX_BONES=%d", MAX_BONES);
            continue;
        }

        for (unsigned int w = 0; w < aiB->mNumWeights; ++w) {
            unsigned int vid = aiB->mWeights[w].mVertexId;
            float       wt  = aiB->mWeights[w].mWeight;
            if (vid >= out.boneData.size()) continue;
            VertexBoneData& vbd = out.boneData[vid];
            for (int s = 0; s < MAX_BONES_PER_VERTEX; ++s) {
                if (vbd.weights[s] == 0.0f) {
                    vbd.boneIds[s] = boneId;
                    vbd.weights[s] = wt;
                    break;
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Animation loading: convert each aiAnimation into AnimClip with POD keys.
// ---------------------------------------------------------------------------
void ModelLoader::loadAnimations(const aiScene* scene) {
    animations.clear();
    if (!scene->HasAnimations()) return;
    animations.reserve(scene->mNumAnimations);
    for (unsigned int a = 0; a < scene->mNumAnimations; ++a) {
        const aiAnimation* anim = scene->mAnimations[a];
        AnimClip clip;
        clip.name = anim->mName.C_Str();
        clip.duration = anim->mDuration;
        clip.ticksPerSecond = (anim->mTicksPerSecond > 1e-6) ? anim->mTicksPerSecond : 25.0;
        clip.channels.reserve(anim->mNumChannels);
        for (unsigned int c = 0; c < anim->mNumChannels; ++c) {
            const aiNodeAnim* na = anim->mChannels[c];
            AnimChannel ch;
            ch.nodeName = na->mNodeName.C_Str();
            ch.positions.reserve(na->mNumPositionKeys);
            ch.rotations.reserve(na->mNumRotationKeys);
            ch.scales.reserve(na->mNumScalingKeys);
            for (unsigned int i = 0; i < na->mNumPositionKeys; ++i) {
                const aiVectorKey& k = na->mPositionKeys[i];
                VecKey vk{ k.mTime, { k.mValue.x, k.mValue.y, k.mValue.z } };
                ch.positions.push_back(vk);
            }
            for (unsigned int i = 0; i < na->mNumRotationKeys; ++i) {
                const aiQuatKey& k = na->mRotationKeys[i];
                QuatKey qk{ k.mTime, { k.mValue.x, k.mValue.y, k.mValue.z, k.mValue.w } };
                ch.rotations.push_back(qk);
            }
            for (unsigned int i = 0; i < na->mNumScalingKeys; ++i) {
                const aiVectorKey& k = na->mScalingKeys[i];
                VecKey vk{ k.mTime, { k.mValue.x, k.mValue.y, k.mValue.z } };
                ch.scales.push_back(vk);
            }
            clip.channels.push_back(std::move(ch));
        }
        LOGI("AnimClip[%u] '%s' duration=%.3f tps=%.3f channels=%zu",
             a, clip.name.c_str(), clip.duration, clip.ticksPerSecond, clip.channels.size());
        animations.push_back(std::move(clip));
    }
}

// ---------------------------------------------------------------------------
// dumpAssimpHierarchy
// ---------------------------------------------------------------------------
static bool isIdentity(const aiMatrix4x4& m) {
    const float eps = 1e-5f;
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            float expected = (r == c) ? 1.0f : 0.0f;
            if (std::abs(m[r][c] - expected) > eps) return false;
        }
    }
    return true;
}

static void dumpNodeRecursive(const aiNode* node, const aiScene* scene, int depth,
                              int& nodeCount, int& nonIdentityCount,
                              int& meshRefCount, int maxDepth, int& observedMaxDepth) {
    ++nodeCount;
    if (depth > observedMaxDepth) observedMaxDepth = depth;
    bool ident = isIdentity(node->mTransformation);
    if (!ident) ++nonIdentityCount;
    meshRefCount += node->mNumMeshes;

    // Skip noisy `$AssimpFbx$_RotationPivot/...` nodes in the dump, but still
    // count + recurse into them. Only log nodes that are "interesting":
    //   - have meshes attached, OR
    //   - have a "clean" name (no $AssimpFbx$ marker), OR
    //   - are the root
    const char* nm = node->mName.C_Str();
    bool isFbxHelper = (std::strstr(nm, "$AssimpFbx$") != nullptr);
    bool shouldLog = (depth == 0) || (node->mNumMeshes > 0) || !isFbxHelper;

    if (shouldLog) {
        char indent[128] = {0};
        int n = depth * 2; if (n > 120) n = 120;
        for (int i = 0; i < n; ++i) indent[i] = ' ';

        LOGI("%s[d=%d] '%s'  meshes=%u  children=%u  identityT=%s",
             indent, depth, nm,
             node->mNumMeshes, node->mNumChildren,
             ident ? "Y" : "N");

        for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
            unsigned int mi = node->mMeshes[i];
            const aiMesh* mesh = scene->mMeshes[mi];
            LOGI("%s    mesh[%u] aiIdx=%u name='%s' verts=%u",
                 indent, i, mi, mesh->mName.C_Str(), mesh->mNumVertices);
        }
    }

    if (depth < maxDepth) {
        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            dumpNodeRecursive(node->mChildren[i], scene, depth + 1,
                              nodeCount, nonIdentityCount, meshRefCount,
                              maxDepth, observedMaxDepth);
        }
    }
}

// Walk RAW tree, but only print nodes that own a mesh — and for each such
// node also log the accumulated parent-chain translation (the "pivot").
// This tells us whether each part has its own pivot point baked into the
// $AssimpFbx$_*Pivot chain (= per-door hinge available).
static void dumpRawNodeRecursive(const aiNode* node, const aiScene* scene, int depth,
                                 const aiMatrix4x4& accumulated,
                                 int& nodeCount, int& meshOwnerCount,
                                 int& observedMaxDepth) {
    ++nodeCount;
    if (depth > observedMaxDepth) observedMaxDepth = depth;
    aiMatrix4x4 worldXform = accumulated * node->mTransformation;

    if (node->mNumMeshes > 0) {
        ++meshOwnerCount;
        // decompose translation only
        aiVector3D pos(worldXform.a4, worldXform.b4, worldXform.c4);
        LOGI("[raw d=%d] mesh-owner '%s'  meshes=%u  worldPivot=(%.2f, %.2f, %.2f)",
             depth, node->mName.C_Str(), node->mNumMeshes, pos.x, pos.y, pos.z);
        for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
            unsigned int mi = node->mMeshes[i];
            const aiMesh* mesh = scene->mMeshes[mi];
            LOGI("           mesh '%s' (aiIdx=%u verts=%u)",
                 mesh->mName.C_Str(), mi, mesh->mNumVertices);
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        dumpRawNodeRecursive(node->mChildren[i], scene, depth + 1, worldXform,
                             nodeCount, meshOwnerCount, observedMaxDepth);
    }
}

void ModelLoader::dumpAssimpHierarchy(const std::string& path) {
    LOGI("=== ModelLoader::dumpAssimpHierarchy '%s' ===", path.c_str());

    // ---- Pass 1: OptimizeGraph (clean view) ----
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate | aiProcess_GenSmoothNormals |
            aiProcess_OptimizeGraph);
        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
            LOGE("dumpAssimpHierarchy(optimized): load failed: %s", importer.GetErrorString());
        } else {
            LOGI("--- PASS 1: with aiProcess_OptimizeGraph (clean tree) ---");
            LOGI("scene: meshes=%u  materials=%u  textures=%u  animations=%u",
                 scene->mNumMeshes, scene->mNumMaterials,
                 scene->mNumTextures, scene->mNumAnimations);
            int nodeCount = 0, nonIdentityCount = 0, meshRefCount = 0, observedMaxDepth = 0;
            dumpNodeRecursive(scene->mRootNode, scene, 0,
                              nodeCount, nonIdentityCount, meshRefCount,
                              32, observedMaxDepth);
            LOGI("PASS1 summary: nodes=%d  nonIdentityXform=%d  meshRefs=%d  maxDepth=%d",
                 nodeCount, nonIdentityCount, meshRefCount, observedMaxDepth);
        }
    }

    // ---- Pass 2: RAW (incl. $AssimpFbx$ pivot chain) - print mesh-owners + their world pivot ----
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate | aiProcess_GenSmoothNormals);
        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
            LOGE("dumpAssimpHierarchy(raw): load failed: %s", importer.GetErrorString());
            return;
        }
        LOGI("--- PASS 2: RAW (pivots intact) - listing per-mesh world pivots ---");
        int nodeCount = 0, meshOwnerCount = 0, observedMaxDepth = 0;
        dumpRawNodeRecursive(scene->mRootNode, scene, 0, aiMatrix4x4(),
                             nodeCount, meshOwnerCount, observedMaxDepth);
        LOGI("PASS2 summary: rawNodes=%d  meshOwners=%d  maxDepth=%d",
             nodeCount, meshOwnerCount, observedMaxDepth);
        LOGI("=== verdict: %s ===",
             (meshOwnerCount > 5)
                ? "each mesh owns its own pivot subtree (USABLE for auto-pivot)"
                : "meshes share owners (need name-based grouping)");
    }
}
