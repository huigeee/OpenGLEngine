#include "../include/BloomEffect.h"
#include "../include/Common.h"

const char* QUAD_VS = R"(
#version 300 es
layout(location = 0) in vec2 aPos;
out vec2 vUV;
void main() {
    vUV = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* BRIGHT_FS = R"(
#version 300 es
precision highp float;
uniform sampler2D uColor;
uniform float uThreshold;
in vec2 vUV;
out vec4 FragColor;
void main() {
    vec3 color = texture(uColor, vUV).rgb;
    float brightness = max(max(color.r, color.g), color.b);
    float soft = brightness - uThreshold + 0.5;
    soft = clamp(soft, 0.0, 1.0);
    soft = soft * soft;
    FragColor = vec4(color * soft, 1.0);
}
)";

const char* BLUR_FS = R"(
#version 300 es
precision highp float;
uniform sampler2D uColor;
uniform vec2 uDirection;
in vec2 vUV;
out vec4 FragColor;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(uColor, 0));
    vec3 result = vec3(0.0);

    float w[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

    result += texture(uColor, vUV).rgb * w[0];

    for (int i = 1; i < 5; i++) {
        vec2 offset = uDirection * texelSize * float(i) * 2.0;
        result += texture(uColor, vUV + offset).rgb * w[i];
        result += texture(uColor, vUV - offset).rgb * w[i];
    }

    FragColor = vec4(result, 1.0);
}
)";

const char* COMPOSITE_FS = R"(
#version 300 es
precision highp float;
uniform sampler2D uColor;
uniform sampler2D uBloom;
uniform float uIntensity;
in vec2 vUV;
out vec4 FragColor;
void main() {
    vec3 color = texture(uColor, vUV).rgb;
    vec3 bloom = texture(uBloom, vUV).rgb;
    FragColor = vec4(color + bloom * uIntensity, 1.0);
}
)";

BloomEffect::BloomEffect(float t, float i, int n)
    : threshold(t), intensity(i), iterations(n) {
}

BloomEffect::~BloomEffect() {
    release();
}

const char* BloomEffect::getQuadVS() {
    return QUAD_VS;
}

void BloomEffect::initFBOs(int w, int h) {
    blurW = w / 4;
    blurH = h / 4;
    if (blurW < 2) blurW = 2;
    if (blurH < 2) blurH = 2;

    int numBlur = 2;
    blurFBO.resize(numBlur);
    blurTex.resize(numBlur);
    for (int i = 0; i < numBlur; i++) {
        if (blurFBO[i]) glDeleteFramebuffers(1, &blurFBO[i]);
        if (blurTex[i]) glDeleteTextures(1, &blurTex[i]);

        glGenTextures(1, &blurTex[i]);
        glBindTexture(GL_TEXTURE_2D, blurTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, blurW, blurH, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenFramebuffers(1, &blurFBO[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurTex[i], 0);
        GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (st != GL_FRAMEBUFFER_COMPLETE) LOGE("Bloom blurFBO[%d] incomplete: 0x%x", i, st);
    }

    if (compositeFBO) glDeleteFramebuffers(1, &compositeFBO);
    if (compositeTex) glDeleteTextures(1, &compositeTex);
    glGenTextures(1, &compositeTex);
    glBindTexture(GL_TEXTURE_2D, compositeTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &compositeFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, compositeFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, compositeTex, 0);
    GLenum st2 = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (st2 != GL_FRAMEBUFFER_COMPLETE) LOGE("Bloom compositeFBO incomplete: 0x%x", st2);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    LOGI("Bloom initFBOs: blur %dx%d, composite %dx%d", blurW, blurH, w, h);
}

void BloomEffect::init(int w, int h) {
    currentWidth = w;
    currentHeight = h;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &QUAD_VS, nullptr);
    glCompileShader(vs);

    GLuint brightFs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(brightFs, 1, &BRIGHT_FS, nullptr);
    glCompileShader(brightFs);

    brightProgram = glCreateProgram();
    glAttachShader(brightProgram, vs);
    glAttachShader(brightProgram, brightFs);
    glLinkProgram(brightProgram);
    loc_brightThreshold = glGetUniformLocation(brightProgram, "uThreshold");
    loc_brightColor = glGetUniformLocation(brightProgram, "uColor");

    GLuint blurFs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(blurFs, 1, &BLUR_FS, nullptr);
    glCompileShader(blurFs);

    blurProgram = glCreateProgram();
    glAttachShader(blurProgram, vs);
    glAttachShader(blurProgram, blurFs);
    glLinkProgram(blurProgram);
    loc_blurColor = glGetUniformLocation(blurProgram, "uColor");
    loc_blurDir = glGetUniformLocation(blurProgram, "uDirection");

    GLuint compositeFs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(compositeFs, 1, &COMPOSITE_FS, nullptr);
    glCompileShader(compositeFs);

    compositeProgram = glCreateProgram();
    glAttachShader(compositeProgram, vs);
    glAttachShader(compositeProgram, compositeFs);
    glLinkProgram(compositeProgram);
    loc_compositeColor = glGetUniformLocation(compositeProgram, "uColor");
    loc_compositeBloom = glGetUniformLocation(compositeProgram, "uBloom");
    loc_compositeIntensity = glGetUniformLocation(compositeProgram, "uIntensity");

    glDeleteShader(vs);
    glDeleteShader(brightFs);
    glDeleteShader(blurFs);
    glDeleteShader(compositeFs);

    float quad[] = { -1,-1, 1,-1, -1,1, 1,1 };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    initFBOs(w, h);

    LOGI("BloomEffect init: %dx%d threshold=%.2f intensity=%.2f iterations=%d",
         w, h, threshold, intensity, iterations);
}

void BloomEffect::release() {
    if (brightProgram) { glDeleteProgram(brightProgram); brightProgram = 0; }
    if (blurProgram) { glDeleteProgram(blurProgram); blurProgram = 0; }
    if (compositeProgram) { glDeleteProgram(compositeProgram); compositeProgram = 0; }
    for (auto f : blurFBO) if (f) glDeleteFramebuffers(1, &f);
    for (auto t : blurTex) if (t) glDeleteTextures(1, &t);
    blurFBO.clear(); blurTex.clear();
    if (compositeFBO) { glDeleteFramebuffers(1, &compositeFBO); compositeFBO = 0; }
    if (compositeTex) { glDeleteTextures(1, &compositeTex); compositeTex = 0; }
    if (quadVAO) { glDeleteVertexArrays(1, &quadVAO); quadVAO = 0; }
    if (quadVBO) { glDeleteBuffers(1, &quadVBO); quadVBO = 0; }
}

RenderContext BloomEffect::render(const RenderContext& input) {
    if (brightProgram == 0) return input;
    if (input.colorTex == 0) return input;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    int ping = 0;
    int pong = 1;

    glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[pong]);
    glViewport(0, 0, blurW, blurH);
    glUseProgram(brightProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, input.colorTex);
    glUniform1i(loc_brightColor, 0);
    glUniform1f(loc_brightThreshold, threshold);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    std::swap(ping, pong);

    for (int iter = 0; iter < iterations; iter++) {
        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[pong]);
        glUseProgram(blurProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, blurTex[ping]);
        glUniform1i(loc_blurColor, 0);
        glUniform2f(loc_blurDir, 1.0f, 0.0f);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[ping]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, blurTex[pong]);
        glUniform2f(loc_blurDir, 0.0f, 1.0f);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        std::swap(ping, pong);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, compositeFBO);
    glViewport(0, 0, input.width, input.height);
    glUseProgram(compositeProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, input.colorTex);
    glUniform1i(loc_compositeColor, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blurTex[ping]);
    glUniform1i(loc_compositeBloom, 1);
    glUniform1f(loc_compositeIntensity, intensity);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    RenderContext output = input;
    output.colorTex = compositeTex;
    output.depthTex = 0;
    output.velocityTex = 0;
    output.fbo = compositeFBO;
    return output;
}
