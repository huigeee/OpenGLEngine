#include "../include/EGLCore.h"
#include "../MainScene.h"
#include <android/log.h>
#include <unistd.h>
#include <cstring>
#include <time.h>
#include "../include/Common.h"

EGLCore::EGLCore() : eglDisplay(EGL_NO_DISPLAY), eglContext(EGL_NO_CONTEXT),
            eglSurface(EGL_NO_SURFACE), eglConfig(nullptr), nativeWindow(nullptr),
            scene(nullptr), renderThread(0), running(false), paused(false), initSuccess(false),
            pendingSurface(nullptr), javaVM(nullptr), threadEnv(nullptr),
            pendingDeltaX(0.0f), pendingDeltaY(0.0f),
            mutex(PTHREAD_MUTEX_INITIALIZER), cond(PTHREAD_COND_INITIALIZER),
            currentFPS(0.0f), frameCount(0), lastFpsUpdateTime(0.0) {
}

EGLCore::~EGLCore() {
    release();
}

void EGLCore::setJavaVM(JavaVM* vm) {
    javaVM = vm;
}

void EGLCore::setFilesDir(const char* path) {
    filesDir = path ? path : "";
}

const char* EGLCore::getFilesDir() const {
    return filesDir.c_str();
}

void EGLCore::setRotation(float deltaX, float deltaY) {
    pthread_mutex_lock(&mutex);
    pendingDeltaX += deltaX;
    pendingDeltaY += deltaY;
    pthread_mutex_unlock(&mutex);
}

bool EGLCore::init(JNIEnv* env, jobject surface) {
    pendingSurface = env->NewGlobalRef(surface);
    LOGI("Surface saved, will init in render thread");
    return true;
}

void EGLCore::requestReConnectSurface(JNIEnv* env, jobject surface) {
    pthread_mutex_lock(&mutex);
    
    // 删除旧的引用
    if (surfaceReconnectRequest.surface != nullptr) {
        threadEnv->DeleteGlobalRef(surfaceReconnectRequest.surface);
    }
    
    // 创建新引用并设置请求
    surfaceReconnectRequest.surface = env->NewGlobalRef(surface);
    surfaceReconnectRequest.pending = true;
    
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    
    LOGI("Surface reconnection requested");
}

void EGLCore::applySurfaceReconnect() {
    if (!surfaceReconnectRequest.pending || surfaceReconnectRequest.surface == nullptr) {
        return;
    }
    
    JNIEnv* env = getJNIEnv();
    if (env == nullptr) {
        LOGE("Failed to get JNIEnv in applySurfaceReconnect");
        surfaceReconnectRequest.pending = false;
        return;
    }
    
    // 删除旧的 Surface
    if (eglSurface != EGL_NO_SURFACE) {
        eglDestroySurface(eglDisplay, eglSurface);
        eglSurface = EGL_NO_SURFACE;
    }
    
    // 释放旧的 NativeWindow
    if (nativeWindow != nullptr) {
        ANativeWindow_release(nativeWindow);
        nativeWindow = nullptr;
    }
    
    // 从新 Surface 创建 NativeWindow
    nativeWindow = ANativeWindow_fromSurface(env, surfaceReconnectRequest.surface);
    if (!nativeWindow) {
        LOGE("Failed to get native window from new surface");
        surfaceReconnectRequest.pending = false;
        if (surfaceReconnectRequest.surface) {
            env->DeleteGlobalRef(surfaceReconnectRequest.surface);
            surfaceReconnectRequest.surface = nullptr;
        }
        return;
    }
    LOGI("Got new native window: %p", (void*)nativeWindow);
    
    // 创建新的 EGLSurface
    eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, nativeWindow, nullptr);
    if (eglSurface == EGL_NO_SURFACE) {
        LOGE("Failed to create new EGL surface");
        surfaceReconnectRequest.pending = false;
        if (surfaceReconnectRequest.surface) {
            env->DeleteGlobalRef(surfaceReconnectRequest.surface);
            surfaceReconnectRequest.surface = nullptr;
        }
        return;
    }
    LOGI("Created new EGL surface");
    
    // 重新绑定 Context
    if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext)) {
        LOGE("Failed to make EGL context current after reconnect");
        surfaceReconnectRequest.pending = false;
        if (surfaceReconnectRequest.surface) {
            env->DeleteGlobalRef(surfaceReconnectRequest.surface);
            surfaceReconnectRequest.surface = nullptr;
        }
        return;
    }
    
    // 更新 Viewport
    int width = ANativeWindow_getWidth(nativeWindow);
    int height = ANativeWindow_getHeight(nativeWindow);
    if (scene != nullptr) {
        scene->setViewport(width, height);
    }
    
    // 清理请求
    if (surfaceReconnectRequest.surface) {
        env->DeleteGlobalRef(surfaceReconnectRequest.surface);
        surfaceReconnectRequest.surface = nullptr;
    }
    surfaceReconnectRequest.pending = false;
    
    LOGI("Surface reconnected: %dx%d", width, height);
}

void EGLCore::start() {
    if (renderThread != 0) {
        LOGI("Render thread already running");
        return;
    }
    running = true;
    LOGI("Creating render thread...");
    pthread_create(&renderThread, nullptr, renderLoop, this);
    LOGI("Render thread created");
}

void EGLCore::stop() {
    if (renderThread == 0) {
        return;
    }
    running = false;
    pthread_cond_signal(&cond);
    pthread_join(renderThread, nullptr);
    renderThread = 0;
    LOGI("Render thread stopped");
}

void EGLCore::pause() {
    // 设置暂停标志，让线程进入空循环，不销毁线程
    pthread_mutex_lock(&mutex);
    paused = true;
    pthread_mutex_unlock(&mutex);
    LOGI("Render thread pause requested");
}

void EGLCore::resume() {
    // 清除暂停标志，唤醒线程继续渲染
    pthread_mutex_lock(&mutex);
    paused = false;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    LOGI("Render thread resume requested");
}

void EGLCore::release() {
    stop();

    if (scene != nullptr) {
        scene->release();
        delete scene;
        scene = nullptr;
    }

    if (eglDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (eglSurface != EGL_NO_SURFACE) {
            eglDestroySurface(eglDisplay, eglSurface);
            eglSurface = EGL_NO_SURFACE;
        }
        if (eglContext != EGL_NO_CONTEXT) {
            eglDestroyContext(eglDisplay, eglContext);
            eglContext = EGL_NO_CONTEXT;
        }
        eglTerminate(eglDisplay);
        eglDisplay = EGL_NO_DISPLAY;
    }

    if (nativeWindow != nullptr) {
        ANativeWindow_release(nativeWindow);
        nativeWindow = nullptr;
    }

    if (pendingSurface != nullptr) {
        if (threadEnv) {
            threadEnv->DeleteGlobalRef(pendingSurface);
        }
        pendingSurface = nullptr;
    }
    
    // 清理参数队列
    {
        pthread_mutex_lock(&mutex);
        while (!paramQueue.empty()) {
            paramQueue.pop();
        }
        pthread_mutex_unlock(&mutex);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    LOGI("EGL released");
}

bool EGLCore::isValid() const {
    return initSuccess;
}

bool EGLCore::isPaused() const {
    return paused;
}

// -------------------------------------------------------------------------
// Inspector 参数批量设置（线程安全，推入队列）
// -------------------------------------------------------------------------
void EGLCore::updateCameraParams(float posX, float posY, float posZ,
                                float targetX, float targetY, float targetZ,
                                float fov, bool orthographic) {
    pthread_mutex_lock(&mutex);
    ParamUpdateRequest req;
    req.hasCameraParams = true;
    req.camPosX = posX;
    req.camPosY = posY;
    req.camPosZ = posZ;
    req.camTargetX = targetX;
    req.camTargetY = targetY;
    req.camTargetZ = targetZ;
    req.camFov = fov;
    req.camOrthographic = orthographic;
    paramQueue.push(req);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

void EGLCore::updateSceneConfig(float bgR, float bgG, float bgB,
                               float ambient, bool shadow, bool reflection, bool ssao,
                               float lightPosX, float lightPosY, float lightPosZ,
                               float shadowBias, float shadowSoftness, bool shadowPCF) {
    LOGI("EGLCore::updateSceneConfig - shadow=%d bias=%.4f softness=%.2f pcf=%d", 
         shadow, shadowBias, shadowSoftness, shadowPCF);
    pthread_mutex_lock(&mutex);
    ParamUpdateRequest req;
    req.hasSceneConfig = true;
    req.sceneBgR = bgR;
    req.sceneBgG = bgG;
    req.sceneBgB = bgB;
    req.sceneAmbient = ambient;
    req.sceneShadow = shadow;
    req.sceneReflection = reflection;
    req.sceneSsao = ssao;
    req.sceneLightPosX = lightPosX;
    req.sceneLightPosY = lightPosY;
    req.sceneLightPosZ = lightPosZ;
    req.sceneShadowBias = shadowBias;
    req.sceneShadowSoft = shadowSoftness;
    req.sceneShadowPCF = shadowPCF;
    paramQueue.push(req);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

void EGLCore::updatePostProcessParams(jint aaMode, jboolean bloom, jfloat bloomIntensity,
                                      jboolean tonemap, jint tonemapOp, jfloat exposure,
                                      jboolean fog, jint fogMode, jfloat fogDensity,
                                      jfloat fogStart, jfloat fogEnd, jfloat fogR, jfloat fogG, jfloat fogB,
                                      jboolean dof, jfloat dofFocusDist, jfloat dofNear, jfloat dofFar,
                                      jboolean volumetric, jfloat volDensity, jfloat volScattering,
                                      jint volSteps, jfloat volMaxDistance, jfloat volIntensity,
                                      jfloat volColorR, jfloat volColorG, jfloat volColorB) {
    pthread_mutex_lock(&mutex);
    ParamUpdateRequest req;
    req.hasPostProcessParams = true;
    req.ppAaMode = aaMode;
    req.ppBloom = bloom;
    req.ppBloomIntensity = bloomIntensity;
    req.ppTonemap = tonemap;
    req.ppTonemapOp = tonemapOp;
    req.ppExposure = exposure;
    req.ppFog = fog;
    req.ppFogMode = fogMode;
    req.ppFogDensity = fogDensity;
    req.ppFogStart = fogStart;
    req.ppFogEnd = fogEnd;
    req.ppFogR = fogR;
    req.ppFogG = fogG;
    req.ppFogB = fogB;
    req.ppDof = dof;
    req.ppDofFocusDist = dofFocusDist;
    req.ppDofNear = dofNear;
    req.ppDofFar = dofFar;
    req.ppVolumetric = volumetric;
    req.ppVolDensity = volDensity;
    req.ppVolScattering = volScattering;
    req.ppVolSteps = volSteps;
    req.ppVolMaxDistance = volMaxDistance;
    req.ppVolIntensity = volIntensity;
    req.ppVolColorR = volColorR;
    req.ppVolColorG = volColorG;
    req.ppVolColorB = volColorB;
    paramQueue.push(req);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

void EGLCore::updateMaterialParams(float baseR, float baseG, float baseB,
                                  float metallic, float roughness, float ao) {
    pthread_mutex_lock(&mutex);
    ParamUpdateRequest req;
    req.hasMaterialParams = true;
    req.matBaseR = baseR;
    req.matBaseG = baseG;
    req.matBaseB = baseB;
    req.matMetallic = metallic;
    req.matRoughness = roughness;
    req.matAo = ao;
    paramQueue.push(req);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

void EGLCore::setViewportAsync(int width, int height) {
    pthread_mutex_lock(&mutex);
    viewportRequest.width = width;
    viewportRequest.height = height;
    viewportRequest.pending = true;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    LOGI("Viewport request queued: %dx%d", width, height);
}

void EGLCore::setViewport(int width, int height) {
    if (scene != nullptr) {
        scene->setViewport(width, height);
        LOGI("EGLCore setViewport (render thread): %dx%d", width, height);
    }
}

void EGLCore::applyViewport() {
    if (!viewportRequest.pending || scene == nullptr) {
        return;
    }
    
    scene->setViewport(viewportRequest.width, viewportRequest.height);
    viewportRequest.pending = false;
    LOGI("Viewport applied (render thread): %dx%d", viewportRequest.width, viewportRequest.height);
}

// 在渲染线程中应用参数更新
void EGLCore::applyParamUpdates() {
    if (paramQueue.empty() || scene == nullptr) return;
    
    // 合并所有队列中的请求（只保留每个参数的最后一次更新）
    ParamUpdateRequest merged;
    while (!paramQueue.empty()) {
        ParamUpdateRequest req = paramQueue.front();
        paramQueue.pop();
        
        if (req.hasCameraParams) {
            merged.hasCameraParams = true;
            merged.camPosX = req.camPosX;
            merged.camPosY = req.camPosY;
            merged.camPosZ = req.camPosZ;
            merged.camTargetX = req.camTargetX;
            merged.camTargetY = req.camTargetY;
            merged.camTargetZ = req.camTargetZ;
            merged.camFov = req.camFov;
            merged.camOrthographic = req.camOrthographic;
        }
        if (req.hasSceneConfig) {
            merged.hasSceneConfig = true;
            merged.sceneBgR = req.sceneBgR;
            merged.sceneBgG = req.sceneBgG;
            merged.sceneBgB = req.sceneBgB;
            merged.sceneAmbient = req.sceneAmbient;
            merged.sceneShadow = req.sceneShadow;
            merged.sceneReflection = req.sceneReflection;
            merged.sceneSsao = req.sceneSsao;
            merged.sceneLightPosX = req.sceneLightPosX;
            merged.sceneLightPosY = req.sceneLightPosY;
            merged.sceneLightPosZ = req.sceneLightPosZ;
            merged.sceneShadowBias = req.sceneShadowBias;
            merged.sceneShadowSoft = req.sceneShadowSoft;
            merged.sceneShadowPCF = req.sceneShadowPCF;
        }
        if (req.hasPostProcessParams) {
            merged.hasPostProcessParams = true;
            merged.ppAaMode = req.ppAaMode;
            merged.ppBloom = req.ppBloom;
            merged.ppBloomIntensity = req.ppBloomIntensity;
            merged.ppTonemap = req.ppTonemap;
            merged.ppTonemapOp = req.ppTonemapOp;
            merged.ppExposure = req.ppExposure;
            // Fog
            merged.ppFog = req.ppFog;
            merged.ppFogMode = req.ppFogMode;
            merged.ppFogDensity = req.ppFogDensity;
            merged.ppFogStart = req.ppFogStart;
            merged.ppFogEnd = req.ppFogEnd;
            merged.ppFogR = req.ppFogR;
            merged.ppFogG = req.ppFogG;
            merged.ppFogB = req.ppFogB;
            // DoF
            merged.ppDof = req.ppDof;
            merged.ppDofFocusDist = req.ppDofFocusDist;
            merged.ppDofNear = req.ppDofNear;
            merged.ppDofFar = req.ppDofFar;
            // Volumetric
            merged.ppVolumetric = req.ppVolumetric;
            merged.ppVolDensity = req.ppVolDensity;
            merged.ppVolScattering = req.ppVolScattering;
            merged.ppVolSteps = req.ppVolSteps;
            merged.ppVolMaxDistance = req.ppVolMaxDistance;
            merged.ppVolIntensity = req.ppVolIntensity;
            merged.ppVolColorR = req.ppVolColorR;
            merged.ppVolColorG = req.ppVolColorG;
            merged.ppVolColorB = req.ppVolColorB;
        }
        if (req.hasMaterialParams) {
            merged.hasMaterialParams = true;
            merged.matBaseR = req.matBaseR;
            merged.matBaseG = req.matBaseG;
            merged.matBaseB = req.matBaseB;
            merged.matMetallic = req.matMetallic;
            merged.matRoughness = req.matRoughness;
            merged.matAo = req.matAo;
        }
    }
    
    // 应用合并后的参数
    if (merged.hasCameraParams) {
        scene->updateCameraParams(merged.camPosX, merged.camPosY, merged.camPosZ,
                                 merged.camTargetX, merged.camTargetY, merged.camTargetZ,
                                 merged.camFov, merged.camOrthographic);
    }
    if (merged.hasSceneConfig) {
        // Update scene config (background, ambient, shadow switch, ssao)
        scene->updateSceneConfig(merged.sceneBgR, merged.sceneBgG, merged.sceneBgB,
                                merged.sceneAmbient, merged.sceneShadow, merged.sceneSsao);
        
        // Update light position if provided
        if (merged.sceneLightPosX != 0 || merged.sceneLightPosY != 0 || merged.sceneLightPosZ != 0) {
            scene->config.lightPosX = merged.sceneLightPosX;
            scene->config.lightPosY = merged.sceneLightPosY;
            scene->config.lightPosZ = merged.sceneLightPosZ;
            // Reinitialize shadow effect with new light position if enabled
            ShadowEffect* shadowEff = scene->getShadowEffect();
            if (merged.sceneShadow && shadowEff) {
                shadowEff->setLightPosition(merged.sceneLightPosX, merged.sceneLightPosY, merged.sceneLightPosZ);
            }
        }
        
        // Update shadow bias and softness in scene config
        scene->config.shadowBias = merged.sceneShadowBias;
        scene->config.shadowSoftness = merged.sceneShadowSoft;
        scene->config.shadowPCFEnabled = merged.sceneShadowPCF;
        
        // Apply to shadow effect if exists
        ShadowEffect* shadowEff = scene->getShadowEffect();
        if (merged.sceneShadow && shadowEff) {
            shadowEff->setShadowBias(merged.sceneShadowBias);
            shadowEff->setShadowSoftness(merged.sceneShadowSoft);
        }
    }
    if (merged.hasPostProcessParams) {
        scene->updatePostProcessParams((int)merged.ppAaMode, (bool)merged.ppBloom, (float)merged.ppBloomIntensity,
                                       (bool)merged.ppTonemap, (int)merged.ppTonemapOp, (float)merged.ppExposure,
                                       (bool)merged.ppFog, (int)merged.ppFogMode, (float)merged.ppFogDensity,
                                       (float)merged.ppFogStart, (float)merged.ppFogEnd, (float)merged.ppFogR, (float)merged.ppFogG, (float)merged.ppFogB,
                                       (bool)merged.ppDof, (float)merged.ppDofFocusDist, (float)merged.ppDofNear, (float)merged.ppDofFar,
                                       (bool)merged.ppVolumetric, (float)merged.ppVolDensity, (float)merged.ppVolScattering,
                                       (int)merged.ppVolSteps, (float)merged.ppVolMaxDistance, (float)merged.ppVolIntensity,
                                       (float)merged.ppVolColorR, (float)merged.ppVolColorG, (float)merged.ppVolColorB);
    }
    if (merged.hasMaterialParams) {
        scene->updateMaterialParams(merged.matBaseR, merged.matBaseG, merged.matBaseB,
                                   merged.matMetallic, merged.matRoughness, merged.matAo);
    }
}

void* EGLCore::renderLoop(void* arg) {
    EGLCore* core = static_cast<EGLCore*>(arg);
    core->renderLoopInternal();
    return nullptr;
}

JNIEnv* EGLCore::getJNIEnv() {
    JNIEnv* env;
    if (javaVM->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        return nullptr;
    }
    return env;
}

void EGLCore::renderLoopInternal() {
    LOGI("Render thread: attaching to JVM");
    JavaVMAttachArgs args = {JNI_VERSION_1_6, "RenderThread", nullptr};
    javaVM->AttachCurrentThread(&threadEnv, &args);
    LOGI("Render thread: attached to JVM");

    LOGI("Render thread: calling initEGL");
    initSuccess = initEGL(threadEnv);
    LOGI("Render thread: initEGL returned %d", initSuccess);

    pthread_mutex_lock(&mutex);
    while (running) {
        // 获取帧开始时间
        struct timespec frameStart;
        clock_gettime(CLOCK_MONOTONIC, &frameStart);
        double frameStartTimeMs = frameStart.tv_sec * 1000.0 + frameStart.tv_nsec / 1e6;
        
        // 检查暂停状态
        if (paused) {
            pthread_mutex_unlock(&mutex);
            usleep(16667);  // 空循环，等待恢复
            pthread_mutex_lock(&mutex);
            continue;
        }
        
        pthread_mutex_unlock(&mutex);

        if (initSuccess && scene != nullptr) {
            // 应用 Surface 重新链接请求（在渲染线程中安全执行）
            applySurfaceReconnect();
            
            // 应用 Viewport 设置（在渲染线程中安全执行）
            applyViewport();
            
            // 应用参数更新（在渲染线程中安全执行）
            applyParamUpdates();
            
            pthread_mutex_lock(&mutex);
            float dx = pendingDeltaX;
            float dy = pendingDeltaY;
            pendingDeltaX = 0.0f;
            pendingDeltaY = 0.0f;
            pthread_mutex_unlock(&mutex);
            if (dx != 0.0f || dy != 0.0f) {
                scene->setRotation(dx, dy);
            }
            scene->render();
            eglSwapBuffers(eglDisplay, eglSurface);
            
            // FPS 统计
            frameCount++;
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            double currentTime = ts.tv_sec + ts.tv_nsec / 1e9;
            if (currentTime - lastFpsUpdateTime >= 0.5) {
                currentFPS = frameCount / (currentTime - lastFpsUpdateTime);
                frameCount = 0;
                lastFpsUpdateTime = currentTime;
            }
        } else {
            LOGI("Render: initSuccess=%d, scene=%p", initSuccess, (void*)scene);
        }

        // 动态计算 sleep 时间，保证 60 FPS（每帧 16.67ms）
        struct timespec frameEnd;
        clock_gettime(CLOCK_MONOTONIC, &frameEnd);
        double frameEndTimeMs = frameEnd.tv_sec * 1000.0 + frameEnd.tv_nsec / 1e6;
        double frameTimeMs = frameEndTimeMs - frameStartTimeMs;
        double sleepTimeMs = 16.67 - frameTimeMs;
        
        if (sleepTimeMs > 0) {
            usleep((useconds_t)(sleepTimeMs * 1000));
        }
        // 如果 frameTimeMs >= 16.67，说明渲染太慢，不 sleep 直接进入下一帧

        pthread_mutex_lock(&mutex);
    }
    pthread_mutex_unlock(&mutex);

    javaVM->DetachCurrentThread();
}

bool EGLCore::initEGL(JNIEnv* env) {
    LOGI("Getting native window from surface");
    nativeWindow = ANativeWindow_fromSurface(env, pendingSurface);
    if (!nativeWindow) {
        LOGE("Failed to get native window from surface");
        return false;
    }
    LOGI("Got native window: %p", (void*)nativeWindow);

    LOGI("Getting EGL display");
    eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (eglDisplay == EGL_NO_DISPLAY) {
        LOGE("Failed to get EGL display");
        return false;
    }
    LOGI("Got EGL display");

    EGLint majorVersion, minorVersion;
    LOGI("Initializing EGL");
    if (!eglInitialize(eglDisplay, &majorVersion, &minorVersion)) {
        LOGE("Failed to initialize EGL");
        return false;
    }
    LOGI("EGL version: %d.%d", majorVersion, minorVersion);

    EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_NONE
    };

    EGLint numConfigs;
    LOGI("Choosing EGL config");
    if (!eglChooseConfig(eglDisplay, configAttribs, &eglConfig, 1, &numConfigs)) {
        LOGE("Failed to choose EGL config");
        return false;
    }
    LOGI("Chose EGL config");

    EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    LOGI("Creating EGL context");
    eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttribs);
    if (eglContext == EGL_NO_CONTEXT) {
        LOGE("Failed to create EGL context");
        return false;
    }
    LOGI("Created EGL context");

    LOGI("Creating EGL surface");
    eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, nativeWindow, nullptr);
    if (eglSurface == EGL_NO_SURFACE) {
        LOGE("Failed to create EGL surface");
        return false;
    }
    LOGI("Created EGL surface");

    LOGI("Making EGL context current");
    if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext)) {
        LOGE("Failed to make EGL context current");
        return false;
    }
    LOGI("Made EGL context current");

    LOGI("Creating scene");
    scene = new MainScene();
    scene->setFilesDir(filesDir.c_str());
    scene->init();
    LOGI("Scene created");

    int width = ANativeWindow_getWidth(nativeWindow);
    int height = ANativeWindow_getHeight(nativeWindow);
    LOGI("Viewport: %dx%d", width, height);
    scene->setViewport(width, height);

    LOGI("EGL initialized in render thread");
    return true;
}

void EGLCore::setupScene() {
}