#include "RippleEffect.h"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstdlib>
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "Ripple", __VA_ARGS__)

RippleEffect::RippleEffect() {}
RippleEffect::~RippleEffect() { release(); }

void RippleEffect::init(int gridCols, int gridRows, float spacing) {
    release();

    pointCount_ = gridCols * gridRows;
    points_.resize(pointCount_);

    // Place points in a grid centered at origin
    float halfW = (gridCols - 1) * spacing * 0.5f;
    float halfD = (gridRows - 1) * spacing * 0.5f;
    int idx = 0;
    for (int r = 0; r < gridRows; r++) {
        for (int c = 0; c < gridCols; c++) {
            points_[idx].x = c * spacing - halfW;
            points_[idx].z = r * spacing - halfD;
            points_[idx].y = 0.0f;
            idx++;
        }
    }

    rippleTime_ = 0.0f;
    totalTime_ = 0.0f;
    rippleRadius_ = 0.0f;
    rippleActive_ = false;

    // --- Shader ---
    const char* vs = R"(#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aColor;
uniform mat4 uVP;
uniform float uPointSize;
out vec3 vColor;
void main() {
    gl_Position = uVP * vec4(aPos, 1.0);
    gl_PointSize = uPointSize / gl_Position.w * 200.0;
    vColor = aColor;
}
)";
    const char* fs = R"(#version 300 es
precision mediump float;
in vec3 vColor;
out vec4 FragColor;
void main() {
    float d = length(gl_PointCoord - vec2(0.5));
    if (d > 0.5) discard;
    float a = 1.0 - d * 2.0;
    a = a * a;
    FragColor = vec4(vColor, a);
}
)";

    GLuint vsId = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vsId, 1, &vs, nullptr);
    glCompileShader(vsId);
    GLuint fsId = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fsId, 1, &fs, nullptr);
    glCompileShader(fsId);
    program_ = glCreateProgram();
    glAttachShader(program_, vsId);
    glAttachShader(program_, fsId);
    glLinkProgram(program_);
    glDeleteShader(vsId);
    glDeleteShader(fsId);

    locVP_ = glGetUniformLocation(program_, "uVP");
    locSize_ = glGetUniformLocation(program_, "uPointSize");

    // VAO/VBO
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, pointCount_ * 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    // aPos (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // aColor (3 floats)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // Soft circle texture
    const int SZ = 32;
    unsigned char td[SZ * SZ];
    float ctr = (SZ - 1) * 0.5f;
    for (int y = 0; y < SZ; y++) {
        for (int x = 0; x < SZ; x++) {
            float d = sqrtf((x-ctr)*(x-ctr) + (y-ctr)*(y-ctr)) / ctr;
            float v = 1.0f - fminf(d, 1.0f);
            v = v * v;
            td[y * SZ + x] = (unsigned char)(v * 255);
        }
    }
    glGenTextures(1, &texId_);
    glBindTexture(GL_TEXTURE_2D, texId_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, SZ, SZ, 0, GL_RED, GL_UNSIGNED_BYTE, td);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    LOGI("RippleEffect init: %d points (%dx%d)", pointCount_, gridCols, gridRows);
}

void RippleEffect::update(float dt) {
    totalTime_ += dt;

    // Start a new ripple every rippleInterval seconds
    if (!rippleActive_) {
        rippleActive_ = true;
        rippleTime_ = 0.0f;
        rippleRadius_ = 0.0f;
    }

    // Advance ripple
    rippleTime_ += dt;
    rippleRadius_ = rippleTime_ * rippleSpeed;

    // Max ripple radius = half the grid diagonal, then reset
    float maxR = 5.0f;  // grid covers ~10m
    if (rippleRadius_ > maxR) {
        rippleActive_ = false;
    }

    // Compute height for each point
    for (int i = 0; i < pointCount_; i++) {
        float dx = points_[i].x - center.x;
        float dz = points_[i].z - center.z;
        float dist = sqrtf(dx * dx + dz * dz);

        // Ripple wave: a ring at radius = rippleRadius_
        // Height = sin(phase) * amplitude * falloff
        float ringWidth = 1.5f;  // width of the ripple ring
        float diff = fabsf(dist - rippleRadius_);
        float envelope = 1.0f - fminf(diff / ringWidth, 1.0f);
        envelope = envelope * envelope;  // smooth falloff

        // Amplitude decreases with distance
        float ampFalloff = 1.0f - fminf(dist / maxR, 1.0f);
        ampFalloff = 0.3f + 0.7f * ampFalloff;

        float height = sinf(dist * 4.0f - rippleTime_ * 8.0f) * maxHeight * envelope * ampFalloff;
        points_[i].y = height;
    }

    rebuildVBO();
}

void RippleEffect::rebuildVBO() {
    std::vector<float> data(pointCount_ * 6);
    for (int i = 0; i < pointCount_; i++) {
        float py = points_[i].y;
        // Color: blue when raised, white when at rest
        float r = 0.2f + py / maxHeight * 0.8f;
        float g = 0.3f + py / maxHeight * 0.6f;
        float b = 0.8f + py / maxHeight * 0.2f;

        data[i * 6 + 0] = points_[i].x + center.x;
        data[i * 6 + 1] = groundY + points_[i].y;
        data[i * 6 + 2] = points_[i].z + center.z;
        data[i * 6 + 3] = r;
        data[i * 6 + 4] = g;
        data[i * 6 + 5] = b;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, data.size() * sizeof(float), data.data());
}

void RippleEffect::render(const float* viewMat, const float* projMat) {
    if (pointCount_ == 0 || program_ == 0) return;

    glUseProgram(program_);
    glm::mat4 vp = glm::make_mat4(projMat) * glm::make_mat4(viewMat);
    glUniformMatrix4fv(locVP_, 1, GL_FALSE, glm::value_ptr(vp));
    glUniform1f(locSize_, pointSize);

    // 不混合，每个点独立渲染
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId_);

    glBindVertexArray(vao_);
    glDrawArrays(GL_POINTS, 0, pointCount_);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void RippleEffect::release() {
    if (program_) { glDeleteProgram(program_); program_ = 0; }
    if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
    if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
    if (texId_) { glDeleteTextures(1, &texId_); texId_ = 0; }
    points_.clear();
    pointCount_ = 0;
}
