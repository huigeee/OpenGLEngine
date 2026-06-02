#include <jni.h>
#include <android/log.h>
#include <string>
#include "ogl/include/EGLCore.h"
#include "ogl/include/Common.h"

static EGLCore* eglCore = nullptr;

extern "C" JNIEXPORT jboolean JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeInit(JNIEnv* env, jobject thiz, jobject surface, jstring filesDir) {
    LOGD("initEgl called");
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

    if (!eglCore->init(env, surface)) {
        LOGD("eglCore->init returned false");
        return JNI_FALSE;
    }
    LOGD("Starting render thread");
    eglCore->start();
    LOGD("Render thread started");
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeReConnectSurface(JNIEnv* env, jobject thiz, jobject surface) {
    LOGD("requestReConnectSurface called");
    if (eglCore == nullptr) {
        LOGE("eglCore not initialized");
        return JNI_FALSE;
    }
    // 在渲染线程中处理 Surface 重新链接（线程安全）
    eglCore->requestReConnectSurface(env, surface);
    LOGD("Surface reconnection requested to render thread");
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeRelease(JNIEnv* env, jobject thiz) {
    if (eglCore != nullptr) {
        eglCore->release();
        delete eglCore;
        eglCore = nullptr;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativePause(JNIEnv* env, jobject thiz) {
    if (eglCore != nullptr) {
        eglCore->pause();
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeResume(JNIEnv* env, jobject thiz) {
    if (eglCore != nullptr) {
        eglCore->resume();
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeSetRotation(JNIEnv* env, jobject thiz, jfloat deltaX, jfloat deltaY) {
    if (eglCore != nullptr) {
        eglCore->setRotation(deltaX, deltaY);
    }
}

// -------------------------------------------------------------------------
// Additional JNI stubs (implement as needed)
// -------------------------------------------------------------------------
extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeSetViewport(JNIEnv*, jobject, jint width, jint height) {
    if (eglCore != nullptr) {
        eglCore->setViewportAsync(width, height);
        LOGI("nativeSetViewport requested: %dx%d", width, height);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeSetScale(JNIEnv*, jobject, jfloat) {}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeSetPosition(JNIEnv*, jobject, jfloat, jfloat, jfloat) {}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeSetBaseColor(JNIEnv*, jobject, jfloat, jfloat, jfloat) {}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeSetMetallic(JNIEnv*, jobject, jfloat) {}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeSetRoughness(JNIEnv*, jobject, jfloat) {}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeSetAO(JNIEnv*, jobject, jfloat) {}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeSetCameraPosition(JNIEnv*, jobject, jfloat, jfloat, jfloat) {}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeSetCameraTarget(JNIEnv*, jobject, jfloat, jfloat, jfloat) {}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeSetTAAEnabled(JNIEnv*, jobject, jboolean) {}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeSetBloomEnabled(JNIEnv*, jobject, jboolean) {}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeSetBloomIntensity(JNIEnv*, jobject, jfloat) {}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeSetTonemapEnabled(JNIEnv*, jobject, jboolean) {}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeSetTonemapOp(JNIEnv*, jobject, jint) {}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeSetExposure(JNIEnv*, jobject, jfloat) {}

// -------------------------------------------------------------------------
// Inspector 参数批量设置
// -------------------------------------------------------------------------
extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeUpdateCameraParams(
        JNIEnv* env, jobject thiz,
        jfloat posX, jfloat posY, jfloat posZ,
        jfloat targetX, jfloat targetY, jfloat targetZ,
        jfloat fov, jboolean orthographic) {
    if (eglCore != nullptr) {
        eglCore->updateCameraParams(posX, posY, posZ, targetX, targetY, targetZ, fov, orthographic);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeUpdateSceneConfig(
        JNIEnv* env, jobject thiz,
        jfloat bgR, jfloat bgG, jfloat bgB,
        jfloat ambient, jboolean shadow, jboolean ssao,
        jfloat lightPosX, jfloat lightPosY, jfloat lightPosZ,
        jfloat shadowBias, jfloat shadowSoftness, jboolean shadowPCF) {
    LOGI("nativeUpdateSceneConfig called - shadow=%d lightPos=(%.2f, %.2f, %.2f) bias=%.3f softness=%.2f pcf=%d", 
          shadow, lightPosX, lightPosY, lightPosZ, shadowBias, shadowSoftness, shadowPCF);
    if (eglCore != nullptr) {
        eglCore->updateSceneConfig(bgR, bgG, bgB, ambient, shadow, false, ssao,
                                   lightPosX, lightPosY, lightPosZ, shadowBias, shadowSoftness, shadowPCF);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeUpdatePostProcessParams(
        JNIEnv* env, jobject thiz,
        jint aaMode, jboolean bloom, jfloat bloomIntensity,
        jboolean tonemap, jint tonemapOp, jfloat exposure,
        jboolean fog, jint fogMode, jfloat fogDensity,
        jfloat fogStart, jfloat fogEnd, jfloat fogR, jfloat fogG, jfloat fogB,
        jboolean dof, jfloat dofFocusDist, jfloat dofNear, jfloat dofFar,
        jboolean volumetric, jfloat volDensity, jfloat volScattering,
        jint volSteps, jfloat volMaxDistance, jfloat volIntensity,
        jfloat volColorR, jfloat volColorG, jfloat volColorB) {
    LOGI("nativeUpdatePostProcessParams JNI called - aaMode=%d bloom=%d bloomInt=%.2f tonemap=%d tonemapOp=%d exposure=%.2f fog=%d fogMode=%d fogDensity=%.2f dof=%d focusDist=%.2f vol=%d volDensity=%.3f",
          aaMode, bloom, bloomIntensity, tonemap, tonemapOp, exposure, fog, fogMode, fogDensity, dof, dofFocusDist, volumetric, volDensity);
    if (eglCore != nullptr) {
        eglCore->updatePostProcessParams(aaMode, bloom, bloomIntensity, tonemap, tonemapOp, exposure,
                                         fog, fogMode, fogDensity, fogStart, fogEnd, fogR, fogG, fogB,
                                         dof, dofFocusDist, dofNear, dofFar,
                                         volumetric, volDensity, volScattering, volSteps,
                                         volMaxDistance, volIntensity, volColorR, volColorG, volColorB);
        LOGI("nativeUpdatePostProcessParams eglCore->updatePostProcessParams completed");
    } else {
        LOGI("nativeUpdatePostProcessParams eglCore is null, skipping update");
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeUpdateMaterialParams(
        JNIEnv* env, jobject thiz,
        jfloat baseR, jfloat baseG, jfloat baseB,
        jfloat metallic, jfloat roughness, jfloat ao) {
    if (eglCore != nullptr) {
        eglCore->updateMaterialParams(baseR, baseG, baseB, metallic, roughness, ao);
    }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeIsInitialized(JNIEnv*, jobject) {
    return eglCore != nullptr ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_iauto_myapplication_service_OglRendererService_00024Companion_nativeGetFPS(JNIEnv*, jobject) {
    if (eglCore != nullptr) {
        return eglCore->getFPS();
    }
    return 0.0f;
}
