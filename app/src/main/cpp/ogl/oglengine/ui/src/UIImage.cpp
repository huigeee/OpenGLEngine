#include "../include/UIImage.h"
#include <GLES3/gl3.h>
#include "../../include/Common.h"
#include <png.h>
#include <cstring>
#include <vector>

UIImage::UIImage(int id)
    : Object2D(id)
    , textureId_(0)
    , texWidth_(0), texHeight_(0)
    , scaleType_(FILL)
    , useSrcRect_(false)
    , srcX_(0), srcY_(0), srcW_(0), srcH_(0)
    , borderRadius_(0)
    , hasBorder_(false)
    , borderWidth_(0)
    , borderR_(1), borderG_(1), borderB_(1), borderA_(1)
{}

UIImage::~UIImage() {
    if (textureId_) glDeleteTextures(1, &textureId_);
}

bool UIImage::setTextureFromFile(const std::string& path) {
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp) { LOGE("Cannot open: %s", path.c_str()); return false; }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) { fclose(fp); return false; }
    png_infop info = png_create_info_struct(png);
    if (!info) { png_destroy_read_struct(&png, nullptr, nullptr); fclose(fp); return false; }
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return false;
    }

    png_init_io(png, fp);
    png_read_info(png, info);
    int w = png_get_image_width(png, info);
    int h = png_get_image_height(png, info);
    png_byte colorType = png_get_color_type(png, info);
    png_byte bitDepth = png_get_bit_depth(png, info);

    if (bitDepth == 16) png_set_strip_16(png);
    if (colorType == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8) png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    if (colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_PALETTE) {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }
    if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }
    png_read_update_info(png, info);

    std::vector<unsigned char> imageData(w * h * 4);
    std::vector<png_bytep> rows(h);
    for (int y = 0; y < h; y++) rows[y] = &imageData[y * w * 4];
    png_read_image(png, rows.data());
    png_destroy_read_struct(&png, &info, nullptr);
    fclose(fp);

    uploadTexture(imageData.data(), w, h);
    return true;
}

void UIImage::setTextureId(GLuint id, int texW, int texH) {
    if (textureId_ && textureId_ != id) glDeleteTextures(1, &textureId_);
    textureId_ = id;
    texWidth_ = texW;
    texHeight_ = texH;
}

void UIImage::uploadTexture(const unsigned char* rgba, int w, int h) {
    if (textureId_) glDeleteTextures(1, &textureId_);
    texWidth_ = w;
    texHeight_ = h;
    glGenTextures(1, &textureId_);
    glBindTexture(GL_TEXTURE_2D, textureId_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    LOGI("Texture uploaded: %dx%d id=%u", w, h, textureId_);
}

void UIImage::setSrcRect(int x, int y, int w, int h) {
    useSrcRect_ = true;
    srcX_ = (float)x; srcY_ = (float)y; srcW_ = (float)w; srcH_ = (float)h;
}

void UIImage::resetSrcRect() { useSrcRect_ = false; }

void UIImage::setBorder(float width, float r, float g, float b, float a) {
    hasBorder_ = (width > 0);
    borderWidth_ = width;
    borderR_ = r; borderG_ = g; borderB_ = b; borderA_ = a;
}

void UIImage::drawContent() {
    if (!textureId_) return;

    float u0 = 0, v0 = 0, u1 = 1, v1 = 1;
    if (useSrcRect_ && texWidth_ > 0 && texHeight_ > 0) {
        u0 = srcX_ / texWidth_;
        v0 = srcY_ / texHeight_;
        u1 = (srcX_ + srcW_) / texWidth_;
        v1 = (srcY_ + srcH_) / texHeight_;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId_);
    drawTexturedQuad(0, 0, 1, 1, u0, v0, u1, v1);
    glBindTexture(GL_TEXTURE_2D, 0);
}
