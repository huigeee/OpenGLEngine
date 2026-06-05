#pragma once

// ---------------------------------------------------------------------------
// Animator — samples a ModelLoader's AnimClip + bone palette, producing a
// per-frame array of MAX_BONES mat4 values that SkinnedMaterial uploads to
// uBones[]. The animator owns its time cursor and play/pause state.
//
//   • The SceneNode tree (P3/P4) is NOT used here. Skeletal hierarchy lives
//     in the original aiNode tree (ModelLoader::getRootNode()) and channels
//     are keyed by node name. We walk the NodeDesc tree directly.
//
//   • Bones not animated by any channel use their bind-pose local matrix
//     (NodeDesc::localMatrix), which is what assimp would have given us
//     naturally if the channel were missing.
//
//   • finalBoneMatrices[boneId] = globalInverse * globalNodeTransform(bone)
//                                  * boneOffset(boneId)
//
//   • Non-bone nodes (skeleton joints with no associated weights) are still
//     traversed because they parent bones.
// ---------------------------------------------------------------------------

#include "ModelLoader.h"
#include <glm/glm.hpp>
#include <vector>

class Animator {
public:
    Animator() = default;

    // Bind to a loaded model. Caller retains ownership of `loader`.
    // Defaults to clip 0 if any animations exist.
    void setModel(const ModelLoader* loader);

    // Pick which AnimClip to play (index into ModelLoader::getAnimations()).
    void setClip(int clipIndex);
    int  getClip() const { return clipIndex; }

    // Time advance (seconds).
    void update(float dt);

    // Play / pause / scrub
    void play()                 { playing = true; }
    void pause()                { playing = false; }
    void toggle()               { playing = !playing; }
    bool isPlaying() const      { return playing; }
    void reset()                { currentTimeSec = 0.0; }
    void setTimeScale(float s)  { timeScale = s; }
    float getTimeScale() const  { return timeScale; }

    // Compute and cache final bone matrices for the current time.
    // Call once per frame before SkinnedMaterial::use().
    void computeBoneMatrices();

    // mat4 column-major; size == ModelLoader::getBones().size() (≤ MAX_BONES)
    const std::vector<glm::mat4>& getBoneMatrices() const { return finalBoneMatrices; }

    bool hasSkeleton() const { return loader && loader->hasSkeleton(); }
    int  boneCount()   const { return loader ? (int)loader->getBones().size() : 0; }

private:
    void readChannelTransform(const AnimChannel& ch, double tickTime,
                              glm::mat4& outLocal) const;
    const AnimChannel* findChannel(const std::string& nodeName) const;
    void recurse(const NodeDesc* node, const glm::mat4& parentGlobal,
                 double tickTime);

private:
    const ModelLoader* loader = nullptr;
    int                clipIndex = -1;
    double             currentTimeSec = 0.0;
    bool               playing = true;
    float              timeScale = 1.0f;
    std::vector<glm::mat4> finalBoneMatrices;
    glm::mat4              globalInverse{1.0f};
};
