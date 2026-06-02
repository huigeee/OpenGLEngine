package com.iauto.myapplication.renderer;

import android.view.Surface;
import com.iauto.myapplication.data.CameraParams;
import com.iauto.myapplication.data.SceneConfig;
import com.iauto.myapplication.data.PostProcessConfig;
import com.iauto.myapplication.data.MaterialParams;

interface IOglRenderer {
    void init(in Surface surface, String filesDir);
    void reConnectSurface(in Surface surface);
    void pause();
    void resume();
    void release();
    void setViewport(int width, int height);
    void setRotation(float deltaX, float deltaY);
    void setScale(float scale);
    void setPosition(float x, float y, float z);
    void setBaseColor(float r, float g, float b);
    void setMetallic(float value);
    void setRoughness(float value);
    void setAO(float value);
    void setCameraPosition(float x, float y, float z);
    void setCameraTarget(float x, float y, float z);
    void setTAAEnabled(boolean enabled);
    void setBloomEnabled(boolean enabled);
    void setBloomIntensity(float intensity);
    void setTonemapEnabled(boolean enabled);
    void setTonemapOp(int op);
    void setExposure(float exposure);
    boolean isInitialized();
    float getFPS();
    
    // -------------------------------------------------------------------------
    // 批量参数设置（用于 Inspector 面板）
    // -------------------------------------------------------------------------
    void updateCameraParams(in CameraParams params);
    void updateSceneConfig(in SceneConfig config);
    void updatePostProcessConfig(in PostProcessConfig config);
    void updateMaterialParams(in MaterialParams params);
    
    // -------------------------------------------------------------------------
    // 获取当前参数
    // -------------------------------------------------------------------------
    CameraParams getCameraParams();
    SceneConfig getSceneConfig();
    PostProcessConfig getPostProcessConfig();
    MaterialParams getMaterialParams();
}
