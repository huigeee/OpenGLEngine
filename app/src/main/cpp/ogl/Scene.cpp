#include "include/Scene.h"
#include "include/Sphere2.h"
#include "include/Cube2.h"
#include "include/ResourceLoader.h"
#include "include/TAAEffect.h"
#include "include/BloomEffect.h"
#include "include/DoFEffect.h"
#include "include/FXAAEffect.h"
#include "include/FogEffect.h"
#include "include/VolumetricLightEffect.h"
#include "include/TonemapEffect.h"
#include "include/IBLPBRMaterial.h"
#include "include/ClothMaterial.h"
#include "include/UnlitMaterial.h"
#include "include/Common.h"
#include "shadow/ShadowEffect.h"
#include "model/SceneNode.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
Scene::Scene() : resourceLoader(nullptr), camera(nullptr) {
    projectionMatrix[0]  = 1; projectionMatrix[1]  = 0; projectionMatrix[2]  = 0; projectionMatrix[3]  = 0;
    projectionMatrix[4]  = 0; projectionMatrix[5]  = 1; projectionMatrix[6]  = 0; projectionMatrix[7]  = 0;
    projectionMatrix[8]  = 0; projectionMatrix[9]  = 0; projectionMatrix[10] = 1; projectionMatrix[11] = 0;
    projectionMatrix[12] = 0; projectionMatrix[13] = 0; projectionMatrix[14] = 0; projectionMatrix[15] = 1;
    jitterOffset[0] = jitterOffset[1] = 0.0f;
    memset(prevFrameProjMatrix, 0, sizeof(prevFrameProjMatrix));
    prevFrameProjMatrix[0] = prevFrameProjMatrix[5] = prevFrameProjMatrix[10] = prevFrameProjMatrix[15] = 1.0f;
    memset(prevFrameViewMatrix, 0, sizeof(prevFrameViewMatrix));
    prevFrameViewMatrix[0] = prevFrameViewMatrix[5] = prevFrameViewMatrix[10] = prevFrameViewMatrix[15] = 1.0f;
}

Scene::~Scene() {
    release();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void Scene::setFilesDir(const char* dir) {
    filesDir = dir ? dir : "";
    resourceLoader = new ResourceLoader();
    resourceLoader->setFilesDir(filesDir.c_str());
}

void Scene::setRotation(float deltaX, float deltaY) {
    if (camera) camera->setRotation(deltaX, deltaY);
}

// ---------------------------------------------------------------------------
// addObject — protected helper for subclasses
// ---------------------------------------------------------------------------
void Scene::addObject(Object3D* obj, Material* mat) {
    objects.push_back(obj);
    materials.push_back(mat);
    prevObjectModels.resize(objects.size());
}

// ---------------------------------------------------------------------------
// addSceneNodeRoot — register a transform-tree root.
// Scene takes ownership and deletes the tree on release().
// ---------------------------------------------------------------------------
void Scene::addSceneNodeRoot(SceneNode* root) {
    if (root) sceneNodeRoots.push_back(root);
}

// ---------------------------------------------------------------------------
// rebuildProjectionMatrix
// ---------------------------------------------------------------------------
void Scene::rebuildProjectionMatrix() {
    float aspect = (float)sceneWidth / (float)sceneHeight;
    float fovY   = config.camFovY * 3.14159f / 180.0f;
    Matrix::perspective(projectionMatrix, fovY, aspect, config.camNear, config.camFar);
}

// ---------------------------------------------------------------------------
// buildSceneFBO
// ---------------------------------------------------------------------------
void Scene::buildSceneFBO(int w, int h) {
    if (sceneFBO) {
        glDeleteFramebuffers(1, &sceneFBO);
        glDeleteTextures(1, &sceneColorTex);
        glDeleteTextures(1, &sceneDepthTex);
        glDeleteTextures(1, &sceneVelocityTex);
    }
    sceneWidth  = w;
    sceneHeight = h;

    glGenTextures(1, &sceneColorTex);
    glBindTexture(GL_TEXTURE_2D, sceneColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &sceneDepthTex);
    glBindTexture(GL_TEXTURE_2D, sceneDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &sceneVelocityTex);
    glBindTexture(GL_TEXTURE_2D, sceneVelocityTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, w, h, 0, GL_RG, GL_HALF_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &sceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColorTex,    0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, sceneVelocityTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_TEXTURE_2D, sceneDepthTex,    0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        LOGE("Scene FBO incomplete: 0x%x", status);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    for (auto e : postEffects) e->onSurfaceChanged(w, h);
}

// ---------------------------------------------------------------------------
// Post-effect chain management
// ---------------------------------------------------------------------------
void Scene::releasePostEffects() {
    for (auto e : postEffects) { e->release(); delete e; }
    postEffects.clear();
    fogEffect     = nullptr;
    taaEffect     = nullptr;
    fxaaEffect    = nullptr;
    bloomEffect   = nullptr;
    dofEffect     = nullptr;
    tonemapEffect = nullptr;
    volumetricEffect = nullptr;
}

void Scene::buildPostEffectsInternal() {
    releasePostEffects();

    // TAA and FXAA are mutually exclusive; TAA takes priority
    bool useTAA  = (config.aaMode == SceneConfig::AAMode::TAA);
    bool useFXAA = (config.aaMode == SceneConfig::AAMode::FXAA);

    // Pipeline order: Fog → TAA → DoF → Bloom → FXAA
    // TAA: early, operates on HDR scene data, needs velocity buffer
    // FXAA: last, screen-space pass on the final composited image
    if (config.fogEnabled) {
        fogEffect = new FogEffect(config.fogMode, config.fogDensity,
                                  config.fogStart, config.fogEnd,
                                  config.fogR, config.fogG, config.fogB);
        postEffects.push_back(fogEffect);
    }
    if (useTAA) {
        taaEffect = new TAAEffect();
        postEffects.push_back(taaEffect);
    }
    if (config.dofEnabled) {
        dofEffect = new DoFEffect(config.dofFocusDist, config.dofNear, config.dofFar);
        postEffects.push_back(dofEffect);
    }
    if (config.bloomEnabled) {
        bloomEffect = new BloomEffect(config.bloomThreshold, config.bloomIntensity, config.bloomBlurPasses);
        postEffects.push_back(bloomEffect);
    }
    if (config.volumetricEnabled) {
        volumetricEffect = new VolumetricLightEffect();
        volumetricEffect->setDensity(config.volumetricDensity);
        volumetricEffect->setScattering(config.volumetricScattering);
        volumetricEffect->setSteps(config.volumetricSteps);
        volumetricEffect->setMaxDistance(config.volumetricMaxDistance);
        volumetricEffect->setIntensity(config.volumetricIntensity);
        volumetricEffect->setLightColor(config.volumetricColorR, config.volumetricColorG, config.volumetricColorB);
        postEffects.push_back(volumetricEffect);
    }
    if (config.tonemapEnabled) {
        tonemapEffect = new TonemapEffect(config.tonemapOp, config.tonemapExposure);
        postEffects.push_back(tonemapEffect);
    }
    if (useFXAA) {
        fxaaEffect = new FXAAEffect();
        postEffects.push_back(fxaaEffect);
    }

    // Init all effects with current surface size (may be 0 before setViewport)
    if (sceneWidth > 0 && sceneHeight > 0) {
        for (auto e : postEffects) e->init(sceneWidth, sceneHeight);
    }
}

void Scene::rebuildPostEffects() {
    buildPostEffectsInternal();
    // re-init already handled inside buildPostEffectsInternal
}

// ---------------------------------------------------------------------------
// init
// ---------------------------------------------------------------------------
void Scene::init() {
    camera = new Camera();
    camera->lookAt(config.camEyeX,    config.camEyeY,    config.camEyeZ,
                   config.camTargetX, config.camTargetY, config.camTargetZ,
                   config.camUpX,     config.camUpY,     config.camUpZ);

    std::string fullPath = filesDir + "/texture/environment_info2.hdr";
    LOGI("Scene::init - HDR path: %s", fullPath.c_str());
    skyBoxEffect = SkyBoxEffect::getInstance();
    skyBoxEffect->setHdrPath(fullPath);
    skyBoxEffect->InitProgram();
    skyBoxEffect->generatePBRData(&irradianceMap, &prefilterMap, &brdfLUT);
    LOGI("Scene::init - irradianceMap=%u prefilterMap=%u brdfLUT=%u", irradianceMap, prefilterMap, brdfLUT);

    // Build post-effect chain from config
    buildPostEffectsInternal();
    
    // Initialize shadow effect if enabled (deferred to setViewport)
    if (config.shadowEnabled) {
        shadowEffect = new ShadowEffect();
        shadowEffect->enabled = true;
        shadowEffect->setLightPosition(config.lightPosX, config.lightPosY, config.lightPosZ);
        shadowEffect->setSceneRef(this);
        // Apply shadow settings from config
        shadowEffect->setShadowBias(config.shadowBias);
        shadowEffect->setShadowSoftness(config.shadowSoftness);
        // Will be initialized in setViewport when we have valid dimensions
        if (sceneWidth > 0 && sceneHeight > 0) {
            shadowEffect->init(sceneWidth, sceneHeight);
        }
    }

    // Build blit quad
    static const char* VS = R"(
#version 300 es
layout(location = 0) in vec2 aPos;
out vec2 vUV;
void main() {
    vUV = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";
    static const char* FS = R"(
#version 300 es
precision highp float;
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uTex;
void main() {
    FragColor = texture(uTex, vUV);
}
)";
    GLuint vs = compileShader(GL_VERTEX_SHADER, VS);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, FS);
    debugQuadProg = glCreateProgram();
    glAttachShader(debugQuadProg, vs);
    glAttachShader(debugQuadProg, fs);
    glLinkProgram(debugQuadProg);
    glDeleteShader(vs);
    glDeleteShader(fs);

    float quad[] = {-1.0f,-1.0f, 1.0f,-1.0f, -1.0f,1.0f, 1.0f,1.0f};
    glGenVertexArrays(1, &debugQuadVAO);
    GLuint qvbo;
    glGenBuffers(1, &qvbo);
    glBindVertexArray(debugQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, qvbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);

    // Subclass hook
    onInit();
}

// ---------------------------------------------------------------------------
// setViewport
// ---------------------------------------------------------------------------
void Scene::setViewport(int width, int height) {
    sceneWidth  = width;
    sceneHeight = height;
    rebuildProjectionMatrix();
    buildSceneFBO(width, height);
    
    // Initialize shadow effect if enabled (deferred from init())
    if (config.shadowEnabled && shadowEffect && !shadowEffect->isShadowEnabled()) {
        shadowEffect->init(width, height);
    }
}

// ---------------------------------------------------------------------------
// renderSkybox / renderObjects
// ---------------------------------------------------------------------------
void Scene::renderSkybox() {
    float* viewMat = camera->getViewMatrix();
    skyBoxEffect->DrawFrame(projectionMatrix, viewMat);
}

void Scene::renderForShadowMap() {
    // Render all opaque objects for shadow map
    for (size_t i = 0; i < objects.size(); i++) {
        Object3D* obj = objects[i];
        obj->render();
    }
}

void Scene::renderObjectsForShadow(GLuint program, GLint locModel, GLint locLightSpace) {
    // Render all objects with proper model matrix for shadow pass
    int renderedCount = 0;
    for (size_t i = 0; i < objects.size(); i++) {
        Object3D* obj = objects[i];
        if (!obj->isVisible()) continue;
        
        float modelMat[16];
        obj->getModelMatrix(modelMat);
        
        // Set model matrix for shadow shader
        glUniformMatrix4fv(locModel, 1, GL_FALSE, modelMat);
        
        obj->render();
        renderedCount++;
    }
//    LOGI("renderObjectsForShadow: rendered %d objects", renderedCount);
}

void Scene::updateShadowLightPosition(float x, float y, float z) {
    if (shadowEffect) {
        shadowEffect->setLightPosition(x, y, z);
    }
}

void Scene::setShadowBias(float bias) {
    config.shadowBias = bias;
    if (shadowEffect) {
        shadowEffect->setShadowBias(bias);
    }
}

void Scene::setShadowSoftness(float softness) {
    config.shadowSoftness = softness;
    if (shadowEffect) {
        shadowEffect->setShadowSoftness(softness);
    }
}

void Scene::renderObjects() {
    // Disable cull face to render ALL faces (both front and back)
    glDisable(GL_CULL_FACE);  // FIX: Disable culling to see all geometry
    
    // Use the same light position as shadow for consistency
    float lightX = config.lightPosX;
    float lightY = config.lightPosY;
    float lightZ = config.lightPosZ;
    
    float lp[4][3] = {
        { lightX, lightY, lightZ }, {-10.0f,  10.0f, 10.0f},
        { 10.0f, -10.0f, 10.0f}, {-10.0f, -10.0f, 10.0f}
    };
    float lc[4][3] = {
        {300.0f, 300.0f, 300.0f}, {300.0f, 300.0f, 300.0f},
        {300.0f, 300.0f, 300.0f}, {300.0f, 300.0f, 300.0f}
    };

    float* viewMat = camera->getViewMatrix();
    float* camPos  = camera->getPosition();
    
    // Save blend state to restore later
    GLboolean prevBlendEnabled = glIsEnabled(GL_BLEND);

    // Helper lambda to set up material + render one object
    auto renderOne = [&](size_t i) {
        Object3D* obj = objects[i];
        if (!obj->isVisible()) {
            return;
        }

        float modelMat[16];
        obj->getModelMatrix(modelMat);

        // Prefer the object's own material (MeshNode path) over the parallel slot
        Material* mat = obj->getMaterial();
        if (!mat) mat = materials[i];
        
        obj->onPreRender();
        obj->beforeRender();
        if (mat) {
            // Set per-object prevModel (from saved array) right before use
            if (i < prevObjectModels.size()) {
                mat->setPrevModelMatrix(prevObjectModels[i].data());
            }

            IBLPBRMaterial* ibl = dynamic_cast<IBLPBRMaterial*>(mat);
            if (ibl) {
                glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
                glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
                glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, brdfLUT);
                ibl->setViewMatrix(viewMat);
                ibl->setModelMatrix(modelMat);
                ibl->setProjectionMatrix(projectionMatrix);
                ibl->setCamPos(camPos[0], camPos[1], camPos[2]);
                for (int j = 0; j < 4; j++) ibl->setLight(j, lp[j], lc[j]);
                
                // Set shadow uniforms if shadow is enabled
                if (shadowEffect && shadowEffect->isShadowEnabled()) {
                    ibl->setShadowMapTexture(shadowEffect->getShadowMapTexture());
                    ibl->setLightSpaceMatrix(shadowEffect->getLightSpaceMatrix());
                    ibl->setShadowBias(config.shadowBias);
                    ibl->setShadowEnabled(true);
                } else {
                    ibl->setShadowEnabled(false);
                }
                
                ibl->use();
            } else if (ClothMaterial* cloth = dynamic_cast<ClothMaterial*>(mat)) {
                glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
                glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
                glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, brdfLUT);
                cloth->setViewMatrix(viewMat);
                cloth->setModelMatrix(modelMat);
                cloth->setProjectionMatrix(projectionMatrix);
                cloth->setCamPos(camPos[0], camPos[1], camPos[2]);
                for (int j = 0; j < 4; j++) cloth->setLight(j, lp[j], lc[j]);
                if (shadowEffect && shadowEffect->isShadowEnabled()) {
                    cloth->setShadowMapTexture(shadowEffect->getShadowMapTexture());
                    cloth->setLightSpaceMatrix(shadowEffect->getLightSpaceMatrix());
                    cloth->setShadowBias(config.shadowBias);
                    cloth->setShadowEnabled(true);
                } else {
                    cloth->setShadowEnabled(false);
                }
                cloth->use();
            } else {
                mat->setTransform(modelMat, viewMat, projectionMatrix);
                
                // Handle UnlitMaterial shadow uniforms
                UnlitMaterial* unlit = dynamic_cast<UnlitMaterial*>(mat);
                if (unlit) {
                    if (shadowEffect && shadowEffect->isShadowEnabled()) {
                        unlit->setShadowMap(shadowEffect->getShadowMapTexture());
                        unlit->setLightSpaceMatrix(shadowEffect->getLightSpaceMatrix());
                        unlit->setShadowEnabled(true);
                        unlit->setShadowBias(config.shadowBias);
                        unlit->setShadowPCFEnabled(config.shadowPCFEnabled);  // Use config value
                        unlit->setShadowIntensity(0.7f);  // Softer shadows (70% intensity)
                    } else {
                        unlit->setShadowEnabled(false);
                    }
                }
                
                mat->use();
            }
        }
        obj->render();
        if (mat) mat->onPostRender();
        obj->afterRender();
    };

    // ---- Opaque pass ----
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    for (size_t i = 0; i < objects.size(); ++i) {
        Material* mat = objects[i]->getMaterial();
        if (!mat) mat = materials[i];
        if (mat && mat->isTransparent()) continue;  // skip transparent
        renderOne(i);
    }

    // ---- Transparent pass — sort back-to-front by camera distance ----
    // Collect transparent indices
    std::vector<std::pair<float, size_t>> transparentItems;
    glm::mat4 viewGlm = glm::make_mat4(viewMat);
    for (size_t i = 0; i < objects.size(); ++i) {
        if (!objects[i]->isVisible()) continue;
        Material* mat = objects[i]->getMaterial();
        if (!mat) mat = materials[i];
        if (!mat || !mat->isTransparent()) continue;
        float* pos = objects[i]->getPosition();
        glm::vec4 vPos = viewGlm * glm::vec4(pos[0], pos[1], pos[2], 1.0f);
        transparentItems.push_back({vPos.z, i});  // more negative = farther
    }
    // Sort: most negative z first (farthest first)
    std::sort(transparentItems.begin(), transparentItems.end(),
        [](const auto& a, const auto& b){ return a.first < b.first; });

    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);  // Ensure blend is enabled for transparent objects
    for (auto& [depth, idx] : transparentItems) {
        renderOne(idx);
    }
    glDepthMask(GL_TRUE);
    
    // Restore blend state
    if (prevBlendEnabled) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
}

// ---------------------------------------------------------------------------
// renderSceneToFBO
// ---------------------------------------------------------------------------
void Scene::renderSceneToFBO() {
    // Verify we're bound to the correct FBO
    GLint currentFBO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
    if (currentFBO != (GLint)sceneFBO) {
        LOGE("renderSceneToFBO: FBO binding error! current=%d expected=%d", currentFBO, sceneFBO);
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glViewport(0, 0, sceneWidth, sceneHeight);
    glDrawBuffers(2, (const GLenum[]){GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});
    glClearColor(config.bgColorR, config.bgColorG, config.bgColorB, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    
    // Shadow pass - render shadow map before main rendering
    if (shadowEffect && shadowEffect->isShadowEnabled()) {
        shadowEffect->renderShadowPass();
    }
    
    // Re-bind sceneFBO (shadow pass changes FBO binding)
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glDrawBuffers(2, (const GLenum[]){GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});

    // Render skybox with only attachment0 active — skybox FS has no velocity output,
    // and leaving attachment1 active would write undefined values for background pixels.
    glDrawBuffers(1, (const GLenum[]){GL_COLOR_ATTACHMENT0});
    renderSkybox();

    // Clear velocity (attachment1) to zero for background pixels, then restore 2-buffer mode.
    const float zeroVelocity[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    glDrawBuffers(2, (const GLenum[]){GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});
    glClearBufferfv(GL_COLOR, 1, zeroVelocity);

    glClearDepthf(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    renderObjects();

    // Restore draw buffers to 2-buffer mode for subsequent frames
    glDrawBuffers(2, (const GLenum[]){GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});
    // Restore depth function to LEQUAL for consistent depth testing
    glDepthFunc(GL_LEQUAL);
}

// ---------------------------------------------------------------------------
// blitToScreen
// ---------------------------------------------------------------------------
void Scene::blitToScreen(GLuint readFBO, GLuint) {
    if (readFBO == 0) readFBO = sceneFBO;
    
    glBindFramebuffer(GL_READ_FRAMEBUFFER, readFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, sceneWidth, sceneHeight,
                      0, 0, sceneWidth, sceneHeight,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ---------------------------------------------------------------------------
// render — main per-frame entry point
// ---------------------------------------------------------------------------
void Scene::render() {
    if (camera == nullptr || sceneFBO == 0) return;

    // Delta time
    long long nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    float dt = (lastFrameTimeNs == 0) ? 0.016f : (float)((nowNs - lastFrameTimeNs) * 1e-9);
    if (dt > 0.1f) dt = 0.1f;
    lastFrameTimeNs = nowNs;

    // Tween
    TweenManager::instance().update(dt);

    // Subclass update hook
    onUpdate(dt);

    // Walk SceneNode hierarchies and push world matrices into MeshNodes.
    // Must run after onUpdate (so user-set rotations apply this frame) and
    // before any matrix sampling on objects.
    if (!sceneNodeRoots.empty()) {
        static const float kIdentity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
        for (SceneNode* root : sceneNodeRoots) {
            if (root) root->updateWorld(kIdentity);
        }
    }

    float* viewMat = camera->getViewMatrix();

    // TAA jitter
    bool taaEnabled = (taaEffect && taaEffect->enabled);
    float jitter[2] = {0.0f, 0.0f};
    if (taaEnabled) {
        static const float Halton_2_3[8][2] = {
            { 0.0f,        -1.0f/3.0f},
            {-1.0f/2.0f,   1.0f/3.0f},
            { 1.0f/2.0f,  -7.0f/9.0f},
            {-3.0f/4.0f,  -1.0f/9.0f},
            { 1.0f/4.0f,   5.0f/9.0f},
            {-1.0f/4.0f,  -5.0f/9.0f},
            { 3.0f/4.0f,   1.0f/9.0f},
            {-7.0f/8.0f,   7.0f/9.0f}
        };
        int idx   = frameCount % 8;
        jitter[0] = Halton_2_3[idx][0] / (float)sceneWidth;
        jitter[1] = Halton_2_3[idx][1] / (float)sceneHeight;
    }

    float prevProj[16];
    memcpy(prevProj, prevFrameProjMatrix, 16 * sizeof(float));

    float prevView[16];
    memcpy(prevView, prevFrameViewMatrix, 16 * sizeof(float));

    // Push TAA / per-frame state to all materials (except prevModel — set per-object in renderOne)
    for (size_t i = 0; i < objects.size(); ++i) {
        // Prefer the object's own material (e.g. MeshNode) over the parallel slot
        Material* mat = objects[i]->getMaterial();
        if (!mat) mat = materials[i];
        if (!mat) continue;

        mat->setScreenSize(sceneWidth, sceneHeight);
        mat->setPrevViewMatrix(prevView);
        mat->setPrevProjectionMatrix(prevProj);
        mat->setJitterOffset(jitter);
        mat->setFrameCount(frameCount);
        IBLPBRMaterial* ibl = dynamic_cast<IBLPBRMaterial*>(mat);
        if (ibl) { 
            ibl->setViewMatrix(viewMat); 
            ibl->setProjectionMatrix(projectionMatrix); 
            ibl->setScreenSize(sceneWidth, sceneHeight);
        }
    }

    // Save current model matrices as next frame's prevModel (per-object)
    for (size_t i = 0; i < objects.size(); ++i) {
        objects[i]->getModelMatrix(prevObjectModels[i].data());
    }

    // Save unjittered projection and current view for next frame's prev matrices
    memcpy(prevFrameProjMatrix, projectionMatrix, 16 * sizeof(float));
    memcpy(prevFrameViewMatrix, viewMat, 16 * sizeof(float));
    
    // Scene FBO pass - always bind to sceneFBO explicitly
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    renderSceneToFBO();

    // Extra render (particles, etc.) — subclass hook, already inside sceneFBO
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glDrawBuffers(1, (const GLenum[]){GL_COLOR_ATTACHMENT0});
    onRenderExtra(viewMat, projectionMatrix);
    // Keep sceneFBO bound for post-processing

    // Update FogEffect invViewProj
    if (fogEffect && fogEffect->enabled) {
        glm::mat4 proj = glm::make_mat4(projectionMatrix);
        glm::mat4 view = glm::make_mat4(viewMat);
        glm::mat4 invVP = glm::inverse(proj * view);
        float arr[16];
        const float* p = glm::value_ptr(invVP);
        for (int i = 0; i < 16; i++) arr[i] = p[i];
        fogEffect->setInvViewProj(arr);
    }

    // Update VolumetricLightEffect per-frame uniforms
    if (volumetricEffect && volumetricEffect->enabled) {
        glm::mat4 proj = glm::make_mat4(projectionMatrix);
        glm::mat4 view = glm::make_mat4(viewMat);
        glm::mat4 invVP = glm::inverse(proj * view);
        float arr[16];
        const float* p = glm::value_ptr(invVP);
        for (int i = 0; i < 16; i++) arr[i] = p[i];
        volumetricEffect->setInvViewProj(arr);

        float* camPos = camera->getPosition();
        volumetricEffect->setCameraPos(camPos[0], camPos[1], camPos[2]);

        // Light direction = from light toward scene center (assumed origin)
        float lx = config.lightPosX, ly = config.lightPosY, lz = config.lightPosZ;
        float dx = -lx, dy = -ly, dz = -lz;
        float len = sqrtf(dx*dx + dy*dy + dz*dz);
        if (len > 1e-4f) { dx /= len; dy /= len; dz /= len; }
        volumetricEffect->setLightDir(dx, dy, dz);
        volumetricEffect->setLightColor(config.volumetricColorR, config.volumetricColorG, config.volumetricColorB);

        bool shadowOn = config.shadowEnabled && shadowEffect && shadowEffect->isShadowEnabled();
        if (shadowOn) {
            volumetricEffect->setShadowMap(shadowEffect->getShadowMapTexture());
            volumetricEffect->setLightSpaceMatrix(shadowEffect->getLightSpaceMatrix());
            volumetricEffect->setUseShadow(true);
        } else {
            volumetricEffect->setShadowMap(0);
            volumetricEffect->setUseShadow(false);
        }
        // Live knobs
        volumetricEffect->setDensity(config.volumetricDensity);
        volumetricEffect->setScattering(config.volumetricScattering);
        volumetricEffect->setSteps(config.volumetricSteps);
        volumetricEffect->setMaxDistance(config.volumetricMaxDistance);
        volumetricEffect->setIntensity(config.volumetricIntensity);
    }

    // Post-effect chain
    RenderContext ctx;
    ctx.colorTex   = sceneColorTex;
    ctx.depthTex   = sceneDepthTex;
    ctx.velocityTex = sceneVelocityTex;
    ctx.fbo        = sceneFBO;
    ctx.width      = sceneWidth;
    ctx.height     = sceneHeight;
    ctx.isScreen   = false;

    for (auto effect : postEffects) {
        if (!effect->enabled) {
            continue;
        }
        ctx = effect->render(ctx);
    }
    
    blitToScreen(ctx.fbo, ctx.colorTex);
    onPostBlit();
    frameCount++;
}

// ---------------------------------------------------------------------------
// release
// ---------------------------------------------------------------------------
void Scene::release() {
    // Subclass cleanup first
    onRelease();

    for (Object3D* obj : objects) { obj->release(); delete obj; }
    objects.clear();
    for (auto m : materials) { if (!m) continue; m->release(); delete m; }
    materials.clear();

    // SceneNode trees: must be deleted AFTER Object3Ds, since SceneNode holds
    // non-owning MeshNode pointers — but MeshNodes were already destroyed
    // above. SceneNode dtor only walks its own children list, so this is OK.
    for (SceneNode* root : sceneNodeRoots) { delete root; }
    sceneNodeRoots.clear();

    releasePostEffects();
    
    // Release shadow effect
    if (shadowEffect) {
        shadowEffect->release();
        delete shadowEffect;
        shadowEffect = nullptr;
    }
    
    if (camera) { delete camera; camera = nullptr; }

    TweenManager::instance().clear();

    if (sceneFBO)         { glDeleteFramebuffers(1, &sceneFBO);         sceneFBO = 0; }
    if (sceneColorTex)    { glDeleteTextures(1, &sceneColorTex);         sceneColorTex = 0; }
    if (sceneDepthTex)    { glDeleteTextures(1, &sceneDepthTex);         sceneDepthTex = 0; }
    if (sceneVelocityTex) { glDeleteTextures(1, &sceneVelocityTex);      sceneVelocityTex = 0; }
    if (debugQuadProg)    { glDeleteProgram(debugQuadProg);               debugQuadProg = 0; }
    if (debugQuadVAO)     { glDeleteVertexArrays(1, &debugQuadVAO);       debugQuadVAO = 0; }
}

// -------------------------------------------------------------------------
// Inspector 参数批量设置
// -------------------------------------------------------------------------
void Scene::updateCameraParams(float posX, float posY, float posZ,
                              float targetX, float targetY, float targetZ,
                              float fov, bool orthographic) {
    if (camera == nullptr) return;
    
    // 使用 direct 方法，不重置 yaw/pitch，保持用户旋转的角度
    camera->setPositionDirect(posX, posY, posZ);
    camera->setTargetDirect(targetX, targetY, targetZ);
    
    // 更新配置
    config.camEyeX = posX;
    config.camEyeY = posY;
    config.camEyeZ = posZ;
    config.camTargetX = targetX;
    config.camTargetY = targetY;
    config.camTargetZ = targetZ;
    config.camFovY = fov;
    if (orthographic) {
        config.camOrthographic = true;
        config.camOrthoSize = fov / 2.0f;
    } else {
        config.camOrthographic = false;
    }
    
    rebuildProjectionMatrix();
}

void Scene::updateSceneConfig(float bgR, float bgG, float bgB,
                              float ambient, bool shadow, bool ssao) {
    config.bgColorR = bgR;
    config.bgColorG = bgG;
    config.bgColorB = bgB;
    config.ambientIntensity = ambient;
    config.ssaoEnabled = ssao;
    
    // Update shadow bias and softness if shadow effect exists
    if (shadowEffect) {
        shadowEffect->setShadowBias(config.shadowBias);
        shadowEffect->setShadowSoftness(config.shadowSoftness);
    }
    
    // Handle shadow enable/disable
    if (shadow != config.shadowEnabled) {
        config.shadowEnabled = shadow;
        
        if (shadow) {
            // Enable shadow
            if (!shadowEffect) {
                shadowEffect = new ShadowEffect();
                shadowEffect->setSceneRef(this);
                shadowEffect->setLightPosition(config.lightPosX, config.lightPosY, config.lightPosZ);
                shadowEffect->setShadowBias(config.shadowBias);
                shadowEffect->setShadowSoftness(config.shadowSoftness);
                shadowEffect->init(sceneWidth, sceneHeight);
            } else {
                // Already exists, just enable it
                shadowEffect->enabled = true;
            }
        } else {
            // Disable shadow
            if (shadowEffect) {
                shadowEffect->release();
                delete shadowEffect;
                shadowEffect = nullptr;
            }
        }
    }
}

void Scene::updatePostProcessParams(int aaMode, bool bloom, float bloomIntensity,
                                    bool tonemap, int tonemapOp, float exposure,
                                    bool fog, int fogMode, float fogDensity,
                                    float fogStart, float fogEnd, float fogR, float fogG, float fogB,
                                    bool dof, float dofFocusDist, float dofNear, float dofFar,
                                    bool volumetric, float volDensity, float volScattering, int volSteps,
                                    float volMaxDistance, float volIntensity,
                                    float volColorR, float volColorG, float volColorB) {
    bool aaModeChanged = (int)config.aaMode != aaMode;
    bool bloomChanged = (config.bloomEnabled != bloom);
    bool tonemapChanged = (config.tonemapEnabled != tonemap);
    bool fogChanged = (config.fogEnabled != fog);
    bool dofChanged = (config.dofEnabled != dof);
    bool bloomIntensityChanged = (config.bloomIntensity != bloomIntensity);
    bool tonemapOpChanged = (config.tonemapOp != tonemapOp);
    bool exposureChanged = (config.tonemapExposure != exposure);
    
    // Fog 参数变化检测
    bool fogModeChanged = (config.fogMode != fogMode);
    bool fogDensityChanged = (config.fogDensity != fogDensity);
    bool fogStartChanged = (config.fogStart != fogStart);
    bool fogEndChanged = (config.fogEnd != fogEnd);
    bool fogColorChanged = (fabsf(config.fogR - fogR) > 0.001f || 
                           fabsf(config.fogG - fogG) > 0.001f || 
                           fabsf(config.fogB - fogB) > 0.001f);
    
    // DoF 参数变化检测
    bool dofFocusDistChanged = (config.dofFocusDist != dofFocusDist);
    bool dofNearChanged = (config.dofNear != dofNear);
    bool dofFarChanged = (config.dofFar != dofFar);
    
    // AA Mode - directly set from Java (0=TAA, 1=FXAA, 2=None)
    config.aaMode = (SceneConfig::AAMode)aaMode;
    
    // Bloom
    config.bloomEnabled = bloom;
    config.bloomIntensity = bloomIntensity;
    
    // Tonemap
    config.tonemapEnabled = tonemap;
    config.tonemapOp = tonemapOp;
    config.tonemapExposure = exposure;
    
    // Fog
    config.fogEnabled = fog;
    config.fogMode = fogMode;
    config.fogDensity = fogDensity;
    config.fogStart = fogStart;
    config.fogEnd = fogEnd;
    config.fogR = fogR;
    config.fogG = fogG;
    config.fogB = fogB;
    
    // DoF
    config.dofEnabled = dof;
    config.dofFocusDist = dofFocusDist;
    config.dofNear = dofNear;
    config.dofFar = dofFar;

    // Volumetric
    bool volChanged = (config.volumetricEnabled != volumetric);
    bool volParamChanged = (config.volumetricDensity != volDensity) ||
                           (config.volumetricScattering != volScattering) ||
                           (config.volumetricSteps != volSteps) ||
                           (config.volumetricMaxDistance != volMaxDistance) ||
                           (config.volumetricIntensity != volIntensity) ||
                           (config.volumetricColorR != volColorR) ||
                           (config.volumetricColorG != volColorG) ||
                           (config.volumetricColorB != volColorB);
    config.volumetricEnabled = volumetric;
    config.volumetricDensity = volDensity;
    config.volumetricScattering = volScattering;
    config.volumetricSteps = volSteps;
    config.volumetricMaxDistance = volMaxDistance;
    config.volumetricIntensity = volIntensity;
    config.volumetricColorR = volColorR;
    config.volumetricColorG = volColorG;
    config.volumetricColorB = volColorB;
    
    // 当开关或参数值改变时重建后处理链
    bool rebuildNeeded = aaModeChanged || bloomChanged || tonemapChanged || 
                        bloomIntensityChanged || tonemapOpChanged || exposureChanged ||
                        fogChanged || dofChanged ||
                        fogModeChanged || fogDensityChanged || fogStartChanged || fogEndChanged || fogColorChanged ||
                        dofFocusDistChanged || dofNearChanged || dofFarChanged ||
                        volChanged;
    // Note: volParamChanged does NOT need rebuild — Scene::render() pushes those
    // uniforms live every frame. Just suppress the unused-var warning:
    (void)volParamChanged;
    
    if (rebuildNeeded) {
        rebuildPostEffects();
    }
}

void Scene::updateMaterialParams(float baseR, float baseG, float baseB,
                                float metallic, float roughness, float ao) {
    // TODO: 材质设置需要基于点击选择的对象，而不是全局修改
    // 当前注释掉，避免修改所有对象的材质
    // 
    // for (auto& obj : objects) {
    //     Material* mat = obj->getMaterial();
    //     if (mat && mat->isIBL()) {
    //         if (auto* pbrMat = dynamic_cast<IBLPBRMaterial*>(mat)) {
    //             pbrMat->setBaseColor(baseR, baseG, baseB);
    //             pbrMat->setMetallic(metallic);
    //             pbrMat->setRoughness(roughness);
    //             pbrMat->setAO(ao);
    //         }
    //     }
    // }
}
