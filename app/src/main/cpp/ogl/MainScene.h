#pragma once

#include "include/Scene.h"
#include "particle/FireEffect.h"
#include "particle/RainEffect.h"
#include "particle/SnowEffect.h"
#include "particle/CloudEffect.h"
#include "include/Cube2.h"
#include "model/SceneNode.h"
#include "model/ModelLoader.h"
#include "model/Animator.h"
#include <glm/glm.hpp>

// ---------------------------------------------------------------------------
// MainScene — user-maintained subclass of Scene
//   All application-specific objects, materials, and particle effects live
//   here.  The engine-side Scene pipeline (FBO, skybox, TAA/FXAA, fog, etc.)
//   is configured via Scene::config and managed entirely by the base class.
// ---------------------------------------------------------------------------
class MainScene : public Scene {
public:
    MainScene();
    ~MainScene() override = default;

protected:
    // Scene virtual hooks
    void onInit()    override;
    void onRelease() override;
    void onUpdate(float dt) override;
    void onRenderExtra(const float* viewMat, const float* projMat) override;
    void onPostBlit() override;

private:
    void loadCarModel();
    void loadCarModel1();

private:
    // --- Particle effects ---
    FireEffect*  fireEffect  = nullptr;
    RainEffect*  rainEffect  = nullptr;
    SnowEffect*  snowEffect  = nullptr;
    CloudEffect* cloudEffect = nullptr;

    glm::vec3 firePos  = {  0.8f, -0.5f, -1.5f };
    glm::vec3 rainPos  = {  0.0f,  0.0f,  0.0f };
    glm::vec3 snowPos  = {  0.0f,  0.0f,  0.0f };
    glm::vec3 cloudPos = {  0.0f,  3.0f, -3.0f };

    // Tween-driven cube
    Cube2* tweenedCube = nullptr;
    float  cubeRotY    = 0.0f;

    // Hierarchy demo: front-left door, animated open/close
    SceneNode* doorFL = nullptr;

    // Cowboy COLLADA — skeletal animation demo
    ModelLoader* cowboyLoader   = nullptr;   // owns bones/animations/NodeDesc
    Animator*    cowboyAnimator = nullptr;

    // Debug overlay: show depth + velocity textures
    GLuint debugQuadVAO = 0;
    GLuint debugQuadVBO = 0;
    GLuint debugProgram = 0;
    GLint  debugLoc_tex = -1;
    GLint  debugLoc_mode = -1;
};
