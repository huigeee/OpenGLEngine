#include "../include/TonemapEffect.h"
#include "../include/Common.h"

const char* TonemapEffect::VERTEX_SHADER = R"(
#version 300 es
layout(location = 0) in vec2 aPos;
out vec2 vUV;
void main() {
    vUV = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* TonemapEffect::FRAGMENT_SHADER = R"(
#version 300 es
precision highp float;
uniform sampler2D uColor;
uniform float uExposure;
uniform int   uOp;       // 0=Reinhard 1=ACES 2=Uncharted2
in  vec2 vUV;
out vec4 FragColor;

// --- Reinhard ---
vec3 reinhard(vec3 c) {
    return c / (c + vec3(1.0));
}

// --- ACES Filmic (Hill approximation) ---
vec3 aces(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

// --- Uncharted2 / Hejl ---
vec3 uncharted2Partial(vec3 x) {
    float A=0.15, B=0.50, C=0.10, D=0.20, E=0.02, F=0.30;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}
vec3 uncharted2(vec3 c) {
    const float W = 11.2;
    vec3 curr = uncharted2Partial(c * 2.0);
    vec3 whiteScale = vec3(1.0) / uncharted2Partial(vec3(W));
    return curr * whiteScale;
}

void main() {
    vec3 hdr = texture(uColor, vUV).rgb * uExposure;

    vec3 ldr;
    if      (uOp == 0) ldr = reinhard(hdr);
    else if (uOp == 2) ldr = clamp(uncharted2(hdr), 0.0, 1.0);
    else               ldr = aces(hdr);   // default: ACES

    // Gamma correction (linear -> sRGB 2.2)
    ldr = pow(max(ldr, vec3(0.0)), vec3(1.0 / 2.2));

    FragColor = vec4(ldr, 1.0);
}
)";

TonemapEffect::TonemapEffect(int op, float exposure)
    : tonemapOp(op), exposure(exposure) {}

TonemapEffect::~TonemapEffect() {
    release();
}

void TonemapEffect::initFBO(int w, int h) {
    if (outputTex) glDeleteTextures(1, &outputTex);
    if (outputFBO) glDeleteFramebuffers(1, &outputFBO);

    glGenTextures(1, &outputTex);
    glBindTexture(GL_TEXTURE_2D, outputTex);
    // LDR output — GL_RGBA8 is sufficient after tone mapping
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &outputFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTex, 0);
    GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (st != GL_FRAMEBUFFER_COMPLETE) LOGE("Tonemap outputFBO incomplete: 0x%x", st);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    LOGI("Tonemap initFBO: %dx%d", w, h);
}

void TonemapEffect::init(int w, int h) {
    currentWidth  = w;
    currentHeight = h;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &VERTEX_SHADER, nullptr);
    glCompileShader(vs);
    { GLint ok=0; glGetShaderiv(vs,GL_COMPILE_STATUS,&ok);
      if(!ok){ GLint l=0; glGetShaderiv(vs,GL_INFO_LOG_LENGTH,&l); char* b=new char[l]; glGetShaderInfoLog(vs,l,&l,b); LOGE("Tonemap VS: %s",b); delete[]b; } }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &FRAGMENT_SHADER, nullptr);
    glCompileShader(fs);
    { GLint ok=0; glGetShaderiv(fs,GL_COMPILE_STATUS,&ok);
      if(!ok){ GLint l=0; glGetShaderiv(fs,GL_INFO_LOG_LENGTH,&l); char* b=new char[l]; glGetShaderInfoLog(fs,l,&l,b); LOGE("Tonemap FS: %s",b); delete[]b; } }

    program.prog = glCreateProgram();
    glAttachShader(program.prog, vs);
    glAttachShader(program.prog, fs);
    glLinkProgram(program.prog);
    { GLint ok=0; glGetProgramiv(program.prog,GL_LINK_STATUS,&ok);
      if(!ok){ GLint l=0; glGetProgramiv(program.prog,GL_INFO_LOG_LENGTH,&l); char* b=new char[l]; glGetProgramInfoLog(program.prog,l,&l,b); LOGE("Tonemap Link: %s",b); delete[]b; }
      else LOGI("Tonemap program linked OK, prog=%d", program.prog); }
    glDeleteShader(vs);
    glDeleteShader(fs);

    program.loc_color    = glGetUniformLocation(program.prog, "uColor");
    program.loc_exposure = glGetUniformLocation(program.prog, "uExposure");
    program.loc_op       = glGetUniformLocation(program.prog, "uOp");

    float quad[] = { -1,-1, 1,-1, -1,1, 1,1 };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);

    initFBO(w, h);
    LOGI("TonemapEffect init: %dx%d op=%d exposure=%.2f", w, h, tonemapOp, exposure);
}

RenderContext TonemapEffect::render(const RenderContext& input) {
    if (program.prog == 0 || input.colorTex == 0) return input;
    
    // Save OpenGL state
    GLint prevFBO, prevVAO, prevProgram;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prevBlend = glIsEnabled(GL_BLEND);
    
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
    glViewport(0, 0, input.width, input.height);

    glUseProgram(program.prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, input.colorTex);
    glUniform1i(program.loc_color,    0);
    glUniform1f(program.loc_exposure, exposure);
    glUniform1i(program.loc_op,       tonemapOp);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    // Restore OpenGL state (except FBO - leave it bound to outputFBO for next effect)
    glBindVertexArray(prevVAO);
    glUseProgram(prevProgram);
    if (prevDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (prevBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);

    RenderContext output = input;
    output.colorTex = outputTex;
    output.fbo      = outputFBO;
    return output;
}

void TonemapEffect::release() {
    if (program.prog) { glDeleteProgram(program.prog); program.prog = 0; }
    if (outputTex)    { glDeleteTextures(1, &outputTex); outputTex = 0; }
    if (outputFBO)    { glDeleteFramebuffers(1, &outputFBO); outputFBO = 0; }
    if (quadVAO)      { glDeleteVertexArrays(1, &quadVAO); quadVAO = 0; }
    if (quadVBO)      { glDeleteBuffers(1, &quadVBO); quadVBO = 0; }
}
