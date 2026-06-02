package com.iauto.myapplication.service

import android.content.Context
import android.util.Log
import android.view.Surface

/**
 * Native Renderer wrapper
 * 封装 JNI 调用，管理渲染生命周期
 */
class NativeRenderer(private val context: Context) {

    private var initialized = false
    private var surface: Surface? = null

    companion object {
        private const val TAG = "NativeRenderer"

        // JNI 接口
        external fun init(surface: Surface, filesDir: String): Boolean
        external fun reConnectSurface(surface: Surface): Boolean
        external fun pause()
        external fun resume()
        external fun release()
        external fun setViewport(width: Int, height: Int)
        external fun setRotation(deltaX: Float, deltaY: Float)
        external fun setScale(scale: Float)
        external fun setPosition(x: Float, y: Float, z: Float)
        external fun setBaseColor(r: Float, g: Float, b: Float)
        external fun setMetallic(value: Float)
        external fun setRoughness(value: Float)
        external fun setAO(value: Float)
        external fun setCameraPosition(x: Float, y: Float, z: Float)
        external fun setCameraTarget(x: Float, y: Float, z: Float)
        external fun setTAAEnabled(enabled: Boolean)
        external fun setBloomEnabled(enabled: Boolean)
        external fun setBloomIntensity(intensity: Float)
        external fun setTonemapEnabled(enabled: Boolean)
        external fun setTonemapOp(op: Int)
        external fun setExposure(exposure: Float)
        
        // Inspector 参数批量设置
        external fun updateCameraParams(posX: Float, posY: Float, posZ: Float,
                                        targetX: Float, targetY: Float, targetZ: Float,
                                        fov: Float, orthographic: Boolean)
        external fun updateSceneConfig(bgR: Float, bgG: Float, bgB: Float,
                                       ambient: Float, shadow: Boolean, ssao: Boolean,
                                       lightPosX: Float, lightPosY: Float, lightPosZ: Float,
                                       shadowBias: Float, shadowSoftness: Float, shadowPCF: Boolean)
        external fun updatePostProcessParams(aaMode: Int, bloom: Boolean,
                                              bloomIntensity: Float, tonemap: Boolean,
                                              tonemapOp: Int, exposure: Float,
                                              fog: Boolean, fogMode: Int, fogDensity: Float,
                                              fogStart: Float, fogEnd: Float, fogR: Float, fogG: Float, fogB: Float,
                                              dof: Boolean, dofFocusDist: Float, dofNear: Float, dofFar: Float,
                                              volumetric: Boolean, volDensity: Float, volScattering: Float,
                                              volSteps: Int, volMaxDistance: Float, volIntensity: Float,
                                              volColorR: Float, volColorG: Float, volColorB: Float)
        external fun updateMaterialParams(baseR: Float, baseG: Float, baseB: Float,
                                          metallic: Float, roughness: Float, ao: Float)
        
        external fun isInitialized(): Boolean
        external fun getFPS(): Float

        init {
            System.loadLibrary("native-renderer")
        }
    }

    fun initialize(surface: Surface): Boolean {
        if (initialized) {
            Log.w(TAG, "Already initialized")
            return false
        }

        val filesDir = context.filesDir.absolutePath
        val success = init(surface, filesDir)
        if (success) {
            this.surface = surface
            initialized = true
            Log.i(TAG, "Initialized successfully")
        } else {
            Log.e(TAG, "Initialization failed")
        }
        return success
    }

    fun reconnectSurface(surface: Surface): Boolean {
        if (!initialized) {
            Log.w(TAG, "Not initialized, cannot reconnect")
            return false
        }
        val success = reConnectSurface(surface)
        if (success) {
            this.surface = surface
            Log.i(TAG, "Surface reconnected successfully")
        } else {
            Log.e(TAG, "Surface reconnection failed")
        }
        return success
    }

    fun release() {
        if (!initialized) return
        NativeRenderer.release()
        initialized = false
        surface = null
        Log.i(TAG, "Released")
    }

    fun pause() {
        NativeRenderer.pause()
        Log.i(TAG, "Paused")
    }

    fun resume() {
        NativeRenderer.resume()
        Log.i(TAG, "Resumed")
    }

    fun setViewport(width: Int, height: Int) {
        NativeRenderer.setViewport(width, height)
    }

    fun setRotation(deltaX: Float, deltaY: Float) {
        NativeRenderer.setRotation(deltaX, deltaY)
    }

    fun setScale(scale: Float) {
        NativeRenderer.setScale(scale)
    }

    fun setPosition(x: Float, y: Float, z: Float) {
        NativeRenderer.setPosition(x, y, z)
    }

    fun setBaseColor(r: Float, g: Float, b: Float) {
        NativeRenderer.setBaseColor(r, g, b)
    }

    fun setMetallic(value: Float) {
        NativeRenderer.setMetallic(value)
    }

    fun setRoughness(value: Float) {
        NativeRenderer.setRoughness(value)
    }

    fun setAO(value: Float) {
        NativeRenderer.setAO(value)
    }

    fun setCameraPosition(x: Float, y: Float, z: Float) {
        NativeRenderer.setCameraPosition(x, y, z)
    }

    fun setCameraTarget(x: Float, y: Float, z: Float) {
        NativeRenderer.setCameraTarget(x, y, z)
    }

    fun setTAAEnabled(enabled: Boolean) {
        NativeRenderer.setTAAEnabled(enabled)
    }

    fun setBloomEnabled(enabled: Boolean) {
        NativeRenderer.setBloomEnabled(enabled)
    }

    fun setBloomIntensity(intensity: Float) {
        NativeRenderer.setBloomIntensity(intensity)
    }

    fun setTonemapEnabled(enabled: Boolean) {
        NativeRenderer.setTonemapEnabled(enabled)
    }

    fun setTonemapOp(op: Int) {
        NativeRenderer.setTonemapOp(op)
    }

    fun setExposure(exposure: Float) {
        NativeRenderer.setExposure(exposure)
    }

    // -------------------------------------------------------------------------
    // Inspector 参数批量设置
    // -------------------------------------------------------------------------
    fun updateCameraParams(posX: Float, posY: Float, posZ: Float,
                           targetX: Float, targetY: Float, targetZ: Float,
                           fov: Float, orthographic: Boolean) {
        NativeRenderer.updateCameraParams(posX, posY, posZ, targetX, targetY, targetZ, fov, orthographic)
    }

    fun updateSceneConfig(bgR: Float, bgG: Float, bgB: Float,
                          ambient: Float, shadow: Boolean, ssao: Boolean,
                          lightPosX: Float, lightPosY: Float, lightPosZ: Float,
                          shadowBias: Float, shadowSoftness: Float, shadowPCF: Boolean) {
        NativeRenderer.updateSceneConfig(bgR, bgG, bgB, ambient, shadow, ssao,
                                        lightPosX, lightPosY, lightPosZ, shadowBias, shadowSoftness, shadowPCF)
    }

    fun updatePostProcessParams(aaMode: Int, bloom: Boolean,
                                bloomIntensity: Float, tonemap: Boolean,
                                tonemapOp: Int, exposure: Float,
                                fog: Boolean, fogMode: Int, fogDensity: Float,
                                fogStart: Float, fogEnd: Float, fogR: Float, fogG: Float, fogB: Float,
                                dof: Boolean, dofFocusDist: Float, dofNear: Float, dofFar: Float,
                                volumetric: Boolean = false, volDensity: Float = 0.03f,
                                volScattering: Float = 0.75f, volSteps: Int = 48,
                                volMaxDistance: Float = 30.0f, volIntensity: Float = 1.0f,
                                volColorR: Float = 1.0f, volColorG: Float = 0.95f, volColorB: Float = 0.85f) {
        NativeRenderer.updatePostProcessParams(aaMode, bloom, bloomIntensity, tonemap, tonemapOp, exposure,
                                              fog, fogMode, fogDensity, fogStart, fogEnd, fogR, fogG, fogB,
                                              dof, dofFocusDist, dofNear, dofFar,
                                              volumetric, volDensity, volScattering, volSteps,
                                              volMaxDistance, volIntensity, volColorR, volColorG, volColorB)
    }

    fun updateMaterialParams(baseR: Float, baseG: Float, baseB: Float,
                             metallic: Float, roughness: Float, ao: Float) {
        NativeRenderer.updateMaterialParams(baseR, baseG, baseB, metallic, roughness, ao)
    }

    fun isInitialized(): Boolean = initialized
    fun getFPS(): Float = NativeRenderer.getFPS()
}
