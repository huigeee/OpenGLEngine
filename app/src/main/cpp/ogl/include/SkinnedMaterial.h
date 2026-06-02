#pragma once

#include "Material.h"
#include <string>

// ---------------------------------------------------------------------------
// SkinnedMaterial — minimal diffuse-mapped material with 4-bone vertex
// skinning. Designed for COLLADA cowboy-style models (one diffuse map, no
// PBR). Pairs with Animator: call setAnimator(animator) and the material
// uploads the bone palette during use().
//
//   • Vertex layout = standard Vertex (location 0..3) + bone VBO
//     (location 4 ivec4 boneIds, location 5 vec4 weights).
//   • Uniform `uBones[MAX_BONES]` — column-major mat4 array.
//   • Uniform `uHasSkeleton` — when 0, the shader skips skinning (lets a
//     skinned mesh render in bind pose if no animator is wired).
// ---------------------------------------------------------------------------

class Shader;
class Animator;

class SkinnedMaterial : public Material {
public:
    SkinnedMaterial();
    ~SkinnedMaterial() override;

    void init() override;
    void release() override;
    void setTransform(const float* model, const float* view, const float* projection) override;
    void use() override;

    void setAnimator(Animator* a) { animator = a; }
    Animator* getAnimator() const { return animator; }

    void setBaseColorMap(GLuint tex) { baseColorTex = tex; }
    GLuint getBaseColorMap() const   { return baseColorTex; }

    void setColor(float r, float g, float b, float a) {
        color[0]=r; color[1]=g; color[2]=b; color[3]=a;
    }

    const VertexAttrib* getVertexAttribs(int& count) override;

private:
    Shader* shaderProg = nullptr;
    bool    linked     = false;

    GLint locModel = -1, locView = -1, locProj = -1;
    GLint locPrevModel = -1, locPrevView = -1, locPrevProj = -1;
    GLint locJitter = -1;
    GLint locColor = -1, locBaseColor = -1;
    GLint locHasSkeleton = -1;
    GLint locBones = -1;

    Animator* animator = nullptr;
    GLuint    baseColorTex = 0;
    float     color[4] = {1,1,1,1};

    static std::string buildVS();
    static std::string buildFS();
};
