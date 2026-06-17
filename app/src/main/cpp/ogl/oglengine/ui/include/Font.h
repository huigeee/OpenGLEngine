#ifndef FONT_H
#define FONT_H

#include "../../../stb_truetype/stb_truetype.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

/**
 * Font — 共享字体资源。
 *
 * 封装 stb_truetype 字体数据，多个 UIText 通过字体路径共享同一实例。
 * 使用引用计数自动管理生命周期。
 */
class Font {
public:
    using Ptr = std::shared_ptr<Font>;

    /** 从文件路径加载字体，返回共享指针。相同路径复用缓存实例。 */
    static Ptr load(const std::string& path);

    /** 获取 stb_truetype fontinfo（用于栅格化） */
    stbtt_fontinfo& getInfo() { return info_; }

    /** 获取缩放比例 */
    float getScaleForPixelHeight(float fontSize) const {
        return stbtt_ScaleForPixelHeight(info_, fontSize);
    }

    /** 是否有效 */
    bool isValid() const { return valid_; }

    Font() = default;
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    static std::unordered_map<std::string, std::weak_ptr<Font>> s_cache;
    static std::mutex s_mutex;

private:
    bool initFromFile(const std::string& path);

    std::vector<unsigned char> data_;
    stbtt_fontinfo info_{};
    bool valid_ = false;
};

#endif // FONT_H
