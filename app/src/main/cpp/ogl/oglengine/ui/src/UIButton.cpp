#include "../include/UIButton.h"
#include "../include/UIRenderer.h"
#include <GLES3/gl3.h>
#include "../../include/Common.h"

UIButton::UIButton(int id)
    : Object2D(id)
    , state_(NORMAL)
    , label_(id * 1000 + 1)
    , normalImg_(nullptr)
    , pressedImg_(nullptr)
    , disabledImg_(nullptr)
    , ownImages_(false)
{}

UIButton::~UIButton() {
    if (ownImages_) {
        delete normalImg_;
        delete pressedImg_;
        delete disabledImg_;
    }
}

void UIButton::setLabel(const std::string& text, float fontSize) {
    label_.setText(text);
    label_.setFontSize(fontSize);
    label_.onBoundsChanged();
}

void UIButton::setNormalImage(const std::string& path) {
    if (!normalImg_) { normalImg_ = new UIImage(id_ * 1000 + 2); ownImages_ = true; }
    normalImg_->setTextureFromFile(path);
    normalImg_->setSize(width_, height_);
}

void UIButton::setPressedImage(const std::string& path) {
    if (!pressedImg_) { pressedImg_ = new UIImage(id_ * 1000 + 3); ownImages_ = true; }
    pressedImg_->setTextureFromFile(path);
    pressedImg_->setSize(width_, height_);
}

void UIButton::setDisabledImage(const std::string& path) {
    if (!disabledImg_) { disabledImg_ = new UIImage(id_ * 1000 + 4); ownImages_ = true; }
    disabledImg_->setTextureFromFile(path);
    disabledImg_->setSize(width_, height_);
}

void UIButton::setState(State s) { state_ = s; }

void UIButton::drawContent() {
    // 绘制背景
    UIImage* bg = nullptr;
    switch (state_) {
        case NORMAL:   bg = normalImg_; break;
        case PRESSED:  bg = pressedImg_; break;
        case DISABLED: bg = disabledImg_; break;
    }
    if (bg) {
        bg->setSize(width_, height_);
        bg->drawContent();
    } else {
        float r, g, b, a;
        switch (state_) {
            case NORMAL:   r = 0.3f; g = 0.5f; b = 0.9f; a = 0.8f; break;
            case PRESSED:  r = 0.2f; g = 0.4f; b = 0.8f; a = 1.0f; break;
            case DISABLED: r = 0.5f; g = 0.5f; b = 0.5f; a = 0.5f; break;
        }
        drawSolidRect(0, 0, width_, height_, r, g, b, a);
    }

    // 绘制文字
    label_.drawContent();
}

bool UIButton::onTouch(float screenX, float screenY, int action) {
    if (!visible_ || !enabled_) return false;
    if (!hitTest(screenX, screenY)) {
        if (state_ == PRESSED) state_ = NORMAL;
        return false;
    }
    if (action == 0) {
        state_ = PRESSED;
        return true;
    } else if (action == 1) {
        if (state_ == PRESSED && onClick_) onClick_(id_);
        state_ = NORMAL;
        return true;
    }
    return false;
}

void UIButton::onBoundsChanged() {
    label_.setSize(width_, height_);
    if (normalImg_) normalImg_->setSize(width_, height_);
    if (pressedImg_) pressedImg_->setSize(width_, height_);
    if (disabledImg_) disabledImg_->setSize(width_, height_);
}
