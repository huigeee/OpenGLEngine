#ifndef UI_GROUP_H
#define UI_GROUP_H

#include "Object2D.h"
#include <vector>
#include <algorithm>

/**
 * UIGroup — UI 分组容器。
 *
 * 继承 Object2D，可包含多个子 Object2D，按 zOrder 排序后递归渲染。
 * 支持裁剪子元素、按 id/类型查找。
 */
class UIGroup : public Object2D {
public:
    UIGroup(int id = -1);
    virtual ~UIGroup();

    // ====================================================================
    // 子元素管理
    // ====================================================================
    void addChild(Object2D* child);
    void removeChild(Object2D* child);
    void removeChild(int id);
    void clear();

    Object2D* findById(int id) const;

    template<typename T>
    std::vector<T*> findByType() const {
        std::vector<T*> result;
        for (auto* child : children_) {
            T* casted = dynamic_cast<T*>(child);
            if (casted) result.push_back(casted);
            // 如果是 UIGroup，递归查找
            UIGroup* group = dynamic_cast<UIGroup*>(child);
            if (group) {
                auto sub = group->findByType<T>();
                result.insert(result.end(), sub.begin(), sub.end());
            }
        }
        return result;
    }

    const std::vector<Object2D*>& getChildren() const { return children_; }
    size_t getChildCount() const { return children_.size(); }

    // 是否裁剪子元素到本容器边界（默认 false）
    void setClipChildren(bool clip) { clipChildren_ = clip; }
    bool isClipChildren() const { return clipChildren_; }

    // ====================================================================
    // Object2D 重写
    // ====================================================================
    virtual void render() override;
    virtual bool onTouch(float screenX, float screenY, int action) override;
    virtual void onAdded() override;
    virtual void onRemoved() override;

protected:
    virtual void drawContent() override;

    /** 按 zOrder 排序 children */
    void sortChildren();

    std::vector<Object2D*> children_;
    bool clipChildren_;

    /** 用于触摸的倒序迭代器 */
    using ReverseIter = std::vector<Object2D*>::reverse_iterator;
};

#endif // UI_GROUP_H
