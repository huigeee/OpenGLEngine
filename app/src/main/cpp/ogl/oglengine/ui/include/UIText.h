#ifndef UI_TEXT_H
#define UI_TEXT_H

#include "Object2D.h"
#include "Font.h"
#include <string>
#include <GLES3/gl3.h>

/**
 * UIText — 文本显示控件。
 *
 * 使用 stb_truetype 将文字栅格化为纹理并绘制。
 * 多个 UIText 通过共享 Font 实例避免重复加载字体文件。
 */
class UIText : public Object2D {
public:
    UIText(int id = -1);
    virtual ~UIText();

    // ====================================================================
    // 文本属性
    // ====================================================================
    void setText(const std::string& text);
    const std::string& getText() const { return text_; }

    void setFontSize(float size);
    float getFontSize() const { return fontSize_; }

    void setFontPath(const std::string& path);
    bool loadFont(const std::string& path);

    // 水平对齐: 0=LEFT, 1=CENTER, 2=RIGHT
    void setHorizontalAlign(int align) { hAlign_ = align; }
    int getHorizontalAlign() const { return hAlign_; }

    // 垂直对齐: 0=TOP, 1=MIDDLE, 2=BOTTOM
    void setVerticalAlign(int align) { vAlign_ = align; }
    int getVerticalAlign() const { return vAlign_; }

    void setLineSpacing(float spacing) { lineSpacing_ = spacing; }
    float getLineSpacing() const { return lineSpacing_; }

    void setAutoWrap(bool wrap, float maxWidth = 0);
    bool isAutoWrap() const { return autoWrap_; }

    // ====================================================================
    // Object2D 重写
    // ====================================================================
    virtual void drawContent() override;
    virtual void onBoundsChanged() override;

private:
    void rebuildTexture();
    void splitLines(const std::string& text, std::vector<std::string>& lines, float maxW);

    std::string text_;
    float fontSize_;
    std::string fontPath_;

    int hAlign_;    // 0=LEFT, 1=CENTER, 2=RIGHT
    int vAlign_;    // 0=TOP, 1=MIDDLE, 2=BOTTOM
    float lineSpacing_;
    bool autoWrap_;
    float autoWrapWidth_;

    // 纹理
    GLuint textureId_;
    int texWidth_;
    int texHeight_;
    bool textureDirty_;
    bool textureBuilt_;

    // 共享字体资源
    Font::Ptr font_;
};

#endif // UI_TEXT_H
