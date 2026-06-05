#include <jni.h>
#include <android/log.h>
#include <string>
#include "ogl/oglengine/include/EGLCore.h"
#include "ogl/oglengine/include/Common.h"
#include "ogl/project/MainScene.h"

static EGLCore* eglCore = nullptr;
static MainScene* mainScene = nullptr;

#ifdef __cplusplus
extern "C" {
#endif

// -------------------------------------------------------------------------
// 实现函数（内部命名，不暴露为导出符号）
// -------------------------------------------------------------------------

static jboolean nativeInit(JNIEnv* env, jclass clazz, jobject surface, jstring filesDir) {
    LOGD("nativeInit called");
    if (eglCore == nullptr) {
        eglCore = new EGLCore();
    }
    JavaVM* vm;
    env->GetJavaVM(&vm);
    eglCore->setJavaVM(vm);

    if (filesDir != nullptr) {
        const char* path = env->GetStringUTFChars(filesDir, nullptr);
        eglCore->setFilesDir(path);
        env->ReleaseStringUTFChars(filesDir, path);
    }

    mainScene = new MainScene();
    mainScene->setFilesDir(eglCore->getFilesDir());
    eglCore->setScene(mainScene);

    if (!eglCore->init(env, surface)) {
        LOGD("eglCore->init returned false");
        return JNI_FALSE;
    }
    LOGD("Starting render thread");
    eglCore->start();
    LOGD("Render thread started");
    return JNI_TRUE;
}

static jboolean nativeReConnectSurface(JNIEnv* env, jclass clazz, jobject surface) {
    LOGD("nativeReConnectSurface called");
    if (eglCore == nullptr) {
        LOGE("eglCore not initialized");
        return JNI_FALSE;
    }
    eglCore->requestReConnectSurface(env, surface);
    LOGD("Surface reconnection requested to render thread");
    return JNI_TRUE;
}

static void nativeRelease(JNIEnv* env, jclass clazz) {
    if (eglCore != nullptr) {
        eglCore->release();
        delete eglCore;
        eglCore = nullptr;
    }
}

static void nativePause(JNIEnv* env, jclass clazz) {
    if (eglCore != nullptr) eglCore->pause();
}

static void nativeResume(JNIEnv* env, jclass clazz) {
    if (eglCore != nullptr) eglCore->resume();
}

static jboolean nativeIsInitialized(JNIEnv* env, jclass clazz) {
    return eglCore != nullptr ? JNI_TRUE : JNI_FALSE;
}

static jfloat nativeGetFPS(JNIEnv* env, jclass clazz) {
    if (eglCore != nullptr) return eglCore->getFPS();
    return 0.0f;
}

// Viewport
static void nativeSetViewport(JNIEnv* env, jclass clazz, jint width, jint height) {
    if (eglCore != nullptr) {
        eglCore->setViewportAsync(width, height);
        LOGI("nativeSetViewport requested: %dx%d", width, height);
    }
}

// Touch
static void nativeProcessTouchEvent(JNIEnv* env, jclass clazz,
    jint actionMasked, jint pointerCount,
    jfloatArray xs, jfloatArray ys, jintArray ids) {
    if (eglCore == nullptr) return;
    jsize count = env->GetArrayLength(xs);
    if (count > 10) count = 10;
    float xbuf[10], ybuf[10];
    int   idbuf[10];
    env->GetFloatArrayRegion(xs, 0, count, xbuf);
    env->GetFloatArrayRegion(ys, 0, count, ybuf);
    env->GetIntArrayRegion(ids, 0, count, idbuf);
    eglCore->processTouchEvent(actionMasked, pointerCount, xbuf, ybuf, idbuf);
}

// 简单设置（stub，留给后续扩展）
static void nativeSetScale(JNIEnv*, jclass, jfloat) {}
static void nativeSetPosition(JNIEnv*, jclass, jfloat, jfloat, jfloat) {}
static void nativeSetBaseColor(JNIEnv*, jclass, jfloat, jfloat, jfloat) {}
static void nativeSetMetallic(JNIEnv*, jclass, jfloat) {}
static void nativeSetRoughness(JNIEnv*, jclass, jfloat) {}
static void nativeSetAO(JNIEnv*, jclass, jfloat) {}
static void nativeSetCameraPosition(JNIEnv*, jclass, jfloat, jfloat, jfloat) {}
static void nativeSetCameraTarget(JNIEnv*, jclass, jfloat, jfloat, jfloat) {}
static void nativeSetTAAEnabled(JNIEnv*, jclass, jboolean) {}
static void nativeSetBloomEnabled(JNIEnv*, jclass, jboolean) {}
static void nativeSetBloomIntensity(JNIEnv*, jclass, jfloat) {}
static void nativeSetTonemapEnabled(JNIEnv*, jclass, jboolean) {}
static void nativeSetTonemapOp(JNIEnv*, jclass, jint) {}
static void nativeSetExposure(JNIEnv*, jclass, jfloat) {}

// 批量参数
static void nativeUpdateCameraParams(JNIEnv* env, jclass clazz,
        jfloat posX, jfloat posY, jfloat posZ,
        jfloat targetX, jfloat targetY, jfloat targetZ,
        jfloat fov, jboolean orthographic) {
    if (eglCore != nullptr) {
        eglCore->updateCameraParams(posX, posY, posZ, targetX, targetY, targetZ, fov, orthographic);
    }
}

static void nativeUpdateSceneConfig(JNIEnv* env, jclass clazz,
        jfloat bgR, jfloat bgG, jfloat bgB,
        jfloat ambient, jboolean shadow, jboolean ssao,
        jfloat lightPosX, jfloat lightPosY, jfloat lightPosZ,
        jfloat shadowBias, jfloat shadowSoftness, jboolean shadowPCF) {
    LOGI("nativeUpdateSceneConfig - shadow=%d lightPos=(%.2f,%.2f,%.2f) bias=%.3f softness=%.2f pcf=%d",
          shadow, lightPosX, lightPosY, lightPosZ, shadowBias, shadowSoftness, shadowPCF);
    if (eglCore != nullptr) {
        eglCore->updateSceneConfig(bgR, bgG, bgB, ambient, shadow, false, ssao,
                                   lightPosX, lightPosY, lightPosZ, shadowBias, shadowSoftness, shadowPCF);
    }
}

static void nativeUpdatePostProcessParams(JNIEnv* env, jclass clazz,
        jint aaMode, jboolean bloom, jfloat bloomIntensity,
        jboolean tonemap, jint tonemapOp, jfloat exposure,
        jboolean fog, jint fogMode, jfloat fogDensity,
        jfloat fogStart, jfloat fogEnd, jfloat fogR, jfloat fogG, jfloat fogB,
        jboolean dof, jfloat dofFocusDist, jfloat dofNear, jfloat dofFar,
        jboolean volumetric, jfloat volDensity, jfloat volScattering,
        jint volSteps, jfloat volMaxDistance, jfloat volIntensity,
        jfloat volColorR, jfloat volColorG, jfloat volColorB) {
    LOGI("nativeUpdatePostProcessParams - aaMode=%d bloom=%d bloomInt=%.2f tonemap=%d tonemapOp=%d"
         " exposure=%.2f fog=%d fogMode=%d fogDensity=%.2f dof=%d focusDist=%.2f vol=%d volDensity=%.3f",
          aaMode, bloom, bloomIntensity, tonemap, tonemapOp, exposure,
          fog, fogMode, fogDensity, dof, dofFocusDist, volumetric, volDensity);
    if (eglCore != nullptr) {
        eglCore->updatePostProcessParams(aaMode, bloom, bloomIntensity, tonemap, tonemapOp, exposure,
                                         fog, fogMode, fogDensity, fogStart, fogEnd, fogR, fogG, fogB,
                                         dof, dofFocusDist, dofNear, dofFar,
                                         volumetric, volDensity, volScattering, volSteps,
                                         volMaxDistance, volIntensity, volColorR, volColorG, volColorB);
    } else {
        LOGI("nativeUpdatePostProcessParams - eglCore is null, skipping");
    }
}

static void nativeUpdateMaterialParams(JNIEnv* env, jclass clazz,
        jfloat baseR, jfloat baseG, jfloat baseB,
        jfloat metallic, jfloat roughness, jfloat ao) {
    if (eglCore != nullptr) {
        eglCore->updateMaterialParams(baseR, baseG, baseB, metallic, roughness, ao);
    }
}

// -------------------------------------------------------------------------
// JNI_OnLoad — 动态注册所有方法
// -------------------------------------------------------------------------
jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env = nullptr;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    jclass cls = env->FindClass("com/iauto/ogl/service/NativeRenderer");
    if (cls == nullptr) {
        LOGE("JNI_OnLoad: FindClass com/iauto/ogl/service/NativeRenderer failed");
        return JNI_ERR;
    }

    static const JNINativeMethod methods[] = {
        // 生命周期
        {"nativeInit",              "(Landroid/view/Surface;Ljava/lang/String;)Z", (void*)nativeInit},
        {"nativeReConnectSurface",  "(Landroid/view/Surface;)Z",                   (void*)nativeReConnectSurface},
        {"nativeRelease",           "()V",                                          (void*)nativeRelease},
        {"nativePause",             "()V",                                          (void*)nativePause},
        {"nativeResume",            "()V",                                          (void*)nativeResume},
        {"nativeIsInitialized",     "()Z",                                          (void*)nativeIsInitialized},
        {"nativeGetFPS",            "()F",                                          (void*)nativeGetFPS},
        // Viewport
        {"nativeSetViewport",       "(II)V",                                        (void*)nativeSetViewport},
        // Touch
        {"nativeProcessTouchEvent", "(II[F[F[I)V",                                  (void*)nativeProcessTouchEvent},
        // 简单设置
        {"nativeSetScale",          "(F)V",                                         (void*)nativeSetScale},
        {"nativeSetPosition",       "(FFF)V",                                       (void*)nativeSetPosition},
        {"nativeSetBaseColor",      "(FFF)V",                                       (void*)nativeSetBaseColor},
        {"nativeSetMetallic",       "(F)V",                                         (void*)nativeSetMetallic},
        {"nativeSetRoughness",      "(F)V",                                         (void*)nativeSetRoughness},
        {"nativeSetAO",             "(F)V",                                         (void*)nativeSetAO},
        {"nativeSetCameraPosition", "(FFF)V",                                       (void*)nativeSetCameraPosition},
        {"nativeSetCameraTarget",   "(FFF)V",                                       (void*)nativeSetCameraTarget},
        {"nativeSetTAAEnabled",     "(Z)V",                                         (void*)nativeSetTAAEnabled},
        {"nativeSetBloomEnabled",   "(Z)V",                                         (void*)nativeSetBloomEnabled},
        {"nativeSetBloomIntensity", "(F)V",                                         (void*)nativeSetBloomIntensity},
        {"nativeSetTonemapEnabled", "(Z)V",                                         (void*)nativeSetTonemapEnabled},
        {"nativeSetTonemapOp",      "(I)V",                                         (void*)nativeSetTonemapOp},
        {"nativeSetExposure",       "(F)V",                                         (void*)nativeSetExposure},
        // 批量参数
        {"nativeUpdateCameraParams",
            "(FFFFFFFZ)V",                                                          (void*)nativeUpdateCameraParams},
        {"nativeUpdateSceneConfig",
            "(FFFFZZFFFFFZ)V",                                                      (void*)nativeUpdateSceneConfig},
        {"nativeUpdatePostProcessParams",
            "(IZFZIFZIFFFFFFZFFFZFFIFFFFF)V",                                       (void*)nativeUpdatePostProcessParams},
        {"nativeUpdateMaterialParams",
            "(FFFFFF)V",                                                            (void*)nativeUpdateMaterialParams},
    };

    jint result = env->RegisterNatives(cls, methods, sizeof(methods) / sizeof(methods[0]));
    if (result != JNI_OK) {
        LOGE("JNI_OnLoad: RegisterNatives failed, result = %d", result);
        return JNI_ERR;
    }
    LOGD("JNI_OnLoad: RegisterNatives OK");
    return JNI_VERSION_1_6;
}

#ifdef __cplusplus
}
#endif
