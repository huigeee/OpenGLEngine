#include "../include/DoFEffect.h"
#include "../include/Common.h"

const char* DoFEffect::VERTEX_SHADER = R"(
#version 300 es
layout(location = 0) in vec2 aPos;
out vec2 vUV;
void main() {
    vUV = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* DoFEffect::FRAGMENT_SHADER = R"(
#version 300 es
precision highp float;
uniform sampler2D uColor;
uniform sampler2D uDepth;
uniform float uFocalDist;
uniform float uAperture;
uniform float uMaxBlur;
uniform vec2 uTexelSize;
in vec2 vUV;
out vec4 FragColor;

float linearDepth(float d) {
    float near = 0.1;
    float far = 10.0;
    float z = d * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));
}

float getCoC(float rawDepth) {
    float depth = linearDepth(rawDepth);
    float linearNorm = (depth - 0.1) / (10.0 - 0.1);
    linearNorm = clamp(linearNorm, 0.0, 1.0);
    float coc = abs(linearNorm - uFocalDist) * uAperture;
    return min(coc * uMaxBlur, uMaxBlur);
}

vec2 diskSample(int i, float radius) {
    float goldenAngle = 2.39996323;
    float r = sqrt(float(i) + 0.5) / sqrt(20.0);
    float theta = float(i) * goldenAngle;
    return vec2(cos(theta), sin(theta)) * r * radius;
}

void main() {
    float rawDepth = texture(uDepth, vUV).r;
    float coc = getCoC(rawDepth);

    vec3 col = texture(uColor, vUV).rgb;
    float weight = 1.0;

    if (coc > 0.5) {
        for (int i = 0; i < 20; i++) {
            vec2 offset = diskSample(i, coc) * uTexelSize;
            vec3 s = texture(uColor, vUV + offset).rgb;
            float sd = linearDepth(texture(uDepth, vUV + offset).r);
            float sCoC = abs((sd - 0.1) / 9.9 - uFocalDist) * uAperture * uMaxBlur;

            float sw = (sCoC >= coc) ? 1.0 : (sCoC / max(coc, 0.001));
            col += s * sw;
            weight += sw;
        }
        col /= weight;
    }

    FragColor = vec4(col, 1.0);
}
)";

DoFEffect::DoFEffect(float focalDist, float aperture, float maxBlur)
    : focalDistance(focalDist), aperture(aperture), maxBlur(maxBlur) {
}

DoFEffect::~DoFEffect() {
    release();
}

void DoFEffect::initFBO(int w, int h) {
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
    if (st != GL_FRAMEBUFFER_COMPLETE) LOGE("DoF outputFBO incomplete: 0x%x", st);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    LOGI("DoF initFBO: %dx%d, outTex=%d", w, h, outputTex);
}

void DoFEffect::init(int w, int h) {
    currentWidth = w;
    currentHeight = h;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &VERTEX_SHADER, nullptr);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &FRAGMENT_SHADER, nullptr);
    glCompileShader(fs);

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    loc_color = glGetUniformLocation(program, "uColor");
    loc_depth = glGetUniformLocation(program, "uDepth");
    loc_focalDist = glGetUniformLocation(program, "uFocalDist");
    loc_aperture = glGetUniformLocation(program, "uAperture");
    loc_maxBlur = glGetUniformLocation(program, "uMaxBlur");
    loc_texelSize = glGetUniformLocation(program, "uTexelSize");

    float quad[] = { -1,-1, 1,-1, -1,1, 1,1 };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    initFBO(w, h);

    LOGI("DoFEffect init: %dx%d focal=%.2f aperture=%.2f maxBlur=%.1f",
         w, h, focalDistance, aperture, maxBlur);
}

RenderContext DoFEffect::render(const RenderContext& input) {
    if (program == 0) return input;
    if (input.colorTex == 0 || input.depthTex == 0) return input;

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

    glUniform1f(loc_focalDist, focalDistance);
    glUniform1f(loc_aperture, aperture);
    glUniform1f(loc_maxBlur, maxBlur);
    glUniform2f(loc_texelSize, 1.0f / input.width, 1.0f / input.height);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    RenderContext output = input;
    output.colorTex = outputTex;
    output.depthTex = input.depthTex;
    output.velocityTex = input.velocityTex;
    output.fbo = outputFBO;
    return output;
}

void DoFEffect::release() {
    if (program) { glDeleteProgram(program); program = 0; }
    if (outputTex) { glDeleteTextures(1, &outputTex); outputTex = 0; }
    if (outputFBO) { glDeleteFramebuffers(1, &outputFBO); outputFBO = 0; }
    if (quadVAO) { glDeleteVertexArrays(1, &quadVAO); quadVAO = 0; }
    if (quadVBO) { glDeleteBuffers(1, &quadVBO); quadVBO = 0; }
}
