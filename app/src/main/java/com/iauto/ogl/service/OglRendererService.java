package com.iauto.ogl.service;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.IBinder;
import android.util.Log;
import android.view.Surface;

import com.iauto.ogl.data.CameraParams;
import com.iauto.ogl.data.MaterialParams;
import com.iauto.ogl.data.PostProcessConfig;
import com.iauto.ogl.data.SceneConfig;
import com.iauto.ogl.renderer.IOglRenderer;

public class OglRendererService extends Service {

    private static final String TAG = "OglRendererService";
    private static final int NOTIFICATION_ID = 1001;
    private static final String CHANNEL_ID = "ogl_renderer_channel";
    private static final String CHANNEL_NAME = "OpenGL Renderer";

    private boolean isForeground = false;

    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "onCreate");
        createNotificationChannel();
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.i(TAG, "onBind");
        return oglRendererBinder;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i(TAG, "onStartCommand");
        if (!isForeground) {
            startForeground(NOTIFICATION_ID, createNotification());
            isForeground = true;
            Log.i(TAG, "Started as foreground service");
        }
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.i(TAG, "onDestroy");
        NativeRenderer.nativeRelease();
    }

    private final IOglRenderer.Stub oglRendererBinder = new IOglRenderer.Stub() {

        @Override
        public void init(Surface surface, String filesDir) {
            if (NativeRenderer.nativeIsInitialized()) {
                // 引擎已在另一个客户端初始化，直接切换 Surface
                Log.i(TAG, "init called but engine already running — reconnecting surface");
                NativeRenderer.nativeReConnectSurface(surface);
            } else {
                Log.i(TAG, "init surface=" + filesDir);
                NativeRenderer.nativeInit(surface, filesDir);
            }
        }

        @Override
        public void reConnectSurface(Surface surface) {
            NativeRenderer.nativeReConnectSurface(surface);
        }

        @Override
        public void pause() {
            NativeRenderer.nativePause();
        }

        @Override
        public void resume() {
            NativeRenderer.nativeResume();
        }

        @Override
        public void release() {
            NativeRenderer.nativeRelease();
        }

        @Override
        public void setViewport(int width, int height) {
            NativeRenderer.nativeSetViewport(width, height);
        }

        @Override
        public void setScale(float scale) {
            NativeRenderer.nativeSetScale(scale);
        }

        @Override
        public void setPosition(float x, float y, float z) {
            NativeRenderer.nativeSetPosition(x, y, z);
        }

        @Override
        public void setBaseColor(float r, float g, float b) {
            NativeRenderer.nativeSetBaseColor(r, g, b);
        }

        @Override
        public void setMetallic(float value) {
            NativeRenderer.nativeSetMetallic(value);
        }

        @Override
        public void setRoughness(float value) {
            NativeRenderer.nativeSetRoughness(value);
        }

        @Override
        public void setAO(float value) {
            NativeRenderer.nativeSetAO(value);
        }

        @Override
        public void setCameraPosition(float x, float y, float z) {
            NativeRenderer.nativeSetCameraPosition(x, y, z);
        }

        @Override
        public void setCameraTarget(float x, float y, float z) {
            NativeRenderer.nativeSetCameraTarget(x, y, z);
        }

        @Override
        public void setTAAEnabled(boolean enabled) {
            NativeRenderer.nativeSetTAAEnabled(enabled);
        }

        @Override
        public void setBloomEnabled(boolean enabled) {
            NativeRenderer.nativeSetBloomEnabled(enabled);
        }

        @Override
        public void setBloomIntensity(float intensity) {
            NativeRenderer.nativeSetBloomIntensity(intensity);
        }

        @Override
        public void setTonemapEnabled(boolean enabled) {
            NativeRenderer.nativeSetTonemapEnabled(enabled);
        }

        @Override
        public void setTonemapOp(int op) {
            NativeRenderer.nativeSetTonemapOp(op);
        }

        @Override
        public void setExposure(float exposure) {
            NativeRenderer.nativeSetExposure(exposure);
        }

        @Override
        public boolean isInitialized() {
            return NativeRenderer.nativeIsInitialized();
        }

        @Override
        public float getFPS() {
            return NativeRenderer.nativeGetFPS();
        }

        @Override
        public void processTouchEvent(int actionMasked, int pointerCount, float[] xs, float[] ys, int[] ids) {
            NativeRenderer.nativeProcessTouchEvent(actionMasked, pointerCount, xs, ys, ids);
        }

        @Override
        public void updateCameraParams(CameraParams params) {
            NativeRenderer.nativeUpdateCameraParams(
                    params.getPosition().getX(), params.getPosition().getY(), params.getPosition().getZ(),
                    params.getTarget().getX(), params.getTarget().getY(), params.getTarget().getZ(),
                    params.getFov(), params.getOrthographic());
        }

        @Override
        public void updateSceneConfig(SceneConfig config) {
            NativeRenderer.nativeUpdateSceneConfig(
                    config.getBgColor().getX(), config.getBgColor().getY(), config.getBgColor().getZ(),
                    config.getAmbientIntensity(), config.getShadowEnabled(), config.getSsaoEnabled(),
                    config.getLightPosX(), config.getLightPosY(), config.getLightPosZ(),
                    config.getShadowBias(), config.getShadowSoftness(), config.getShadowPCFEnabled());
        }

        @Override
        public void updatePostProcessConfig(PostProcessConfig config) {
            NativeRenderer.nativeUpdatePostProcessParams(
                    config.getAaMode(), config.getBloomEnabled(), config.getBloomIntensity(),
                    config.getTonemapEnabled(), config.getTonemapOp(), config.getExposure(),
                    config.getFogEnabled(), config.getFogMode(), config.getFogDensity(),
                    config.getFogStart(), config.getFogEnd(), config.getFogR(), config.getFogG(), config.getFogB(),
                    config.getDofEnabled(), config.getDofFocusDist(), config.getDofNear(), config.getDofFar(),
                    config.getVolumetricEnabled(), config.getVolumetricDensity(), config.getVolumetricScattering(),
                    config.getVolumetricSteps(), config.getVolumetricMaxDistance(), config.getVolumetricIntensity(),
                    config.getVolumetricColorR(), config.getVolumetricColorG(), config.getVolumetricColorB());
        }

        @Override
        public void updateMaterialParams(MaterialParams params) {
            NativeRenderer.nativeUpdateMaterialParams(
                    params.getBaseColor().getX(), params.getBaseColor().getY(), params.getBaseColor().getZ(),
                    params.getMetallic(), params.getRoughness(), params.getAo());
        }

        @Override
        public CameraParams getCameraParams() {
            return new CameraParams();
        }

        @Override
        public SceneConfig getSceneConfig() {
            return new SceneConfig();
        }

        @Override
        public PostProcessConfig getPostProcessConfig() {
            return new PostProcessConfig();
        }

        @Override
        public MaterialParams getMaterialParams() {
            return new MaterialParams();
        }
    };

    // ========================================================================
    // Foreground Service 通知
    // ========================================================================

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(
                    CHANNEL_ID, CHANNEL_NAME, NotificationManager.IMPORTANCE_LOW);
            channel.setDescription("OpenGL Renderer Service");
            NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
            if (manager != null) {
                manager.createNotificationChannel(channel);
            }
        }
    }

    @SuppressWarnings("deprecation")
    private Notification createNotification() {
        Notification.Builder builder;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            builder = new Notification.Builder(this, CHANNEL_ID);
        } else {
            builder = new Notification.Builder(this);
        }
        return builder
                .setContentTitle("Render Service")
                .setContentText("Running")
                .setSmallIcon(android.R.drawable.ic_menu_camera)
                .setOngoing(true)
                .build();
    }
}
