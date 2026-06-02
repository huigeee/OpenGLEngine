#include "../include/FXAAEffect.h"
#include "../include/Common.h"

const char* FXAAEffect::VERTEX_SHADER = R"(
#version 300 es
layout(location = 0) in vec2 aPos;
out vec2 vUV;
void main() {
    vUV = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

// FXAA with edge walking
const char* FXAAEffect::FRAGMENT_SHADER = R"(
#version 300 es
precision highp float;
uniform sampler2D uColor;
uniform vec2 uTexelSize;
uniform float uContrastThreshold;
uniform float uRelativeThreshold;
in vec2 vUV;
out vec4 FragColor;

float luma(vec3 c) {
    // 先做简单 reinhard tonemap 再算亮度，避免 HDR 数值过大导致对比度检测失效
    c = c / (c + vec3(1.0));
    return dot(c, vec3(0.299, 0.587, 0.114));
}

#define EDGE_STEPS 10
const float STEP_SIZES[EDGE_STEPS] = float[](1.0,1.0,1.0,1.0,1.5,2.0,2.0,2.0,2.0,4.0);

void main() {
    vec2 ts = uTexelSize;

    float lumaM  = luma(texture(uColor, vUV).rgb);
    float lumaN  = luma(texture(uColor, vUV + vec2( 0.0,  ts.y)).rgb);
    float lumaS  = luma(texture(uColor, vUV + vec2( 0.0, -ts.y)).rgb);
    float lumaE  = luma(texture(uColor, vUV + vec2( ts.x,  0.0)).rgb);
    float lumaW  = luma(texture(uColor, vUV + vec2(-ts.x,  0.0)).rgb);

    float rangeMax = max(lumaM, max(max(lumaN, lumaS), max(lumaE, lumaW)));
    float rangeMin = min(lumaM, min(min(lumaN, lumaS), min(lumaE, lumaW)));
    float range = rangeMax - rangeMin;

    vec3 colorM = texture(uColor, vUV).rgb;

    if (range < max(uContrastThreshold, rangeMax * uRelativeThreshold)) {
        FragColor = vec4(colorM, 1.0);
        return;
    }

    float lumaNW = luma(texture(uColor, vUV + vec2(-ts.x,  ts.y)).rgb);
    float lumaNE = luma(texture(uColor, vUV + vec2( ts.x,  ts.y)).rgb);
    float lumaSW = luma(texture(uColor, vUV + vec2(-ts.x, -ts.y)).rgb);
    float lumaSE = luma(texture(uColor, vUV + vec2( ts.x, -ts.y)).rgb);

    // 子像素混合因子
    float lumaAvg4  = (lumaN + lumaS + lumaE + lumaW) * 0.25;
    float lumaAvgD  = (lumaNW + lumaNE + lumaSW + lumaSE) * 0.25;
    float lumaAvgAll = lumaAvg4 * 0.5 + lumaAvgD * 0.5;
    float subPixelBlend = clamp(abs(lumaAvgAll - lumaM) / range, 0.0, 1.0);
    subPixelBlend = smoothstep(0.0, 1.0, subPixelBlend);
    subPixelBlend = subPixelBlend * subPixelBlend * 0.75;

    // 边缘方向
    float edgeH = abs(lumaNW + 2.0*lumaN + lumaNE - lumaSW - 2.0*lumaS - lumaSE);
    float edgeV = abs(lumaNW + 2.0*lumaW + lumaSW - lumaNE - 2.0*lumaE - lumaSE);
    bool isHorizontal = edgeH >= edgeV;

    float luma1 = isHorizontal ? lumaS : lumaW;
    float luma2 = isHorizontal ? lumaN : lumaE;
    float grad1 = abs(luma1 - lumaM);
    float grad2 = abs(luma2 - lumaM);
    bool isSteep1 = grad1 >= grad2;

    vec2 edgeNormal  = isHorizontal ? vec2(0.0, ts.y) : vec2(ts.x, 0.0);
    if (!isSteep1) edgeNormal = -edgeNormal;
    float lumaEdge   = isSteep1 ? luma1 : luma2;
    float gradScale  = 0.25 * max(grad1, grad2);
    float edgeLuma   = (lumaM + lumaEdge) * 0.5;
    vec2  edgeTangent = isHorizontal ? vec2(ts.x, 0.0) : vec2(0.0, ts.y);
    vec2  edgeUV = vUV + edgeNormal * 0.5;

    vec2 uvP = edgeUV + edgeTangent;
    vec2 uvN = edgeUV - edgeTangent;
    float deltaP = luma(texture(uColor, uvP).rgb) - edgeLuma;
    float deltaN = luma(texture(uColor, uvN).rgb) - edgeLuma;
    bool doneP = abs(deltaP) >= gradScale;
    bool doneN = abs(deltaN) >= gradScale;

    for (int i = 0; i < EDGE_STEPS && !(doneP && doneN); i++) {
        if (!doneP) {
            uvP += edgeTangent * STEP_SIZES[i];
            deltaP = luma(texture(uColor, uvP).rgb) - edgeLuma;
            doneP = abs(deltaP) >= gradScale;
        }
        if (!doneN) {
            uvN -= edgeTangent * STEP_SIZES[i];
            deltaN = luma(texture(uColor, uvN).rgb) - edgeLuma;
            doneN = abs(deltaN) >= gradScale;
        }
    }

    float distP = isHorizontal ? abs(uvP.x - vUV.x) : abs(uvP.y - vUV.y);
    float distN = isHorizontal ? abs(uvN.x - vUV.x) : abs(uvN.y - vUV.y);
    bool  closerP   = distP <= distN;
    float edgeSpan  = distP + distN;
    float edgeDist  = min(distP, distN);

    float deltaFinal   = closerP ? deltaP : deltaN;
    bool  correctDir   = (lumaM - edgeLuma) * deltaFinal < 0.0;
    float edgeBlend    = correctDir ? (0.5 - edgeDist / edgeSpan) : 0.0;

    float finalBlend = max(subPixelBlend, edgeBlend);
    vec2  finalUV    = vUV + edgeNormal * finalBlend;

    FragColor = vec4(texture(uColor, finalUV).rgb, 1.0);
}
)";

FXAAEffect::FXAAEffect(float contrastThreshold, float relativeThreshold)
    : contrastThreshold(contrastThreshold), relativeThreshold(relativeThreshold) {
}

FXAAEffect::~FXAAEffect() {
    release();
}

void FXAAEffect::initFBO(int w, int h) {
    if (outputTex) glDeleteTextures(1, &outputTex);
    if (outputFBO) glDeleteFramebuffers(1, &outputFBO);

    glGenTextures(1, &outputTex);
    glBindTexture(GL_TEXTURE_2D, outputTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &outputFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTex, 0);
    GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (st != GL_FRAMEBUFFER_COMPLETE) LOGE("FXAA outputFBO incomplete: 0x%x", st);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    LOGI("FXAA initFBO: %dx%d", w, h);
}

void FXAAEffect::init(int w, int h) {
    currentWidth = w;
    currentHeight = h;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &VERTEX_SHADER, nullptr);
    glCompileShader(vs);
    { GLint ok=0; glGetShaderiv(vs,GL_COMPILE_STATUS,&ok);
      if(!ok){ GLint l=0; glGetShaderiv(vs,GL_INFO_LOG_LENGTH,&l); char* b=new char[l]; glGetShaderInfoLog(vs,l,&l,b); LOGE("FXAA VS: %s",b); delete[]b; } }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &FRAGMENT_SHADER, nullptr);
    glCompileShader(fs);
    { GLint ok=0; glGetShaderiv(fs,GL_COMPILE_STATUS,&ok);
      if(!ok){ GLint l=0; glGetShaderiv(fs,GL_INFO_LOG_LENGTH,&l); char* b=new char[l]; glGetShaderInfoLog(fs,l,&l,b); LOGE("FXAA FS: %s",b); delete[]b; } }

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    { GLint ok=0; glGetProgramiv(program,GL_LINK_STATUS,&ok);
      if(!ok){ GLint l=0; glGetProgramiv(program,GL_INFO_LOG_LENGTH,&l); char* b=new char[l]; glGetProgramInfoLog(program,l,&l,b); LOGE("FXAA Link: %s",b); delete[]b; }
      else LOGI("FXAA program linked OK, prog=%d", program); }
    glDeleteShader(vs);
    glDeleteShader(fs);

    loc_color              = glGetUniformLocation(program, "uColor");
    loc_texelSize          = glGetUniformLocation(program, "uTexelSize");
    loc_contrastThreshold  = glGetUniformLocation(program, "uContrastThreshold");
    loc_relativeThreshold  = glGetUniformLocation(program, "uRelativeThreshold");

    float quad[] = { -1,-1, 1,-1, -1,1, 1,1 };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    initFBO(w, h);

    LOGI("FXAAEffect init: %dx%d", w, h);
}

RenderContext FXAAEffect::render(const RenderContext& input) {
    if (program == 0 || input.colorTex == 0) return input;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
    glViewport(0, 0, input.width, input.height);

    glUseProgram(program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, input.colorTex);
    glUniform1i(loc_color, 0);
    glUniform2f(loc_texelSize, 1.0f / input.width, 1.0f / input.height);
    glUniform1f(loc_contrastThreshold, contrastThreshold);
    glUniform1f(loc_relativeThreshold, relativeThreshold);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    RenderContext output = input;
    output.colorTex = outputTex;
    output.fbo = outputFBO;
    return output;
}

void FXAAEffect::release() {
    if (program)   { glDeleteProgram(program); program = 0; }
    if (outputTex) { glDeleteTextures(1, &outputTex); outputTex = 0; }
    if (outputFBO) { glDeleteFramebuffers(1, &outputFBO); outputFBO = 0; }
    if (quadVAO)   { glDeleteVertexArrays(1, &quadVAO); quadVAO = 0; }
    if (quadVBO)   { glDeleteBuffers(1, &quadVBO); quadVBO = 0; }
}
