#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include "Object2D.h"
#include "UIGroup.h"
#include <GLES3/gl3.h>

/**
 * UIRenderer — 2D UI 渲染器单例。
 *
 * 管理一个根 UIGroup，提供全局渲染、触摸分发、视口更新。
 * 在 Scene::render() 末尾调用 render()。
 */
class UIRenderer {
public:
    static UIRenderer* instance();

    /** 获取根容器（所有 UI 元素的顶层父节点） */
    UIGroup* getRoot() { return &root_; }

    /** 设置屏幕尺寸，更新正交投影矩阵 */
    void setViewport(int width, int height);

    /** 渲染所有 UI（在默认 framebuffer 上，blitToScreen 之后调用） */
    void render();

    /** 触摸分发：从根容器开始命中检测，返回 true 表示 UI 消费了事件 */
    bool handleTouch(float x, float y, int action);

    int getScreenWidth() const { return screenWidth_; }
    int getScreenHeight() const { return screenHeight_; }

private:
    UIRenderer();
    ~UIRenderer();
    UIRenderer(const UIRenderer&) = delete;
    UIRenderer& operator=(const UIRenderer&) = delete;

    UIGroup root_;
    int screenWidth_;
    int screenHeight_;

    // 触摸状态
    bool touchDown_;
    int touchAction_;
};

#endif // UI_RENDERER_H
