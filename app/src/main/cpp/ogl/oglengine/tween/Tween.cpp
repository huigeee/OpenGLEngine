#include "Tween.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// -----------------------------------------------------------------------
// Easing functions  (t in [0,1])
// -----------------------------------------------------------------------
float Tween::applyEase(float t) const {
    switch (_ease) {
        case Ease::Linear:      return t;
        case Ease::InQuad:      return t * t;
        case Ease::OutQuad:     return t * (2.0f - t);
        case Ease::InOutQuad:   return t < 0.5f ? 2*t*t : -1+(4-2*t)*t;
        case Ease::InCubic:     return t * t * t;
        case Ease::OutCubic:    { float s = t-1; return s*s*s + 1; }
        case Ease::InOutCubic:  return t < 0.5f ? 4*t*t*t : (t-1)*(2*t-2)*(2*t-2)+1;
        case Ease::InSine:      return 1.0f - cosf(t * (float)M_PI * 0.5f);
        case Ease::OutSine:     return sinf(t * (float)M_PI * 0.5f);
        case Ease::InOutSine:   return 0.5f * (1.0f - cosf(t * (float)M_PI));
        case Ease::InBack:      { float c = 1.70158f; return t*t*((c+1)*t - c); }
        case Ease::OutBack:     { float c = 1.70158f; float s = t-1; return s*s*((c+1)*s+c)+1; }
        case Ease::InOutBack:   {
            float c = 1.70158f * 1.525f;
            return t < 0.5f
                ? (powf(2*t,2)*((c+1)*2*t - c)) / 2
                : (powf(2*t-2,2)*((c+1)*(2*t-2)+c)+2) / 2;
        }
        default: return t;
    }
}

// -----------------------------------------------------------------------
// Tween::update
// -----------------------------------------------------------------------
bool Tween::update(float dt) {
    if (!_playing || _done) return _done;

    // delay
    if (_delayElapsed < _delay) {
        _delayElapsed += dt;
        return false;
    }

    _elapsed += dt;
    float raw = std::min(_elapsed / _duration, 1.0f);
    float t   = _forward ? raw : (1.0f - raw);
    _value    = _from + (_to - _from) * applyEase(t);

    if (_onUpdate) _onUpdate(_value);

    if (_elapsed >= _duration) {
        if (_loop) {
            _elapsed = 0;
            if (_pingPong) _forward = !_forward;
        } else {
            _playing = false;
            _done    = true;
            if (_onComplete) _onComplete();
            return true;
        }
    }
    return false;
}

// -----------------------------------------------------------------------
// TweenManager
// -----------------------------------------------------------------------
Tween* TweenManager::create(float from, float to, float duration, Ease ease) {
    auto t = std::make_unique<Tween>();
    t->from(from).to(to).duration(duration).ease(ease);
    t->play();
    Tween* raw = t.get();
    tweens.push_back(std::move(t));
    return raw;
}

void TweenManager::update(float dt) {
    tweens.erase(
        std::remove_if(tweens.begin(), tweens.end(),
            [dt](std::unique_ptr<Tween>& t) {
                return t->update(dt);  // true = done, remove
            }),
        tweens.end());
}

void TweenManager::clear() {
    tweens.clear();
}
