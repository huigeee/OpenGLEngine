#pragma once

#include <GLES3/gl3.h>
#include <android/asset_manager.h>

class Shader;
class Camera;
class Object3D;
class Animator;

class Material {
public:
    Material();
    virtual ~Material();

    virtual void init() = 0;
    virtual void onPreRender() {}
    virtual void setTransform(const float* model, const float* view, const float* projection) = 0;
    virtual void use() {}
    virtual void beforeRenderVirtual() {}
    virtual bool isIBL() const { return false; }
    virtual void onPostRender() {}
    virtual void release() = 0;

    // Transparency support
    virtual float getOpacity() const { return 1.0f; }
    virtual bool isTransparent() const { return getOpacity() < 1.0f; }

    struct VertexAttrib {
        const char* name;
        GLint size;
        GLint location;
        int offset;
    };
    virtual const VertexAttrib* getVertexAttribs(int& count) = 0;

    Shader* getShader() { return shader; }
    void setCamera(Camera* cam) { camera = cam; }
    Camera* getCamera() { return camera; }

    void setPrevViewMatrix(const float* v);
    void setPrevProjectionMatrix(const float* p);
    void setPrevModelMatrix(const float* m);
    void setJitterOffset(const float* offset);
    void setScreenSize(int w, int h);
    void setFrameCount(int count);

    void setPrevMVP(const float* mvp, const float* model);

    // ------------------------------------------------------------------
    // Skeletal animation support (uniform-skinning, GPU 4-weight blend).
    //
    // Each concrete material decides via its own ShaderFlag bit whether
    // skinning is active. When active, the material's VS builder must
    // include `Material::skinningVSChunk(true)` and call `skinPos()` /
    // `skinNormal()` on the input attributes; the geometry's VAO must
    // expose `ivec4 aBoneIds @ loc 4` and `vec4 aWeights @ loc 5`
    // (Mesh::upload() already does this when the mesh has bone data).
    // The material then calls `uploadBonePalette()` from its `use()` to
    // push the 100-mat4 palette to the program.
    // ------------------------------------------------------------------
    void setAnimator(Animator* a) { animator = a; }
    Animator* getAnimator() const { return animator; }

    // Returns a GLSL chunk to be appended to the VS preamble. When
    // `enabled == true` it declares the bone attributes / uBones uniform
    // and provides skinPos()/skinNormal(); when `enabled == false` it
    // emits identity stubs so the same VS body can be reused unchanged.
    static const char* skinningVSChunk(bool enabled);

    // Maximum number of bones supported by the uniform palette. Mirrors
    // ModelLoader::MAX_BONES; kept independent so material code does not
    // need to include ModelLoader.h.
    static constexpr int MAX_BONES_UNIFORM = 100;

    // Upload the current Animator bone palette to `uBones[]` of the
    // currently-bound program. Pass the cached uniform location of
    // `uBones[0]`. No-op if `animator == nullptr` or `loc < 0`.
    void uploadBonePalette(GLint locBones) const;

protected:
    Shader* shader;
    Camera* camera;
    Animator* animator = nullptr;

public:
    bool initialized;
    float prevViewMatrix[16];
    float prevProjMatrix[16];
    float prevModelMatrix[16];
    float jitterOffset[2];
    int screenWidth = 0;
    int screenHeight = 0;
    int frameIndex = 0;
};
