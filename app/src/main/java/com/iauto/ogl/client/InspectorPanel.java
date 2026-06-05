package com.iauto.ogl.client;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewParent;
import android.widget.AdapterView;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TextView;
import androidx.core.widget.NestedScrollView;
import com.google.android.material.slider.Slider;
import com.google.android.material.switchmaterial.SwitchMaterial;
import com.iauto.ogl.R;
import com.iauto.ogl.data.CameraParams;
import com.iauto.ogl.data.MaterialParams;
import com.iauto.ogl.data.PostProcessConfig;
import com.iauto.ogl.data.SceneConfig;
import com.iauto.ogl.data.Vec3;

public class InspectorPanel extends FrameLayout implements RenderService.Callback {

    public interface Listener {
        void onParameterChanged();
    }

    public enum ParamType { CAMERA, SCENE, POST_PROCESS, MATERIAL }

    private static final String TAG = "InspectorPanel";

    private Listener listener;
    private RenderService renderService;
    private boolean isSyncing = false;
    private boolean isDragging = false;
    private float dragStartX = 0f;
    private float dragStartY = 0f;
    private float viewStartX = 0f;
    private ParamType lastModifiedParam = null;

    private NestedScrollView scrollContent;

    // �������
    private Slider cameraPosX, cameraPosY, cameraPosZ;
    private Slider cameraTargetX, cameraTargetY, cameraTargetZ;
    private Slider cameraFov;
    private SwitchMaterial cameraOrthographic;

    private TextView cameraPosXVal, cameraPosYVal, cameraPosZVal;
    private TextView cameraTargetXVal, cameraTargetYVal, cameraTargetZVal;
    private TextView cameraFovVal;

    // ��������
    private Slider sceneBgR, sceneBgG, sceneBgB;
    private Slider sceneAmbient;
    private SwitchMaterial sceneShadow;
    private Slider sceneLightPosX, sceneLightPosY, sceneLightPosZ;
    private Slider sceneShadowBias;
    private SwitchMaterial sceneShadowSoft, sceneSsao;

    private TextView sceneBgRVal, sceneBgGVal, sceneBgBVal;
    private TextView sceneAmbientVal;
    private TextView sceneLightPosXVal, sceneLightPosYVal, sceneLightPosZVal;
    private TextView sceneShadowBiasVal;

    // ��������
    private Spinner ppAaMode;
    private SwitchMaterial ppBloom;
    private Slider ppBloomIntensity;
    private SwitchMaterial ppTonemap;
    private Spinner ppTonemapOp;
    private Slider ppExposure;

    private SwitchMaterial ppFog;
    private Spinner ppFogMode;
    private Slider ppFogDensity, ppFogStart, ppFogEnd;

    private SwitchMaterial ppDof;
    private Slider ppDofFocusDist, ppDofNear, ppDofFar;

    private SwitchMaterial ppVolumetric;
    private Slider ppVolDensity, ppVolScattering, ppVolSteps, ppVolIntensity;
    private TextView ppVolDensityVal, ppVolScatteringVal, ppVolStepsVal, ppVolIntensityVal;

    private TextView ppBloomIntensityVal, ppExposureVal;

    // ���ʲ���
    private Slider matBaseR, matBaseG, matBaseB;
    private Slider matMetallic, matRoughness, matAo;

    private TextView matBaseRVal, matBaseGVal, matBaseBVal;
    private TextView matMetallicVal, matRoughnessVal, matAoVal;

    // ͳ����Ϣ
    private TextView statsFps;
    private LinearLayout fpsContainer;

    public InspectorPanel(Context context) {
        this(context, null, 0);
    }

    public InspectorPanel(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public InspectorPanel(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        LayoutInflater.from(context).inflate(R.layout.inspector_panel, this, true);
        initViews();
        setupListeners();
        setBackgroundColor(0xCC1E1E1E);
        setClickable(true);
        setFocusable(false);
        setFocusableInTouchMode(false);
        statsFps.setTextColor(0xFF00FF00);
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        renderService = RenderService.init(getContext());
        renderService.addCallback(this);
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        if (renderService != null) {
            renderService.removeCallback(this);
            renderService = null;
        }
    }

    @Override
    public void onServiceConnected() {
        syncFromEngine();
    }

    @Override
    public void onServiceDisconnected() {}

    private void syncToRenderService(ParamType type) {
        if (renderService == null) return;
        switch (type) {
            case CAMERA:       renderService.updateCameraParams(getCurrentCameraParams()); break;
            case SCENE:        renderService.updateSceneConfig(getCurrentSceneConfig()); break;
            case POST_PROCESS: renderService.updatePostProcessConfig(getCurrentPostProcessConfig()); break;
            case MATERIAL:     renderService.updateMaterialParams(getCurrentMaterialParams()); break;
        }
    }

    private void notifyChanged() {
        if (isSyncing) return;
        if (lastModifiedParam != null) syncToRenderService(lastModifiedParam);
    }

    public void syncFromEngine() {
        if (renderService == null) return;
        isSyncing = true;
        CameraParams cp = renderService.getCameraParams();
        if (cp != null) setCameraParams(cp);
        SceneConfig sc = renderService.getSceneConfig();
        if (sc != null) setSceneConfig(sc);
        PostProcessConfig pp = renderService.getPostProcessConfig();
        if (pp != null) setPostProcessConfig(pp);
        MaterialParams mp = renderService.getMaterialParams();
        if (mp != null) setMaterialParams(mp);
        isSyncing = false;
    }

    private void initViews() {
        scrollContent = findViewById(R.id.scroll_content);
        fpsContainer = findViewById(R.id.fps_container);
        statsFps = findViewById(R.id.stats_fps);

        cameraPosX = findViewById(R.id.camera_pos_x);
        cameraPosY = findViewById(R.id.camera_pos_y);
        cameraPosZ = findViewById(R.id.camera_pos_z);
        cameraTargetX = findViewById(R.id.camera_target_x);
        cameraTargetY = findViewById(R.id.camera_target_y);
        cameraTargetZ = findViewById(R.id.camera_target_z);
        cameraFov = findViewById(R.id.camera_fov);
        cameraOrthographic = findViewById(R.id.camera_orthographic);

        cameraPosXVal = findViewById(R.id.camera_pos_x_val);
        cameraPosYVal = findViewById(R.id.camera_pos_y_val);
        cameraPosZVal = findViewById(R.id.camera_pos_z_val);
        cameraTargetXVal = findViewById(R.id.camera_target_x_val);
        cameraTargetYVal = findViewById(R.id.camera_target_y_val);
        cameraTargetZVal = findViewById(R.id.camera_target_z_val);
        cameraFovVal = findViewById(R.id.camera_fov_val);

        sceneBgR = findViewById(R.id.scene_bg_r);
        sceneBgG = findViewById(R.id.scene_bg_g);
        sceneBgB = findViewById(R.id.scene_bg_b);
        sceneAmbient = findViewById(R.id.scene_ambient);
        sceneShadow = findViewById(R.id.scene_shadow);
        sceneLightPosX = findViewById(R.id.scene_light_pos_x);
        sceneLightPosY = findViewById(R.id.scene_light_pos_y);
        sceneLightPosZ = findViewById(R.id.scene_light_pos_z);
        sceneShadowBias = findViewById(R.id.scene_shadow_bias);
        sceneShadowSoft = findViewById(R.id.scene_shadow_soft);
        sceneSsao = findViewById(R.id.scene_ssao);

        sceneBgRVal = findViewById(R.id.scene_bg_r_val);
        sceneBgGVal = findViewById(R.id.scene_bg_g_val);
        sceneBgBVal = findViewById(R.id.scene_bg_b_val);
        sceneAmbientVal = findViewById(R.id.scene_ambient_val);
        sceneLightPosXVal = findViewById(R.id.scene_light_pos_x_val);
        sceneLightPosYVal = findViewById(R.id.scene_light_pos_y_val);
        sceneLightPosZVal = findViewById(R.id.scene_light_pos_z_val);
        sceneShadowBiasVal = findViewById(R.id.scene_shadow_bias_val);

        ppAaMode = findViewById(R.id.pp_aa_mode);
        ppBloom = findViewById(R.id.pp_bloom);
        ppBloomIntensity = findViewById(R.id.pp_bloom_intensity);
        ppTonemap = findViewById(R.id.pp_tonemap);
        ppTonemapOp = findViewById(R.id.pp_tonemap_op);
        ppExposure = findViewById(R.id.pp_exposure);

        ppFog = findViewById(R.id.pp_fog);
        ppFogMode = findViewById(R.id.pp_fog_mode);
        ppFogDensity = findViewById(R.id.pp_fog_density);
        ppFogStart = findViewById(R.id.pp_fog_start);
        ppFogEnd = findViewById(R.id.pp_fog_end);

        ppDof = findViewById(R.id.pp_dof);
        ppDofFocusDist = findViewById(R.id.pp_dof_focus_dist);
        ppDofNear = findViewById(R.id.pp_dof_near);
        ppDofFar = findViewById(R.id.pp_dof_far);

        ppVolumetric = findViewById(R.id.pp_volumetric);
        ppVolDensity = findViewById(R.id.pp_vol_density);
        ppVolScattering = findViewById(R.id.pp_vol_scattering);
        ppVolSteps = findViewById(R.id.pp_vol_steps);
        ppVolIntensity = findViewById(R.id.pp_vol_intensity);
        ppVolDensityVal = findViewById(R.id.pp_vol_density_val);
        ppVolScatteringVal = findViewById(R.id.pp_vol_scattering_val);
        ppVolStepsVal = findViewById(R.id.pp_vol_steps_val);
        ppVolIntensityVal = findViewById(R.id.pp_vol_intensity_val);

        ppBloomIntensityVal = findViewById(R.id.pp_bloom_intensity_val);
        ppExposureVal = findViewById(R.id.pp_exposure_val);

        matBaseR = findViewById(R.id.mat_base_r);
        matBaseG = findViewById(R.id.mat_base_g);
        matBaseB = findViewById(R.id.mat_base_b);
        matMetallic = findViewById(R.id.mat_metallic);
        matRoughness = findViewById(R.id.mat_roughness);
        matAo = findViewById(R.id.mat_ao);

        matBaseRVal = findViewById(R.id.mat_base_r_val);
        matBaseGVal = findViewById(R.id.mat_base_g_val);
        matBaseBVal = findViewById(R.id.mat_base_b_val);
        matMetallicVal = findViewById(R.id.mat_metallic_val);
        matRoughnessVal = findViewById(R.id.mat_roughness_val);
        matAoVal = findViewById(R.id.mat_ao_val);
    }

    private void setupListeners() {
        cameraPosX.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { cameraPosXVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.CAMERA; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        cameraPosY.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { cameraPosYVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.CAMERA; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        cameraPosZ.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { cameraPosZVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.CAMERA; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        cameraTargetX.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { cameraTargetXVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.CAMERA; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        cameraTargetY.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { cameraTargetYVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.CAMERA; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        cameraTargetZ.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { cameraTargetZVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.CAMERA; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        cameraFov.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { cameraFovVal.setText(String.format("%.1f", value)); lastModifiedParam = ParamType.CAMERA; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        cameraOrthographic.setOnCheckedChangeListener((btn, checked) -> {
            lastModifiedParam = ParamType.CAMERA; if (listener != null) listener.onParameterChanged(); notifyChanged();
        });

        sceneBgR.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { sceneBgRVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.SCENE; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        sceneBgG.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { sceneBgGVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.SCENE; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        sceneBgB.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { sceneBgBVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.SCENE; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        sceneAmbient.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { sceneAmbientVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.SCENE; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        sceneShadow.setOnCheckedChangeListener((btn, checked) -> {
            lastModifiedParam = ParamType.SCENE; if (listener != null) listener.onParameterChanged(); notifyChanged();
        });
        sceneLightPosX.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { sceneLightPosXVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.SCENE; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        sceneLightPosY.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { sceneLightPosYVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.SCENE; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        sceneLightPosZ.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { sceneLightPosZVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.SCENE; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        sceneShadowBias.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { sceneShadowBiasVal.setText(String.format("%.3f", value)); lastModifiedParam = ParamType.SCENE; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        sceneShadowSoft.setOnCheckedChangeListener((btn, checked) -> {
            lastModifiedParam = ParamType.SCENE; if (listener != null) listener.onParameterChanged(); notifyChanged();
        });
        sceneSsao.setOnCheckedChangeListener((btn, checked) -> {
            lastModifiedParam = ParamType.SCENE; if (listener != null) listener.onParameterChanged(); notifyChanged();
        });

        ppAaMode.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> p, View v, int pos, long id) {
                lastModifiedParam = ParamType.POST_PROCESS; if (listener != null) listener.onParameterChanged(); notifyChanged();
            }
            public void onNothingSelected(AdapterView<?> p) {}
        });
        ppBloom.setOnCheckedChangeListener((btn, checked) -> {
            lastModifiedParam = ParamType.POST_PROCESS; if (listener != null) listener.onParameterChanged(); notifyChanged();
        });
        ppBloomIntensity.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { ppBloomIntensityVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.POST_PROCESS; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        ppTonemap.setOnCheckedChangeListener((btn, checked) -> {
            lastModifiedParam = ParamType.POST_PROCESS; if (listener != null) listener.onParameterChanged(); notifyChanged();
        });
        ppTonemapOp.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> p, View v, int pos, long id) {
                lastModifiedParam = ParamType.POST_PROCESS; if (listener != null) listener.onParameterChanged(); notifyChanged();
            }
            public void onNothingSelected(AdapterView<?> p) {}
        });
        ppExposure.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { ppExposureVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.POST_PROCESS; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        ppFog.setOnCheckedChangeListener((btn, checked) -> {
            lastModifiedParam = ParamType.POST_PROCESS; if (listener != null) listener.onParameterChanged(); notifyChanged();
        });
        ppFogMode.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> p, View v, int pos, long id) {
                lastModifiedParam = ParamType.POST_PROCESS; if (listener != null) listener.onParameterChanged(); notifyChanged();
            }
            public void onNothingSelected(AdapterView<?> p) {}
        });
        ppFogDensity.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { lastModifiedParam = ParamType.POST_PROCESS; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        ppFogStart.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { lastModifiedParam = ParamType.POST_PROCESS; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        ppFogEnd.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { lastModifiedParam = ParamType.POST_PROCESS; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        ppDof.setOnCheckedChangeListener((btn, checked) -> {
            lastModifiedParam = ParamType.POST_PROCESS; if (listener != null) listener.onParameterChanged(); notifyChanged();
        });
        ppDofFocusDist.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { lastModifiedParam = ParamType.POST_PROCESS; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        ppDofNear.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { lastModifiedParam = ParamType.POST_PROCESS; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        ppDofFar.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { lastModifiedParam = ParamType.POST_PROCESS; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        ppVolumetric.setOnCheckedChangeListener((btn, checked) -> {
            lastModifiedParam = ParamType.POST_PROCESS; if (listener != null) listener.onParameterChanged(); notifyChanged();
        });
        ppVolDensity.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { ppVolDensityVal.setText(String.format("%.3f", value)); lastModifiedParam = ParamType.POST_PROCESS; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        ppVolScattering.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { ppVolScatteringVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.POST_PROCESS; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        ppVolSteps.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { ppVolStepsVal.setText(String.valueOf((int) value)); lastModifiedParam = ParamType.POST_PROCESS; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        ppVolIntensity.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { ppVolIntensityVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.POST_PROCESS; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });

        matBaseR.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { matBaseRVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.MATERIAL; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        matBaseG.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { matBaseGVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.MATERIAL; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        matBaseB.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { matBaseBVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.MATERIAL; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        matMetallic.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { matMetallicVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.MATERIAL; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        matRoughness.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { matRoughnessVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.MATERIAL; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
        matAo.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser) { matAoVal.setText(String.format("%.2f", value)); lastModifiedParam = ParamType.MATERIAL; if (listener != null) listener.onParameterChanged(); notifyChanged(); }
        });
    }

    private boolean isTouchOnDragHandle(MotionEvent event) {
        View dragHandle = findViewById(R.id.drag_handle);
        int[] location = new int[2];
        dragHandle.getLocationOnScreen(location);
        int left = location[0], top = location[1];
        int right = left + dragHandle.getWidth(), bottom = top + dragHandle.getHeight();
        return event.getRawX() >= left && event.getRawX() <= right
            && event.getRawY() >= top && event.getRawY() <= bottom;
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        if (ev == null) return false;
        if (ev.getAction() == MotionEvent.ACTION_DOWN && isTouchOnDragHandle(ev)) {
            isDragging = true;
            dragStartX = ev.getRawX();
            dragStartY = ev.getRawY();
            int[] viewLocation = new int[2];
            getLocationOnScreen(viewLocation);
            viewStartX = viewLocation[0];
            return true;
        }
        return false;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (!isDragging) return false;
        if (event.getAction() == MotionEvent.ACTION_MOVE) {
            float deltaX = event.getRawX() - dragStartX;
            float newPosX = viewStartX + deltaX;
            ViewParent parentView = getParent();
            int parentWidth = (parentView instanceof View) ? ((View) parentView).getWidth() : 1000;
            float clampedX = Math.max(0f, Math.min(newPosX, parentWidth - getWidth()));
            setX(clampedX);
            return true;
        } else if (event.getAction() == MotionEvent.ACTION_UP || event.getAction() == MotionEvent.ACTION_CANCEL) {
            isDragging = false;
            return true;
        }
        return false;
    }

    // -------------------------------------------------------------------------
    // ���ò���
    // -------------------------------------------------------------------------
    public void setCameraParams(CameraParams params) {
        cameraPosX.setValue(params.getPosition().getX());
        cameraPosY.setValue(params.getPosition().getY());
        cameraPosZ.setValue(params.getPosition().getZ());
        cameraTargetX.setValue(params.getTarget().getX());
        cameraTargetY.setValue(params.getTarget().getY());
        cameraTargetZ.setValue(params.getTarget().getZ());
        cameraFov.setValue(params.getFov());
        cameraOrthographic.setChecked(params.getOrthographic());

        cameraPosXVal.setText(String.format("%.2f", params.getPosition().getX()));
        cameraPosYVal.setText(String.format("%.2f", params.getPosition().getY()));
        cameraPosZVal.setText(String.format("%.2f", params.getPosition().getZ()));
        cameraTargetXVal.setText(String.format("%.2f", params.getTarget().getX()));
        cameraTargetYVal.setText(String.format("%.2f", params.getTarget().getY()));
        cameraTargetZVal.setText(String.format("%.2f", params.getTarget().getZ()));
        cameraFovVal.setText(String.format("%.1f", params.getFov()));
    }

    public void setSceneConfig(SceneConfig config) {
        sceneBgR.setValue(config.getBgColor().getX());
        sceneBgG.setValue(config.getBgColor().getY());
        sceneBgB.setValue(config.getBgColor().getZ());
        sceneAmbient.setValue(config.getAmbientIntensity());
        sceneShadow.setChecked(config.getShadowEnabled());
        sceneLightPosX.setValue(config.getLightPosX());
        sceneLightPosY.setValue(config.getLightPosY());
        sceneLightPosZ.setValue(config.getLightPosZ());
        sceneShadowBias.setValue(config.getShadowBias());
        sceneShadowSoft.setChecked(config.getShadowPCFEnabled());
        sceneSsao.setChecked(config.getSsaoEnabled());

        sceneBgRVal.setText(String.format("%.2f", config.getBgColor().getX()));
        sceneBgGVal.setText(String.format("%.2f", config.getBgColor().getY()));
        sceneBgBVal.setText(String.format("%.2f", config.getBgColor().getZ()));
        sceneAmbientVal.setText(String.format("%.2f", config.getAmbientIntensity()));
        sceneLightPosXVal.setText(String.format("%.2f", config.getLightPosX()));
        sceneLightPosYVal.setText(String.format("%.2f", config.getLightPosY()));
        sceneLightPosZVal.setText(String.format("%.2f", config.getLightPosZ()));
        sceneShadowBiasVal.setText(String.format("%.3f", config.getShadowBias()));
    }

    public void setPostProcessConfig(PostProcessConfig config) {
        ppAaMode.setSelection(config.getAaMode());
        ppBloom.setChecked(config.getBloomEnabled());
        ppBloomIntensity.setValue(config.getBloomIntensity());
        ppTonemap.setChecked(config.getTonemapEnabled());
        ppTonemapOp.setSelection(config.getTonemapOp());
        ppExposure.setValue(config.getExposure());
        ppFog.setChecked(config.getFogEnabled());
        ppFogMode.setSelection(config.getFogMode());
        ppFogDensity.setValue(config.getFogDensity());
        ppFogStart.setValue(config.getFogStart());
        ppFogEnd.setValue(config.getFogEnd());
        ppDof.setChecked(config.getDofEnabled());
        ppDofFocusDist.setValue(config.getDofFocusDist());
        ppDofNear.setValue(config.getDofNear());
        ppDofFar.setValue(config.getDofFar());
        ppVolumetric.setChecked(config.getVolumetricEnabled());
        ppVolDensity.setValue(config.getVolumetricDensity());
        ppVolScattering.setValue(config.getVolumetricScattering());
        float volSteps = Math.max(8f, Math.min(config.getVolumetricSteps(), 128f));
        ppVolSteps.setValue(volSteps);
        ppVolIntensity.setValue(config.getVolumetricIntensity());
        ppVolDensityVal.setText(String.format("%.3f", config.getVolumetricDensity()));
        ppVolScatteringVal.setText(String.format("%.2f", config.getVolumetricScattering()));
        ppVolStepsVal.setText(String.valueOf(config.getVolumetricSteps()));
        ppVolIntensityVal.setText(String.format("%.2f", config.getVolumetricIntensity()));
        ppBloomIntensityVal.setText(String.format("%.2f", config.getBloomIntensity()));
        ppExposureVal.setText(String.format("%.2f", config.getExposure()));
    }

    public void setMaterialParams(MaterialParams params) {
        matBaseR.setValue(params.getBaseColor().getX());
        matBaseG.setValue(params.getBaseColor().getY());
        matBaseB.setValue(params.getBaseColor().getZ());
        matMetallic.setValue(params.getMetallic());
        matRoughness.setValue(params.getRoughness());
        matAo.setValue(params.getAo());

        matBaseRVal.setText(String.format("%.2f", params.getBaseColor().getX()));
        matBaseGVal.setText(String.format("%.2f", params.getBaseColor().getY()));
        matBaseBVal.setText(String.format("%.2f", params.getBaseColor().getZ()));
        matMetallicVal.setText(String.format("%.2f", params.getMetallic()));
        matRoughnessVal.setText(String.format("%.2f", params.getRoughness()));
        matAoVal.setText(String.format("%.2f", params.getAo()));
    }

    public void updateFps(float fps) {
        statsFps.setText(String.format("FPS: %.1f", fps));
        int color;
        if (fps >= 55) color = 0xFF00FF00;
        else if (fps >= 30) color = 0xFFFFFF00;
        else color = 0xFFFF0000;
        statsFps.setTextColor(color);
    }

    // -------------------------------------------------------------------------
    // ��ȡ��ǰ����
    // -------------------------------------------------------------------------
    public CameraParams getCurrentCameraParams() {
        return new CameraParams(
            new Vec3(cameraPosX.getValue(), cameraPosY.getValue(), cameraPosZ.getValue()),
            new Vec3(cameraTargetX.getValue(), cameraTargetY.getValue(), cameraTargetZ.getValue()),
            cameraFov.getValue(),
            0.1f,
            100f,
            cameraOrthographic.isChecked(),
            10f
        );
    }

    public SceneConfig getCurrentSceneConfig() {
        return new SceneConfig(
            new Vec3(sceneBgR.getValue(), sceneBgG.getValue(), sceneBgB.getValue()),
            sceneAmbient.getValue(),
            sceneShadow.isChecked(),
            sceneLightPosX.getValue(),
            sceneLightPosY.getValue(),
            sceneLightPosZ.getValue(),
            sceneShadowSoft.isChecked() ? 0.5f : 0f,
            sceneShadowBias.getValue(),
            2048,
            sceneShadowSoft.isChecked(),
            sceneSsao.isChecked(),
            0.5f,
            1f,
            false,
            new Vec3(0f, 1f, 0f),
            0f
        );
    }

    public PostProcessConfig getCurrentPostProcessConfig() {
        return new PostProcessConfig(
            ppAaMode.getSelectedItemPosition(),
            ppBloom.isChecked(),
            ppBloomIntensity.getValue(),
            0.8f,
            ppTonemap.isChecked(),
            ppTonemapOp.getSelectedItemPosition(),
            ppExposure.getValue(),
            2.2f,
            false,
            0.5f,
            ppFog.isChecked(),
            ppFogMode.getSelectedItemPosition(),
            ppFogDensity.getValue(),
            ppFogStart.getValue(),
            ppFogEnd.getValue(),
            0.7f, 0.75f, 0.8f,
            ppDof.isChecked(),
            ppDofFocusDist.getValue(),
            ppDofNear.getValue(),
            ppDofFar.getValue(),
            ppVolumetric.isChecked(),
            ppVolDensity.getValue(),
            ppVolScattering.getValue(),
            (int) ppVolSteps.getValue(),
            30.0f,
            ppVolIntensity.getValue(),
            1.0f, 0.95f, 0.85f
        );
    }

    public MaterialParams getCurrentMaterialParams() {
        return new MaterialParams(
            new Vec3(matBaseR.getValue(), matBaseG.getValue(), matBaseB.getValue()),
            matMetallic.getValue(),
            matRoughness.getValue(),
            matAo.getValue(),
            new Vec3(0f, 0f, 0f),
            0f, 0f, 0f
        );
    }

    public void setListener(Listener listener) {
        this.listener = listener;
    }

    public ParamType getLastModifiedParam() {
        return lastModifiedParam;
    }
}
