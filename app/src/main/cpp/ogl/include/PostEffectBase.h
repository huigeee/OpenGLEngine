#pragma once

#include <GLES3/gl3.h>
#include "RenderContext.h"

// Effect type enum for logging
enum class PostEffectType {
    Unknown = 0,
    Fog,
    TAA,
    DoF,
    Bloom,
    Tonemap,
    FXAA,
    Shadow
};

class PostEffectBase {
public:
    bool enabled = true;
    virtual ~PostEffectBase() = default;
    virtual void init(int w, int h) = 0;
    virtual RenderContext render(const RenderContext& input) = 0;
    virtual void release() = 0;
    virtual void onSurfaceChanged(int w, int h) {
        if (w > 0 && h > 0 && (w != currentWidth || h != currentHeight)) {
            release();
            init(w, h);
        }
    }
    virtual PostEffectType getType() const { return PostEffectType::Unknown; }
protected:
    int currentWidth = 0;
    int currentHeight = 0;
};
