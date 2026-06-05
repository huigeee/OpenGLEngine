#include "../include/FogEffect.h"
#include "../include/Common.h"

const char* FogEffect::VERTEX_SHADER = R"(
#version 300 es
layout(location = 0) in vec2 aPos;
out vec2 vUV;
void main() {
    vUV = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* FogEffect::FRAGMENT_SHADER = R"(
#version 300 es
precision highp float;
uniform sampler2D uColor;
uniform sampler2D uDepth;
uniform int       uMode;
uniform float     uDensity;
uniform float     uFogStart;
uniform float     uFogEnd;
uniform vec3      uFogColor;
uniform float     uTime;
uniform mat4      uInvViewProj;
in vec2 vUV;
out vec4 FragColor;

// --- 世界坐标重建 ---
vec3 worldPosFromDepth(float rawDepth) {
    vec4 ndc = vec4(vUV * 2.0 - 1.0, rawDepth * 2.0 - 1.0, 1.0);
    vec4 world = uInvViewProj * ndc;
    return world.xyz / world.w;
}

// --- 哈希 / 噪声基础 ---
float hash(vec2 p) {
    p = fract(p * vec2(127.1, 311.7));
    p += dot(p, p + 19.19);
    return fract(p.x * p.y);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);   // smoothstep
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

// --- FBM：4层叠加 ---
float fbm(vec2 p) {
    float v = 0.0;
    float amp = 0.5;
    float freq = 1.0;
    for (int i = 0; i < 4; i++) {
        v += amp * noise(p * freq);
        freq *= 2.1;
        amp  *= 0.45;
    }
    return v;
}

float linearDepth(float rawDepth) {
    float near = 0.1;
    float far  = 10.0;
    float z = rawDepth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main() {
    vec3  color  = texture(uColor, vUV).rgb;
    float raw    = texture(uDepth, vUV).r;

    // 天空像素 raw≈1.0，linearDepth 会给出 far plane 距离（10.0），同样参与雾计算
    float depth = linearDepth(raw);

    // 流动 FBM：用屏幕 UV + 时间
    vec2  flowUV   = vUV * 3.0 + vec2(uTime * 0.15, uTime * 0.08);
    float noiseVal = fbm(flowUV);
    noiseVal = noiseVal * 1.6 + 0.2;    // 0.2~1.8，流动对比更明显

    float noiseDensity = uDensity * noiseVal;

    float fogFactor;
    if (uMode == 0) {
        fogFactor = clamp((uFogEnd - depth) / (uFogEnd - uFogStart), 0.0, 1.0);
    } else if (uMode == 1) {
        fogFactor = exp(-noiseDensity * depth);
    } else {
        float fd = noiseDensity * depth;
        fogFactor = exp(-fd * fd);
    }

    vec3 result = mix(uFogColor, color, fogFactor);
    FragColor = vec4(result, 1.0);
}
)";

// ---------------------------------------------------------------

FogEffect::FogEffect(int mode, float density, float fogStart, float fogEnd,
                     float fogR, float fogG, float fogB)
    : mode(mode), density(density), fogStart(fogStart), fogEnd(fogEnd),
      fogR(fogR), fogG(fogG), fogB(fogB) {
}

FogEffect::~FogEffect() {
    release();
}

void FogEffect::initFBO(int w, int h) {
    if (outputTex) glDeleteTextures(1, &outputTex);
    if (outputFBO) glDeleteFramebuffers(1, &outputFBO);

    glGenTextures(1, &outputTex);
    glBindTexture(GL_TEXTURE_2D, outputTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &outputFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTex, 0);
    GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (st != GL_FRAMEBUFFER_COMPLETE) LOGE("FogEffect FBO incomplete: 0x%x", st);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    LOGI("FogEffect initFBO: %dx%d", w, h);
}

void FogEffect::init(int w, int h) {
    currentWidth  = w;
    currentHeight = h;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &VERTEX_SHADER, nullptr);
    glCompileShader(vs);
    { GLint ok=0; glGetShaderiv(vs,GL_COMPILE_STATUS,&ok);
      if(!ok){ GLint l=0; glGetShaderiv(vs,GL_INFO_LOG_LENGTH,&l); char* b=new char[l]; glGetShaderInfoLog(vs,l,&l,b); LOGE("Fog VS: %s",b); delete[]b; } }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &FRAGMENT_SHADER, nullptr);
    glCompileShader(fs);
    { GLint ok=0; glGetShaderiv(fs,GL_COMPILE_STATUS,&ok);
      if(!ok){ GLint l=0; glGetShaderiv(fs,GL_INFO_LOG_LENGTH,&l); char* b=new char[l]; glGetShaderInfoLog(fs,l,&l,b); LOGE("Fog FS: %s",b); delete[]b; } }

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    { GLint ok=0; glGetProgramiv(program,GL_LINK_STATUS,&ok);
      if(!ok){ GLint l=0; glGetProgramiv(program,GL_INFO_LOG_LENGTH,&l); char* b=new char[l]; glGetProgramInfoLog(program,l,&l,b); LOGE("Fog Link: %s",b); delete[]b; }
      else LOGI("FogEffect linked OK prog=%d", program); }
    glDeleteShader(vs);
    glDeleteShader(fs);

    loc_color      = glGetUniformLocation(program, "uColor");
    loc_depth      = glGetUniformLocation(program, "uDepth");
    loc_mode       = glGetUniformLocation(program, "uMode");
    loc_density    = glGetUniformLocation(program, "uDensity");
    loc_fogStart   = glGetUniformLocation(program, "uFogStart");
    loc_fogEnd     = glGetUniformLocation(program, "uFogEnd");
    loc_fogColor   = glGetUniformLocation(program, "uFogColor");
    loc_time       = glGetUniformLocation(program, "uTime");
    loc_invViewProj = glGetUniformLocation(program, "uInvViewProj");

    float quad[] = { -1,-1, 1,-1, -1,1, 1,1 };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    initFBO(w, h);
    LOGI("FogEffect init: mode=%d density=%.2f", mode, density);
}

RenderContext FogEffect::render(const RenderContext& input) {
    if (program == 0 || input.colorTex == 0 || input.depthTex == 0) return input;

    // 累计时间
    time += 0.016f;  // 约 60fps，每帧 ~16ms

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
    glViewport(0, 0, input.width, input.height);
    glUseProgram(program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, input.colorTex);
    glUniform1i(loc_color, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, input.depthTex);
    glUniform1i(loc_depth, 1);

    glUniform1i(loc_mode,      mode);
    glUniform1f(loc_density,   density);
    glUniform1f(loc_fogStart,  fogStart);
    glUniform1f(loc_fogEnd,    fogEnd);
    glUniform3f(loc_fogColor,  fogR, fogG, fogB);
    glUniform1f(loc_time,      time);
    glUniformMatrix4fv(loc_invViewProj, 1, GL_FALSE, invViewProj);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    RenderContext output  = input;
    output.colorTex = outputTex;
    output.depthTex = input.depthTex;
    output.fbo      = outputFBO;
    return output;
}

void FogEffect::release() {
    if (program)   { glDeleteProgram(program);            program   = 0; }
    if (outputTex) { glDeleteTextures(1, &outputTex);     outputTex = 0; }
    if (outputFBO) { glDeleteFramebuffers(1, &outputFBO); outputFBO = 0; }
    if (quadVAO)   { glDeleteVertexArrays(1, &quadVAO);   quadVAO   = 0; }
    if (quadVBO)   { glDeleteBuffers(1, &quadVBO);        quadVBO   = 0; }
}
