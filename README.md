# OGL Render Engine

只做学习用。
涉及的库：Assimp，glm，libpng，stb_image,都不用于商业，本仓库也禁止其他人直接引入商业用途，只做学习和讨论交流

Android 上的独立进程 OpenGL 渲染引擎，支持**多个 App 共享一个渲染引擎实例**，前后台切换时引擎不重启。

---

## 架构概览

```
┌─────────────────┐     AIDL IPC     ┌──────────────────────────┐
│  test1 / test2   │ ◄──────────────► │  :render 进程            │
│  (UI 进程)       │                  │  OglRendererService      │
│                  │                  │    ├── EGLCore           │
│  OglSurfaceView  │                  │    │   └── renderLoop    │
│  RenderService   │                  │    ├── MainScene         │
│  MainActivity    │                  │    │   └── Scene::render │
└─────────────────┘                  │    └── NativeRenderer    │
                                      └──────────────────────────┘
```

三个完全独立的进程：

| 进程 | 包名 | 职责 |
|---|---|---|
| test1 | `com.iauto.test1` | App A 的 UI 进程 |
| test2 | `com.iauto.test2` | App B 的 UI 进程 |
| 渲染引擎 | `com.iauto.test1:render` | 独立 OpenGL 渲染进程 |

客户端（test1/test2）通过 `RenderService` 封装 AIDL 通信来操作引擎。

---

## 项目结构

```
aitest111/
├── app/                          # 引擎库（Android library）
│   ├── src/main/
│   │   ├── aidl/                 # AIDL 接口定义（IPC 协议）
│   │   │   └── com/iauto/ogl/
│   │   │       ├── renderer/IOglRenderer.aidl
│   │   │       └── data/         # 参数 data class 的 AIDL
│   │   ├── java/.../ogl/
│   │   │   ├── client/
│   │   │   │   ├── OglSurfaceView.java    # SurfaceView 封装
│   │   │   │   └── RenderService.java     # 客户端 AIDL 代理
│   │   │   ├── service/
│   │   │   │   ├── OglRendererService.java # Service 端 AIDL 实现
│   │   │   │   └── NativeRenderer.java    # JNI native 方法声明
│   │   │   └── data/             # 参数 data class
│   │   ├── cpp/
│   │   │   ├── native-renderer.cpp         # JNI_OnLoad + native 实现
│   │   │   └── ogl/oglengine/             # 引擎核心
│   │   │       ├── include/EGLCore.h      # EGL + 渲染线程
│   │   │       ├── utils/EGLCore.cpp
│   │   │       ├── Scene.cpp              # 场景管理 + FBO + 后处理
│   │   │       ├── MainScene.cpp          # 具体场景（cube 等）
│   │   │       ├── shadow/               # 阴影
│   │   │       ├── skybox/               # 天空盒
│   │   │       ├── taa/                  # TAA 抗锯齿
│   │   │       ├── bloom/                # 泛光
│   │   │       ├── tonemap/              # 色调映射
│   │   │       └── ...                   # 其他后处理效果
│   │   └── AndroidManifest.xml
│   └── build.gradle
├── test1/                         # App A
│   ├── src/main/
│   │   ├── java/.../MainActivity.java
│   │   ├── res/layout/activity_main.xml
│   │   └── AndroidManifest.xml
│   └── build.gradle
├── test2/                         # App B
│   ├── src/main/
│   │   ├── java/.../MainActivity.java
│   │   ├── res/layout/activity_main.xml
│   │   └── AndroidManifest.xml
│   └── build.gradle
├── settings.gradle
└── build.gradle
```

---

## 引擎启动流程

### 时序图

```
MainActivity        OglSurfaceView      RenderService        Service进程        渲染线程
     │                    │                  │                  │                │
     │  onCreate()        │                  │                  │                │
     │──────►init(pkg,dir)│                  │                  │                │
     │                    │RenderService.init│                  │                │
     │                    │─────────────────►│  创建/获取单例    │                │
     │                    │addCallback(this) │                  │                │
     │                    │tryStartRender()  │                  │                │
     │                    │──►isConnected?   │                  │                │
     │                    │  false           │                  │                │
     │                    │──►start()        │                  │                │
     │                    │                  │──bindService────►│                │
     │                    │                  │                  │  onCreate()    │
     │                    │                  │                  │  onBind()      │
     │                    │                  │◄──Binder 返回────│                │
     │                    │onServiceConnected()                │                │
     │◄───────────────────│                  │                  │                │
     │   surfaceCreated    │                  │                  │                │
     │──────────────────►│                  │                  │                │
     │                    │tryStartRender()  │                  │                │
     │                    │──►isConnected?   │                  │                │
     │                    │  true            │                  │                │
     │                    │──►isInitialized? │                  │                │
     │                    │  false           │                  │                │
     │                    │──►init(surf,dir) │                  │                │
     │                    │                  │──AIDL init──────►│                │
     │                    │                  │                  │nativeInit()    │
     │                    │                  │                  │EGLCore::init() │
     │                    │                  │                  │EGLCore::start()│
     │                    │                  │                  │────────────────►│
     │                    │                  │                  │                │
     │                    │                  │                  │         renderLoop:
     │                    │                  │                  │  AttachCurrentThread
     │                    │                  │                  │  initEGL()
     │                    │                  │                  │  while(running):
     │                    │                  │                  │    scene->render()
     │                    │                  │                  │    eglSwapBuffers()
```

### 详细步骤

#### 1. `MainActivity.onCreate()` — 入口

```java
surfaceView.init(RENDER_SERVICE_PKG, filesDir);
```

- `RENDER_SERVICE_PKG` = `null`（test1 用自己包名）或 `"com.iauto.test1"`（test2 指向 test1 的 Service）
- `filesDir` = 资源文件路径（纹理、shader、模型等）

#### 2. `OglSurfaceView.init()` — 初始化客户端

```java
renderService = RenderService.init(getContext(), servicePkgName);
renderService.addCallback(this);
tryStartRender();
```

- 创建/获取 `RenderService` 单例（按包名缓存）
- `tryStartRender()` 尝试启动，但此时 Surface 可能还未就绪，直接返回

#### 3. `tryStartRender()` — 三态决策

| 状态 | 条件 | 行为 |
|---|---|---|
| **未连接** | `!isConnected()` | `bindService` 拉起 Service |
| **已连接+引擎未初始化** | `isInitialized() == false` | AIDL `init()` 首次初始化 |
| **已连接+引擎已初始化** | `isInitialized() == true` | `resume()` + `reConnectSurface()` 换 Surface |

#### 4. `RenderService.start()` — 跨进程 Bind

```java
Intent intent = new Intent("com.iauto.ogl.ACTION_RENDER_SERVICE");
intent.setClassName(servicePkgName, "com.iauto.ogl.service.OglRendererService");
appContext.bindService(intent, this, Context.BIND_AUTO_CREATE);
```

- Android 发现 Service 未运行 → 创建 `:render` 进程
- `BIND_AUTO_CREATE` 保证 Service 存活
- 同包时加 `startForegroundService` 保活

#### 5. `OglRendererService.onBind()` — 返回 AIDL Stub

```java
public IBinder onBind(Intent intent) {
    return oglRendererBinder;  // IOglRenderer.Stub 实现
}
```

客户端收到 Binder → `onServiceConnected()` → 第二次触发 `tryStartRender()`

#### 6. `nativeInit()` — C++ 层启动

```cpp
eglCore = new EGLCore();
eglCore->setJavaVM(vm);
mainScene = new MainScene();   // 创建场景（加载模型、纹理）
eglCore->setScene(mainScene);
eglCore->init(env, surface);   // 保存 Surface 的 global ref
eglCore->start();              // 创建渲染线程
```

#### 7. 渲染线程 `renderLoopInternal()`

```cpp
javaVM->AttachCurrentThread(&threadEnv, ...);  // 附加到 JVM
initEGL(threadEnv);                              // 创建 EGL context + surface
while (running) {
    applySurfaceReconnect();  // 处理 Surface 切换
    applyViewport();          // 处理尺寸变化
    applyParamUpdates();      // 处理参数更新
    scene->render();          // 渲染一帧
    eglSwapBuffers(...);      // 交换缓冲
}
```

---

## Surface 切换（前后台切换）

```
App 退后台 → surfaceDestroyed → pause() → paused = true (渲染线程进入空循环)
App 回前台 → surfaceCreated → tryStartRender()
  → isInitialized == true
  → resume() + reConnectSurface(newSurface)
  → AIDL 到 Service 进程
  → EGLCore::requestReConnectSurface()
  → 渲染线程 applySurfaceReconnect():
      1. eglDestroySurface(旧 surface)
      2. ANativeWindow_fromSurface(新 surface)
      3. eglCreateWindowSurface(新)
      4. eglMakeCurrent(重新绑定 context)
      5. scene->setViewport() → buildSceneFBO()
      6. scene->render() + eglSwapBuffers() → 显示在新 Surface 上
```

引擎不重启，模型不重新加载，只是**把渲染目标从旧 Surface 换成新 Surface**。

---

## 跨 App 切换

```
test1 运行中 (Service 在 :render 进程，引擎已初始化)

test2 启动 →
  OglSurfaceView.init("com.iauto.test1", null)
  bindService → 同一个 :render 进程
  isInitialized == true
  resume() + reConnectSurface(test2 的 Surface)
  → test2 显示 cube

test1 回前台 →
  surfaceDestroyed → pause()
  surfaceCreated → resume() + reConnectSurface(test1 的 Surface)
  → test1 继续显示
```

两个 App 共享同一个引擎实例，来回切换只是更换渲染 Surface。

---

## 关键组件

### `app` 模块 — 引擎库

| 文件 | 职责 |
|---|---|
| `NativeRenderer.java` | native 方法声明，JNI 接口 |
| `OglRendererService.java` | AIDL Stub 实现（Service 端），接收跨进程调用 |
| `IOglRenderer.aidl` | AIDL 接口定义（IPC 协议） |
| `EGLCore.h/.cpp` | EGL 生命周期管理、渲染线程循环 |
| `Scene.h/.cpp` | 场景管理：FBO、后处理链、Blit to screen |
| `MainScene.h/.cpp` | 具体场景实现 |
| `native-renderer.cpp` | JNI_OnLoad + RegisterNatives |
| `OglSurfaceView.java` | UI 端 SurfaceView 封装 |
| `RenderService.java` | 客户端单例，封装 AIDL 调用 |
| `InspectorPanel.java` | 调试面板 |

### `test1` / `test2` 模块

| 文件 | 职责 |
|---|---|
| `MainActivity.java` | 创建 OglSurfaceView，传入 servicePkgName |
| `activity_main.xml` | 布局 |
| `AndroidManifest.xml` | 声明 Activity + 权限 |

---

## Service 声明

```xml
<service
    android:name=".service.OglRendererService"
    android:exported="true"
    android:process=":render"
    android:foregroundServiceType="mediaProjection">
    <intent-filter>
        <action android:name="com.iauto.ogl.ACTION_RENDER_SERVICE" />
    </intent-filter>
</service>
```

关键：
- `android:process=":render"` — 独立进程
- `exported="true"` — 其他 App 可跨进程 bind
- intent-filter — 供 intent 解析

---

## AIDL 接口

```java
interface IOglRenderer {
    void init(in Surface surface, String filesDir);
    void reConnectSurface(in Surface surface);
    void pause();
    void resume();
    void release();
    void setViewport(int width, int height);
    void processTouchEvent(int actionMasked, int pointerCount,
                           in float[] xs, in float[] ys, in int[] ids);
    boolean isInitialized();
    float getFPS();
    void updateCameraParams(in CameraParams params);
    void updateSceneConfig(in SceneConfig config);
    void updatePostProcessConfig(in PostProcessConfig config);
    void updateMaterialParams(in MaterialParams params);
    // ...
}
```

所有引擎操作都通过 AIDL 跨进程调用，因为 `NativeRenderer` 的 native 方法在 Service 进程。

---

## 构建

```bash
# 编译 test1 APK（包含引擎库）
./gradlew :test1:assembleDebug

# 编译 test2 APK
./gradlew :test2:assembleDebug
```

---

## 关于 FBO 重建

每次 Surface 切换后（reconnect），`scene->setViewport()` 会触发 FBO 重建，原因是新 Surface 的尺寸可能不同于旧 Surface。虽然 OpenGL FBO/texture 对象在 EGL context 存活时不会失效，但它们的尺寸需要匹配新的窗口尺寸。如果尺寸没变，技术上可以复用，但当前实现采用 safe 的重建策略。
