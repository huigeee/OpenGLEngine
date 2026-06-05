#include "ParticleSystem.h"
#include <android/log.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>
#include <cmath>
#include "../include/Common.h"

// -----------------------------------------------------------------------
// 默认 Shader（点精灵，支持自定义贴图）
// -----------------------------------------------------------------------
const char* ParticleSystem::DEFAULT_VS = R"(
#version 300 es
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in float aSize;
uniform mat4 uVP;
uniform float uSizeScale;
out vec4 vColor;
void main() {
    vColor = aColor;
    gl_Position = uVP * vec4(aPos, 1.0);
    gl_PointSize = aSize / gl_Position.w * uSizeScale;
}
)";

const char* ParticleSystem::DEFAULT_FS = R"(
#version 300 es
precision mediump float;
in vec4 vColor;
uniform sampler2D uTex;
out vec4 FragColor;
void main() {
    float a = texture(uTex, gl_PointCoord).r;
    FragColor = vec4(vColor.rgb, vColor.a * a);
}
)";

// -----------------------------------------------------------------------
// 工具
// -----------------------------------------------------------------------
float ParticleSystem::randF(float lo, float hi) {
    return lo + (hi - lo) * ((float)rand() / (float)RAND_MAX);
}

GLuint ParticleSystem::compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        char* buf = new char[len]; glGetShaderInfoLog(s, len, &len, buf);
        LOGE("Shader compile error: %s", buf); delete[] buf;
    }
    return s;
}

void ParticleSystem::buildDefaultShaders() {
    const char* vs = vertSrc ? vertSrc : DEFAULT_VS;
    const char* fs = fragSrc ? fragSrc : DEFAULT_FS;
    GLuint v = compileShader(GL_VERTEX_SHADER, vs);
    GLuint f = compileShader(GL_FRAGMENT_SHADER, fs);
    program = glCreateProgram();
    glAttachShader(program, v); glAttachShader(program, f);
    glLinkProgram(program);
    glDeleteShader(v); glDeleteShader(f);
    locVP  = glGetUniformLocation(program, "uVP");
    locTex = glGetUniformLocation(program, "uTex");
    GLint locScale = glGetUniformLocation(program, "uSizeScale");
    if (locScale >= 0) {
        glUseProgram(program);
        glUniform1f(locScale, sizeScale);
        glUseProgram(0);
    }
    LOGI("ParticleSystem shader linked: prog=%d", program);
}

// -----------------------------------------------------------------------
// init
// -----------------------------------------------------------------------
void ParticleSystem::init(int max) {
    maxCount = max;
    particles.resize(max);

    buildDefaultShaders();

    // VBO: pos(3) + color(4) + size(1) = 8 floats per particle
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, max * 8 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(7*sizeof(float)));
    glBindVertexArray(0);

    buildTexture();
}

// -----------------------------------------------------------------------
// update
// -----------------------------------------------------------------------
void ParticleSystem::update(float dt, const glm::vec3& emitPos) {
    for (auto& p : particles) {
        if (!p.active) continue;
        p.life -= dt;
        if (p.life <= 0.0f) { p.active = false; continue; }
        onUpdate(p, dt);
    }

    emitAccum += emitRate * dt;
    while (emitAccum >= 1.0f) {
        int idx = firstDead();
        if (idx >= 0) {
            Particle& p = particles[idx];
            p.active = true;
            onEmit(p, emitPos);
        }
        emitAccum -= 1.0f;
    }
}

// -----------------------------------------------------------------------
// render
// -----------------------------------------------------------------------
void ParticleSystem::render(const float* viewMat, const float* projMat) {
    if (program == 0) return;

    std::vector<float> buf;
    buf.reserve(maxCount * 8);
    int count = 0;
    for (auto& p : particles) {
        if (!p.active) continue;
        buf.push_back(p.position.x); buf.push_back(p.position.y); buf.push_back(p.position.z);
        buf.push_back(p.color.r);    buf.push_back(p.color.g);    buf.push_back(p.color.b); buf.push_back(p.color.a);
        buf.push_back(p.size);
        count++;
    }
    if (count == 0) return;

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * 8 * sizeof(float), buf.data());

    glm::mat4 view = glm::make_mat4(viewMat);
    glm::mat4 proj = glm::make_mat4(projMat);
    glm::mat4 vp   = proj * view;

    setupBlend();
    glDepthMask(GL_FALSE);

    glUseProgram(program);
    glUniformMatrix4fv(locVP, 1, GL_FALSE, glm::value_ptr(vp));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);
    glUniform1i(locTex, 0);

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, count);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    teardownBlend();
}

// -----------------------------------------------------------------------
// release
// -----------------------------------------------------------------------
void ParticleSystem::release() {
    if (vao)     { glDeleteVertexArrays(1, &vao);  vao = 0; }
    if (vbo)     { glDeleteBuffers(1, &vbo);        vbo = 0; }
    if (program) { glDeleteProgram(program);         program = 0; }
    if (texId)   { glDeleteTextures(1, &texId);     texId = 0; }
    particles.clear();
    emitAccum = 0.0f;
}

// -----------------------------------------------------------------------
// 默认虚函数实现
// -----------------------------------------------------------------------
void ParticleSystem::onEmit(Particle& p, const glm::vec3& emitPos) {
    p.position = emitPos + glm::vec3(randF(-0.05f, 0.05f), 0.0f, randF(-0.05f, 0.05f));
    p.velocity = glm::vec3(
        randF(minVelocity.x, maxVelocity.x),
        randF(minVelocity.y, maxVelocity.y),
        randF(minVelocity.z, maxVelocity.z));
    p.maxLife = randF(minLife, maxLife);
    p.life    = p.maxLife;
    p.size    = randF(minSize, maxSize);
    p.color   = colorStart;
    p.custom0 = 0.0f;
    p.custom1 = 0.0f;
}

void ParticleSystem::onUpdate(Particle& p, float dt) {
    p.position += p.velocity * dt;
    float t = 1.0f - (p.life / p.maxLife);
    p.color = glm::mix(colorStart, colorEnd, t);
}

void ParticleSystem::buildTexture() {
    // 程序生成软圆（平方衰减），存为 GL_R8
    const int SZ = 64;
    unsigned char texData[SZ * SZ];
    float center = (SZ - 1) * 0.5f;
    for (int y = 0; y < SZ; y++) {
        for (int x = 0; x < SZ; x++) {
            float dx = (x - center) / center;
            float dy = (y - center) / center;
            float d  = sqrtf(dx*dx + dy*dy);
            float v  = 1.0f - glm::clamp(d, 0.0f, 1.0f);
            v = v * v;
            texData[y * SZ + x] = (unsigned char)(v * 255);
        }
    }
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, SZ, SZ, 0, GL_RED, GL_UNSIGNED_BYTE, texData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ParticleSystem::setupBlend() {
    glEnable(GL_BLEND);
    glBlendFunc(blendSrc, blendDst);
}

void ParticleSystem::teardownBlend() {
    glDisable(GL_BLEND);
}

// -----------------------------------------------------------------------
int ParticleSystem::firstDead() {
    for (int i = 0; i < maxCount; i++) {
        if (!particles[i].active) return i;
    }
    return -1;
}
