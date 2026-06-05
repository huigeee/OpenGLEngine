#include "MainScene.h"
#include "../oglengine/include/Shader.h"
#include "../oglengine/include/Sphere2.h"
#include "../oglengine/include/Cube2.h"
#include "../oglengine/include/IBLPBRMaterial.h"
#include "../oglengine/include/ClothMaterial.h"
#include "../oglengine/include/ClothMesh.h"
#include "../oglengine/include/PBRMaterial.h"
#include "../oglengine/include/LitMaterial.h"
#include "../oglengine/include/UnlitMaterial.h"
#include "../oglengine/include/SkinnedMaterial.h"
#include "../oglengine/include/ResourceLoader.h"
#include "../oglengine/model/ModelObject.h"
#include "../oglengine/model/ModelLoader.h"
#include "../oglengine/model/MeshNode.h"
#include "../oglengine/model/Animator.h"
#include "../oglengine/include/GroundPlane.h"
#include "../oglengine/include/Common.h"
#include "../oglengine/tween/Tween.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ---------------------------------------------------------------------------
// Constructor: configure the rendering pipeline via Scene::config
// ---------------------------------------------------------------------------
MainScene::MainScene() {
    // --- 世界单位：米制 ---
    // 车模原始单位毫米，load 时 scale(0.001) 转换为米
    // 车长约 4.5m，车高约 1.5m

    // --- Camera ---
    // 从车侧后方斜上方俯视，距车约 7m
    config.camEyeX    =  4.0f;
    config.camEyeY    =  0.0f;   // 2m 高
    config.camEyeZ    =  5.0f;   // 车后方 7m
    config.camTargetX =  0.0f;
    config.camTargetY =  0.5f;   // 看向车身中部
    config.camTargetZ =  0.0f;
    config.camFovY    = 45.0f;
    config.camNear    =  0.1f;
    config.camFar     = 100.0f;  // 米制场景，100m 足够

    config.fogEnabled    = false;
    config.fogMode       = 1;
    config.fogDensity    = 0.25f;
    config.fogStart      = 1.0f;
    config.fogEnd        = 8.0f;
    config.fogR          = 0.7f;
    config.fogG          = 0.75f;
    config.fogB          = 0.8f;

    config.bloomEnabled  = false;
    config.dofEnabled    = false;
    config.aaMode = SceneConfig::AAMode::TAA;

    config.tonemapEnabled  = true;
    config.tonemapOp       = 1;     // ACES Filmic
    config.tonemapExposure = 1.0f;  // Reset to default for UnlitMaterial
    
    // Shadow
    config.shadowEnabled = false;
    config.lightPosX = 5.0f;
    config.lightPosY = 10.0f;
    config.lightPosZ = 5.0f;

    // 降低环境光，让体积光柱与阴影区域有更强对比
    config.ambientIntensity = 0.35f;
}

// ---------------------------------------------------------------------------
// onInit — create all scene objects, materials, and particles
// ---------------------------------------------------------------------------
void MainScene::onInit() {
    Camera*      cam  = getCamera();
    float*       proj = getProjectionMatrix();
    ResourceLoader* rl = getResourceLoader();
    GLuint irr  = getIrradianceMap();
    GLuint pref = getPrefilterMap();
    GLuint lut  = getBrdfLUT();

//    // ----- IBL PBR sphere grid (5×5) -----
//    const int ROWS = 5, COLS = 5;
//    const float spacing = 0.4f;
//    for (int row = 0; row < ROWS; ++row) {
//        for (int col = 0; col < COLS; ++col) {
//            IBLPBRMaterial* mat = new IBLPBRMaterial();
//            mat->init();
//            mat->setBaseColor(1.0f, 1.0f, 1.0f);
//            mat->setMetallic((float)row / (float)ROWS);
//            float rough = (float)col / (float)COLS;
//            if (rough < 0.05f) rough = 0.05f;
//            mat->setRoughness(rough);
//            mat->setAO(1.0f);
//
//            float x = (float)(col - COLS / 2) * spacing;
//            float y = (float)(row - ROWS / 2) * spacing;
//
//            Sphere2* s = new Sphere2();
//            s->init();
//            s->initGeometry(0.2f);
//            s->setMaterial(mat);
//            s->setCamera(cam);
//            s->setProjectionMatrix(proj);
//            s->setPosition(x, y, -2.0f);
//            addObject(s, mat);
//        }
//    }

    // ----- Tween-driven cube (IBL PBR) -----
    IBLPBRMaterial* cubeMat = new IBLPBRMaterial();
    cubeMat->init();
    cubeMat->setBaseColor(0.9f, 0.9f, 0.9f);   // Red
    cubeMat->setMetallic(0.1f);
    cubeMat->setRoughness(0.1f);
    cubeMat->setAO(1.0f);

    Cube2* cube = new Cube2();
    cube->init();
    cube->setMaterial(cubeMat);
    cube->setCamera(cam);
    cube->setProjectionMatrix(proj);
    cube->setPosition(1.6f, 0.3f, -1.5f);
    cube->setScale(0.8f);
    addObject(cube, cubeMat);
    tweenedCube = cube;

    // ----- Ground plane (40m × 40m, Y≈-0.73, IBL PBR) -----
    IBLPBRMaterial* groundMat = new IBLPBRMaterial();
    groundMat->init();
    groundMat->setBaseColor(0.4f, 0.4f, 0.4f);
    groundMat->setMetallic(0.0f);
    groundMat->setRoughness(0.8f);
    groundMat->setAO(1.0f);

    GroundPlane* ground = new GroundPlane();
    ground->setHalfSize(20.0f);   // 40m × 40m — big enough to fill view
    ground->setUVTile(20.0f);
    ground->init();
    ground->setMaterial(groundMat);
    ground->setPosition(0.0f, -0.73f, 0.0f);
    addObject(ground, groundMat);

    // ----- Unlit sphere -----
    UnlitMaterial* unlitMat = new UnlitMaterial();
    unlitMat->init();
    unlitMat->setColor(0.2f, 0.8f, 1.0f, 1.0f);
    Sphere2* unlitSphere = new Sphere2();
    unlitSphere->init();
    unlitSphere->initGeometry(0.15f);
    unlitSphere->setMaterial(unlitMat);
    unlitSphere->setCamera(cam);
    unlitSphere->setProjectionMatrix(proj);
    unlitSphere->setPosition(-1.6f, 0.9f, -2.0f);
    addObject(unlitSphere, unlitMat);

    // ----- Lit sphere -----
    LitMaterial* litMat = new LitMaterial();
    litMat->init();
    litMat->setBaseColor(1.0f, 0.6f, 0.2f, 1.0f);
    litMat->setLightPos(5.0f, 5.0f, 5.0f);
    Sphere2* litSphere = new Sphere2();
    litSphere->init();
    litSphere->initGeometry(0.15f);
    litSphere->setMaterial(litMat);
    litSphere->setCamera(cam);
    litSphere->setProjectionMatrix(proj);
    litSphere->setPosition(-1.6f, 0.3f, -2.0f);
    addObject(litSphere, litMat);

//    // ----- Cloth (Charlie sheen) sphere — red velvet to verify ClothMaterial -----
//    ClothMaterial* clothMat = new ClothMaterial();
//    clothMat->setBaseColor(0.5f, 0.05f, 0.05f);
//    clothMat->setSheenColor(1.0f, 0.3f, 0.3f);
//    clothMat->setSubsurfaceColor(0.2f, 0.02f, 0.02f);
//    clothMat->setRoughness(0.6f);
//    clothMat->setAO(1.0f);
//    clothMat->init();
//    Sphere2* clothSphere = new Sphere2();
//    clothSphere->init();
//    clothSphere->initGeometry(0.15f);
//    clothSphere->setMaterial(clothMat);
//    clothSphere->setCamera(cam);
//    clothSphere->setProjectionMatrix(proj);
//    clothSphere->setPosition(-1.6f, -0.3f, -2.0f);
//    addObject(clothSphere, clothMat);

//    // ----- Cloth dropped onto the red sphere, centered on it -----
//    ClothMaterial* dropMat = new ClothMaterial();
//    dropMat->setBaseColor(0.05f, 0.15f, 0.4f);
//    dropMat->setSheenColor(0.4f, 0.5f, 0.9f);
//    dropMat->setSubsurfaceColor(0.05f, 0.1f, 0.2f);
//    dropMat->setRoughness(0.7f);
//    dropMat->setAO(1.0f);
//    dropMat->init();
//    ClothMesh* dropCloth = new ClothMesh();
//    // Cloth just slightly above sphere top, centered on it
//    dropCloth->initCloth(24, 24, 0.06f);                   // ~1.44m × 1.44m
//    dropCloth->setMaterial(dropMat);
//    dropCloth->setCamera(cam);
//    dropCloth->setProjectionMatrix(proj);
//    // Cloth center slightly below sphere top so it rests immediately on it
//    dropCloth->setPositionWorld(-1.6f, -0.04f, -2.0f);          // sphere (-1.6,-0.3,-2.0) r=0.28 → top at Y=-0.02
//    // Collision sphere at same XZ — cloth starts slightly inside sphere top
//    CollisionWorld* dropCollision = new CollisionWorld();
//    dropCollision->addSphere(glm::vec3(-1.6f, -0.3f, -2.0f), 0.28f);  // sphere catches cloth
//    dropCollision->addPlaneFromPoint(glm::vec3(-1.6f, -0.7f, -2.0f), glm::vec3(0.0f, 1.0f, 0.0f));  // floor catches fall
//    dropCloth->setCollisionWorld(dropCollision);
//    addObject(dropCloth, dropMat);
//
//    // ----- Hanging cloth on a coat-hanger, swaying in the wind -----
//    ClothMaterial* hangerMat = new ClothMaterial();
//    hangerMat->setBaseColor(0.2f, 0.6f, 0.2f);             // green fabric
//    hangerMat->setSheenColor(0.6f, 0.9f, 0.5f);
//    hangerMat->setSubsurfaceColor(0.05f, 0.1f, 0.05f);
//    hangerMat->setRoughness(0.7f);
//    hangerMat->setAO(1.0f);
//    hangerMat->init();
//    ClothMesh* hangerCloth = new ClothMesh();
//    hangerCloth->initCloth(16, 24, 0.05f);                  // 0.8m wide × 1.2m tall — t-shirt proportion
//    hangerCloth->setMaterial(hangerMat);
//    hangerCloth->setCamera(cam);
//    hangerCloth->setProjectionMatrix(proj);
//    hangerCloth->setPositionWorld(0.8f, 1.2f, -2.5f);      // right side of scene, at eye level
//    // Pin only the center vertex of the top row — like a coat hanger hook
//    hangerCloth->getSimulation().pinVertex(8, 0);          // single-point hanger at top-center
//    // Noticeable wind that makes the fabric sway and flutter
//    hangerCloth->getSimulation().setWind(glm::vec3(3.5f, 0.5f, 1.0f));
//    hangerCloth->getSimulation().setGravity(glm::vec3(0.0f, -4.0f, 0.0f));
//    hangerCloth->getSimulation().setConstraintIterations(3);  // looser = more flutter
//    hangerCloth->getSimulation().setDamping(0.93f);           // less damping = motion persists
//    addObject(hangerCloth, hangerMat);

//    // ----- Simple PBR sphere -----
//    PBRMaterial* pbrMat = new PBRMaterial(false);
//    pbrMat->init();
//    pbrMat->setBaseColor(1.0f, 0.5f, 0.3f);
//    pbrMat->setMetallic(0.5f);
//    pbrMat->setRoughness(0.3f);
//    pbrMat->setLightPos(0, 0, 5.0f, 5.0f);
//    pbrMat->setLightColor(0, 10.0f, 10.0f, 10.0f);
//    pbrMat->setLightPos(1, -5.0f, 5.0f, 5.0f);
//    pbrMat->setLightColor(1, 10.0f, 10.0f, 10.0f);
//    pbrMat->setIBLFromScene(irr, pref, lut);
//    Sphere2* pbrSphere = new Sphere2();
//    pbrSphere->init();
//    pbrSphere->initGeometry(0.15f);
//    pbrSphere->setMaterial(pbrMat);
//    pbrSphere->setCamera(cam);
//    pbrSphere->setProjectionMatrix(proj);
//    pbrSphere->setPosition(-1.6f, -0.3f, -2.0f);
//    addObject(pbrSphere, pbrMat);
//
//    // ----- IBL PBR sphere with texture maps (rusted iron) -----
//    GLuint albedo    = PBRMaterial::uploadTextureFromFile(rl, "texture/pbr/rusted_iron1/albedo.jpg");
//    GLuint normal    = PBRMaterial::uploadTextureFromFile(rl, "texture/pbr/rusted_iron1/normal.jpg");
//    GLuint metallic  = PBRMaterial::uploadTextureFromFile(rl, "texture/pbr/rusted_iron1/metallic.jpg");
//    GLuint roughness = PBRMaterial::uploadTextureFromFile(rl, "texture/pbr/rusted_iron1/roughness.jpg");
//    GLuint ao        = PBRMaterial::uploadTextureFromFile(rl, "texture/pbr/rusted_iron1/ao.jpg");
//
//    IBLPBRMaterial* goldMat = new IBLPBRMaterial();
//    goldMat->init();
//    goldMat->setBaseColor(1.0f, 1.0f, 1.0f);
//    goldMat->setMetallic(1.0f);
//    goldMat->setRoughness(0.1f);
//    goldMat->setAO(1.0f);
//    goldMat->setAlbedoMap(albedo);
//    goldMat->setNormalMap(normal);
//    goldMat->setMetallicMap(metallic);
//    goldMat->setRoughnessMap(roughness);
//    goldMat->setAOMap(ao);
//
//    Sphere2* goldSphere = new Sphere2();
//    goldSphere->init();
//    goldSphere->initGeometry(0.15f);
//    goldSphere->setMaterial(goldMat);
//    goldSphere->setCamera(cam);
//    goldSphere->setProjectionMatrix(proj);
//    goldSphere->setPosition(-1.6f, -0.9f, -2.0f);
//    addObject(goldSphere, goldMat);

    // ----- Volumetric Light Occluders (window-blind slats) -----
    // 在场景上方放一组细条遮挡物，留出缝隙；阴光从 (5,10,5) 倾斜射下时，
    // 缝隙处会形成条纹状的体积光（god rays）。
//    {
//        UnlitMaterial* slatMat = new UnlitMaterial();
//        slatMat->init();
//        slatMat->setColor(0.05f, 0.05f, 0.06f, 1.0f);
//
//        // Cube2 是 1m 立方体，仅支持均匀缩放。
//        // 用多个小立方体沿 X 拼接成一根"条"，沿 Z 排列形成百叶窗。
//        // 扩大覆盖范围，让光柱遍布整个相机视野。
//        const float CUBE_SCALE = 0.30f;   // 每个 cube 边长
//        const float SLAT_Y     = 4.5f;    // 高度（车上方更高）
//        const float GAP_Z      = 0.55f;   // 片间距 (>CUBE_SCALE 即有缝)
//        const int   SLAT_COUNT = 25;      // 沿 z 的片数 (覆盖 ~13m)
//        const int   X_TILES    = 50;      // 沿 x 的拼接数 (覆盖 ~15m)
//        const float startZ     = -((SLAT_COUNT - 1) * GAP_Z) * 0.5f;
//        const float startX     = -((X_TILES    - 1) * CUBE_SCALE) * 0.5f;
//        for (int j = 0; j < SLAT_COUNT; ++j) {
//            float z = startZ + j * GAP_Z;
//            for (int i = 0; i < X_TILES; ++i) {
//                Cube2* c = new Cube2();
//                c->init();
//                c->setMaterial(slatMat);
//                c->setCamera(cam);
//                c->setProjectionMatrix(proj);
//                c->setScale(CUBE_SCALE);
//                float x = startX + i * CUBE_SCALE;
//                c->setPosition(x, SLAT_Y, z);
//                addObject(c, slatMat);
//            }
//        }
//    }

    // ----- Car FBX model -----
    loadCarModel();


//    loadCarModel1();

    // ----- Tween: spin cube on Y axis -----
//    TweenManager::instance().create(0.0f, 360.0f, 4.0f, Ease::Linear)
//        ->loop(true)
//        .onUpdate([this](float v) {
//            cubeRotY = v;
//            if (tweenedCube) tweenedCube->setRotationY(v);
//        })
//        .play();

    // ----- Tween: open/close front-left door (P1 hierarchy demo) -----
    if (doorFL) {
        TweenManager::instance().create(0.0f, 60.0f, 2.5f, Ease::OutQuad)
            ->loop(true).pingPong(true)
            .onUpdate([this](float v) {
                if (doorFL) doorFL->setRotationY(v);
            })
            .play();
    }

    // ----- Particle effects -----
//    fireEffect = new FireEffect();
//    fireEffect->init(400);

//    rainEffect = new RainEffect();
//    rainEffect->spawnWidth  = 10.0f;
//    rainEffect->spawnDepth  = 10.0f;
//    rainEffect->spawnHeight = 6.0f;
//    rainEffect->wind        = { 0.3f, 0.1f };
//    rainEffect->groundY     = -2.5f;
//    rainEffect->init(600);

//    snowEffect = new SnowEffect();
//    snowEffect->spawnWidth  = 10.0f;
//    snowEffect->spawnDepth  = 10.0f;
//    snowEffect->spawnHeight = 6.0f;
//    snowEffect->groundY     = -2.5f;
//    snowEffect->init(300);
//
//    cloudEffect = new CloudEffect();
//    cloudEffect->spawnWidth = 12.0f;
//    cloudEffect->spawnDepth = 12.0f;
//    cloudEffect->heightVar  = 0.6f;
//    cloudEffect->driftSpeed = 0.12f;
//    cloudEffect->init(400);

//    // ---- Jet engine effect on the rotating cube ----
//    jetEffect = new JetEffect();
//    jetEffect->spreadAngle = 0.08f;     // tight
//    jetEffect->minSpeed = 10.0f;
//    jetEffect->maxSpeed = 18.0f;
//    jetEffect->emitRate = 2000.0f;
//    jetEffect->init(800);

//    // ---- Ripple effect — ground points that float up with passing wave ----
//    rippleEffect = new RippleEffect();
//    rippleEffect->center = glm::vec3(0.0f, 0.0f, 0.0f);
//    rippleEffect->groundY = -0.54f;
//    rippleEffect->rippleSpeed = 3.0f;
//    rippleEffect->rippleInterval = 2.5f;
//    rippleEffect->maxHeight = 0.3f;
//    rippleEffect->pointSize = 1.0f;
//    rippleEffect->init(24, 24, 0.35f);

    // ---- CircleFlame effect — around the car ---
//    circleFlameEffect = new CircleFlameEffect();
//    circleFlameEffect->center = glm::vec3(0.0f, 0.0f, 0.0f);
//    circleFlameEffect->groundY = -0.72f;
//    circleFlameEffect->ringRadius = 2.6f;
//    circleFlameEffect->ringWidth = 0.5f;
//    circleFlameEffect->burnSpeed = 0.6f;
//    circleFlameEffect->init();

//    // ---- Programmatic motorcycle ----
//    motorcycle = new Motorcycle();
//    motorcycle->position = glm::vec3(2.0f, 0.3f, 1.5f);
//    motorcycle->heading = 0.8f;
//    motorcycle->scale = 0.8f;
//    motorcycle->wheelSpeed = 3.0f;
//    motorcycle->init();

    LOGI("MainScene::onInit done");
}

// ---------------------------------------------------------------------------
// onRelease — free all resources owned by MainScene
// ---------------------------------------------------------------------------
void MainScene::onRelease() {
    if (fireEffect)  { fireEffect->release();  delete fireEffect;  fireEffect  = nullptr; }
    if (rainEffect)  { rainEffect->release();  delete rainEffect;  rainEffect  = nullptr; }
    if (snowEffect)  { snowEffect->release();  delete snowEffect;  snowEffect  = nullptr; }
    if (cloudEffect) { cloudEffect->release(); delete cloudEffect; cloudEffect = nullptr; }
    if (jetEffect)   { jetEffect->release();   delete jetEffect;   jetEffect   = nullptr; }
    if (rippleEffect) { rippleEffect->release(); delete rippleEffect; rippleEffect = nullptr; }
    if (circleFlameEffect) { circleFlameEffect->release(); delete circleFlameEffect; circleFlameEffect = nullptr; }
    if (motorcycle) { motorcycle->release(); delete motorcycle; motorcycle = nullptr; }
    if (cowboyAnimator) { delete cowboyAnimator; cowboyAnimator = nullptr; }
    if (cowboyLoader)   { cowboyLoader->release(); delete cowboyLoader; cowboyLoader = nullptr; }
    tweenedCube = nullptr;   // owned by objects[], freed by base class
    if (debugProgram) { glDeleteProgram(debugProgram); debugProgram = 0; }
    if (debugQuadVAO) { glDeleteVertexArrays(1, &debugQuadVAO); debugQuadVAO = 0; }
    if (debugQuadVBO) { glDeleteBuffers(1, &debugQuadVBO); debugQuadVBO = 0; }
}

// ---------------------------------------------------------------------------
// onUpdate — per-frame logic
// ---------------------------------------------------------------------------
void MainScene::onUpdate(float dt) {
    if (fireEffect)  fireEffect->update(dt, firePos);
    if (rainEffect)  rainEffect->update(dt, rainPos);
    if (snowEffect)  snowEffect->update(dt, snowPos);
    if (cloudEffect) cloudEffect->update(dt, cloudPos);
    if (jetEffect) {
        // Rotate jet direction with cube
        glm::mat4 rot = glm::rotate(glm::mat4(1.0f), cubeRotY, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 rotatedDir = glm::vec3(rot * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
        jetEffect->jetDirection = rotatedDir;
        // Emit from cube tail edge (cube center + half size backward along rotated direction)
        float cubeHalf = 0.6f;
        glm::vec3 emitPos = jetPos + rotatedDir * cubeHalf;  // cube center - backward = tail
        jetEffect->update(dt, emitPos);
    }
    if (rippleEffect) rippleEffect->update(dt);
    if (circleFlameEffect) circleFlameEffect->update(dt);
    if (motorcycle) motorcycle->update(dt);

    if (cowboyAnimator) {
        cowboyAnimator->update(dt);
        cowboyAnimator->computeBoneMatrices();
    }
}

// ---------------------------------------------------------------------------
// onRenderExtra — transparent / particle pass after opaque geometry
// ---------------------------------------------------------------------------
void MainScene::onRenderExtra(const float* viewMat, const float* projMat) {
    if (fireEffect)  fireEffect->render(viewMat, projMat);
    if (rainEffect)  rainEffect->render(viewMat, projMat);
    if (snowEffect)  snowEffect->render(viewMat, projMat);
    if (cloudEffect) cloudEffect->render(viewMat, projMat);
    if (jetEffect)   jetEffect->render(viewMat, projMat);
    if (rippleEffect) rippleEffect->render(viewMat, projMat);
    if (circleFlameEffect) circleFlameEffect->render(viewMat, projMat);
    if (motorcycle) motorcycle->render(viewMat, projMat);
}

// ---------------------------------------------------------------------------
// onPostBlit — draw depth + velocity overlays on screen after blit
// ---------------------------------------------------------------------------
void MainScene::onPostBlit() {
    return;
    // Lazy init
    if (debugProgram == 0) {
        const char* vs = R"(#version 300 es
layout(location=0) in vec2 aPos;
out vec2 vUV;
void main() {
    vUV = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
})";
        const char* fs = R"(#version 300 es
precision highp float;
uniform sampler2D uTex;
uniform int uMode; // 0=depth, 1=velocity
in vec2 vUV;
out vec4 FragColor;
void main() {
    vec4 c = texture(uTex, vUV);
    if (uMode == 0) {
        float d = c.r;
        FragColor = vec4(d, d, d, 1.0);
    } else {
        // velocity: encode as color (red=X, green=Y)
        float vx = clamp(c.r * 10.0 + 0.5, 0.0, 1.0);
        float vy = clamp(c.g * 10.0 + 0.5, 0.0, 1.0);
        FragColor = vec4(vx, vy, 0.0, 1.0);
    }
})";
        Shader sh;
        if (sh.init(vs, fs)) {
            GLuint prog = sh.getProgram();
            sh.stealProgram(debugProgram);
            sh.release();
            debugLoc_tex  = glGetUniformLocation(debugProgram, "uTex");
            debugLoc_mode = glGetUniformLocation(debugProgram, "uMode");
        }
        // Fullscreen quad
        float verts[] = {-1,-1, 1,-1, -1,1, 1,1};
        glGenVertexArrays(1, &debugQuadVAO);
        glGenBuffers(1, &debugQuadVBO);
        glBindVertexArray(debugQuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, debugQuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    if (debugProgram == 0) return;

    int w = getWidth();
    int h = getHeight();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glUseProgram(debugProgram);

    glViewport(0, h - h/3, w/3, h/3);  // top-left: depth
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, getSceneDepthTex());
    glUniform1i(debugLoc_tex, 0);
    glUniform1i(debugLoc_mode, 0);
    glBindVertexArray(debugQuadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glViewport(w - w/3, h - h/3, w/3, h/3);  // top-right: velocity
    glBindTexture(GL_TEXTURE_2D, getSceneVelocityTex());
    glUniform1i(debugLoc_mode, 1);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0);
    glViewport(0, 0, w, h);
}

void MainScene::loadCarModel() {
    // 使用默认模型路径
    std::string carPath = getFilesDir() + "/models/BEV_ES500e/car_exterior.fbx";

    // [P3 diagnostic] Dump aiNode hierarchy before doing real load.
    // Decides whether reworking ModelLoader to preserve the tree is worthwhile.
    // Disabled now that P3/P4 are done — re-enable if you need to inspect a
    // new model file's pivot/transform layout. (Each call re-parses the FBX
    // twice and writes ~800 lines of logcat.)
    // ModelLoader::dumpAssimpHierarchy(carPath);

    ModelObject carLoader;

    ResourceLoader* rl = getResourceLoader();
    IBLPBRMaterial* mat_back_light_n = new IBLPBRMaterial();
    mat_back_light_n->init();
    mat_back_light_n->setAlbedoMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/BaseColor/back_light_n.jpg", false));
    mat_back_light_n->setMetallicMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Metallic/back_light_n.jpg", false));
    mat_back_light_n->setRoughnessMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Roughness/back_light_n.jpg", false));
    mat_back_light_n->setNormalMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Normal/back_light_n.jpg", false));
    mat_back_light_n->setAOMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Ao/back_light_n.jpg", false));
    IBLPBRMaterial* mat_door_handle_n = new IBLPBRMaterial();
    mat_door_handle_n->init();
    mat_door_handle_n->setAlbedoMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/BaseColor/door_handle_n.jpg", false));
    mat_door_handle_n->setMetallicMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Metallic/door_handle_n.jpg", false));
    mat_door_handle_n->setRoughnessMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Roughness/door_handle_n.jpg", false));
    mat_door_handle_n->setNormalMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Normal/door_handle_n.jpg", false));
    mat_door_handle_n->setAOMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Ao/door_handle_n.jpg", false));
    IBLPBRMaterial* mat_exterior_body_panels_n = new IBLPBRMaterial();
    mat_exterior_body_panels_n->init();
    mat_exterior_body_panels_n->setAlbedoMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/BaseColor/exterior_body_panels_n.jpg", false));
    mat_exterior_body_panels_n->setMetallicMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Metallic/exterior_body_panels_n.jpg", false));
    mat_exterior_body_panels_n->setRoughnessMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Roughness/exterior_body_panels_n.jpg", false));
    mat_exterior_body_panels_n->setNormalMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Normal/exterior_body_panels_n.jpg", false));
    mat_exterior_body_panels_n->setAOMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Ao/exterior_body_panels_n.jpg", false));
    IBLPBRMaterial* mat_exterior_doors_F_n = new IBLPBRMaterial();
    mat_exterior_doors_F_n->init();
    mat_exterior_doors_F_n->setAlbedoMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/BaseColor/exterior_doors_F_n.jpg", false));
    mat_exterior_doors_F_n->setMetallicMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Metallic/exterior_doors_F_n.jpg", false));
    mat_exterior_doors_F_n->setRoughnessMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Roughness/exterior_doors_F_n.jpg", false));
    mat_exterior_doors_F_n->setNormalMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Normal/exterior_doors_F_n.jpg", false));
    mat_exterior_doors_F_n->setAOMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Ao/exterior_doors_F_n.jpg", false));
    IBLPBRMaterial* mat_exterior_doors_R_n = new IBLPBRMaterial();
    mat_exterior_doors_R_n->init();
    mat_exterior_doors_R_n->setAlbedoMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/BaseColor/exterior_doors_R_n.jpg", false));
    mat_exterior_doors_R_n->setMetallicMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Metallic/exterior_doors_R_n.jpg", false));
    mat_exterior_doors_R_n->setRoughnessMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Roughness/exterior_doors_R_n.jpg", false));
    mat_exterior_doors_R_n->setNormalMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Normal/exterior_doors_R_n.jpg", false));
    mat_exterior_doors_R_n->setAOMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Ao/exterior_doors_R_n.jpg", false));
    IBLPBRMaterial* mat_exterior_trunk_n = new IBLPBRMaterial();
    mat_exterior_trunk_n->init();
    mat_exterior_trunk_n->setAlbedoMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/BaseColor/exterior_trunk_n.jpg", false));
    mat_exterior_trunk_n->setMetallicMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Metallic/exterior_trunk_n.jpg", false));
    mat_exterior_trunk_n->setRoughnessMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Roughness/exterior_trunk_n.jpg", false));
    mat_exterior_trunk_n->setNormalMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Normal/exterior_trunk_n.jpg", false));
    mat_exterior_trunk_n->setAOMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Ao/exterior_trunk_n.jpg", false));
    IBLPBRMaterial* mat_front_light_n = new IBLPBRMaterial();
    mat_front_light_n->init();
    mat_front_light_n->setAlbedoMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/BaseColor/front_light_n.jpg", false));
    mat_front_light_n->setMetallicMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Metallic/front_light_n.jpg", false));
    mat_front_light_n->setRoughnessMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Roughness/front_light_n.jpg", false));
    mat_front_light_n->setNormalMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Normal/front_light_n.jpg", false));
    mat_front_light_n->setAOMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Ao/front_light_n.jpg", false));
    IBLPBRMaterial* mat_interior_n = new IBLPBRMaterial();
    mat_interior_n->init();
    mat_interior_n->setBaseColor(0.1, 0.1, 0.1);
//    mat_interior_n->setAlbedoMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/BaseColor/interior_n.jpg", false));
    mat_interior_n->setMetallicMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Metallic/interior_n.jpg", false));
    mat_interior_n->setRoughnessMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Roughness/interior_n.jpg", false));
    mat_interior_n->setNormalMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Normal/interior_n.jpg", false));
    mat_interior_n->setAOMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Ao/interior_n.jpg", false));
    IBLPBRMaterial* mat_wheel_tire_n = new IBLPBRMaterial();
    mat_wheel_tire_n->init();
    mat_wheel_tire_n->setAlbedoMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/BaseColor/wheel_tire_n.jpg", false));
    mat_wheel_tire_n->setMetallicMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Metallic/wheel_tire_n.jpg", false));
    mat_wheel_tire_n->setRoughnessMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Roughness/wheel_tire_n.jpg", false));
    mat_wheel_tire_n->setNormalMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Normal/wheel_tire_n.jpg", false));
    mat_wheel_tire_n->setAOMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/Ao/wheel_tire_n.jpg", false));

    IBLPBRMaterial* mat_graound = new IBLPBRMaterial();
    mat_graound->init();
    mat_graound->setAlbedoMap(PBRMaterial::uploadTextureFromFile(rl, "models/BEV_ES500e/texture/ground-n.png", false));
    mat_graound->setBaseColor(0.1f, 0.1f, 0.1f);
    mat_graound->setMetallic(0.0f);
    mat_graound->setRoughness(0.0f);
    mat_graound->setAO(1.0f);

    UnlitMaterial* unlitMaterial = new UnlitMaterial();
    unlitMaterial->init();
    unlitMaterial->setColor(0.9f, 0.9f, 0.9f, 1.0f);

    IBLPBRMaterial* mat_glass = new IBLPBRMaterial();
    mat_glass->init();
    mat_glass->setBaseColor(0.1f, 0.1f, 0.1f);
    mat_glass->setMetallic(0.01f);
    mat_glass->setRoughness(0.01f);
    mat_glass->setAO(1.0f);
    mat_glass->setOpacity(0.9f);

    if (!carLoader.load(carPath, [&](MeshNode* node, int /*idx*/) {
        LOGI("loadCarModel %s ", node->getName().c_str());
        if (node->getName().compare("buttonCarKey") == 0 || node->getName().compare("buttonWheel") == 0
        || node->getName().compare("buttonDoor") == 0 || node->getName().compare("buttonLock") == 0
        || node->getName().compare("buttonTrunk") == 0 || node->getName().compare("buttonChargingPort") == 0
        || node->getName().compare("buttonVehicleHeight") == 0 || node->getName().compare("buttonHeadlamp") == 0
        || node->getName().compare("buttonDoorWindow") == 0 || node->getName().compare("buttonSkyWindow") == 0
        || node->getName().compare("light_a_l") == 0 || node->getName().compare("light_a_r") == 0 || node->getName().compare("light_b") == 0
        || node->getName().compare("light_h_l") == 0 || node->getName().compare("light_h_r") == 0
        || node->getName().compare("exterior_doors_RL_surface") == 0 || node->getName().compare("exterior_doors_FL_surface") == 0
        || node->getName().compare("exterior_doors_RR_surface") == 0 || node->getName().compare("exterior_doors_FR_surface") == 0) {
            LOGI("loadCarModel unlitMaterial");
            node->setMaterial(unlitMaterial);
            node->setVisible(false);
        } else if (node->getName().compare("exterior_doors_RR_other_2") == 0 || node->getName().compare("exterior_doors_RL_other_2") == 0
            || node->getName().compare("exterior_doorshandle_RR") == 0 || node->getName().compare("exterior_doorshandle_RR_other") == 0
            /*|| node->getName().compare("exterior_windows_RR") == 0 */|| node->getName().compare("exterior_doors_RR") == 0
            || node->getName().compare("exterior_doors_RR_other") == 0 || node->getName().compare("exterior_doorshandle_RL") == 0
            || node->getName().compare("exterior_doorshandle_RL_other") == 0 || node->getName().compare("exterior_doors_RL_other") == 0
            /*|| node->getName().compare("exterior_windows_RL") == 0 */|| node->getName().compare("exterior_doors_RL") == 0) {
            LOGI("loadCarModel mat_exterior_doors_R_n");
            node->setMaterial(mat_exterior_doors_R_n);
        } else if (node->getName().compare("exterior_doors_FR_other_2") == 0 || node->getName().compare("exterior_doors_FL_other_2") == 0
           || node->getName().compare("exterior_doors_FR") == 0 /*|| node->getName().compare("exterior_windows_FR") == 0*/
           || node->getName().compare("exterior_doors_FR_other") == 0 || node->getName().compare("exterior_mirror_FR") == 0
           || node->getName().compare("exterior_mirror_FR_other2") == 0 || node->getName().compare("exterior_mirror_FR_other") == 0
           /*|| node->getName().compare("exterior_windows_FR_other") == 0*/ /*|| node->getName().compare("exterior_windows_FL_other") == 0*/
           || node->getName().compare("exterior_doors_FL_other") == 0 /*|| node->getName().compare("exterior_windows_FL") == 0*/
           || node->getName().compare("exterior_mirror_FL") == 0 || node->getName().compare("exterior_mirror_FL_other2") == 0
           || node->getName().compare("exterior_mirror_FL_other") == 0 || node->getName().compare("exterior_doors_FL") == 0) {
            LOGI("loadCarModel mat_exterior_doors_F_n");
            node->setMaterial(mat_exterior_doors_F_n);
        } else if (node->getName().compare("exterior_doorshandle_FR_other") == 0 || node->getName().compare("exterior_doorshandle_FR") == 0
                   || node->getName().compare("exterior_doorshandle_FL") == 0 || node->getName().compare("exterior_doorshandle_FL_other") == 0) {
            LOGI("loadCarModel mat_door_handle_n");
            node->setMaterial(mat_door_handle_n);
        } else if (node->getName().compare("exterior_front_light_L") == 0 || node->getName().compare("exterior_front_light_R") == 0) {
            LOGI("loadCarModel mat_front_light_n");
            node->setMaterial(mat_front_light_n);
        } else if (node->getName().compare("exterior_back_light_L") == 0 || node->getName().compare("exterior_back_light_R") == 0) {
            LOGI("loadCarModel mat_back_light_n");
            node->setMaterial(mat_back_light_n);
        } else if (/*node->getName().compare("exterior_front_windshield") == 0 *//*|| node->getName().compare("exterior_roof_window") == 0*/
                   /*|| node->getName().compare("exterior_back_window") == 0 *//*||*/ node->getName().compare("exterior_body_edit") == 0
                   || node->getName().compare("exterior_body") == 0 || node->getName().compare("exterior_charging_port_R") == 0
                   || node->getName().compare("exterior_charging_port_L") == 0 /*|| node->getName().compare("exterior_window_other") == 0*/
                   || node->getName().compare("exterior_front_windshield_wiper_R") == 0 || node->getName().compare("exterior_front_windshield_wiper_L") == 0) {
            LOGI("loadCarModel mat_exterior_body_panels_n");
            node->setMaterial(mat_exterior_body_panels_n);
        } else if (node->getName().compare("exterior_trunk") == 0 || node->getName().compare("exterior_trunk_other") == 0
                   || node->getName().compare("exterior_trunk_light_L") == 0 || node->getName().compare("exterior_trunk_light_R") == 0
                   || node->getName().compare("exterior_trunk_light_other") == 0) {
            LOGI("loadCarModel mat_exterior_trunk_n");
            node->setMaterial(mat_exterior_trunk_n);
        } else if (node->getName().compare("interior_body") == 0 || node->getName().compare("interior_body_B") == 0
                   || node->getName().compare("interior_seat_d") == 0 || node->getName().compare("interior_seat_p") == 0
                   || node->getName().compare("interior_roof") == 0) {
            LOGI("loadCarModel mat_interior_n");
            node->setMaterial(mat_interior_n);
        } else if (node->getName().compare("exterior_wheel_tire_FR") == 0 || node->getName().compare("exterior_wheel_tire_RR") == 0
                   || node->getName().compare("exterior_wheel_tire_FL") == 0 || node->getName().compare("exterior_wheel_tire_RL") == 0) {
            LOGI("loadCarModel mat_wheel_tire_n");
            node->setMaterial(mat_wheel_tire_n);
        } else if (node->getName().compare("ground") == 0) {
            LOGI("loadCarModel mat_graound");
            node->setMaterial(mat_graound);
            node->setVisible(false);
        } else if (node->getName().compare("exterior_windows_RL") == 0 || node->getName().compare("exterior_windows_RR") == 0
                   || node->getName().compare("exterior_windows_FR") == 0 || node->getName().compare("exterior_windows_FR_other") == 0
                   || node->getName().compare("exterior_windows_FL") == 0 || node->getName().compare("exterior_windows_FL_other") == 0
                 || node->getName().compare("exterior_roof_window") == 0 || node->getName().compare("exterior_back_window") == 0
                 || node->getName().compare("exterior_window_other") == 0 || node->getName().compare("exterior_front_windshield") == 0) {
            LOGI("loadCarModel mat_glass");
            node->setMaterial(mat_glass);
        } else {
            LOGI("loadCarModel other");
            node->setMaterial(unlitMaterial);
            node->setVisible(false);
        }
        node->setPosition(0.0f, 0.0f, 0.0f);
        node->setScale(1.0f);
    })) {
        LOGI("MainScene: failed to load car model");
    } else {
        LOGI("MainScene: car model loaded, meshes=%d", carLoader.meshCount());
        for (MeshNode* node : carLoader.getMeshNodes()) {
            addObject(node, nullptr);
        }
        // Scene now owns the MeshNodes; prevent double-delete
        carLoader.releaseOwnership();

        // -------------------------------------------------------------------
        // P3 hierarchy: ModelLoader now preserves the aiNode tree, so
        //   • carLoader.getRoot() is the FBX scene root
        //   • each mesh-owning aiNode → SceneNode whose pivot/transform
        //     matches the source FBX (wheels, ground = real pivots).
        //   • body/doors have identity local transforms in the FBX (verts
        //     baked in world space) — we still need an explicit createGroup
        //     to give them a hinge.
        //
        // We apply unit conversion (mm → m) and offset on the model's root
        // SceneNode bind matrix so MeshNodes don't need per-mesh setScale().
        // -------------------------------------------------------------------
        SceneNode* root = carLoader.getRoot();

        // root bind = T(0,-0.5,0) * S(0.001)  in column-major
        const float S = 0.001f;
        const float TX = 0.0f, TY = -0.5f, TZ = 0.0f;
        float rootBind[16] = {
            S,  0,  0,  0,
            0,  S,  0,  0,
            0,  0,  S,  0,
            TX, TY, TZ, 1
        };
        root->setBindMatrix(rootBind);

        // Door FL group: pivot in METERS, in root-local space (because the
        // group sits as a direct child of root; its bind=identity from FBX
        // means group's local space = root-local space, which is METERS
        // after our scale=0.001 root bind).
        // Approximate hinge: front edge of FL door, mid-height, slightly
        // offset to driver side. (-0.7m, +0.5m, +0.6m) for a ~4.5m sedan.
//        auto isDoorFL = [](MeshNode* m) -> bool {
//            const std::string& n = m->getName();
//            return n == "exterior_doors_FL"
//                || n == "exterior_doors_FL_other"
//                || n == "exterior_doors_FL_other_2"
//                || n == "exterior_doorshandle_FL"
//                || n == "exterior_doorshandle_FL_other"
//                || n == "exterior_mirror_FL";
//        };
//        SceneNode* group = carLoader.createGroup(root, "Door_FL", isDoorFL,
//                                                 -0.7f, 0.5f, 0.6f);

        SceneNode* hierarchyRoot = carLoader.releaseHierarchy();
        addSceneNodeRoot(hierarchyRoot);

        // After releaseHierarchy(), getRoot()/findByName() still work — the
        // ModelObject keeps the cached pointer, it just no longer owns it.
        // NB: the FBX leaf nodes are exterior_doors_FL / _FR / _RL / _RR;
        // there is no node literally named "exterior_doors_F".
//        SceneNode* doorRoot = hierarchyRoot ? hierarchyRoot->findByName("exterior_doors_F") : nullptr;
//        if (doorRoot) {
//            doorFL = doorRoot;
//        } else {
//            LOGI("MainScene: WARNING exterior_doors_FL not found in hierarchy");
//        }
//        LOGI("MainScene: hierarchy installed, doorFL=%p", (void*)doorFL);
    }
}

void MainScene::loadCarModel1() {
    std::string carPath = getFilesDir() + "/models/cowboy/cowboy.dae";

    // Heap-allocate the loader so it lives as long as the Animator references
    // its bones / animations / NodeDesc tree.
    cowboyLoader = new ModelLoader();
    if (!cowboyLoader->load(carPath)) {
        LOGE("MainScene: failed to load cowboy model: %s", carPath.c_str());
        delete cowboyLoader; cowboyLoader = nullptr;
        return;
    }
    LOGI("MainScene: cowboy loaded  meshes=%zu  bones=%zu  anims=%zu",
         cowboyLoader->getMeshes().size(),
         cowboyLoader->getBones().size(),
         cowboyLoader->getAnimations().size());

    // Animator drives bone palette every frame.
    cowboyAnimator = new Animator();
    cowboyAnimator->setModel(cowboyLoader);
    cowboyAnimator->play();

    // Diffuse texture (cowboy.png in assets/models/cowboy/).
    ResourceLoader* rl = getResourceLoader();
    GLuint cowboyTex = PBRMaterial::uploadTextureFromFile(
        rl, "models/cowboy/cowboy.png", false);

    // Take meshes out of the loader. The loader retains bones / animations
    // / NodeDesc so the Animator can keep sampling.
    std::vector<Mesh> meshes = cowboyLoader->takeMeshes();

    // Cowboy DAE is in cm-ish units, often facing -Z. Position on the floor
    // beside the car.
    const float SCALE = 0.15f;
    const float TX = 3.0f, TY = -0.5f, TZ = 0.0f;
    for (size_t i = 0; i < meshes.size(); ++i) {
//        // BBox
//        float mn[3] = { 1e30f, 1e30f, 1e30f };
//        float mx[3] = {-1e30f,-1e30f,-1e30f };
//        for (const auto& v : meshes[i].vertices) {
//            for (int k=0;k<3;k++) {
//                if (v.position[k] < mn[k]) mn[k] = v.position[k];
//                if (v.position[k] > mx[k]) mx[k] = v.position[k];
//            }
//        }
//        LOGI("Cowboy mesh[%zu] bbox min=(%.3f,%.3f,%.3f) max=(%.3f,%.3f,%.3f)",
//             i, mn[0],mn[1],mn[2], mx[0],mx[1],mx[2]);
        std::string name = meshes[i].name;
        bool hasBones = !meshes[i].boneData.empty();

        // Use IBLPBRMaterial with FLAG_SKINNED — full PBR + IBL path with
        // GPU skinning (works on any material that opts in via setSkinned).
        UnlitMaterial* litMat = new UnlitMaterial();
        litMat->setBaseColorMap(cowboyTex);
        if (hasBones) {
            litMat->setSkinned(true);
            litMat->setAnimator(cowboyAnimator);
        }
        litMat->init();

        MeshNode* node = new MeshNode(std::move(meshes[i]), name);
        node->setMaterial(litMat);
        node->setPosition(TX, TY, TZ);
        node->setRotation(-90.0f, 0.0f, 0.0f); // Z-up DAE -> Y-up, face camera
        node->setScale(SCALE);
        addObject(node, nullptr);
        LOGI("Cowboy mesh[%zu] '%s' verts=%d bones=%d", i, name.c_str(),
             node->getVertexCount(), hasBones ? 1 : 0);
    }
}
