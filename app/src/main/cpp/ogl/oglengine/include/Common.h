#pragma once

#include <android/log.h>

#define OGL_LOG_TAG "ogl"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, OGL_LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, OGL_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, OGL_LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, OGL_LOG_TAG, __VA_ARGS__)
