#ifndef OBJECT2D_H
#define OBJECT2D_H

#include <GLES3/gl3.h>
#include <vector>
#include <functional>
#include <string>

// Forward declarations
class UIGroup;

/**
 * Object2D — 2D UI 元素基类。
 */
class Object2D {
public:
    Object2D(int id = -1);
    virtual ~Object2D();

    int getId() const { return id_; }
    void setId(int id) { id_ = id; }

    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }

    float getX() const { return x_; }
    float getY() const { return y_; }
    virtual void setPosition(float x, float y);

    float getWidth() const { return width_; }
    float getHeight() const { return height_; }
    virtual void setSize(float w, float h);

    float getAnchorX() const { return anchorX_; }
    float getAnchorY() const { return anchorY_; }
    void setAnchor(float ax, float ay);

    float getRotation() const { return rotation_; }
    void setRotation(float degrees);

    float getScaleX() const { return scaleX_; }
    float getScaleY() const { return scaleY_; }
    void setScale(float sx, float sy);

    bool isVisible() const { return visible_; }
    virtual void setVisible(bool v);
    void show() { setVisible(true); }
    void hide() { setVisible(false); }

    bool isEnabled() const { return enabled_; }
    virtual void setEnabled(bool e) { enabled_ = e; }

    float getAlpha() const { return alpha_; }
    void setAlpha(float a);

    void setColor(float r, float g, float b, float a = 1.0f);

    int getZOrder() const { return zOrder_; }
    void setZOrder(int z);

    void setUserData(void* data) { userData_ = data; }
    void* getUserData() const { return userData_; }

    UIGroup* getParent() const { return parent_; }

    float getAbsoluteX() const;
    float getAbsoluteY() const;

    virtual bool hitTest(float screenX, float screenY) const;
    virtual bool onTouch(float screenX, float screenY, int action);
    virtual void render();

    virtual void onAdded() {}
    virtual void onRemoved() {}
    virtual void onBoundsChanged() {}

    void removeFromParent();

protected:
    // 子类实现具体的 OpenGL 绘制（已绑定正交投影、开启 blend）
    virtual void drawContent() {}

    // 绘制一个带纹理的四边形（x,y,w,h 在本地坐标，uv 0~1）
    void drawTexturedQuad(float x, float y, float w, float h,
                          float u0, float v0, float u1, float v1);

    // 绘制一个纯色矩形
    void drawSolidRect(float x, float y, float w, float h,
                       float r, float g, float b, float a);

    int id_;
    std::string name_;
    float x_, y_, width_, height_;
    float anchorX_, anchorY_;
    float rotation_;
    float scaleX_, scaleY_;
    bool visible_;
    bool enabled_;
    float alpha_;
    float colorR_, colorG_, colorB_, colorA_;
    int zOrder_;
    void* userData_;
    UIGroup* parent_;

    friend class UIGroup;
};

#endif // OBJECT2D_H
