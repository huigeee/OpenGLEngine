package com.iauto.ogl.client;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import com.iauto.ogl.service.NativeRenderer;

public class OglSurfaceView extends SurfaceView implements RenderService.Callback {

    private static final String TAG = "OglSurfaceView";

    private RenderService renderService;
    private String filesDir;

    private final SurfaceHolder.Callback holderCallback = new SurfaceHolder.Callback() {
        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            Log.i(TAG, "surfaceCreated");
            tryStartRender();
        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            Log.i(TAG, "surfaceChanged: " + width + "x" + height);
            if (renderService != null) {
                renderService.setViewport(width, height);
            }
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            // Surface 被销毁（app 退后台），通知引擎暂停，不释放引擎
            Log.i(TAG, "surfaceDestroyed — pausing render");
            if (renderService != null) {
                renderService.pause();
            }
        }
    };

    public OglSurfaceView(Context context) {
        super(context);
        initView();
    }

    public OglSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initView();
    }

    public OglSurfaceView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        initView();
    }

    private void initView() {
        getHolder().addCallback(holderCallback);
        setClickable(true);
        setFocusable(false);
    }

    public void init(String servicePkgName, String filesDir) {
        this.filesDir = (filesDir != null) ? filesDir : getContext().getFilesDir().getAbsolutePath();
        renderService = RenderService.init(getContext(), servicePkgName);
        if (renderService != null) {
            renderService.addCallback(this);
        }
        Log.i(TAG, "init done");
        tryStartRender();
    }

    @Override
    public void onServiceConnected() {
        Log.i(TAG, "onServiceConnected — retrying render start");
        tryStartRender();
    }

    @Override
    public void onServiceDisconnected() {
        Log.i(TAG, "onServiceDisconnected");
    }

    private void tryStartRender() {
        RenderService rs = renderService;
        if (rs == null) return;
        android.view.Surface surf = getHolder().getSurface();
        if (surf == null || !surf.isValid()) return;
        String dir = (filesDir != null) ? filesDir : getContext().getFilesDir().getAbsolutePath();
        if (!rs.isConnected()) {
            Log.i(TAG, "service not yet connected — starting service");
            boolean ok = rs.start(surf, dir);
            Log.i(TAG, "start result: " + ok);
        } else if (rs.isInitialized()) {
            // 引擎已在运行（另一个 app 先初始化过），只需切换 Surface
            // 先 resume（清除 paused 状态），再 reconnect Surface
            Log.i(TAG, "engine already running — resuming and reconnecting surface");
            rs.resume();
            rs.reConnectSurface(surf);
        } else {
            // 已 bind 但引擎还未初始化
            Log.i(TAG, "service connected, initializing engine");
            rs.init(surf, dir);
        }
    }

    public void releaseView() {
        getHolder().removeCallback(holderCallback);
        if (renderService != null) {
            renderService.removeCallback(this);
        }
        renderService = null;
    }

    public RenderService getRenderService() {
        return renderService;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (event == null) return false;
        int count = event.getPointerCount();
        float[] xs = new float[count];
        float[] ys = new float[count];
        int[] ids = new int[count];
        for (int i = 0; i < count; i++) {
            xs[i] = event.getX(i);
            ys[i] = event.getY(i);
            ids[i] = event.getPointerId(i);
        }
        // 跨进程：通过 RenderService 转发到 Service 进程的 NativeRenderer
        if (renderService != null) {
            renderService.processTouchEvent(event.getActionMasked(), count, xs, ys, ids);
        } else {
            NativeRenderer.nativeProcessTouchEvent(event.getActionMasked(), count, xs, ys, ids);
        }
        return true;
    }
}
