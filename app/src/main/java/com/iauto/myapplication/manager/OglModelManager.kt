package com.iauto.myapplication.manager

import android.content.ComponentName
import android.content.Context
import android.content.ServiceConnection
import android.os.IBinder
import android.util.Log
import com.iauto.myapplication.renderer.IOglRenderer
import com.iauto.myapplication.service.OglRendererService
import com.iauto.myapplication.view.OglSurfaceView
import com.iauto.myapplication.data.CameraParams
import com.iauto.myapplication.data.SceneConfig
import com.iauto.myapplication.data.PostProcessConfig
import com.iauto.myapplication.data.MaterialParams

/**
 * OglModelManager
 * - AIDL 客户端，连接 OglRendererService
 * - 处理触摸事件，转发给 Renderer
 * - 提供高层 API 给 UI 层使用
 */
class OglModelManager(private val context: Context) : OglSurfaceView.Listener {

    private var renderer: IOglRenderer? = null
    private var isBound = false
    private var isInitialized = false
    private var resourcesDir: String? = null
    private var pendingSurface: android.view.Surface? = null

    private val serviceConnection = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
            Log.i(TAG, "onServiceConnected")
            val binder = service as OglRendererService.LocalBinder
            val rendererService = binder.getService()
            renderer = rendererService.getRendererBinder()
            isBound = true
            onRendererReady()
        }

        override fun onServiceDisconnected(name: ComponentName?) {
            Log.w(TAG, "onServiceDisconnected")
            renderer = null
            isBound = false
            isInitialized = false
        }
    }

    companion object {
        private const val TAG = "OglModelManager"
    }

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------
    fun bind(resourcesDir: String?) {
        if (isBound) {
            Log.w(TAG, "Already bound")
            return
        }
        val intent = android.content.Intent(context, OglRendererService::class.java)
        // 先启动服务（确保前台服务运行）
        context.startForegroundService(intent)
        // 然后绑定
        context.bindService(intent, serviceConnection, Context.BIND_AUTO_CREATE)
        this.resourcesDir = resourcesDir
        Log.i(TAG, "Binding to service, resourcesDir=$resourcesDir")
    }

    fun unbind() {
        if (!isBound) return
        context.unbindService(serviceConnection)
        isBound = false
        isInitialized = false
        renderer = null
        Log.i(TAG, "Unbound from service")
    }

    fun startForegroundService() {
        // Deprecated: use bind() instead
        Log.w(TAG, "startForegroundService() is deprecated, use bind() instead")
    }

    // -------------------------------------------------------------------------
    // OglSurfaceView.Listener implementation
    // -------------------------------------------------------------------------
    override fun onSurfaceCreated(surface: android.view.Surface) {
        Log.i(TAG, "onSurfaceCreated")
        if (!isBound) {
            Log.w(TAG, "Renderer not ready yet, will init when bound")
            pendingSurface = surface
            return
        }
        val filesDir = resourcesDir ?: context.filesDir.absolutePath
        if (!isInitialized) {
            // 首次初始化
            renderer?.init(surface, filesDir)
            isInitialized = true
        } else {
            // Surface 重建，重新链接
            renderer?.reConnectSurface(surface)
            // 恢复渲染循环
            renderer?.resume()
            // 重新链接后确保 Viewport 正确更新
            // 注意：reConnectSurface 已经在 native 层更新了 viewport，这里不需要重复调用
        }
    }

    override fun onSurfaceChanged(width: Int, height: Int) {
        Log.i(TAG, "onSurfaceChanged: ${width}x${height}")
        renderer?.setViewport(width, height)
    }

    override fun onSurfaceDestroyed() {
        Log.i(TAG, "onSurfaceDestroyed - pausing render thread")
        // Surface 销毁时，暂停渲染循环，避免继续渲染到已销毁的 Surface
        renderer?.pause()
    }

    override fun onRotation(deltaX: Float, deltaY: Float) {
        renderer?.setRotation(deltaX, deltaY)
    }

    // -------------------------------------------------------------------------
    // Internal
    // -------------------------------------------------------------------------
    private fun onRendererReady() {
        Log.i(TAG, "Renderer ready")
        // 如果 Surface 已经创建但还没初始化，现在初始化
        if (pendingSurface != null && !isInitialized) {
            Log.i(TAG, "Initializing with pending surface")
            val surface = pendingSurface
            val filesDir = resourcesDir ?: context.filesDir.absolutePath
            renderer?.init(surface, filesDir)
            isInitialized = true
            pendingSurface = null
        } else if (pendingSurface != null && isInitialized) {
            // 如果已经初始化过，重新链接 Surface
            Log.i(TAG, "Reconnecting pending surface")
            val surface = pendingSurface
            renderer?.reConnectSurface(surface)
            pendingSurface = null
        }
    }

    // -------------------------------------------------------------------------
    // Public API (delegates to Renderer)
    // -------------------------------------------------------------------------
    fun setViewport(width: Int, height: Int) {
        Log.i(TAG, "setViewport: ${width}x${height}")
        renderer?.setViewport(width, height)
    }

    fun setRotation(deltaX: Float, deltaY: Float) {
        renderer?.setRotation(deltaX, deltaY)
    }

    fun setScale(scale: Float) {
        renderer?.setScale(scale)
    }

    fun setPosition(x: Float, y: Float, z: Float) {
        renderer?.setPosition(x, y, z)
    }

    fun setBaseColor(r: Float, g: Float, b: Float) {
        renderer?.setBaseColor(r, g, b)
    }

    fun setMetallic(value: Float) {
        renderer?.setMetallic(value)
    }

    fun setRoughness(value: Float) {
        renderer?.setRoughness(value)
    }

    fun setAO(value: Float) {
        renderer?.setAO(value)
    }

    fun setCameraPosition(x: Float, y: Float, z: Float) {
        renderer?.setCameraPosition(x, y, z)
    }

    fun setCameraTarget(x: Float, y: Float, z: Float) {
        renderer?.setCameraTarget(x, y, z)
    }

    fun setTAAEnabled(enabled: Boolean) {
        renderer?.setTAAEnabled(enabled)
    }

    fun setBloomEnabled(enabled: Boolean) {
        renderer?.setBloomEnabled(enabled)
    }

    fun setBloomIntensity(intensity: Float) {
        renderer?.setBloomIntensity(intensity)
    }

    fun setTonemapEnabled(enabled: Boolean) {
        renderer?.setTonemapEnabled(enabled)
    }

    fun setTonemapOp(op: Int) {
        renderer?.setTonemapOp(op)
    }

    fun setExposure(exposure: Float) {
        renderer?.setExposure(exposure)
    }

    fun isInitialized(): Boolean = isInitialized
    fun isBound(): Boolean = isBound

    // -------------------------------------------------------------------------
    // Inspector 面板参数同步
    // -------------------------------------------------------------------------
    fun updateCameraParams(params: CameraParams) {
        renderer?.updateCameraParams(params)
    }

    fun updateSceneConfig(config: SceneConfig) {
        renderer?.updateSceneConfig(config)
    }

    fun updatePostProcessConfig(config: PostProcessConfig) {
        renderer?.updatePostProcessConfig(config)
    }

    fun updateMaterialParams(params: MaterialParams) {
        renderer?.updateMaterialParams(params)
    }

    fun getCameraParams(): CameraParams? = renderer?.cameraParams
    fun getSceneConfig(): SceneConfig? = renderer?.sceneConfig
    fun getPostProcessConfig(): PostProcessConfig? = renderer?.postProcessConfig
    fun getMaterialParams(): MaterialParams? = renderer?.materialParams

    fun getFPS(): Float = renderer?.fps ?: 0f
}
