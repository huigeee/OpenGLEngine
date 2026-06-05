package com.iauto.test2;

import android.os.Bundle;
import android.view.View;
import android.view.WindowManager;
import androidx.appcompat.app.AppCompatActivity;
import com.iauto.ogl.client.OglSurfaceView;
import com.iauto.ogl.client.RenderService;

/**
 * test2 的主 Activity。
 *
 * 它不自己启动渲染引擎，而是 bind 到 test1 已经运行的 OglRendererService（:render 进程）。
 * OglRendererService 的 init() 实现会检测引擎是否已初始化：
 *   - 已初始化 → 直接调 nativeReConnectSurface，把渲染目标切换到 test2 的 Surface
 *   - 未初始化 → 正常 nativeInit（兜底，test1 未启动时也能独立使用）
 *
 * 切换逻辑（前后台来回切换）：
 *   - onPause  → surfaceDestroyed → nativePause（surface 失效，暂停渲染）
 *   - onResume → surfaceCreated  → nativeReConnectSurface（重新拿到新 surface，恢复渲染）
 *
 * 服务包名硬编码为 "com.iauto.test1"，因为 OglRendererService 在 test1 的进程里运行。
 * 如果将来 Service 迁移到独立 APK，只需改这一行。
 */
public class MainActivity extends AppCompatActivity {

    /** test1 的包名，OglRendererService 运行在它的 :render 进程里 */
    private static final String RENDER_SERVICE_PKG = "com.iauto.test1";

    private OglSurfaceView surfaceView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // 全屏
        getWindow().setFlags(
            WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN
        );
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            | View.SYSTEM_UI_FLAG_FULLSCREEN
            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        );

        setContentView(R.layout.activity_main);
        surfaceView = findViewById(R.id.surface_view);

        // filesDir 传 null：跨进程时 test1 的 Service 已有资源路径，无需重新传
        surfaceView.init(RENDER_SERVICE_PKG, null);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        surfaceView.releaseView();
    }
}
