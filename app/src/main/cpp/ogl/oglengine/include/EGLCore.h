#pragma once

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <jni.h>
#include <pthread.h>
#include <string>
#include <queue>
#include "Scene.h"

// 参数更新请求结构体
struct ParamUpdateRequest {
    bool hasCameraParams;
    float camPosX, camPosY, camPosZ;
    float camTargetX, camTargetY, camTargetZ;
    float camFov;
    bool camOrthographic;
    
    bool hasSceneConfig;
    float sceneBgR, sceneBgG, sceneBgB;
    float sceneAmbient;
    bool sceneShadow, sceneReflection, sceneSsao;
    float sceneLightPosX, sceneLightPosY, sceneLightPosZ;
    float sceneShadowBias;
    float sceneShadowSoft;
    bool sceneShadowPCF;
    
    bool hasPostProcessParams;
    int ppAaMode;  // 0: TAA, 1: FXAA, 2: None
    bool ppBloom;
    float ppBloomIntensity;
    bool ppTonemap;
    int ppTonemapOp;
    float ppExposure;
    // Fog
    bool ppFog;
    int ppFogMode;
    float ppFogDensity, ppFogStart, ppFogEnd;
    float ppFogR, ppFogG, ppFogB;
    // DoF
    bool ppDof;
    float ppDofFocusDist, ppDofNear, ppDofFar;
    // Volumetric
    bool ppVolumetric;
    float ppVolDensity, ppVolScattering;
    int ppVolSteps;
    float ppVolMaxDistance, ppVolIntensity;
    float ppVolColorR, ppVolColorG, ppVolColorB;
    
    bool hasMaterialParams;
    float matBaseR, matBaseG, matBaseB;
    float matMetallic, matRoughness, matAo;
    
    ParamUpdateRequest() {
        memset(this, 0, sizeof(*this));
    }
};

// Surface 重新链接请求
struct SurfaceReconnectRequest {
    bool pending;
    jobject surface;  // Global reference
    
    SurfaceReconnectRequest() : pending(false), surface(nullptr) {}
};

// Viewport 设置请求
struct ViewportRequest {
    bool pending;
    int width;
    int height;
    
    ViewportRequest() : pending(false), width(0), height(0) {}
};

class EGLCore {
public:
    EGLCore();
    ~EGLCore();

    void setJavaVM(JavaVM* vm);
    void setFilesDir(const char* path);
    const char* getFilesDir() const;
    void setRotation(float deltaX, float deltaY);
    bool init(JNIEnv* env, jobject surface);
    void requestReConnectSurface(JNIEnv* env, jobject surface);  // 请求重新链接（线程安全）
    void start();
    void stop();
    void pause();  // 暂停渲染（Surface 销毁时调用，线程进入空循环）
    void resume(); // 恢复渲染（Surface 重建时调用）
    void release();
    bool isValid() const;
    
    bool isPaused() const;  // 检查是否暂停
    
    // Inspector 参数批量设置（线程安全，推入队列）
    void updateCameraParams(float posX, float posY, float posZ,
                           float targetX, float targetY, float targetZ,
                           float fov, bool orthographic);
    void updateSceneConfig(float bgR, float bgG, float bgB,
                           float ambient, bool shadow, bool reflection, bool ssao,
                           float lightPosX, float lightPosY, float lightPosZ,
                           float shadowBias, float shadowSoftness, bool shadowPCF);
    void updatePostProcessParams(jint aaMode, jboolean bloom, jfloat bloomIntensity,
                                jboolean tonemap, jint tonemapOp, jfloat exposure,
                                jboolean fog, jint fogMode, jfloat fogDensity,
                                jfloat fogStart, jfloat fogEnd, jfloat fogR, jfloat fogG, jfloat fogB,
                                jboolean dof, jfloat dofFocusDist, jfloat dofNear, jfloat dofFar,
                                jboolean volumetric, jfloat volDensity, jfloat volScattering,
                                jint volSteps, jfloat volMaxDistance, jfloat volIntensity,
                                jfloat volColorR, jfloat volColorG, jfloat volColorB);
    void updateMaterialParams(float baseR, float baseG, float baseB,
                              float metallic, float roughness, float ao);
    
    // Viewport 设置（线程安全，推入队列）
    void setViewportAsync(int width, int height);  // Android 层调用，线程安全
    void setViewport(int width, int height);  // 仅在渲染线程中直接调用
    
    // FPS 统计
    float getFPS() const { return currentFPS; }

    // 设置 Scene 实例（由用户在 native-renderer 中创建并传入）
    void setScene(Scene* s);

    // 原始 touch 事件（线程安全）— 对应 Android MotionEvent 完整数据
    void processTouchEvent(int actionMasked, int pointerCount,
                           const float* xs, const float* ys, const int* ids);

private:
    EGLDisplay eglDisplay;
    EGLContext eglContext;
    EGLSurface eglSurface;
    EGLConfig eglConfig;
    ANativeWindow* nativeWindow;
    Scene* scene;

    pthread_t renderThread;
    bool running;
    bool paused;  // 暂停标志
    bool initSuccess;
    jobject pendingSurface;

    pthread_mutex_t mutex;
    pthread_cond_t cond;

    JavaVM* javaVM;
    JNIEnv* threadEnv;
    std::string filesDir;

    float pendingDeltaX;
    float pendingDeltaY;
    
    // 参数更新队列（线程安全）
    std::queue<ParamUpdateRequest> paramQueue;
    
    // Touch 事件
    struct TouchEvent {
        int actionMasked;
        int pointerCount;
        float xs[10];
        float ys[10];
        int   ids[10];
    };
    std::queue<TouchEvent> touchQueue;
    
    // Surface 重新链接请求（线程安全）
    SurfaceReconnectRequest surfaceReconnectRequest;
    
    // Viewport 设置请求（线程安全）
    ViewportRequest viewportRequest;
    
    // FPS 统计
    float currentFPS;
    int frameCount;
    double lastFpsUpdateTime;

    static void* renderLoop(void* arg);
    JNIEnv* getJNIEnv();
    void renderLoopInternal();
    bool initEGL(JNIEnv* env);
    void initGL();
    void applyParamUpdates();  // 在渲染线程中应用参数更新
    void applySurfaceReconnect();  // 在渲染线程中重新链接 Surface
    void applyViewport();  // 在渲染线程中应用 Viewport 设置
};
