#ifndef UI_IMAGE_H
#define UI_IMAGE_H

#include "Object2D.h"
#include <GLES3/gl3.h>
#include <string>

/**
 * UIImage — 图片显示控件。
 *
 * 支持两种纹理来源：
 *   1. setTextureFromFile(path) — 内部用 libpng 解码并上传 GL 纹理
 *   2. setTextureId(id) — 外部传入已有 GL 纹理 ID
 *
 * 支持缩放模式、圆角、边框。
 */
class UIImage : public Object2D {
public:
    enum ScaleType {
        FILL,       // 拉伸填满（默认）
        FIT,        // 保持比例，完整显示
        STRETCH,    // 同 FILL
        TILE        // 平铺
    };

    UIImage(int id = -1);
    virtual ~UIImage();

    // ====================================================================
    // 纹理管理
    // ====================================================================
    /** 从文件路径加载纹理（内部用 libpng 解码） */
    bool setTextureFromFile(const std::string& path);

    /** 使用外部传入的 GL 纹理 ID */
    void setTextureId(GLuint id, int texW = 0, int texH = 0);

    GLuint getTextureId() const { return textureId_; }
    bool hasTexture() const { return textureId_ != 0; }

    // ====================================================================
    // 显示属性
    // ====================================================================
    void setScaleType(ScaleType type) { scaleType_ = type; }
    ScaleType getScaleType() const { return scaleType_; }

    // 纹理裁剪区域（像素坐标，默认全纹理）
    void setSrcRect(int x, int y, int w, int h);
    void resetSrcRect();

    // 圆角半径
    void setBorderRadius(float r) { borderRadius_ = r; }
    float getBorderRadius() const { return borderRadius_; }

    // 边框
    void setBorder(float width, float r, float g, float b, float a = 1.0f);

    // ====================================================================
    // Object2D 重写
    // ====================================================================
    virtual void drawContent() override;

private:
    /** 从 RGBA 数据上传 GL 纹理 */
    void uploadTexture(const unsigned char* rgba, int w, int h);

    GLuint textureId_;

    // 原始纹理尺寸
    int texWidth_;
    int texHeight_;

    ScaleType scaleType_;

    // 裁剪区域
    bool useSrcRect_;
    float srcX_, srcY_, srcW_, srcH_;

    // 圆角
    float borderRadius_;

    // 边框
    bool hasBorder_;
    float borderWidth_;
    float borderR_, borderG_, borderB_, borderA_;
};

#endif // UI_IMAGE_H
