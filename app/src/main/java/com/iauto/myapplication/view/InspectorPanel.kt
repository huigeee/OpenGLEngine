package com.iauto.myapplication.view

import android.content.Context
import android.util.AttributeSet
import android.view.LayoutInflater
import android.view.MotionEvent
import android.view.View
import android.view.ViewParent
import android.widget.AdapterView
import android.widget.Spinner
import android.widget.TextView
import android.widget.FrameLayout
import android.widget.LinearLayout
import com.google.android.material.slider.Slider
import com.google.android.material.switchmaterial.SwitchMaterial
import com.iauto.myapplication.R
import com.iauto.myapplication.data.CameraParams
import com.iauto.myapplication.data.SceneConfig
import com.iauto.myapplication.data.PostProcessConfig
import com.iauto.myapplication.data.MaterialParams
import com.iauto.myapplication.data.Vec3

/**
 * Inspector 面板 - 类似 Unity 的属性编辑器
 * 可以拖拽到屏幕边缘，包含相机、场景、后处理、材质等参数配置
 */
class InspectorPanel @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : FrameLayout(context, attrs, defStyleAttr) {

    // 回调接口 - 必须放在最前面，避免前向引用问题
    interface Listener {
        fun onParameterChanged()
    }
    
    // 记录上次用户修改的参数类型
    enum class ParamType { CAMERA, SCENE, POST_PROCESS, MATERIAL }
    
    private var listener: Listener? = null
    private var isDragging = false
    private var dragStartX = 0f
    private var dragStartY = 0f
    private var viewStartX = 0f
    private var lastModifiedParam: ParamType? = null
    
    private lateinit var scrollContent: androidx.core.widget.NestedScrollView
    
    companion object {
        private const val TAG = "InspectorPanel"
    }

    // 相机参数
    private lateinit var cameraPosX: Slider
    private lateinit var cameraPosY: Slider
    private lateinit var cameraPosZ: Slider
    private lateinit var cameraTargetX: Slider
    private lateinit var cameraTargetY: Slider
    private lateinit var cameraTargetZ: Slider
    private lateinit var cameraFov: Slider
    private lateinit var cameraOrthographic: SwitchMaterial

    // 相机数值显示
    private lateinit var cameraPosXVal: TextView
    private lateinit var cameraPosYVal: TextView
    private lateinit var cameraPosZVal: TextView
    private lateinit var cameraTargetXVal: TextView
    private lateinit var cameraTargetYVal: TextView
    private lateinit var cameraTargetZVal: TextView
    private lateinit var cameraFovVal: TextView

    // 场景参数
    private lateinit var sceneBgR: Slider
    private lateinit var sceneBgG: Slider
    private lateinit var sceneBgB: Slider
    private lateinit var sceneAmbient: Slider
    private lateinit var sceneShadow: SwitchMaterial
    private lateinit var sceneLightPosX: Slider
    private lateinit var sceneLightPosY: Slider
    private lateinit var sceneLightPosZ: Slider
    private lateinit var sceneShadowBias: Slider
    private lateinit var sceneShadowSoft: SwitchMaterial
    private lateinit var sceneSsao: SwitchMaterial

    // 场景数值显示
    private lateinit var sceneBgRVal: TextView
    private lateinit var sceneBgGVal: TextView
    private lateinit var sceneBgBVal: TextView
    private lateinit var sceneAmbientVal: TextView
    private lateinit var sceneLightPosXVal: TextView
    private lateinit var sceneLightPosYVal: TextView
    private lateinit var sceneLightPosZVal: TextView
    private lateinit var sceneShadowBiasVal: TextView

    // 后处理参数
    private lateinit var ppAaMode: Spinner
    private lateinit var ppBloom: SwitchMaterial
    private lateinit var ppBloomIntensity: Slider
    private lateinit var ppTonemap: SwitchMaterial
    private lateinit var ppTonemapOp: Spinner
    private lateinit var ppExposure: Slider
    
    // Fog 参数
    private lateinit var ppFog: SwitchMaterial
    private lateinit var ppFogMode: Spinner
    private lateinit var ppFogDensity: Slider
    private lateinit var ppFogStart: Slider
    private lateinit var ppFogEnd: Slider
    
    // DoF 参数
    private lateinit var ppDof: SwitchMaterial
    private lateinit var ppDofFocusDist: Slider
    private lateinit var ppDofNear: Slider
    private lateinit var ppDofFar: Slider

    // Volumetric Light 参数
    private lateinit var ppVolumetric: SwitchMaterial
    private lateinit var ppVolDensity: Slider
    private lateinit var ppVolScattering: Slider
    private lateinit var ppVolSteps: Slider
    private lateinit var ppVolIntensity: Slider
    private lateinit var ppVolDensityVal: TextView
    private lateinit var ppVolScatteringVal: TextView
    private lateinit var ppVolStepsVal: TextView
    private lateinit var ppVolIntensityVal: TextView

    // 后处理数值显示
    private lateinit var ppBloomIntensityVal: TextView
    private lateinit var ppExposureVal: TextView

    // 材质参数
    private lateinit var matBaseR: Slider
    private lateinit var matBaseG: Slider
    private lateinit var matBaseB: Slider
    private lateinit var matMetallic: Slider
    private lateinit var matRoughness: Slider
    private lateinit var matAo: Slider

    // 材质数值显示
    private lateinit var matBaseRVal: TextView
    private lateinit var matBaseGVal: TextView
    private lateinit var matBaseBVal: TextView
    private lateinit var matMetallicVal: TextView
    private lateinit var matRoughnessVal: TextView
    private lateinit var matAoVal: TextView

    // 统计信息
    private lateinit var statsFps: TextView
    private lateinit var fpsContainer: LinearLayout
    
    init {
        LayoutInflater.from(context).inflate(R.layout.inspector_panel, this, true)
        initViews()
        setupListeners()
        
        // 设置半透明背景色（因为是 merge，需要在代码中设置）
        setBackgroundColor(0xCC1E1E1E.toInt()) // 深灰色半透明
        
        // 启用触摸事件处理
        isClickable = true
        isFocusable = false
        isFocusableInTouchMode = false
        
        // 设置 FPS 显示颜色
        statsFps.setTextColor(0xFF00FF00.toInt()) // 绿色
    }

    private fun initViews() {
        // 获取 NestedScrollView
        scrollContent = findViewById(R.id.scroll_content)
        
        // FPS 显示
        fpsContainer = findViewById(R.id.fps_container)
        statsFps = findViewById(R.id.stats_fps)
        
        // 相机
        cameraPosX = findViewById(R.id.camera_pos_x)
        cameraPosY = findViewById(R.id.camera_pos_y)
        cameraPosZ = findViewById(R.id.camera_pos_z)
        cameraTargetX = findViewById(R.id.camera_target_x)
        cameraTargetY = findViewById(R.id.camera_target_y)
        cameraTargetZ = findViewById(R.id.camera_target_z)
        cameraFov = findViewById(R.id.camera_fov)
        cameraOrthographic = findViewById(R.id.camera_orthographic)

        cameraPosXVal = findViewById(R.id.camera_pos_x_val)
        cameraPosYVal = findViewById(R.id.camera_pos_y_val)
        cameraPosZVal = findViewById(R.id.camera_pos_z_val)
        cameraTargetXVal = findViewById(R.id.camera_target_x_val)
        cameraTargetYVal = findViewById(R.id.camera_target_y_val)
        cameraTargetZVal = findViewById(R.id.camera_target_z_val)
        cameraFovVal = findViewById(R.id.camera_fov_val)

        // 场景
        sceneBgR = findViewById(R.id.scene_bg_r)
        sceneBgG = findViewById(R.id.scene_bg_g)
        sceneBgB = findViewById(R.id.scene_bg_b)
        sceneAmbient = findViewById(R.id.scene_ambient)
        sceneShadow = findViewById(R.id.scene_shadow)
        sceneLightPosX = findViewById(R.id.scene_light_pos_x)
        sceneLightPosY = findViewById(R.id.scene_light_pos_y)
        sceneLightPosZ = findViewById(R.id.scene_light_pos_z)
        sceneShadowBias = findViewById(R.id.scene_shadow_bias)
        sceneShadowSoft = findViewById(R.id.scene_shadow_soft)
        sceneSsao = findViewById(R.id.scene_ssao)

        sceneBgRVal = findViewById(R.id.scene_bg_r_val)
        sceneBgGVal = findViewById(R.id.scene_bg_g_val)
        sceneBgBVal = findViewById(R.id.scene_bg_b_val)
        sceneAmbientVal = findViewById(R.id.scene_ambient_val)
        sceneLightPosXVal = findViewById(R.id.scene_light_pos_x_val)
        sceneLightPosYVal = findViewById(R.id.scene_light_pos_y_val)
        sceneLightPosZVal = findViewById(R.id.scene_light_pos_z_val)
        sceneShadowBiasVal = findViewById(R.id.scene_shadow_bias_val)

        // 后处理
        ppAaMode = findViewById(R.id.pp_aa_mode)
        ppBloom = findViewById(R.id.pp_bloom)
        ppBloomIntensity = findViewById(R.id.pp_bloom_intensity)
        ppTonemap = findViewById(R.id.pp_tonemap)
        ppTonemapOp = findViewById(R.id.pp_tonemap_op)
        ppExposure = findViewById(R.id.pp_exposure)
        
        // Fog
        ppFog = findViewById(R.id.pp_fog)
        ppFogMode = findViewById(R.id.pp_fog_mode)
        ppFogDensity = findViewById(R.id.pp_fog_density)
        ppFogStart = findViewById(R.id.pp_fog_start)
        ppFogEnd = findViewById(R.id.pp_fog_end)
        
        // DoF
        ppDof = findViewById(R.id.pp_dof)
        ppDofFocusDist = findViewById(R.id.pp_dof_focus_dist)
        ppDofNear = findViewById(R.id.pp_dof_near)
        ppDofFar = findViewById(R.id.pp_dof_far)

        // Volumetric Light
        ppVolumetric = findViewById(R.id.pp_volumetric)
        ppVolDensity = findViewById(R.id.pp_vol_density)
        ppVolScattering = findViewById(R.id.pp_vol_scattering)
        ppVolSteps = findViewById(R.id.pp_vol_steps)
        ppVolIntensity = findViewById(R.id.pp_vol_intensity)
        ppVolDensityVal = findViewById(R.id.pp_vol_density_val)
        ppVolScatteringVal = findViewById(R.id.pp_vol_scattering_val)
        ppVolStepsVal = findViewById(R.id.pp_vol_steps_val)
        ppVolIntensityVal = findViewById(R.id.pp_vol_intensity_val)

        ppBloomIntensityVal = findViewById(R.id.pp_bloom_intensity_val)
        ppExposureVal = findViewById(R.id.pp_exposure_val)

        // 材质
        matBaseR = findViewById(R.id.mat_base_r)
        matBaseG = findViewById(R.id.mat_base_g)
        matBaseB = findViewById(R.id.mat_base_b)
        matMetallic = findViewById(R.id.mat_metallic)
        matRoughness = findViewById(R.id.mat_roughness)
        matAo = findViewById(R.id.mat_ao)

        matBaseRVal = findViewById(R.id.mat_base_r_val)
        matBaseGVal = findViewById(R.id.mat_base_g_val)
        matBaseBVal = findViewById(R.id.mat_base_b_val)
        matMetallicVal = findViewById(R.id.mat_metallic_val)
        matRoughnessVal = findViewById(R.id.mat_roughness_val)
        matAoVal = findViewById(R.id.mat_ao_val)

        // 统计
        statsFps = findViewById(R.id.stats_fps)
    }

    private fun setupListeners() {
        // 相机位置
        cameraPosX.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                cameraPosXVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.CAMERA
                listener?.onParameterChanged()
            }
        }
        cameraPosY.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                cameraPosYVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.CAMERA
                listener?.onParameterChanged()
            }
        }
        cameraPosZ.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                cameraPosZVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.CAMERA
                listener?.onParameterChanged()
            }
        }

        // 相机目标
        cameraTargetX.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                cameraTargetXVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.CAMERA
                listener?.onParameterChanged()
            }
        }
        cameraTargetY.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                cameraTargetYVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.CAMERA
                listener?.onParameterChanged()
            }
        }
        cameraTargetZ.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                cameraTargetZVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.CAMERA
                listener?.onParameterChanged()
            }
        }

        // 相机 FOV
        cameraFov.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                cameraFovVal.text = String.format("%.1f", value)
                lastModifiedParam = ParamType.CAMERA
                listener?.onParameterChanged()
            }
        }

        // 相机正交
        cameraOrthographic.setOnCheckedChangeListener { _, _ ->
            lastModifiedParam = ParamType.CAMERA
            listener?.onParameterChanged()
        }

        // 场景背景色
        sceneBgR.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                sceneBgRVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.SCENE
                listener?.onParameterChanged()
            }
        }
        sceneBgG.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                sceneBgGVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.SCENE
                listener?.onParameterChanged()
            }
        }
        sceneBgB.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                sceneBgBVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.SCENE
                listener?.onParameterChanged()
            }
        }

        // 场景环境光
        sceneAmbient.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                sceneAmbientVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.SCENE
                listener?.onParameterChanged()
            }
        }

        // 场景开关
        sceneShadow.setOnCheckedChangeListener { _, _ ->
            lastModifiedParam = ParamType.SCENE
            listener?.onParameterChanged()
        }
        
        // 光源位置
        sceneLightPosX.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                sceneLightPosXVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.SCENE
                listener?.onParameterChanged()
            }
        }
        sceneLightPosY.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                sceneLightPosYVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.SCENE
                listener?.onParameterChanged()
            }
        }
        sceneLightPosZ.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                sceneLightPosZVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.SCENE
                listener?.onParameterChanged()
            }
        }
        
        // 阴影质量
        sceneShadowBias.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                sceneShadowBiasVal.text = String.format("%.3f", value)
                lastModifiedParam = ParamType.SCENE
                listener?.onParameterChanged()
            }
        }
        sceneShadowSoft.setOnCheckedChangeListener { _, _ ->
            lastModifiedParam = ParamType.SCENE
            listener?.onParameterChanged()
        }
        sceneSsao.setOnCheckedChangeListener { _, _ ->
            lastModifiedParam = ParamType.SCENE
            listener?.onParameterChanged()
        }

        // 后处理
        ppAaMode.onItemSelectedListener = object : android.widget.AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: android.widget.AdapterView<*>?, view: View?, position: Int, id: Long) {
                lastModifiedParam = ParamType.POST_PROCESS
                listener?.onParameterChanged()
            }
            override fun onNothingSelected(parent: android.widget.AdapterView<*>?) {}
        }
        ppBloom.setOnCheckedChangeListener { _, _ ->
            lastModifiedParam = ParamType.POST_PROCESS
            listener?.onParameterChanged()
        }

        ppBloomIntensity.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                ppBloomIntensityVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.POST_PROCESS
                listener?.onParameterChanged()
            }
        }

        ppTonemap.setOnCheckedChangeListener { _, _ ->
            lastModifiedParam = ParamType.POST_PROCESS
            listener?.onParameterChanged()
        }

        ppTonemapOp.onItemSelectedListener = object : android.widget.AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: android.widget.AdapterView<*>?, view: View?, position: Int, id: Long) {
                lastModifiedParam = ParamType.POST_PROCESS
                listener?.onParameterChanged()
            }
            override fun onNothingSelected(parent: android.widget.AdapterView<*>?) {}
        }

        ppExposure.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                ppExposureVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.POST_PROCESS
                listener?.onParameterChanged()
            }
        }
        
        // Fog
        ppFog.setOnCheckedChangeListener { _, _ ->
            lastModifiedParam = ParamType.POST_PROCESS
            listener?.onParameterChanged()
        }
        
        ppFogMode.onItemSelectedListener = object : android.widget.AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: android.widget.AdapterView<*>?, view: View?, position: Int, id: Long) {
                lastModifiedParam = ParamType.POST_PROCESS
                listener?.onParameterChanged()
            }
            override fun onNothingSelected(parent: android.widget.AdapterView<*>?) {}
        }
        
        ppFogDensity.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                lastModifiedParam = ParamType.POST_PROCESS
                listener?.onParameterChanged()
            }
        }
        
        ppFogStart.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                lastModifiedParam = ParamType.POST_PROCESS
                listener?.onParameterChanged()
            }
        }
        
        ppFogEnd.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                lastModifiedParam = ParamType.POST_PROCESS
                listener?.onParameterChanged()
            }
        }
        
        // DoF
        ppDof.setOnCheckedChangeListener { _, _ ->
            lastModifiedParam = ParamType.POST_PROCESS
            listener?.onParameterChanged()
        }
        
        ppDofFocusDist.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                lastModifiedParam = ParamType.POST_PROCESS
                listener?.onParameterChanged()
            }
        }
        
        ppDofNear.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                lastModifiedParam = ParamType.POST_PROCESS
                listener?.onParameterChanged()
            }
        }
        
        ppDofFar.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                lastModifiedParam = ParamType.POST_PROCESS
                listener?.onParameterChanged()
            }
        }

        // Volumetric Light
        ppVolumetric.setOnCheckedChangeListener { _, _ ->
            lastModifiedParam = ParamType.POST_PROCESS
            listener?.onParameterChanged()
        }
        ppVolDensity.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                ppVolDensityVal.text = String.format("%.3f", value)
                lastModifiedParam = ParamType.POST_PROCESS
                listener?.onParameterChanged()
            }
        }
        ppVolScattering.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                ppVolScatteringVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.POST_PROCESS
                listener?.onParameterChanged()
            }
        }
        ppVolSteps.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                ppVolStepsVal.text = value.toInt().toString()
                lastModifiedParam = ParamType.POST_PROCESS
                listener?.onParameterChanged()
            }
        }
        ppVolIntensity.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                ppVolIntensityVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.POST_PROCESS
                listener?.onParameterChanged()
            }
        }

        // 材质
        matBaseR.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                matBaseRVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.MATERIAL
                listener?.onParameterChanged()
            }
        }
        matBaseG.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                matBaseGVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.MATERIAL
                listener?.onParameterChanged()
            }
        }
        matBaseB.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                matBaseBVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.MATERIAL
                listener?.onParameterChanged()
            }
        }

        matMetallic.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                matMetallicVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.MATERIAL
                listener?.onParameterChanged()
            }
        }

        matRoughness.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                matRoughnessVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.MATERIAL
                listener?.onParameterChanged()
            }
        }

        matAo.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                matAoVal.text = String.format("%.2f", value)
                lastModifiedParam = ParamType.MATERIAL
                listener?.onParameterChanged()
            }
        }
    }

    // -------------------------------------------------------------------------
    // 触摸事件处理 - 只在拖动把手上时才消费事件
    // -------------------------------------------------------------------------
    private fun isTouchOnDragHandle(event: MotionEvent): Boolean {
        val dragHandle: View = findViewById(R.id.drag_handle)
        val location = IntArray(2)
        dragHandle.getLocationOnScreen(location)
        val dragHandleLeft = location[0]
        val dragHandleTop = location[1]
        val dragHandleRight = dragHandleLeft + dragHandle.width
        val dragHandleBottom = dragHandleTop + dragHandle.height

        return event.rawX >= dragHandleLeft && event.rawX <= dragHandleRight &&
               event.rawY >= dragHandleTop && event.rawY <= dragHandleBottom
    }
    
    override fun onInterceptTouchEvent(ev: MotionEvent?): Boolean {
        if (ev == null) return false
        
        // 只在拖动把手上时才拦截事件
        if (ev.action == MotionEvent.ACTION_DOWN && isTouchOnDragHandle(ev)) {
            isDragging = true
            dragStartX = ev.rawX
            dragStartY = ev.rawY
            val viewLocation = IntArray(2)
            getLocationOnScreen(viewLocation)
            viewStartX = viewLocation[0].toFloat()
            return true
        }
        
        // 不拦截，让事件继续传递
        return false
    }
    
    override fun onTouchEvent(event: MotionEvent): Boolean {
        if (!isDragging) {
            return false
        }
        
        if (event.action == MotionEvent.ACTION_MOVE) {
            val deltaX = event.rawX - dragStartX
            val newPosX = viewStartX + deltaX
            val parentView: ViewParent = parent
            val parentWidth = if (parentView is View) parentView.width else 1000
            val viewWidth = width
            val clampedX = newPosX.coerceIn(0f, (parentWidth - viewWidth).toFloat())
            x = clampedX
            return true
        } else if (event.action == MotionEvent.ACTION_UP || event.action == MotionEvent.ACTION_CANCEL) {
            isDragging = false
            return true
        }
        
        return false
    }

    // -------------------------------------------------------------------------
    // 设置参数
    // -------------------------------------------------------------------------
    fun setCameraParams(params: CameraParams) {
        cameraPosX.value = params.position.x
        cameraPosY.value = params.position.y
        cameraPosZ.value = params.position.z
        cameraTargetX.value = params.target.x
        cameraTargetY.value = params.target.y
        cameraTargetZ.value = params.target.z
        cameraFov.value = params.fov
        cameraOrthographic.isChecked = params.orthographic

        cameraPosXVal.text = String.format("%.2f", params.position.x)
        cameraPosYVal.text = String.format("%.2f", params.position.y)
        cameraPosZVal.text = String.format("%.2f", params.position.z)
        cameraTargetXVal.text = String.format("%.2f", params.target.x)
        cameraTargetYVal.text = String.format("%.2f", params.target.y)
        cameraTargetZVal.text = String.format("%.2f", params.target.z)
        cameraFovVal.text = String.format("%.1f", params.fov)
    }

    fun setSceneConfig(config: SceneConfig) {
        sceneBgR.value = config.bgColor.x
        sceneBgG.value = config.bgColor.y
        sceneBgB.value = config.bgColor.z
        sceneAmbient.value = config.ambientIntensity
        sceneShadow.isChecked = config.shadowEnabled
        // Shadow light position
        sceneLightPosX.value = config.lightPosX
        sceneLightPosY.value = config.lightPosY
        sceneLightPosZ.value = config.lightPosZ
        // Shadow quality
        sceneShadowBias.value = config.shadowBias
        sceneShadowSoft.isChecked = config.shadowPCFEnabled
        sceneSsao.isChecked = config.ssaoEnabled

        sceneBgRVal.text = String.format("%.2f", config.bgColor.x)
        sceneBgGVal.text = String.format("%.2f", config.bgColor.y)
        sceneBgBVal.text = String.format("%.2f", config.bgColor.z)
        sceneAmbientVal.text = String.format("%.2f", config.ambientIntensity)
        sceneLightPosXVal.text = String.format("%.2f", config.lightPosX)
        sceneLightPosYVal.text = String.format("%.2f", config.lightPosY)
        sceneLightPosZVal.text = String.format("%.2f", config.lightPosZ)
        sceneShadowBiasVal.text = String.format("%.3f", config.shadowBias)
    }

    fun setPostProcessConfig(config: PostProcessConfig) {
        ppAaMode.setSelection(config.aaMode)
        ppBloom.isChecked = config.bloomEnabled
        ppBloomIntensity.value = config.bloomIntensity
        ppTonemap.isChecked = config.tonemapEnabled
        ppTonemapOp.setSelection(config.tonemapOp)
        ppExposure.value = config.exposure
        
        // Fog
        ppFog.isChecked = config.fogEnabled
        ppFogMode.setSelection(config.fogMode)
        ppFogDensity.value = config.fogDensity
        ppFogStart.value = config.fogStart
        ppFogEnd.value = config.fogEnd
        
        // DoF
        ppDof.isChecked = config.dofEnabled
        ppDofFocusDist.value = config.dofFocusDist
        ppDofNear.value = config.dofNear
        ppDofFar.value = config.dofFar

        // Volumetric Light
        ppVolumetric.isChecked = config.volumetricEnabled
        ppVolDensity.value = config.volumetricDensity
        ppVolScattering.value = config.volumetricScattering
        ppVolSteps.value = config.volumetricSteps.toFloat().coerceIn(8f, 128f)
        ppVolIntensity.value = config.volumetricIntensity
        ppVolDensityVal.text = String.format("%.3f", config.volumetricDensity)
        ppVolScatteringVal.text = String.format("%.2f", config.volumetricScattering)
        ppVolStepsVal.text = config.volumetricSteps.toString()
        ppVolIntensityVal.text = String.format("%.2f", config.volumetricIntensity)

        ppBloomIntensityVal.text = String.format("%.2f", config.bloomIntensity)
        ppExposureVal.text = String.format("%.2f", config.exposure)
    }

    fun setMaterialParams(params: MaterialParams) {
        matBaseR.value = params.baseColor.x
        matBaseG.value = params.baseColor.y
        matBaseB.value = params.baseColor.z
        matMetallic.value = params.metallic
        matRoughness.value = params.roughness
        matAo.value = params.ao

        matBaseRVal.text = String.format("%.2f", params.baseColor.x)
        matBaseGVal.text = String.format("%.2f", params.baseColor.y)
        matBaseBVal.text = String.format("%.2f", params.baseColor.z)
        matMetallicVal.text = String.format("%.2f", params.metallic)
        matRoughnessVal.text = String.format("%.2f", params.roughness)
        matAoVal.text = String.format("%.2f", params.ao)
    }

    fun updateFps(fps: Float) {
        statsFps.text = String.format("FPS: %.1f", fps)
        
        // 根据 FPS 值改变颜色
        val color = when {
            fps >= 55 -> 0xFF00FF00.toInt()  // 绿色 (优秀)
            fps >= 30 -> 0xFFFFFF00.toInt()  // 黄色 (良好)
            else -> 0xFFFF0000.toInt()       // 红色 (差)
        }
        statsFps.setTextColor(color)
    }

    // -------------------------------------------------------------------------
    // 获取当前参数
    // -------------------------------------------------------------------------
    fun getCurrentCameraParams(): CameraParams {
        return CameraParams(
            position = Vec3(cameraPosX.value, cameraPosY.value, cameraPosZ.value),
            target = Vec3(cameraTargetX.value, cameraTargetY.value, cameraTargetZ.value),
            fov = cameraFov.value,
            orthographic = cameraOrthographic.isChecked
        )
    }

    fun getCurrentSceneConfig(): SceneConfig {
        return SceneConfig(
            bgColor = Vec3(sceneBgR.value, sceneBgG.value, sceneBgB.value),
            ambientIntensity = sceneAmbient.value,
            shadowEnabled = sceneShadow.isChecked,
            lightPosX = sceneLightPosX.value,
            lightPosY = sceneLightPosY.value,
            lightPosZ = sceneLightPosZ.value,
            shadowSoftness = if (sceneShadowSoft.isChecked) 0.5f else 0f,
            shadowBias = sceneShadowBias.value,
            shadowPCFEnabled = sceneShadowSoft.isChecked,
            ssaoEnabled = sceneSsao.isChecked
        )
    }

    fun getCurrentPostProcessConfig(): PostProcessConfig {
        return PostProcessConfig(
            aaMode = ppAaMode.selectedItemPosition,
            bloomEnabled = ppBloom.isChecked,
            bloomIntensity = ppBloomIntensity.value,
            tonemapEnabled = ppTonemap.isChecked,
            tonemapOp = ppTonemapOp.selectedItemPosition,
            exposure = ppExposure.value,
            // Fog
            fogEnabled = ppFog.isChecked,
            fogMode = ppFogMode.selectedItemPosition,
            fogDensity = ppFogDensity.value,
            fogStart = ppFogStart.value,
            fogEnd = ppFogEnd.value,
            fogR = 0.7f,
            fogG = 0.75f,
            fogB = 0.8f,
            // DoF
            dofEnabled = ppDof.isChecked,
            dofFocusDist = ppDofFocusDist.value,
            dofNear = ppDofNear.value,
            dofFar = ppDofFar.value,
            // Volumetric Light
            volumetricEnabled = ppVolumetric.isChecked,
            volumetricDensity = ppVolDensity.value,
            volumetricScattering = ppVolScattering.value,
            volumetricSteps = ppVolSteps.value.toInt(),
            volumetricIntensity = ppVolIntensity.value
        )
    }

    fun getCurrentMaterialParams(): MaterialParams {
        return MaterialParams(
            baseColor = Vec3(matBaseR.value, matBaseG.value, matBaseB.value),
            metallic = matMetallic.value,
            roughness = matRoughness.value,
            ao = matAo.value
        )
    }

    fun setListener(listener: Listener?) {
        this.listener = listener
    }
    
    // 获取最后修改的参数类型，用于决定只更新哪个参数
    fun getLastModifiedParam(): ParamType? = lastModifiedParam
}
