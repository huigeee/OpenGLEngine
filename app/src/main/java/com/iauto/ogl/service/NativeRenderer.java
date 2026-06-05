package com.iauto.ogl.service;

import android.view.Surface;

/**
 * Native Renderer JNI 接口类
 * 所有 native 方法在此声明，C++ 侧通过 RegisterNatives 或 JNI 命名绑定。
 */
public final class NativeRenderer {
    static {
        System.loadLibrary("native-renderer");
    }

    private NativeRenderer() {} // 工具类，禁止实例化

    // 生命周期
    public static native boolean nativeInit(Surface surface, String filesDir);
    public static native boolean nativeReConnectSurface(Surface surface);
    public static native void nativeRelease();
    public static native void nativePause();
    public static native void nativeResume();
    public static native boolean nativeIsInitialized();
    public static native float nativeGetFPS();

    // Viewport
    public static native void nativeSetViewport(int width, int height);

    // Touch
    public static native void nativeProcessTouchEvent(int actionMasked, int pointerCount,
                                                       float[] xs, float[] ys, int[] ids);

    // 模型变换
    public static native void nativeSetScale(float scale);
    public static native void nativeSetPosition(float x, float y, float z);

    // 材质
    public static native void nativeSetBaseColor(float r, float g, float b);
    public static native void nativeSetMetallic(float value);
    public static native void nativeSetRoughness(float value);
    public static native void nativeSetAO(float value);

    // 相机
    public static native void nativeSetCameraPosition(float x, float y, float z);
    public static native void nativeSetCameraTarget(float x, float y, float z);

    // 后处理
    public static native void nativeSetTAAEnabled(boolean enabled);
    public static native void nativeSetBloomEnabled(boolean enabled);
    public static native void nativeSetBloomIntensity(float intensity);
    public static native void nativeSetTonemapEnabled(boolean enabled);
    public static native void nativeSetTonemapOp(int op);
    public static native void nativeSetExposure(float exposure);

    // 批量参数
    public static native void nativeUpdateCameraParams(
            float posX, float posY, float posZ,
            float targetX, float targetY, float targetZ,
            float fov, boolean orthographic);

    public static native void nativeUpdateSceneConfig(
            float bgR, float bgG, float bgB,
            float ambient, boolean shadow, boolean ssao,
            float lightPosX, float lightPosY, float lightPosZ,
            float shadowBias, float shadowSoftness, boolean shadowPCF);

    public static native void nativeUpdatePostProcessParams(
            int aaMode, boolean bloom, float bloomIntensity,
            boolean tonemap, int tonemapOp, float exposure,
            boolean fog, int fogMode, float fogDensity,
            float fogStart, float fogEnd, float fogR, float fogG, float fogB,
            boolean dof, float dofFocusDist, float dofNear, float dofFar,
            boolean volumetric, float volDensity, float volScattering,
            int volSteps, float volMaxDistance, float volIntensity,
            float volColorR, float volColorG, float volColorB);

    public static native void nativeUpdateMaterialParams(
            float baseR, float baseG, float baseB,
            float metallic, float roughness, float ao);
}
