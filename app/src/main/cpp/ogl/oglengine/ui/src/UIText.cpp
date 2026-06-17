#define STB_TRUETYPE_IMPLEMENTATION
#include "../include/UIText.h"
#include "../include/Font.h"
#include <GLES3/gl3.h>
#include "../../include/Common.h"
#include <cstring>
#include <vector>

UIText::UIText(int id)
    : Object2D(id)
    , fontSize_(24.0f)
    , hAlign_(0), vAlign_(0)
    , lineSpacing_(1.2f)
    , autoWrap_(false), autoWrapWidth_(0)
    , textureId_(0), texWidth_(0), texHeight_(0)
    , textureDirty_(true), textureBuilt_(false)
{}

UIText::~UIText() {
    if (textureId_) glDeleteTextures(1, &textureId_);
}

void UIText::setText(const std::string& text) {
    if (text_ != text) {
        text_ = text;
        textureDirty_ = true;
        textureBuilt_ = false;
    }
}

void UIText::setFontSize(float size) {
    if (fontSize_ != size) {
        fontSize_ = size;
        textureDirty_ = true;
        textureBuilt_ = false;
    }
}

void UIText::setFontPath(const std::string& path) {
    if (fontPath_ != path) {
        fontPath_ = path;
        font_ = (path.empty()) ? nullptr : Font::load(path);
        textureDirty_ = true;
        textureBuilt_ = false;
    }
}

bool UIText::loadFont(const std::string& path) {
    setFontPath(path);
    return font_ != nullptr;
}

void UIText::setAutoWrap(bool wrap, float maxWidth) {
    autoWrap_ = wrap;
    autoWrapWidth_ = maxWidth;
    textureDirty_ = true;
}

void UIText::rebuildTexture() {
    if (textureId_) {
        glDeleteTextures(1, &textureId_);
        textureId_ = 0;
    }
    textureDirty_ = false;
    if (text_.empty()) { texWidth_ = texHeight_ = 0; return; }

    // 辅助函数：创建 1x1 占位纹理
    auto createPlaceholder = [this]() {
        const unsigned char pixel[] = {255, 255, 255, 255};
        texWidth_ = 1; texHeight_ = 1;
        glGenTextures(1, &textureId_);
        glBindTexture(GL_TEXTURE_2D, textureId_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    };

    // 检查字体是否有效
    if (!font_ || !font_->isValid()) {
        createPlaceholder();
        return;
    }

    stbtt_fontinfo& font = font_->getInfo();
    float scale = font_->getScaleForPixelHeight(fontSize_);

    // 计算总尺寸
    float totalW = 0, curLineW = 0;
    int totalLines = 1;
    for (size_t i = 0; i < text_.size(); i++) {
        unsigned char c = (unsigned char)text_[i];
        if (c == '\n') {
            if (curLineW > totalW) totalW = curLineW;
            curLineW = 0;
            totalLines++;
            continue;
        }
        std::int_fast32_t glyph = stbtt_FindGlyphIndex(font, c);
        if (glyph == 0) continue;
        std::int_fast32_t advance = 0, lsb = 0;
        stbtt_GetGlyphHMetrics(font, glyph, advance, lsb);
        curLineW += advance * scale;
    }
    if (curLineW > totalW) totalW = curLineW;

    float totalH = totalLines * fontSize_ * lineSpacing_;
    if (totalH < fontSize_) totalH = fontSize_;

    int padding = 2;
    int bmpW = std::max((int)(totalW + padding * 2), 1);
    int bmpH = std::max((int)(totalH + padding * 2), 1);

    // 分配 bitmap
    std::vector<unsigned char> bitmap(bmpW * bmpH, 0);

    // 逐个字符渲染
    float xPos = (float)padding;
    float baselineY = (float)padding + fontSize_ * 0.8f;
    float yPos = baselineY;

    for (size_t i = 0; i < text_.size(); i++) {
        unsigned char c = (unsigned char)text_[i];
        if (c == '\n') {
            xPos = (float)padding;
            yPos += fontSize_ * lineSpacing_;
            continue;
        }
        std::int_fast32_t glyph = stbtt_FindGlyphIndex(font, c);
        if (glyph == 0) continue;

        std::int_fast32_t ix0 = 0, iy0 = 0, ix1 = 0, iy1 = 0;
        stbtt_GetGlyphBitmapBox(font, glyph, scale, scale, ix0, iy0, ix1, iy1);
        int cw = (int)(ix1 - ix0);
        int ch = (int)(iy1 - iy0);

        if (cw > 0 && ch > 0) {
            std::vector<unsigned char> charBmp(cw * ch, 0);
            stbtt_MakeGlyphBitmap(font, charBmp.data(), cw, ch, cw, scale, scale, glyph);

            int startX = (int)xPos + (int)ix0;
            int startY = (int)yPos + (int)iy0;
            for (int row = 0; row < ch; row++) {
                for (int col = 0; col < cw; col++) {
                    int dstIdx = (startY + row) * bmpW + (startX + col);
                    int srcIdx = row * cw + col;
                    if (dstIdx >= 0 && dstIdx < bmpW * bmpH && srcIdx < cw * ch) {
                        bitmap[dstIdx] = std::max(bitmap[dstIdx], charBmp[srcIdx]);
                    }
                }
            }
        }

        std::int_fast32_t advance = 0, lsb = 0;
        stbtt_GetGlyphHMetrics(font, glyph, advance, lsb);
        xPos += advance * scale;
    }

    // 转换为 RGBA
    std::vector<unsigned char> rgba(bmpW * bmpH * 4);
    for (int i = 0; i < bmpW * bmpH; i++) {
        unsigned char v = bitmap[i];
        rgba[i * 4 + 0] = 255;
        rgba[i * 4 + 1] = 255;
        rgba[i * 4 + 2] = 255;
        rgba[i * 4 + 3] = v;
    }

    texWidth_ = bmpW;
    texHeight_ = bmpH;

    glGenTextures(1, &textureId_);
    glBindTexture(GL_TEXTURE_2D, textureId_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth_, texHeight_, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void UIText::onBoundsChanged() {
    textureDirty_ = true;
}

void UIText::drawContent() {
    if (text_.empty()) return;
    if (textureBuilt_ && !textureDirty_) {
        // 首次构建后且未变脏，跳过
    } else if (textureDirty_ || !textureId_) {
        rebuildTexture();
        textureBuilt_ = true;
    }
    if (!textureId_) return;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId_);
    drawTexturedQuad(0, 0, 1, 1, 0, 0, 1, 1);
    glBindTexture(GL_TEXTURE_2D, 0);
}
