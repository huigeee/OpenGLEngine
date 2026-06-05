#include "../include/VolumetricLightEffect.h"
#include "../include/Common.h"

// ---------------------------------------------------------------------------
// Shaders
// ---------------------------------------------------------------------------
const char* VolumetricLightEffect::VERTEX_SHADER = R"(
#version 300 es
layout(location = 0) in vec2 aPos;
out vec2 vUV;
void main() {
    vUV = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

// Ray-marched volumetric light. Adds scattered light to scene color.
// - uShadowMap optional (uUseShadow=false → uniform fog toward sun)
// - frame-jittered start offset + Bayer dither to break banding (TAA cleans it up)
const char* VolumetricLightEffect::FRAGMENT_SHADER = R"(
#version 300 es
precision highp float;

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uColor;
uniform sampler2D uDepth;
uniform sampler2D uShadowMap;

uniform mat4  uInvViewProj;
uniform mat4  uLightSpaceMatrix;
uniform vec3  uCamPos;
uniform vec3  uLightDir;       // direction light travels (sun -> scene)
uniform vec3  uLightColor;
uniform int   uUseShadow;      // 0/1
uniform float uDensity;
uniform float uScattering;     // HG g
uniform int   uSteps;
uniform float uMaxDistance;
uniform float uIntensity;
uniform int   uFrame;

const float PI = 3.14159265;

// Bayer 4x4 (0..15 / 16) — dither offset per pixel
float bayer4(ivec2 p) {
    int b[16] = int[16](
         0,  8,  2, 10,
        12,  4, 14,  6,
         3, 11,  1,  9,
        15,  7, 13,  5);
    int idx = (p.y & 3) * 4 + (p.x & 3);
    return float(b[idx]) / 16.0;
}

vec3 worldFromDepth(vec2 uv, float depth) {
    vec4 ndc = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 w   = uInvViewProj * ndc;
    return w.xyz / w.w;
}

float shadowAt(vec3 wp) {
    vec4 lp = uLightSpaceMatrix * vec4(wp, 1.0);
    if (abs(lp.w) < 0.0001) return 1.0;
    lp.xyz /= lp.w;
    vec3 c = lp.xyz * 0.5 + 0.5;
    if (c.x < 0.0 || c.x > 1.0 || c.y < 0.0 || c.y > 1.0 || c.z > 1.0 || c.z < 0.0)
        return 1.0;
    float closest = texture(uShadowMap, c.xy).r;
    // small bias to avoid self-shadowing of the medium
    return (c.z - 0.0005 > closest) ? 0.0 : 1.0;
}

// Henyey-Greenstein phase function — forward/backward scattering.
float HG(float cosT, float g) {
    float g2 = g * g;
    float denom = 1.0 + g2 - 2.0 * g * cosT;
    return (1.0 - g2) / (4.0 * PI * pow(max(denom, 1e-4), 1.5));
}

void main() {
    vec3 sceneColor = texture(uColor, vUV).rgb;

    float depth = texture(uDepth, vUV).r;
    vec3 worldEnd = worldFromDepth(vUV, depth);

    vec3  rayVec = worldEnd - uCamPos;
    float rayLen = length(rayVec);
    vec3  rayDir = (rayLen > 1e-4) ? rayVec / rayLen : vec3(0.0, 0.0, -1.0);

    // Clamp march distance so background pixels don't march to infinity.
    rayLen = min(rayLen, uMaxDistance);

    int   steps   = max(uSteps, 4);
    float stepLen = rayLen / float(steps);

    // Per-pixel temporal-stable jitter (Bayer + frame index)
    float jitter = bayer4(ivec2(gl_FragCoord.xy));
    jitter = fract(jitter + float(uFrame & 31) * 0.61803);

    vec3 step = rayDir * stepLen;
    vec3 pos  = uCamPos + step * jitter;

    // Phase: forward scattering peaks when looking toward the light.
    // -uLightDir points from sample toward the sun.
    float cosT  = dot(rayDir, -normalize(uLightDir));
    float phase = HG(cosT, clamp(uScattering, 0.0, 0.95));

    vec3 accum = vec3(0.0);
    for (int i = 0; i < 256; ++i) {
        if (i >= steps) break;
        float s = (uUseShadow == 1) ? shadowAt(pos) : 1.0;
        accum += s * phase * stepLen;
        pos += step;
    }

    vec3 volumetric = accum * uDensity * uLightColor * uIntensity;

    // Additive composite into HDR scene color.
    FragColor = vec4(sceneColor + volumetric, 1.0);
}
)";

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------
VolumetricLightEffect::VolumetricLightEffect() {}
VolumetricLightEffect::~VolumetricLightEffect() { release(); }

void VolumetricLightEffect::initFBO(int w, int h) {
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
    if (st != GL_FRAMEBUFFER_COMPLETE) LOGE("VolumetricLightEffect FBO incomplete: 0x%x", st);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void VolumetricLightEffect::init(int w, int h) {
    currentWidth  = w;
    currentHeight = h;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &VERTEX_SHADER, nullptr);
    glCompileShader(vs);
    { GLint ok = 0; glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
      if (!ok) { GLint l = 0; glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &l);
                 char* b = new char[l]; glGetShaderInfoLog(vs, l, &l, b);
                 LOGE("Volumetric VS: %s", b); delete[] b; } }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &FRAGMENT_SHADER, nullptr);
    glCompileShader(fs);
    { GLint ok = 0; glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
      if (!ok) { GLint l = 0; glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &l);
                 char* b = new char[l]; glGetShaderInfoLog(fs, l, &l, b);
                 LOGE("Volumetric FS: %s", b); delete[] b; } }

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    { GLint ok = 0; glGetProgramiv(program, GL_LINK_STATUS, &ok);
      if (!ok) { GLint l = 0; glGetProgramiv(program, GL_INFO_LOG_LENGTH, &l);
                 char* b = new char[l]; glGetProgramInfoLog(program, l, &l, b);
                 LOGE("Volumetric Link: %s", b); delete[] b; }
      else LOGI("VolumetricLightEffect linked OK prog=%d", program); }
    glDeleteShader(vs);
    glDeleteShader(fs);

    loc_color      = glGetUniformLocation(program, "uColor");
    loc_depth      = glGetUniformLocation(program, "uDepth");
    loc_shadowMap  = glGetUniformLocation(program, "uShadowMap");
    loc_invVP      = glGetUniformLocation(program, "uInvViewProj");
    loc_lightSpace = glGetUniformLocation(program, "uLightSpaceMatrix");
    loc_camPos     = glGetUniformLocation(program, "uCamPos");
    loc_lightDir   = glGetUniformLocation(program, "uLightDir");
    loc_lightColor = glGetUniformLocation(program, "uLightColor");
    loc_useShadow  = glGetUniformLocation(program, "uUseShadow");
    loc_density    = glGetUniformLocation(program, "uDensity");
    loc_scattering = glGetUniformLocation(program, "uScattering");
    loc_steps      = glGetUniformLocation(program, "uSteps");
    loc_maxDist    = glGetUniformLocation(program, "uMaxDistance");
    loc_intensity  = glGetUniformLocation(program, "uIntensity");
    loc_frame      = glGetUniformLocation(program, "uFrame");

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
    LOGI("VolumetricLightEffect init: %dx%d steps=%d density=%.3f g=%.2f",
         w, h, steps, density, scattering);
}

RenderContext VolumetricLightEffect::render(const RenderContext& input) {
    if (program == 0 || input.colorTex == 0 || input.depthTex == 0) return input;

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

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, useShadow ? shadowMap : 0);
    glUniform1i(loc_shadowMap, 2);

    glUniformMatrix4fv(loc_invVP, 1, GL_FALSE, invViewProj);
    glUniformMatrix4fv(loc_lightSpace, 1, GL_FALSE, lightSpaceMatrix);
    glUniform3fv(loc_camPos, 1, camPos);
    glUniform3fv(loc_lightDir, 1, lightDir);
    glUniform3fv(loc_lightColor, 1, lightColor);
    glUniform1i(loc_useShadow, (useShadow && shadowMap != 0) ? 1 : 0);
    glUniform1f(loc_density, density);
    glUniform1f(loc_scattering, scattering);
    glUniform1i(loc_steps, steps);
    glUniform1f(loc_maxDist, maxDistance);
    glUniform1f(loc_intensity, intensity);
    glUniform1i(loc_frame, frameCounter++);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    RenderContext output = input;
    output.colorTex = outputTex;
    output.fbo      = outputFBO;
    return output;
}

void VolumetricLightEffect::release() {
    if (program)   { glDeleteProgram(program);            program   = 0; }
    if (outputTex) { glDeleteTextures(1, &outputTex);     outputTex = 0; }
    if (outputFBO) { glDeleteFramebuffers(1, &outputFBO); outputFBO = 0; }
    if (quadVAO)   { glDeleteVertexArrays(1, &quadVAO);   quadVAO   = 0; }
    if (quadVBO)   { glDeleteBuffers(1, &quadVBO);        quadVBO   = 0; }
}
