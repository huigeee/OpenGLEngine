#pragma once
#include <functional>
#include <vector>
#include <memory>

// 缓动函数类型
enum class Ease {
    Linear,
    InQuad, OutQuad, InOutQuad,
    InCubic, OutCubic, InOutCubic,
    InSine, OutSine, InOutSine,
    InBack, OutBack, InOutBack,
};

// 单个 Tween 动画
class Tween {
public:
    Tween& to(float target)        { _to = target; return *this; }
    Tween& from(float f)           { _from = f; _value = f; return *this; }
    Tween& duration(float secs)    { _duration = secs; return *this; }
    Tween& ease(Ease e)            { _ease = e; return *this; }
    Tween& loop(bool l)            { _loop = l; return *this; }
    Tween& pingPong(bool p)        { _pingPong = p; return *this; }
    Tween& delay(float d)          { _delay = d; return *this; }
    Tween& onUpdate(std::function<void(float)> cb)   { _onUpdate = cb;   return *this; }
    Tween& onComplete(std::function<void()>   cb)    { _onComplete = cb; return *this; }

    void   play()   { _playing = true;  _elapsed = 0; _delayElapsed = 0; _forward = true; }
    void   pause()  { _playing = false; }
    void   resume() { _playing = true; }
    void   stop()   { _playing = false; _elapsed = 0; }
    bool   isPlaying() const { return _playing; }
    bool   isDone()    const { return _done; }
    float  value()     const { return _value; }

    // 返回 true 表示已结束（非 loop）
    bool update(float dt);

private:
    float applyEase(float t) const;

    float _from = 0, _to = 1, _value = 0;
    float _duration = 1.0f;
    float _elapsed  = 0;
    float _delay = 0, _delayElapsed = 0;
    Ease  _ease = Ease::Linear;
    bool  _loop = false, _pingPong = false;
    bool  _playing = false, _done = false, _forward = true;
    std::function<void(float)> _onUpdate;
    std::function<void()>      _onComplete;
};

// 管理所有 Tween
class TweenManager {
public:
    static TweenManager& instance() {
        static TweenManager inst;
        return inst;
    }

    // 创建并立即播放一个 float Tween
    Tween* create(float from, float to, float duration, Ease ease = Ease::Linear);

    void update(float dt);
    void clear();

private:
    std::vector<std::unique_ptr<Tween>> tweens;
};
