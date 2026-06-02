package com.iauto.myapplication.service

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Context
import android.content.Intent
import android.os.Binder
import android.os.Build
import android.os.IBinder
import android.util.Log
import android.view.Surface
import com.iauto.myapplication.renderer.IOglRenderer
import com.iauto.myapplication.data.CameraParams
import com.iauto.myapplication.data.SceneConfig
import com.iauto.myapplication.data.PostProcessConfig
import com.iauto.myapplication.data.MaterialParams
import java.io.File
import java.io.FileOutputStream

/**
 * OpenGL Renderer Service
 * - Foreground Service: 保活
 * - Bound Service: 暴露 IOglRenderer 接口给 Manager/Activity
 * - 同进程：AIDL 只是接口定义，实际渲染在 Service 进程内
 */
class OglRendererService : Service() {

    private val binder = LocalBinder()
    private var renderer: NativeRenderer? = null
    private var isForeground = false

    inner class LocalBinder : Binder() {
        fun getService(): OglRendererService = this@OglRendererService
    }

    companion object {
        private const val TAG = "OglRendererService"
        private const val NOTIFICATION_ID = 1001
        private const val CHANNEL_ID = "ogl_renderer_channel"
        private const val CHANNEL_NAME = "OpenGL Renderer"

        // JNI 接口（同进程，直接调用）
        private external fun nativeInit(surface: Surface, filesDir: String): Boolean
        private external fun nativeReConnectSurface(surface: Surface): Boolean
        private external fun nativeRelease()
        private external fun nativeSetViewport(width: Int, height: Int)
        private external fun nativeSetRotation(deltaX: Float, deltaY: Float)
        private external fun nativeSetScale(scale: Float)
        private external fun nativeSetPosition(x: Float, y: Float, z: Float)
        private external fun nativeSetBaseColor(r: Float, g: Float, b: Float)
        private external fun nativeSetMetallic(value: Float)
        private external fun nativeSetRoughness(value: Float)
        private external fun nativeSetAO(value: Float)
        private external fun nativeSetCameraPosition(x: Float, y: Float, z: Float)
        private external fun nativeSetCameraTarget(x: Float, y: Float, z: Float)
        private external fun nativeSetTAAEnabled(enabled: Boolean)
        private external fun nativeSetBloomEnabled(enabled: Boolean)
        private external fun nativeSetBloomIntensity(intensity: Float)
        private external fun nativeSetTonemapEnabled(enabled: Boolean)
        private external fun nativeSetTonemapOp(op: Int)
        private external fun nativeSetExposure(exposure: Float)
        
        // Inspector 参数批量设置
        private external fun nativeUpdateCameraParams(posX: Float, posY: Float, posZ: Float,
                                                      targetX: Float, targetY: Float, targetZ: Float,
                                                      fov: Float, orthographic: Boolean)
        private external fun nativeUpdateSceneConfig(bgR: Float, bgG: Float, bgB: Float,
                                                     ambient: Float, shadow: Boolean, ssao: Boolean,
                                                     lightPosX: Float, lightPosY: Float, lightPosZ: Float,
                                                     shadowBias: Float, shadowSoftness: Float, shadowPCF: Boolean)
        private external fun nativeUpdatePostProcessParams(aaMode: Int, bloom: Boolean,
                                                           bloomIntensity: Float, tonemap: Boolean,
                                                           tonemapOp: Int, exposure: Float,
                                                           fog: Boolean, fogMode: Int, fogDensity: Float,
                                                           fogStart: Float, fogEnd: Float,
                                                           fogR: Float, fogG: Float, fogB: Float,
                                                           dof: Boolean, dofFocusDist: Float,
                                                            dofNear: Float, dofFar: Float,
                                                            volumetric: Boolean, volDensity: Float,
                                                            volScattering: Float, volSteps: Int,
                                                            volMaxDistance: Float, volIntensity: Float,
                                                            volColorR: Float, volColorG: Float, volColorB: Float)
         private external fun nativeUpdateMaterialParams(baseR: Float, baseG: Float, baseB: Float,
                                                         metallic: Float, roughness: Float, ao: Float)
         
         private external fun nativeIsInitialized(): Boolean
         private external fun nativeGetFPS(): Float
         
         // Lifecycle control
         private external fun nativePause()
         private external fun nativeResume()

        init {
            System.loadLibrary("native-renderer")
        }
    }

    // -------------------------------------------------------------------------
    // Service lifecycle
    // -------------------------------------------------------------------------
    override fun onCreate() {
        super.onCreate()
        Log.i(TAG, "onCreate")
        renderer = NativeRenderer(this)
    }

    override fun onBind(intent: Intent?): IBinder {
        Log.i(TAG, "onBind")
        return binder
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        Log.i(TAG, "onStartCommand")
        startForeground()
        return START_STICKY
    }

    override fun onDestroy() {
        Log.i(TAG, "onDestroy")
        stopForeground()
        renderer?.release()
        renderer = null
        super.onDestroy()
    }

    // -------------------------------------------------------------------------
    // Foreground service
    // -------------------------------------------------------------------------
    private fun startForeground() {
        if (isForeground) return

        createNotificationChannel()
        val notification = buildNotification()
        startForeground(NOTIFICATION_ID, notification)
        isForeground = true
        Log.i(TAG, "Started as foreground service")
    }

    private fun stopForeground() {
        if (!isForeground) return
        stopForeground(STOP_FOREGROUND_REMOVE)
        isForeground = false
        Log.i(TAG, "Stopped foreground mode")
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                CHANNEL_ID,
                CHANNEL_NAME,
                NotificationManager.IMPORTANCE_LOW
            ).apply {
                description = "OpenGL Rendering Service"
                setShowBadge(false)
            }
            val manager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            manager.createNotificationChannel(channel)
        }
    }

    private fun buildNotification(): Notification {
        return Notification.Builder(this, CHANNEL_ID)
            .setContentTitle("OpenGL Renderer")
            .setContentText("Rendering...")
            .setSmallIcon(android.R.drawable.ic_menu_gallery)
            .build()
    }

    // -------------------------------------------------------------------------
    // IOglRenderer.Stub implementation
    // -------------------------------------------------------------------------
    private val oglRendererBinder = object : IOglRenderer.Stub() {
        override fun init(surface: Surface?, filesDir: String?) {
            if (surface == null || filesDir == null) return
            Log.i(TAG, "init surface=$filesDir")
            // 如果已经初始化过，尝试重新链接 Surface 而不是重新初始化
            if (nativeIsInitialized()) {
                Log.i(TAG, "Renderer already initialized, reconnecting surface")
                nativeReConnectSurface(surface)
            } else {
                nativeInit(surface, filesDir)
            }
        }

        override fun reConnectSurface(surface: Surface?) {
            if (surface == null) return
            Log.i(TAG, "reConnectSurface")
            nativeReConnectSurface(surface)
        }

        override fun release() {
            Log.i(TAG, "release")
            nativeRelease()
        }

        override fun pause() {
            Log.i(TAG, "pause")
            nativePause()
        }

        override fun resume() {
            Log.i(TAG, "resume")
            nativeResume()
        }

        override fun setViewport(width: Int, height: Int) {
            nativeSetViewport(width, height)
        }

        override fun setRotation(deltaX: Float, deltaY: Float) {
            nativeSetRotation(deltaX, deltaY)
        }

        override fun setScale(scale: Float) {
            nativeSetScale(scale)
        }

        override fun setPosition(x: Float, y: Float, z: Float) {
            nativeSetPosition(x, y, z)
        }

        override fun setBaseColor(r: Float, g: Float, b: Float) {
            nativeSetBaseColor(r, g, b)
        }

        override fun setMetallic(value: Float) {
            nativeSetMetallic(value)
        }

        override fun setRoughness(value: Float) {
            nativeSetRoughness(value)
        }

        override fun setAO(value: Float) {
            nativeSetAO(value)
        }

        override fun setCameraPosition(x: Float, y: Float, z: Float) {
            nativeSetCameraPosition(x, y, z)
        }

        override fun setCameraTarget(x: Float, y: Float, z: Float) {
            nativeSetCameraTarget(x, y, z)
        }

        override fun setTAAEnabled(enabled: Boolean) {
            nativeSetTAAEnabled(enabled)
        }

        override fun setBloomEnabled(enabled: Boolean) {
            nativeSetBloomEnabled(enabled)
        }

        override fun setBloomIntensity(intensity: Float) {
            nativeSetBloomIntensity(intensity)
        }

        override fun setTonemapEnabled(enabled: Boolean) {
            nativeSetTonemapEnabled(enabled)
        }

        override fun setTonemapOp(op: Int) {
            nativeSetTonemapOp(op)
        }

        override fun setExposure(exposure: Float) {
            nativeSetExposure(exposure)
        }

        override fun isInitialized(): Boolean {
            return nativeIsInitialized()
        }

        override fun getFPS(): Float {
            return nativeGetFPS()
        }
        
        // -------------------------------------------------------------------------
        // Inspector 参数批量设置
        // -------------------------------------------------------------------------
        override fun updateCameraParams(params: CameraParams?) {
            if (params == null) return
            nativeUpdateCameraParams(
                params.position.x, params.position.y, params.position.z,
                params.target.x, params.target.y, params.target.z,
                params.fov, params.orthographic
            )
        }

        override fun updateSceneConfig(config: SceneConfig?) {
            if (config == null) return
            nativeUpdateSceneConfig(
                config.bgColor.x, config.bgColor.y, config.bgColor.z,
                config.ambientIntensity, config.shadowEnabled, config.ssaoEnabled,
                config.lightPosX, config.lightPosY, config.lightPosZ,
                config.shadowBias, config.shadowSoftness, config.shadowPCFEnabled
            )
        }

        override fun updatePostProcessConfig(config: PostProcessConfig?) {
            if (config == null) return
            nativeUpdatePostProcessParams(
                config.aaMode, config.bloomEnabled, config.bloomIntensity,
                config.tonemapEnabled, config.tonemapOp, config.exposure,
                config.fogEnabled, config.fogMode, config.fogDensity,
                config.fogStart, config.fogEnd, config.fogR, config.fogG, config.fogB,
                config.dofEnabled, config.dofFocusDist, config.dofNear, config.dofFar,
                config.volumetricEnabled, config.volumetricDensity, config.volumetricScattering,
                config.volumetricSteps, config.volumetricMaxDistance, config.volumetricIntensity,
                config.volumetricColorR, config.volumetricColorG, config.volumetricColorB
            )
        }

        override fun updateMaterialParams(params: MaterialParams?) {
            if (params == null) return
            nativeUpdateMaterialParams(
                params.baseColor.x, params.baseColor.y, params.baseColor.z,
                params.metallic, params.roughness, params.ao
            )
        }

        override fun getCameraParams(): CameraParams {
            // TODO: 从引擎获取当前参数，暂时返回默认值
            return CameraParams()
        }

        override fun getSceneConfig(): SceneConfig {
            // TODO: 从引擎获取当前参数，暂时返回默认值
            return SceneConfig()
        }

        override fun getPostProcessConfig(): PostProcessConfig {
            // TODO: 从引擎获取当前参数，暂时返回默认值
            return PostProcessConfig()
        }

        override fun getMaterialParams(): MaterialParams {
            // TODO: 从引擎获取当前参数，暂时返回默认值
            return MaterialParams()
        }
    }

    // -------------------------------------------------------------------------
    // Public API for Manager
    // -------------------------------------------------------------------------
    fun getRendererBinder(): IOglRenderer {
        return oglRendererBinder
    }
}
