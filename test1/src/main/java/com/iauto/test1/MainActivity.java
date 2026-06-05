package com.iauto.test1;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.WindowManager;
import android.view.View;
import androidx.appcompat.app.AppCompatActivity;
import com.iauto.ogl.client.InspectorPanel;
import com.iauto.ogl.client.OglSurfaceView;
import com.iauto.ogl.client.RenderService;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

public class MainActivity extends AppCompatActivity {

    private OglSurfaceView surfaceView;
    private InspectorPanel inspectorPanel;
    private String resourcesDir = null;

    private final Handler fpsHandler = new Handler(Looper.getMainLooper());
    private final Runnable fpsRunnable = new Runnable() {
        @Override
        public void run() {
            RenderService rs = surfaceView.getRenderService();
            float fps = (rs != null) ? rs.getFPS() : 0f;
            if (fps > 0) inspectorPanel.updateFps(fps);
            fpsHandler.postDelayed(this, 500);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

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
        inspectorPanel = findViewById(R.id.inspector_panel);

        resourcesDir = copyAssetsToFilesDir();
        surfaceView.init(null, resourcesDir);
        fpsHandler.post(fpsRunnable);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        surfaceView.releaseView();
        resourcesDir = null;
        fpsHandler.removeCallbacks(fpsRunnable);
    }

    private String copyAssetsToFilesDir() {
        File filesDir = getFilesDir();
        copyAssetDir("texture", filesDir);
        copyAssetDir("shaders", filesDir);
        copyAssetDir("models", filesDir);
        return filesDir.getAbsolutePath();
    }

    private void copyAssetDir(String assetDir, File destDir) {
        try {
            String[] list = getAssets().list(assetDir);
            if (list == null || list.length == 0) return;
            File destSubDir = new File(destDir, assetDir);
            if (!destSubDir.exists()) destSubDir.mkdirs();
            for (String entry : list) {
                String fullAssetPath = assetDir + "/" + entry;
                File destFile = new File(destSubDir, entry);
                try {
                    String[] subList = getAssets().list(fullAssetPath);
                    if (subList != null && subList.length > 0) {
                        if (!destFile.exists()) destFile.mkdirs();
                        copyAssetDir(fullAssetPath, destDir);
                    } else {
                        copyAssetFile(fullAssetPath, destFile);
                    }
                } catch (Exception e) {
                    copyAssetFile(fullAssetPath, destFile);
                }
            }
        } catch (Exception ignored) {}
    }

    private void copyAssetFile(String assetPath, File destFile) {
        try (InputStream input = getAssets().open(assetPath);
             FileOutputStream output = new FileOutputStream(destFile)) {
            byte[] buffer = new byte[8192];
            int len;
            while ((len = input.read(buffer)) != -1) {
                output.write(buffer, 0, len);
            }
        } catch (Exception ignored) {}
    }
}
