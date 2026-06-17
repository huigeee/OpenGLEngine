#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "Object2D.h"
#include "UIText.h"
#include "UIImage.h"
#include <functional>

class UIButton : public Object2D {
public:
    enum State { NORMAL = 0, PRESSED, DISABLED };

    UIButton(int id = -1);
    virtual ~UIButton();

    void setLabel(const std::string& text, float fontSize = 24);
    UIText* getLabel() { return &label_; }

    void setNormalImage(const std::string& path);
    void setPressedImage(const std::string& path);
    void setDisabledImage(const std::string& path);

    void setState(State state);
    State getState() const { return state_; }
    bool isPressed() const { return state_ == PRESSED; }

    // 非 virtual override，直接用父类的 setEnabled
    using Object2D::setEnabled;
    void setButtonEnabled(bool e) {
        Object2D::setEnabled(e);
        state_ = e ? NORMAL : DISABLED;
    }

    using ClickCallback = std::function<void(int id)>;
    void setOnClick(ClickCallback cb) { onClick_ = cb; }

    virtual void drawContent() override;
    virtual bool onTouch(float screenX, float screenY, int action) override;
    virtual void onBoundsChanged() override;

private:
    State state_;
    UIText label_;
    UIImage* normalImg_;
    UIImage* pressedImg_;
    UIImage* disabledImg_;
    bool ownImages_;
    ClickCallback onClick_;
};

#endif // UI_BUTTON_H
