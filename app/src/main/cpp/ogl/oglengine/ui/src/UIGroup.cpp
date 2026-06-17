#include "../include/UIGroup.h"
#include "../include/UIRenderer.h"
#include <GLES3/gl3.h>
#include "../../include/Common.h"

UIGroup::UIGroup(int id) : Object2D(id), clipChildren_(false) {}

UIGroup::~UIGroup() { clear(); }

void UIGroup::addChild(Object2D* child) {
    if (!child) return;
    if (child->parent_) {
        child->parent_->removeChild(child);
    }
    child->parent_ = this;
    children_.push_back(child);
    sortChildren();
    child->onAdded();
}

void UIGroup::removeChild(Object2D* child) {
    if (!child) return;
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        (*it)->parent_ = nullptr;
        (*it)->onRemoved();
        children_.erase(it);
    }
}

void UIGroup::removeChild(int id) {
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if ((*it)->getId() == id) {
            (*it)->parent_ = nullptr;
            (*it)->onRemoved();
            children_.erase(it);
            return;
        }
    }
}

void UIGroup::clear() {
    for (auto* child : children_) {
        child->parent_ = nullptr;
        delete child;
    }
    children_.clear();
}

Object2D* UIGroup::findById(int id) const {
    for (auto* child : children_) {
        if (child->getId() == id) return child;
        UIGroup* group = dynamic_cast<UIGroup*>(child);
        if (group) {
            Object2D* found = group->findById(id);
            if (found) return found;
        }
    }
    return nullptr;
}

void UIGroup::sortChildren() {
    std::sort(children_.begin(), children_.end(),
        [](const Object2D* a, const Object2D* b) {
            return a->zOrder_ < b->zOrder_;
        });
}

void UIGroup::render() {
    if (!visible_) return;
    drawContent();
}

void UIGroup::drawContent() {
    for (auto* child : children_) {
        child->render();
    }
}

bool UIGroup::onTouch(float screenX, float screenY, int action) {
    if (!visible_ || !enabled_) return false;
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if ((*it)->onTouch(screenX, screenY, action)) return true;
    }
    return false;
}

void UIGroup::onAdded() {}
void UIGroup::onRemoved() {}
