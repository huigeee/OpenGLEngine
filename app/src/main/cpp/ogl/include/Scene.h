#pragma once

#include <GLES3/gl3.h>
#include <array>
#include <vector>
#include "ResourceLoader.h"
#include "Camera.h"
#include "Object3D.h"
#include "Material.h"
#include "SkyBoxEffect.h"
#include "FogEffect.h"
#include "TAAEffect.h"
#include "DoFEffect.h"
#include "BloomEffect.h"
#include "TonemapEffect.h"
#include "PostEffectBase.h"
#include "RenderContext.h"
#include "Matrix.h"
#include "Material.h"
#include "SkyBoxEffect.h"
#include "PostEffectBase.h"
#include "RenderContext.h"
#include "TAAEffect.h"
#include "FXAAEffect.h"
#include "FogEffect.h"
#include "BloomEffect.h"
#include "DoFEffect.h"
#include "TonemapEffect.h"
#include "VolumetricLightEffect.h"
#include "../shadow/ShadowEffect.h"
#include <glm/glm.hpp>
#include "../tween/Tween.h"

class ResourceLoader;
class IBLPBRMaterial;
class SceneNode;

// ---------------------------------------------------------------------------
// SceneConfig — user-facing settings applied once before Scene::init()
// ---------------------------------------------------------------------------
struct SceneConfig {
    // --- Camera ---
    float camEyeX = 0.0f, camEyeY = 0.0f, camEyeZ = 3.0f;
    float camTargetX = 0.0f, camTargetY = 0.0f, camTargetZ = 0.0f;
    float camUpX = 0.0f, camUpY = 1.0f, camUpZ = 0.0f;
    float camFovY     = 45.0f;   // degrees
    float camNear     = 0.1f;
    float camFar      = 10.0f;

    // --- Anti-aliasing (TAA and FXAA are mutually exclusive; TAA wins if both set) ---
    // Match Java: 0=TAA, 1=FXAA, 2=None
    enum class AAMode { TAA = 0, FXAA = 1, None = 2 };
    AAMode aaMode = AAMode::TAA;  // Default: TAA

    // Fog
    bool  fogEnabled = false;
    int   fogMode    = 1;       // 0=linear 1=exp 2=exp2
    float fogDensity = 0.25f;
    float fogStart   = 1.0f;
    float fogEnd     = 8.0f;
    float fogR = 0.7f, fogG = 0.75f, fogB = 0.8f;

    // Bloom
    bool  bloomEnabled    = false;
    float bloomThreshold  = 0.8f;
    float bloomIntensity  = 1.5f;
    int   bloomBlurPasses = 6;

    // Depth-of-Field
    bool  dofEnabled   = false;
    float dofFocusDist = 0.5f;
    float dofNear      = 2.0f;
    float dofFar       = 20.0f;

    // Tone Mapping (applied after all HDR effects, before FXAA)
    bool  tonemapEnabled  = false;
    int   tonemapOp       = 1;     // 0=Reinhard 1=ACES(default) 2=Uncharted2
    float tonemapExposure = 1.0f;
    
    // Background color
    float bgColorR = 0.1f;
    float bgColorG = 0.1f;
    float bgColorB = 0.15f;
    
    // Ambient intensity
    float ambientIntensity = 1.0f;
    
    // SSAO
    bool ssaoEnabled = false;
    float ssaoRadius = 0.5f;
    float ssaoIntensity = 1.0f;
    
    // Shadow
    bool shadowEnabled = false;
    float lightPosX = 10.0f;    // Increased for better shadow angle
    float lightPosY = 15.0f;    // Higher light for better shadow coverage
    float lightPosZ = 10.0f;
    float shadowMapSize = 2048;
    float shadowBias = 0.005f;   // Match UnlitMaterial default
    float shadowSoftness = 0.5f;
    bool shadowPCFEnabled = true;  // PCF soft shadow toggle
    float shadowDistance = 20.0f;   // Increased coverage
    float shadowNearPlane = 0.1f;
    float shadowFarPlane = 50.0f;
    
    // Camera orthographic
    bool camOrthographic = false;
    float camOrthoSize = 10.0f;
    
    // TAA
    bool taaEnabled = false;

    // Volumetric light (god rays)
    bool  volumetricEnabled    = false;
    float volumetricDensity    = 0.03f;
    float volumetricScattering = 0.75f;   // HG g
    int   volumetricSteps      = 48;
    float volumetricMaxDistance = 30.0f;
    float volumetricIntensity  = 1.0f;
    float volumetricColorR     = 1.0f;
    float volumetricColorG     = 0.95f;
    float volumetricColorB     = 0.85f;
};

// ---------------------------------------------------------------------------
// Scene — engine-managed rendering pipeline
//   Subclass it and override the virtual hooks to add your own content.
// ---------------------------------------------------------------------------
class Scene {
public:
    Scene();
    virtual ~Scene();

    // Engine API (called by EGLCore / JNI glue)
    void setFilesDir(const char* dir);
    void init();
    void setRotation(float deltaX, float deltaY);
    void setViewport(int width, int height);
    void render();
    void release();

    // Configuration — set before init(), or call rebuildPostEffects() at runtime
    SceneConfig config;
    void rebuildPostEffects();   // tear down and rebuild post-effect chain from config
    
    // Inspector 参数批量设置
    void updateCameraParams(float posX, float posY, float posZ,
                           float targetX, float targetY, float targetZ,
                           float fov, bool orthographic);
    void updateSceneConfig(float bgR, float bgG, float bgB,
                           float ambient, bool shadow, bool ssao);
    void updatePostProcessParams(int aaMode, bool bloom, float bloomIntensity,
                                bool tonemap, int tonemapOp, float exposure,
                                bool fog, int fogMode, float fogDensity,
                                float fogStart, float fogEnd, float fogR, float fogG, float fogB,
                                bool dof, float dofFocusDist, float dofNear, float dofFar,
                                bool volumetric = false, float volDensity = 0.03f,
                                float volScattering = 0.75f, int volSteps = 48,
                                float volMaxDistance = 30.0f, float volIntensity = 1.0f,
                                float volColorR = 1.0f, float volColorG = 0.95f, float volColorB = 0.85f);
    void updateMaterialParams(float baseR, float baseG, float baseB,
                             float metallic, float roughness, float ao);

    // Called by ShadowEffect during shadow pass
    void renderForShadowMap();
    void renderObjectsForShadow(GLuint program, GLint locModel, GLint locLightSpace);
    
    // Shadow control
    void updateShadowLightPosition(float x, float y, float z);
    void setShadowBias(float bias);
    void setShadowSoftness(float softness);
    
    // Accessor for shadow effect (for EGLCore parameter updates)
    ShadowEffect* getShadowEffect() { return shadowEffect; }

protected:
    // -----------------------------------------------------------------------
    // Virtual hooks — override in subclass (MainScene) to add content
    // -----------------------------------------------------------------------
    virtual void onInit()    {}   // called at end of engine init
    virtual void onRelease() {}   // called at start of engine release
    virtual void onUpdate(float /*dt*/) {}  // called every frame before rendering

    // Called after all geometry / skybox has been rendered to sceneFBO.
    // Useful for particles or any transparent pass that must go on top.
    virtual void onRenderExtra(const float* viewMat, const float* projMat) {}

    // -----------------------------------------------------------------------
    // Protected helpers available to subclasses
    // -----------------------------------------------------------------------
    // Add an object + matching material slot (pass nullptr for material if
    // the object manages its own materials, e.g. ModelObject).
    void addObject(Object3D* obj, Material* mat = nullptr);

    // Register a SceneNode tree root. Each frame, before rendering, the engine
    // calls root->updateWorld(identity) which walks the tree and pushes a
    // world matrix into every attached MeshNode. This lets users group
    // sub-parts (e.g. a car door) and animate them as a unit while the
    // MeshNodes themselves remain in objects[] for normal rendering.
    //
    // Ownership: Scene takes ownership and deletes the root on release().
    void addSceneNodeRoot(SceneNode* root);

    // Camera / matrices
    Camera*       getCamera()           { return camera; }
    float*        getProjectionMatrix()       { return projectionMatrix; }

    // IBL maps (for subclass materials)
    GLuint getIrradianceMap() const { return irradianceMap; }
    GLuint getPrefilterMap()  const { return prefilterMap; }
    GLuint getBrdfLUT()       const { return brdfLUT; }

    // Resource loader
    ResourceLoader* getResourceLoader() { return resourceLoader; }
    const std::string& getFilesDir() const { return filesDir; }

    // Convenience: FBO width / height
    int getWidth()  const { return sceneWidth; }
    int getHeight() const { return sceneHeight; }

    // Expose scene textures for debug overlay rendering
    GLuint getSceneDepthTex() const { return sceneDepthTex; }
    GLuint getSceneVelocityTex() const { return sceneVelocityTex; }

    // Hook called after blitToScreen — subclass can draw debug overlays
    virtual void onPostBlit() {}

protected:
    // Protected members accessible by subclasses
    std::string filesDir;
    
    // Effect pointers (accessible for runtime updates)
    ShadowEffect*  shadowEffect  = nullptr;
    FogEffect*     fogEffect     = nullptr;
    TAAEffect*     taaEffect     = nullptr;
    FXAAEffect*    fxaaEffect    = nullptr;
    BloomEffect*   bloomEffect   = nullptr;
    DoFEffect*     dofEffect     = nullptr;
    TonemapEffect* tonemapEffect = nullptr;
    VolumetricLightEffect* volumetricEffect = nullptr;

private:
    // Internal rendering
    void renderSceneToFBO();
    void renderSkybox();
    void renderObjects();
    void blitToScreen(GLuint readFBO, GLuint colorTex);
    void buildSceneFBO(int w, int h);
    void rebuildProjectionMatrix();
    void buildPostEffectsInternal();
    void releasePostEffects();

    std::vector<Object3D*> objects;
    std::vector<Material*> materials;
    
    // Per-object previous-frame model matrix for TAA velocity computation.
    // Must stay in sync with objects.size().
    std::vector<std::array<float, 16>> prevObjectModels;
    std::vector<SceneNode*> sceneNodeRoots;   // transform-tree roots (P1 hierarchy)
    ResourceLoader* resourceLoader = nullptr;
    SkyBoxEffect*   skyBoxEffect   = nullptr;
    Camera*         camera         = nullptr;

    float projectionMatrix[16];
    float prevFrameProjMatrix[16];   // unjittered projection of last frame
    float prevFrameViewMatrix[16];   // view matrix of last frame
    float jitterOffset[2];

    GLuint irradianceMap = 0;
    GLuint prefilterMap  = 0;
    GLuint brdfLUT       = 0;

    GLuint sceneFBO        = 0;
    GLuint sceneColorTex   = 0;
    GLuint sceneDepthTex   = 0;
    GLuint sceneVelocityTex = 0;
    int    sceneWidth  = 0;
    int    sceneHeight = 0;

    // Ordered post-effect chain (rebuilt from config)
    std::vector<PostEffectBase*> postEffects;

    int       frameCount       = 0;
    long long lastFrameTimeNs  = 0;

    // Blit quad (debug / final pass)
    GLuint debugQuadProg = 0;
    GLuint debugQuadVAO  = 0;
};
