package com.iauto.myapplication

import android.animation.Animator
import android.animation.ObjectAnimator
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.WindowManager
import androidx.appcompat.app.AppCompatActivity
import com.iauto.myapplication.manager.OglModelManager
import com.iauto.myapplication.view.OglSurfaceView
import com.iauto.myapplication.view.InspectorPanel
import com.iauto.myapplication.data.CameraParams
import com.iauto.myapplication.data.SceneConfig
import com.iauto.myapplication.data.PostProcessConfig
import com.iauto.myapplication.data.MaterialParams

/**
 * MainActivity
 * - 纯 UI 层，通过 OglModelManager 控制渲染
 * - 全屏沉浸式
 * - 集成 InspectorPanel 参数编辑器
 */
class MainActivity : AppCompatActivity(), InspectorPanel.Listener {

    private lateinit var surfaceView: OglSurfaceView
    private lateinit var inspectorPanel: InspectorPanel
    private lateinit var modelManager: OglModelManager
    private var resourcesDir: String? = null

    // FPS 更新定时器
    private val fpsHandler = Handler(Looper.getMainLooper())
    private val fpsRunnable = object : Runnable {
        override fun run() {
            updateFps()
            fpsHandler.postDelayed(this, 500)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // 全屏沉浸式
        window.setFlags(
            WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN
        )
        window.decorView.systemUiVisibility = (
            android.view.View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            or android.view.View.SYSTEM_UI_FLAG_FULLSCREEN
            or android.view.View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            or android.view.View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
            or android.view.View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            or android.view.View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        )

        setContentView(R.layout.activity_main)

        surfaceView = findViewById(R.id.surface_view)
        inspectorPanel = findViewById(R.id.inspector_panel)
        modelManager = OglModelManager(this)

        // 设置 InspectorPanel 监听器
        inspectorPanel.setListener(this)

        // 复制 assets 到 filesDir
        resourcesDir = copyAssetsToFilesDir()
        Log.i("MainActivity", "Resources copied to: $resourcesDir")

        // 连接 SurfaceView 和 Manager
        surfaceView.listener = modelManager

        // 绑定服务并传递资源路径
        modelManager.bind(resourcesDir)

        // 启动 FPS 更新
        fpsHandler.post(fpsRunnable)

        // 延迟同步 InspectorPanel，等待渲染引擎初始化完成
        android.os.Handler(Looper.getMainLooper()).postDelayed({
            syncInspectorPanel()
        }, 500)
    }

    override fun onResume() {
        super.onResume()
        // 可选：恢复时重新绑定
    }

    override fun onPause() {
        super.onPause()
        // 可选：暂停时保持服务运行
    }

    override fun onDestroy() {
        super.onDestroy()
        surfaceView.listener = null
        modelManager.unbind()
        resourcesDir = null
        fpsHandler.removeCallbacks(fpsRunnable)
    }

    // -------------------------------------------------------------------------
    // Copy assets to filesDir
    // -------------------------------------------------------------------------
    private fun copyAssetsToFilesDir(): String {
        val filesDir = this.filesDir
        copyAssetDir("texture", filesDir)
        copyAssetDir("shaders", filesDir)
        copyAssetDir("models", filesDir)
        return filesDir.absolutePath
    }

    private fun copyAssetDir(assetDir: String, destDir: java.io.File) {
        try {
            val list = assets.list(assetDir) ?: return
            if (list.isEmpty()) return
            val destSubDir = java.io.File(destDir, assetDir)
            if (!destSubDir.exists()) destSubDir.mkdirs()
            for (entry in list) {
                val fullAssetPath = "$assetDir/$entry"
                val destFile = java.io.File(destSubDir, entry)
                try {
                    val subList = assets.list(fullAssetPath)
                    if (subList != null && subList.isNotEmpty()) {
                        if (!destFile.exists()) destFile.mkdirs()
                        copyAssetDir(fullAssetPath, destDir)
                    } else {
                        copyAssetFile(fullAssetPath, destFile)
                    }
                } catch (e: Exception) {
                    Log.e("MainActivity", "Failed to process $fullAssetPath: ${e.message}")
                    copyAssetFile(fullAssetPath, destFile)
                }
            }
        } catch (e: Exception) {
            Log.e("MainActivity", "Failed to copy asset dir $assetDir: ${e.message}")
        }
    }

    private fun copyAssetFile(assetPath: String, destFile: java.io.File) {
        try {
            assets.open(assetPath).use { input ->
                java.io.FileOutputStream(destFile).use { output ->
                    input.copyTo(output)
                }
            }
        } catch (e: Exception) {
            Log.e("MainActivity", "Failed to copy asset $assetPath: ${e.message}")
        }
    }

    // -------------------------------------------------------------------------
    // 对外暴露的 API（如果其他组件需要控制渲染）
    // -------------------------------------------------------------------------
    fun getModelManager(): OglModelManager = modelManager

    // -------------------------------------------------------------------------
    // InspectorPanel.Listener implementation
    // -------------------------------------------------------------------------
    override fun onParameterChanged() {
        // 检查最后修改的是哪个参数类型，只更新该类型的参数
        // 这样可以避免更新其他未修改的参数（例如旋转相机后调整 TAA 不会重置相机位置）
        val lastModifiedParam = inspectorPanel.getLastModifiedParam()
        
        when (lastModifiedParam) {
            InspectorPanel.ParamType.CAMERA -> {
                val cameraParams = inspectorPanel.getCurrentCameraParams()
                modelManager.updateCameraParams(cameraParams)
            }
            InspectorPanel.ParamType.SCENE -> {
                val sceneConfig = inspectorPanel.getCurrentSceneConfig()
                modelManager.updateSceneConfig(sceneConfig)
            }
            InspectorPanel.ParamType.POST_PROCESS -> {
                val postProcessConfig = inspectorPanel.getCurrentPostProcessConfig()
                modelManager.updatePostProcessConfig(postProcessConfig)
            }
            InspectorPanel.ParamType.MATERIAL -> {
                val materialParams = inspectorPanel.getCurrentMaterialParams()
                modelManager.updateMaterialParams(materialParams)
            }
            null -> {
                // 如果没有记录修改类型，更新所有参数（首次初始化等情况）
                val cameraParams = inspectorPanel.getCurrentCameraParams()
                val sceneConfig = inspectorPanel.getCurrentSceneConfig()
                val postProcessConfig = inspectorPanel.getCurrentPostProcessConfig()
                val materialParams = inspectorPanel.getCurrentMaterialParams()

                modelManager.updateCameraParams(cameraParams)
                modelManager.updateSceneConfig(sceneConfig)
                modelManager.updatePostProcessConfig(postProcessConfig)
                modelManager.updateMaterialParams(materialParams)
            }
        }
    }

    // -------------------------------------------------------------------------
    // 同步面板显示到当前值
    // -------------------------------------------------------------------------
    private fun syncInspectorPanel() {
        // 从渲染引擎获取当前参数并更新面板显示
        modelManager.getCameraParams()?.let {
            inspectorPanel.setCameraParams(it)
        }
        modelManager.getSceneConfig()?.let {
            inspectorPanel.setSceneConfig(it)
        }
        modelManager.getPostProcessConfig()?.let {
            inspectorPanel.setPostProcessConfig(it)
        }
        modelManager.getMaterialParams()?.let {
            inspectorPanel.setMaterialParams(it)
        }
    }

    // -------------------------------------------------------------------------
    // FPS 更新
    // -------------------------------------------------------------------------
    private fun updateFps() {
        val fps = modelManager.getFPS()
        if (fps > 0) {
            inspectorPanel.updateFps(fps)
        }
    }
}
