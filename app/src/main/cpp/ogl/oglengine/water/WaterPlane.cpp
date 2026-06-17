#include "../include/WaterPlane.h"
#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <android/log.h>
#include <cmath>
#include <vector>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "WaterPlane", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "WaterPlane", __VA_ARGS__)

    // 水面顶点 shader — XZ 平面，Y 轴向上
    // 使用多方向 Gerstner 波模拟，产生自然的水面波动
    static const char* kWaterVertShader = R"(
    #version 300 es
    in vec3 aPos;
    in vec3 aNormal;

    uniform mat4 uModel;
    uniform mat4 uView;
    uniform mat4 uProj;
    uniform float uTime;
    uniform float uWaveSpeed;
    uniform float uWaveHeight;

    out vec3 vWorldPos;
    out vec3 vNormal;
    out float vHeight;

    void main() {
        vec3 pos = aPos;

        // 多方向波浪——8 个不同方向、频率、相位的波
        // wave(dirX, dirZ, freq, phase, amp)
        vec2 dirs[8] = vec2[8](
            vec2( 1.0,  0.3),
            vec2(-0.7,  0.7),
            vec2( 0.2,  1.0),
            vec2(-0.5, -0.8),
            vec2( 0.9, -0.4),
            vec2(-1.0,  0.1),
            vec2( 0.4, -0.9),
            vec2(-0.2,  0.6)
        );
        float freqs[8] = float[8](1.2, 0.9, 1.5, 0.7, 1.0, 0.6, 1.3, 0.8);
        float phases[8] = float[8](0.0, 1.2, 2.5, 0.5, 3.0, 1.8, 0.8, 2.2);
        float amps[8] = float[8](1.5, 1.0, 0.8, 0.9, 0.6, 0.5, 0.7, 0.5);

        float totalH = 0.0;
        float dx = 0.0, dz = 0.0;

        for (int i = 0; i < 8; i++) {
            vec2 dir = normalize(dirs[i]);
            float freq = freqs[i];
            float phase = phases[i];
            float amp = amps[i] * uWaveHeight;

            float x = pos.x * dir.x + pos.z * dir.y;
            float w = sin(x * freq + uTime * uWaveSpeed + phase);
            totalH += w * amp;

            // 法线偏导数
            float dw = cos(x * freq + uTime * uWaveSpeed + phase) * freq * amp;
            dx += dw * dir.x;
            dz += dw * dir.y;
        }

        pos.y += totalH;
        vHeight = pos.y;

        vec4 worldPos = uModel * vec4(pos, 1.0);
        vWorldPos = worldPos.xyz;

        vec3 normal = normalize(vec3(-dx, 1.0, -dz));
        vNormal = normalize(mat3(uModel) * normal);

        gl_Position = uProj * uView * worldPos;
    }
    )";

// ====================================================================
// Fragment Shader
// ====================================================================
static const char* kWaterFragShader = R"(
#version 300 es
precision highp float;

in vec3 vWorldPos;
in vec3 vNormal;
in float vHeight;

uniform vec3 uColor;
uniform float uAlpha;
uniform vec3 uCameraPos;
uniform vec2 uScreenSize;

layout(location = 0) out vec4 fragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    float NdotV = max(dot(N, V), 0.0);

    // 菲涅尔效应（Schlick 近似）
    float fresnel = pow(1.0 - NdotV, 3.0);
    // 反射颜色（天空/环境色）
    vec3 reflectColor = vec3(0.7, 0.8, 0.9);
    // 水面颜色（随深度变化）
    vec3 shallowColor = vec3(0.05, 0.5, 0.55);
    vec3 deepColor = vec3(0.01, 0.1, 0.15);
    float depthFactor = smoothstep(-0.3, 0.3, vHeight);
    vec3 waterColor = mix(deepColor, shallowColor, depthFactor);

    // 菲涅尔混合：视线垂直水面 → 水色为主；视线掠射 → 反射为主
    vec3 finalColor = mix(waterColor, reflectColor, fresnel * 0.6);

    // 高光
    vec3 lightDir = normalize(vec3(-0.3, 0.5, 0.7));
    vec3 H = normalize(V + lightDir);
    float spec = pow(max(dot(N, H), 0.0), 32.0);
    finalColor += vec3(1.0) * spec * 0.4;

    // 半透明混合：远处（掠射）更不透明，近处（垂直）更透明
    float alpha = mix(uAlpha * 0.5, uAlpha, fresnel * 0.5 + 0.5);
    fragColor = vec4(finalColor, alpha);
}
)";

// ====================================================================
// 构造 / 析构
// ====================================================================
WaterPlane::WaterPlane() = default;
WaterPlane::~WaterPlane() { release(); }

// ====================================================================
// Shader 编译
// ====================================================================
GLuint WaterPlane::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        LOGE("Shader compile error (%s): %s",
             (type == GL_VERTEX_SHADER) ? "vertex" : "fragment", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint WaterPlane::createProgram(const char* vsSrc, const char* fsSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    if (!vs || !fs) return 0;

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        LOGE("Program link error: %s", log);
        glDeleteProgram(prog);
        prog = 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ====================================================================
// 初始化
// ====================================================================
void WaterPlane::init() {
    // 1. 创建 shader
    program_ = createProgram(kWaterVertShader, kWaterFragShader);
    if (!program_) {
        LOGE("Failed to create water shader program");
        return;
    }

    // 2. 获取 uniform 位置
    loc_uMVP_ = glGetUniformLocation(program_, "uMVP");
    loc_uModel_ = glGetUniformLocation(program_, "uModel");
    loc_uView_ = glGetUniformLocation(program_, "uView");
    loc_uProj_ = glGetUniformLocation(program_, "uProj");
    loc_uTime_ = glGetUniformLocation(program_, "uTime");
    loc_uColor_ = glGetUniformLocation(program_, "uColor");
    loc_uAlpha_ = glGetUniformLocation(program_, "uAlpha");
    loc_uWaveSpeed_ = glGetUniformLocation(program_, "uWaveSpeed");
    loc_uWaveHeight_ = glGetUniformLocation(program_, "uWaveHeight");
    loc_uCameraPos_ = glGetUniformLocation(program_, "uCameraPos");
    loc_uScreenSize_ = glGetUniformLocation(program_, "uScreenSize");

    // 3. 生成网格顶点（XZ 平面，Y 向上）
    std::vector<float> vertices;
    std::vector<unsigned short> indices;

    int div = gridDivisions_;
    float halfW = sizeW_ * 0.5f;
    float halfH = sizeH_ * 0.5f;

    for (int row = 0; row <= div; row++) {
        float tz = (float)row / div;  // 0~1
        float z = -halfH + tz * sizeH_;
        for (int col = 0; col <= div; col++) {
            float tx = (float)col / div;  // 0~1
            float x = -halfW + tx * sizeW_;

            // 位置 (x, y=0, z) 和法线 (nx, ny, nz)
            vertices.push_back(x);
            vertices.push_back(0.0f);   // y=0 平面（高度，由 shader 动画）
            vertices.push_back(z);
            vertices.push_back(0.0f);
            vertices.push_back(1.0f);   // 法线朝上
            vertices.push_back(0.0f);
        }
    }

    // 索引（两个三角形形成一个格子）
    for (int row = 0; row < div; row++) {
        for (int col = 0; col < div; col++) {
            int idx0 = row * (div + 1) + col;
            int idx1 = row * (div + 1) + col + 1;
            int idx2 = (row + 1) * (div + 1) + col;
            int idx3 = (row + 1) * (div + 1) + col + 1;

            indices.push_back(idx0);
            indices.push_back(idx1);
            indices.push_back(idx2);

            indices.push_back(idx1);
            indices.push_back(idx3);
            indices.push_back(idx2);
        }
    }

    indexCount_ = (int)indices.size();

    // 4. 创建 VAO / VBO / IBO
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ibo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), indices.data(), GL_STATIC_DRAW);

    // 位置: aPos (3 floats)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

    // 法线: aNormal (3 floats)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

// ====================================================================
// 释放
// ====================================================================
void WaterPlane::release() {
    if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
    if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
    if (ibo_) { glDeleteBuffers(1, &ibo_); ibo_ = 0; }
    if (program_) { glDeleteProgram(program_); program_ = 0; }
}

// ====================================================================
// 更新
// ====================================================================
void WaterPlane::update(float dt) {
    time_ += dt;
}

// ====================================================================
// 渲染
// ====================================================================
void WaterPlane::render(const float* viewMat, const float* projMat,
                         int screenW, int screenH) {
    if (!program_ || !vao_) return;

    glUseProgram(program_);

    // 模型矩阵
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(posX_, posY_, posZ_));

    // uniform 传值
    glUniformMatrix4fv(loc_uModel_, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(loc_uView_, 1, GL_FALSE, viewMat);
    glUniformMatrix4fv(loc_uProj_, 1, GL_FALSE, projMat);

    glUniform1f(loc_uTime_, time_);
    glUniform1f(loc_uWaveSpeed_, waveSpeed_);
    glUniform1f(loc_uWaveHeight_, waveHeight_);

    glUniform3f(loc_uColor_, colorR_, colorG_, colorB_);
    if (loc_uAlpha_ >= 0) glUniform1f(loc_uAlpha_, colorA_);
    glUniform3f(loc_uCameraPos_, 0.0f, 0.0f, 0.0f);  // 相机位置—外部传入更准确

    glUniform2f(loc_uScreenSize_, (float)screenW, (float)screenH);

    // 渲染状态
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);  // 不写深度，让水面透明叠加

    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
}
