#include "ShadowEffect.h"
#include "../include/Shader.h"
#include "../include/Common.h"
#include "../include/Scene.h"  // 需要完整定义来调用 renderForShadowMap()
#include <android/log.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <cstring>

// Shadow pass 着色器 - 正确的深度渲染
static const char* SHADOW_VS = R"(
#version 300 es
layout(location=0) in vec3 aPos;

uniform mat4 uLightSpaceMatrix;
uniform mat4 uModelMatrix;

void main() {
    vec4 worldPos = uModelMatrix * vec4(aPos, 1.0);
    gl_Position = uLightSpaceMatrix * worldPos;
}
)";

static const char* SHADOW_FS = R"(
#version 300 es
precision highp float;

void main() {
    // gl_FragDepth 自动写入深度缓冲区/深度纹理
    // 值范围 [0, 1]，与 gl_FragCoord.z 相同
    gl_FragDepth = gl_FragCoord.z;
}
)";

ShadowEffect::ShadowEffect() {
    enabled = false;  // 默认关闭，性能考虑
    shadowBias = 0.005f;  // Bias for shadow mapping
    shadowSoftness = 0.5f;
    shadowProgram = 0;
    loc_shadowModel = -1;
    loc_shadowLightSpace = -1;
    shadowMapSize = 4096;  // Increase resolution for smoother shadows
}

ShadowEffect::~ShadowEffect() {
    release();
}

void ShadowEffect::init(int w, int h) {
    if (w <= 0 || h <= 0) return;
    
    currentWidth = w;
    currentHeight = h;
    
    LOGI("ShadowEffect::init(%d, %d)", w, h);
    
    // 确保 enabled 标志为 true
    enabled = true;
    
    // 创建阴影着色器程序
    createShadowShader();
    
    // 创建阴影贴图
    buildShadowMap(shadowMapSize, shadowMapSize);
    
    // 更新光源矩阵
    updateLightSpaceMatrix();
}

void ShadowEffect::buildShadowMap(int w, int h) {
    // 释放旧的
    if (shadowFBO != 0) {
        glDeleteFramebuffers(1, &shadowFBO);
        shadowFBO = 0;
    }
    if (shadowMapTexture != 0) {
        glDeleteTextures(1, &shadowMapTexture);
        shadowMapTexture = 0;
    }
    
    // 创建深度纹理 - 用于阴影采样
    glGenTextures(1, &shadowMapTexture);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
    // 使用 DEPTH_COMPONENT24 而不是 32F，兼容性更好
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadowMapSize, shadowMapSize, 0,
                 GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    // 深度纹理必须使用 NEAREST 滤波，不能线性插值！
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // 禁用 mipmap 避免需要生成
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // 创建 FBO
    glGenFramebuffers(1, &shadowFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapTexture, 0);
    
    // 只使用深度附件，不需要颜色附件
    // OpenGL ES 3.0 支持仅深度 FBO
    
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("Shadow FBO incomplete: 0x%x", status);
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ShadowEffect::release() {
    if (shadowProgram) {
        glDeleteProgram(shadowProgram);
        shadowProgram = 0;
    }
    if (shadowFBO != 0) {
        glDeleteFramebuffers(1, &shadowFBO);
        shadowFBO = 0;
    }
    if (shadowMapTexture != 0) {
        glDeleteTextures(1, &shadowMapTexture);
        shadowMapTexture = 0;
    }
    currentWidth = 0;
    currentHeight = 0;
}

void ShadowEffect::onSurfaceChanged(int w, int h) {
    if (w > 0 && h > 0 && (w != currentWidth || h != currentHeight)) {
        if (enabled) {
            release();
            init(w, h);
        }
    }
}

void ShadowEffect::createShadowShader() {
    if (shadowProgram) return;
    
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &SHADOW_VS, nullptr);
    glCompileShader(vs);
    
    GLint compiled = 0;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint logLen = 0;
        glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(logLen);
        glGetShaderInfoLog(vs, logLen, nullptr, log.data());
        LOGE("Shadow VS compile error: %s", log.data());
    }
    
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &SHADOW_FS, nullptr);
    glCompileShader(fs);
    
    glGetShaderiv(fs, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint logLen = 0;
        glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(logLen);
        glGetShaderInfoLog(fs, logLen, nullptr, log.data());
        LOGE("Shadow FS compile error: %s", log.data());
    }
    
    shadowProgram = glCreateProgram();
    glAttachShader(shadowProgram, vs);
    glAttachShader(shadowProgram, fs);
    glLinkProgram(shadowProgram);
    
    GLint linked = 0;
    glGetProgramiv(shadowProgram, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint logLen = 0;
        glGetProgramiv(shadowProgram, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(logLen);
        glGetProgramInfoLog(shadowProgram, logLen, nullptr, log.data());
        LOGE("Shadow program link error: %s", log.data());
    }
    
    loc_shadowModel = glGetUniformLocation(shadowProgram, "uModelMatrix");
    loc_shadowLightSpace = glGetUniformLocation(shadowProgram, "uLightSpaceMatrix");
    
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    LOGI("Shadow shader created: program=%d", shadowProgram);
}

RenderContext ShadowEffect::render(const RenderContext& input) {
    // ShadowEffect 不是后处理效果，它是在主渲染之前执行的
    // 这个函数不会被调用，shadow pass 在 Scene::renderSceneToFBO 之前执行
    return input;
}

void ShadowEffect::renderShadowPass() {
    if (!enabled || shadowProgram == 0 || shadowFBO == 0) {
        LOGI("Shadow pass skipped: enabled=%d program=%d fbo=%d", enabled, shadowProgram, shadowFBO);
        return;
    }
    
    // 保存当前状态
    GLint prevFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    GLint prevProg = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProg);
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    GLint prevDepthMask;
    glGetIntegerv(GL_DEPTH_WRITEMASK, &prevDepthMask);
    GLint prevDepthFunc;
    glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);
    GLint prevCullFace;
    glGetIntegerv(GL_CULL_FACE_MODE, &prevCullFace);
    GLboolean prevCullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    
    // 绑定阴影 FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glViewport(0, 0, shadowMapSize, shadowMapSize);
    
    // 只清除深度缓冲区
    glClear(GL_DEPTH_BUFFER_BIT);
    
    // 启用深度测试
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);  // 启用深度写入
    glDepthFunc(GL_LEQUAL);
    
    // 关闭面剔除 - 渲染两面以避免阴影断裂
    glDisable(GL_CULL_FACE);
    
    // 使用阴影着色器
    glUseProgram(shadowProgram);
    
    // 设置 lightSpaceMatrix（对所有物体相同）
    glUniformMatrix4fv(loc_shadowLightSpace, 1, GL_FALSE, lightSpaceMatrix);
    
    // Set up shadow shader and render all objects
    if (sceneRef) {
        Scene* scene = static_cast<Scene*>(sceneRef);
        scene->renderObjectsForShadow(shadowProgram, loc_shadowModel, loc_shadowLightSpace);
    }
    
    // 恢复状态
    glDepthMask(prevDepthMask != 0);
    glDepthFunc(prevDepthFunc);
    if (prevCullFaceEnabled) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
    glCullFace(prevCullFace);
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glUseProgram(prevProg);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}

void ShadowEffect::setLightPosition(float x, float y, float z) {
    lightPos[0] = x;
    lightPos[1] = y;
    lightPos[2] = z;
    updateLightSpaceMatrix();
}

void ShadowEffect::updateLightSpaceMatrix() {
    // 创建光源的 view 矩阵（从光源看向场景中心）
    glm::mat4 lightViewMat = glm::lookAt(
        glm::vec3(lightPos[0], lightPos[1], lightPos[2]),
        glm::vec3(0.0f, 0.0f, 0.0f),  // 看向场景中心
        glm::vec3(0.0f, 1.0f, 0.0f)   // up 向量
    );
    
    // 创建正交投影（方向光使用正交投影）
    float shadowExtent = shadowDistance;
    float shadowNear = shadowNearPlane;
    float shadowFar = shadowFarPlane;
    
    glm::mat4 lightProjectionMat = glm::ortho(
        -shadowExtent, shadowExtent,
        -shadowExtent, shadowExtent,
        shadowNear, shadowFar
    );
    
    // 组合成 lightSpaceMatrix
    glm::mat4 lightSpace = lightProjectionMat * lightViewMat;
    
    // 复制到数组
    memcpy(lightSpaceMatrix, glm::value_ptr(lightSpace), 16 * sizeof(float));
    memcpy(lightView, glm::value_ptr(lightViewMat), 16 * sizeof(float));
    memcpy(lightProjection, glm::value_ptr(lightProjectionMat), 16 * sizeof(float));
}
