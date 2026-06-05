package com.iauto.ogl.client;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.view.Surface;

import com.iauto.ogl.data.CameraParams;
import com.iauto.ogl.data.MaterialParams;
import com.iauto.ogl.data.PostProcessConfig;
import com.iauto.ogl.data.SceneConfig;
import com.iauto.ogl.renderer.IOglRenderer;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

/**
 * RenderService — 引擎客户端单例
 * 按 packageName 管理单例，内部 bindService 获取 IOglRenderer Binder。
 */
public class RenderService implements ServiceConnection {

    private static final String TAG = "RenderService";
    private static final String SERVICE_CLASS_NAME = "com.iauto.ogl.service.OglRendererService";
    private static final HashMap<String, RenderService> instances = new HashMap<>();

    private final Context appContext;
    private final String servicePkgName;
    private IOglRenderer renderer;
    private boolean isBound = false;
    private volatile boolean isConnected = false;
    private final List<Callback> callbacks = new ArrayList<>();

    public interface Callback {
        void onServiceConnected();
        void onServiceDisconnected();
    }

    public static RenderService init(Context context) {
        return init(context, null);
    }

    public static synchronized RenderService init(Context context, String servicePkgName) {
        String pkg = (servicePkgName != null) ? servicePkgName : context.getPackageName();
        RenderService rs = instances.get(pkg);
        if (rs == null) {
            rs = new RenderService(context.getApplicationContext(), pkg);
            instances.put(pkg, rs);
        }
        return rs;
    }

    public static RenderService get(Context context) {
        return instances.get(context.getPackageName());
    }

    public static RenderService get(String pkgName) {
        return instances.get(pkgName);
    }

    public boolean isConnected() {
        return isConnected;
    }

    private RenderService(Context appContext, String servicePkgName) {
        this.appContext = appContext;
        this.servicePkgName = servicePkgName;
    }

    public void addCallback(Callback cb) {
        synchronized (callbacks) {
            if (!callbacks.contains(cb)) callbacks.add(cb);
        }
        if (isConnected) {
            cb.onServiceConnected();
        }
    }

    public void removeCallback(Callback cb) {
        synchronized (callbacks) {
            callbacks.remove(cb);
        }
    }

    public boolean start(Surface surface, String filesDir) {
        if (isBound) return true;

        Intent intent = makeIntent();
        // 先尝试 bindService（BIND_AUTO_CREATE 会自动创建并启动 Service）
        boolean ok = appContext.bindService(intent, this, Context.BIND_AUTO_CREATE);
        if (ok) {
            isBound = true;
            // 启动 foreground 通知让 Service 保活（同包场景）
            // 跨包时这步可能失败，但 bind 本身已能拉起 Service
            try {
                appContext.startForegroundService(intent);
            } catch (Exception e) {
                Log.w(TAG, "startForegroundService (optional) failed: " + e.getMessage());
            }
        } else {
            Log.e(TAG, "bindService returned false — cannot reach OglRendererService in package " + servicePkgName);
        }
        return ok;
    }

    public void stop() {
        if (!isBound) return;
        appContext.unbindService(this);
        appContext.stopService(makeIntent());
        isBound = false;
        renderer = null;
        isConnected = false;
    }

    /** 构造用于定位 OglRendererService 的 Intent */
    private Intent makeIntent() {
        Intent intent = new Intent("com.iauto.ogl.ACTION_RENDER_SERVICE");
        // 跨包时用 setClassName 最可靠，同包时也兼容
        intent.setClassName(servicePkgName, SERVICE_CLASS_NAME);
        return intent;
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
        Log.i(TAG, "onServiceConnected");
        renderer = IOglRenderer.Stub.asInterface(service);
        isConnected = true;
        notifyConnected();
    }

    @Override
    public void onServiceDisconnected(ComponentName name) {
        Log.w(TAG, "onServiceDisconnected");
        renderer = null;
        isConnected = false;
        notifyDisconnected();
    }

    private void notifyConnected() {
        synchronized (callbacks) {
            for (Callback cb : callbacks) cb.onServiceConnected();
        }
    }

    private void notifyDisconnected() {
        synchronized (callbacks) {
            for (Callback cb : callbacks) cb.onServiceDisconnected();
        }
    }

    // ========================================================================
    // 控制 API
    // ========================================================================

    public void init(Surface surface, String filesDir) {
        try { if (renderer != null) renderer.init(surface, filesDir); }
        catch (RemoteException ignored) {}
    }

    public void reConnectSurface(Surface surface) {
        try { if (renderer != null) renderer.reConnectSurface(surface); }
        catch (RemoteException ignored) {}
    }

    public void release() {
        try { if (renderer != null) renderer.release(); }
        catch (RemoteException ignored) {}
    }

    public void pause() {
        try { if (renderer != null) renderer.pause(); }
        catch (RemoteException ignored) {}
    }

    public void resume() {
        try { if (renderer != null) renderer.resume(); }
        catch (RemoteException ignored) {}
    }

    public void setViewport(int width, int height) {
        try { if (renderer != null) renderer.setViewport(width, height); }
        catch (RemoteException ignored) {}
    }

    public void setScale(float scale) {
        try { if (renderer != null) renderer.setScale(scale); }
        catch (RemoteException ignored) {}
    }

    public void setPosition(float x, float y, float z) {
        try { if (renderer != null) renderer.setPosition(x, y, z); }
        catch (RemoteException ignored) {}
    }

    public void setBaseColor(float r, float g, float b) {
        try { if (renderer != null) renderer.setBaseColor(r, g, b); }
        catch (RemoteException ignored) {}
    }

    public void setMetallic(float value) {
        try { if (renderer != null) renderer.setMetallic(value); }
        catch (RemoteException ignored) {}
    }

    public void setRoughness(float value) {
        try { if (renderer != null) renderer.setRoughness(value); }
        catch (RemoteException ignored) {}
    }

    public void setAO(float value) {
        try { if (renderer != null) renderer.setAO(value); }
        catch (RemoteException ignored) {}
    }

    public void setCameraPosition(float x, float y, float z) {
        try { if (renderer != null) renderer.setCameraPosition(x, y, z); }
        catch (RemoteException ignored) {}
    }

    public void setCameraTarget(float x, float y, float z) {
        try { if (renderer != null) renderer.setCameraTarget(x, y, z); }
        catch (RemoteException ignored) {}
    }

    public void setTAAEnabled(boolean enabled) {
        try { if (renderer != null) renderer.setTAAEnabled(enabled); }
        catch (RemoteException ignored) {}
    }

    public void setBloomEnabled(boolean enabled) {
        try { if (renderer != null) renderer.setBloomEnabled(enabled); }
        catch (RemoteException ignored) {}
    }

    public void setBloomIntensity(float intensity) {
        try { if (renderer != null) renderer.setBloomIntensity(intensity); }
        catch (RemoteException ignored) {}
    }

    public void setTonemapEnabled(boolean enabled) {
        try { if (renderer != null) renderer.setTonemapEnabled(enabled); }
        catch (RemoteException ignored) {}
    }

    public void setTonemapOp(int op) {
        try { if (renderer != null) renderer.setTonemapOp(op); }
        catch (RemoteException ignored) {}
    }

    public void setExposure(float exposure) {
        try { if (renderer != null) renderer.setExposure(exposure); }
        catch (RemoteException ignored) {}
    }

    public boolean isInitialized() {
        try { return renderer != null && renderer.isInitialized(); }
        catch (RemoteException e) { return false; }
    }

    public float getFPS() {
        try { return renderer != null ? renderer.getFPS() : 0f; }
        catch (RemoteException e) { return 0f; }
    }

    // 跨进程 touch
    public void processTouchEvent(int actionMasked, int pointerCount, float[] xs, float[] ys, int[] ids) {
        try { if (renderer != null) renderer.processTouchEvent(actionMasked, pointerCount, xs, ys, ids); }
        catch (RemoteException ignored) {}
    }

    // 批量参数
    public void updateCameraParams(CameraParams params) {
        try { if (renderer != null) renderer.updateCameraParams(params); }
        catch (RemoteException ignored) {}
    }

    public void updateSceneConfig(SceneConfig config) {
        try { if (renderer != null) renderer.updateSceneConfig(config); }
        catch (RemoteException ignored) {}
    }

    public void updatePostProcessConfig(PostProcessConfig config) {
        try { if (renderer != null) renderer.updatePostProcessConfig(config); }
        catch (RemoteException ignored) {}
    }

    public void updateMaterialParams(MaterialParams params) {
        try { if (renderer != null) renderer.updateMaterialParams(params); }
        catch (RemoteException ignored) {}
    }

    public CameraParams getCameraParams() {
        try { return renderer != null ? renderer.getCameraParams() : null; }
        catch (RemoteException e) { return null; }
    }

    public SceneConfig getSceneConfig() {
        try { return renderer != null ? renderer.getSceneConfig() : null; }
        catch (RemoteException e) { return null; }
    }

    public PostProcessConfig getPostProcessConfig() {
        try { return renderer != null ? renderer.getPostProcessConfig() : null; }
        catch (RemoteException e) { return null; }
    }

    public MaterialParams getMaterialParams() {
        try { return renderer != null ? renderer.getMaterialParams() : null; }
        catch (RemoteException e) { return null; }
    }
}
