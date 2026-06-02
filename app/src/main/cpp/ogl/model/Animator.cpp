#include "Animator.h"
#include "../include/Common.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <cstring>

void Animator::setModel(const ModelLoader* l) {
    loader = l;
    finalBoneMatrices.clear();
    if (!loader) return;
    finalBoneMatrices.assign(loader->getBones().size(), glm::mat4(1.0f));
    globalInverse = glm::make_mat4(loader->getGlobalInverse());
    if (!loader->getAnimations().empty()) {
        clipIndex = 0;
    }
    currentTimeSec = 0.0;
    LOGI("Animator::setModel  bones=%d  clips=%d  initialClip=%d",
         (int)loader->getBones().size(), (int)loader->getAnimations().size(), clipIndex);
}

void Animator::setClip(int idx) {
    if (!loader) { clipIndex = -1; return; }
    if (idx < 0 || idx >= (int)loader->getAnimations().size()) {
        clipIndex = -1;
    } else {
        clipIndex = idx;
    }
    currentTimeSec = 0.0;
}

void Animator::update(float dt) {
    if (!playing) return;
    currentTimeSec += (double)dt * (double)timeScale;
}

const AnimChannel* Animator::findChannel(const std::string& nodeName) const {
    if (!loader || clipIndex < 0) return nullptr;
    const AnimClip& clip = loader->getAnimations()[clipIndex];
    for (const auto& ch : clip.channels) {
        if (ch.nodeName == nodeName) return &ch;
    }
    return nullptr;
}

// Helpers: linear search for keyframe interval, returns index i such that
// keys[i].t <= t < keys[i+1].t (clamped at ends).
template <typename Key>
static int findKeyIndex(const std::vector<Key>& keys, double t) {
    if (keys.size() <= 1) return 0;
    for (size_t i = 0; i + 1 < keys.size(); ++i) {
        if (t < keys[i + 1].t) return (int)i;
    }
    return (int)keys.size() - 2;
}

static glm::vec3 sampleVec(const std::vector<VecKey>& keys, double t) {
    if (keys.empty()) return glm::vec3(0.0f);
    if (keys.size() == 1) return glm::vec3(keys[0].v[0], keys[0].v[1], keys[0].v[2]);
    int i = findKeyIndex(keys, t);
    const VecKey& a = keys[i];
    const VecKey& b = keys[i + 1];
    double dt = b.t - a.t;
    float f = (dt > 1e-9) ? (float)((t - a.t) / dt) : 0.0f;
    if (f < 0.f) f = 0.f; if (f > 1.f) f = 1.f;
    glm::vec3 av(a.v[0], a.v[1], a.v[2]);
    glm::vec3 bv(b.v[0], b.v[1], b.v[2]);
    return glm::mix(av, bv, f);
}

static glm::quat sampleQuat(const std::vector<QuatKey>& keys, double t) {
    if (keys.empty()) return glm::quat(1.f, 0.f, 0.f, 0.f);
    if (keys.size() == 1) return glm::quat(keys[0].v[3], keys[0].v[0], keys[0].v[1], keys[0].v[2]);
    int i = findKeyIndex(keys, t);
    const QuatKey& a = keys[i];
    const QuatKey& b = keys[i + 1];
    double dt = b.t - a.t;
    float f = (dt > 1e-9) ? (float)((t - a.t) / dt) : 0.0f;
    if (f < 0.f) f = 0.f; if (f > 1.f) f = 1.f;
    // glm::quat ctor is (w, x, y, z); our key stores xyzw.
    glm::quat qa(a.v[3], a.v[0], a.v[1], a.v[2]);
    glm::quat qb(b.v[3], b.v[0], b.v[1], b.v[2]);
    return glm::normalize(glm::slerp(qa, qb, f));
}

void Animator::readChannelTransform(const AnimChannel& ch, double tickTime,
                                    glm::mat4& outLocal) const {
    glm::vec3 t = sampleVec(ch.positions, tickTime);
    glm::quat r = sampleQuat(ch.rotations, tickTime);
    glm::vec3 s = ch.scales.empty() ? glm::vec3(1.0f) : sampleVec(ch.scales, tickTime);
    glm::mat4 T = glm::translate(glm::mat4(1.0f), t);
    glm::mat4 R = glm::toMat4(r);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), s);
    outLocal = T * R * S;
}

void Animator::recurse(const NodeDesc* node, const glm::mat4& parentGlobal,
                       double tickTime) {
    if (!node) return;

    // Local transform: animated channel if present, else bind pose from NodeDesc.
    glm::mat4 local = glm::make_mat4(node->localMatrix);
    const AnimChannel* ch = findChannel(node->name);
    if (ch) readChannelTransform(*ch, tickTime, local);

    glm::mat4 global = parentGlobal * local;

    // If this node corresponds to a bone, write its final matrix.
    const auto& idx = loader->getBoneIndex();
    auto it = idx.find(node->name);
    if (it != idx.end()) {
        int boneId = it->second;
        if (boneId < (int)finalBoneMatrices.size()) {
            glm::mat4 offset = glm::make_mat4(loader->getBones()[boneId].offset);
            finalBoneMatrices[boneId] = globalInverse * global * offset;
        }
    }

    for (const NodeDesc* c : node->children) {
        recurse(c, global, tickTime);
    }
}

void Animator::computeBoneMatrices() {
    if (!loader) return;
    if (finalBoneMatrices.size() != loader->getBones().size()) {
        finalBoneMatrices.assign(loader->getBones().size(), glm::mat4(1.0f));
    }
    if (clipIndex < 0 || clipIndex >= (int)loader->getAnimations().size()) {
        // No clip: identity (bind pose via offset cancels).
        for (auto& m : finalBoneMatrices) m = glm::mat4(1.0f);
        return;
    }
    const AnimClip& clip = loader->getAnimations()[clipIndex];
    double tps = clip.ticksPerSecond > 1e-6 ? clip.ticksPerSecond : 25.0;
    double duration = clip.duration > 1e-6 ? clip.duration : 1.0;
    double tickTime = currentTimeSec * tps;
    if (duration > 0.0) tickTime = std::fmod(tickTime, duration);
    if (tickTime < 0.0) tickTime += duration;

    recurse(loader->getRootNode(), glm::mat4(1.0f), tickTime);
}
