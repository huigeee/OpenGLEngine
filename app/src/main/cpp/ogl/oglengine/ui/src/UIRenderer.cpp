#include "../include/UIRenderer.h"
#include <GLES3/gl3.h>
#include "../../include/Common.h"

static UIRenderer* g_instance = nullptr;

UIRenderer* UIRenderer::instance() {
    if (!g_instance) {
        g_instance = new UIRenderer();
    }
    return g_instance;
}

UIRenderer::UIRenderer()
    : root_(-1)
    , screenWidth_(0)
    , screenHeight_(0)
    , touchDown_(false)
    , touchAction_(-1)
{
    root_.setName("UI Root");
}

UIRenderer::~UIRenderer() {}

void UIRenderer::setViewport(int width, int height) {
    screenWidth_ = width;
    screenHeight_ = height;
    root_.setSize((float)width, (float)height);
}

void UIRenderer::render() {
    if (screenWidth_ <= 0 || screenHeight_ <= 0) return;

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, screenWidth_, screenHeight_);

    root_.render();

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

bool UIRenderer::handleTouch(float x, float y, int action) {
    touchAction_ = action;
    if (action == 0) touchDown_ = true;
    else if (action == 1 || action == 3) touchDown_ = false;
    return root_.onTouch(x, y, action);
}
