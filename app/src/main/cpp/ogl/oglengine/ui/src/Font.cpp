#define STB_TRUETYPE_IMPLEMENTATION
#include "../include/Font.h"
#include "../../include/Common.h"
#include <fstream>

std::unordered_map<std::string, std::weak_ptr<Font>> Font::s_cache;
std::mutex Font::s_mutex;

Font::Ptr Font::load(const std::string& path) {
    std::lock_guard<std::mutex> lock(s_mutex);

    // 尝试从缓存获取
    auto it = s_cache.find(path);
    if (it != s_cache.end()) {
        auto ptr = it->second.lock();
        if (ptr) {
            LOGI("Font cache hit: %s", path.c_str());
            return ptr;
        }
        // weak_ptr 已过期，移除
        s_cache.erase(it);
    }

    // 创建新的 Font 实例
    auto font = std::make_shared<Font>();
    if (!font->initFromFile(path)) {
        LOGE("Failed to load font: %s", path.c_str());
        return nullptr;
    }

    s_cache[path] = font;
    LOGI("Font loaded: %s", path.c_str());
    return font;
}

bool Font::initFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LOGE("Cannot open font: %s", path.c_str());
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0);
    data_.resize(size);
    if (!file.read((char*)data_.data(), size)) {
        LOGE("Failed to read font: %s", path.c_str());
        return false;
    }
    file.close();

    if (stbtt_InitFont(info_, data_, 0) == 0) {
        LOGE("stbtt_InitFont failed: %s", path.c_str());
        return false;
    }

    valid_ = true;
    return true;
}
