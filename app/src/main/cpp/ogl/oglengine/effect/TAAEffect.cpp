#include "../include/TAAEffect.h"
#include "../include/Shader.h"
#include "../include/Common.h"

const char* TAAEffect::VERTEX_SHADER = R"(
#version 300 es
layout(location = 0) in vec2 aPos;
out vec2 vUV;
void main() {
    vUV = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* TAAEffect::FRAGMENT_SHADER = R"(
#version 300 es
precision highp float;
uniform sampler2D currentColor;
uniform sampler2D previousColor;
uniform sampler2D velocityTexture;
uniform sampler2D currentDepth;
uniform float ScreenWidth;
uniform float ScreenHeight;
uniform int frameCount;
in vec2 vUV;
out vec4 FragColor;

vec3 RGB2YCoCgR(vec3 rgbColor)
{
    vec3 YCoCgRColor;

    YCoCgRColor.y = rgbColor.r - rgbColor.b;
    float temp = rgbColor.b + YCoCgRColor.y / 2.0;
    YCoCgRColor.z = rgbColor.g - temp;
    YCoCgRColor.x = temp + YCoCgRColor.z / 2.0;

    return YCoCgRColor;
}

vec3 YCoCgR2RGB(vec3 YCoCgRColor)
{
    vec3 rgbColor;

    float temp = YCoCgRColor.x - YCoCgRColor.z / 2.0;
    rgbColor.g = YCoCgRColor.z + temp;
    rgbColor.b = temp - YCoCgRColor.y / 2.0;
    rgbColor.r = rgbColor.b + YCoCgRColor.y;

    return rgbColor;
}

float Luminance(vec3 color)
{
    return 0.25 * color.r + 0.5 * color.g + 0.25 * color.b;
}

vec3 ToneMap(vec3 color)
{
    return color / (1.0 + Luminance(color));
}

vec3 UnToneMap(vec3 color)
{
    // 先 clamp 防止 YCoCgR 转换产生的轻微负值分量导致 lum > 1
    color = max(color, vec3(0.0));
    float lum = Luminance(color);
    return color / max(1.0 - lum, 0.0001);
}

vec2 getClosestOffset()
{
    vec2 deltaRes = vec2(1.0 / ScreenWidth, 1.0 / ScreenHeight);
    float closestDepth = 1.0;
    vec2 closestUV = vUV;

    for(int i=-1;i<=1;++i)
    {
        for(int j=-1;j<=1;++j)
        {
            vec2 newUV = vUV + deltaRes * vec2(i, j);

            float depth = texture(currentDepth, newUV).x;

            if(depth < closestDepth)
            {
                closestDepth = depth;
                closestUV = newUV;
            }
        }
    }

    return closestUV;
}

// Returns clipped preColor, and outputs how much it was clipped (0=no clip, 1=fully clipped)
vec3 clipAABB(vec3 nowColor, vec3 preColor, out float clipFactor)
{
    vec2 deltaRes = vec2(1.0 / ScreenWidth, 1.0 / ScreenHeight);
    vec3 m1 = vec3(0.0), m2 = vec3(0.0);

    for(int i=-1;i<=1;++i)
    {
        for(int j=-1;j<=1;++j)
        {
            vec2 newUV = vUV + deltaRes * vec2(i, j);
            vec3 C = RGB2YCoCgR(ToneMap(texture(currentColor, newUV).rgb));
            m1 = m1 + C;
            m2 = m2 + C * C;
        }
    }

    const float N = 9.0;
    const float VarianceClipGamma = 1.5;
    vec3 mu = m1 / N;
    vec3 sigma = sqrt(abs(m2 / N - mu * mu));
    vec3 aabbMin = mu - VarianceClipGamma * sigma;
    vec3 aabbMax = mu + VarianceClipGamma * sigma;

    vec3 p_clip = 0.5 * (aabbMax + aabbMin);
    vec3 e_clip = 0.5 * (aabbMax - aabbMin) + 0.0001;

    vec3 v_clip = preColor - p_clip;
    vec3 v_unit = v_clip / e_clip;
    vec3 a_unit = abs(v_unit);
    float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

    vec3 result;
    if (ma_unit > 1.0)
        result = p_clip + v_clip / ma_unit;
    else
        result = preColor;

    // clipFactor: 0=历史完全在AABB内（可信），1=被大幅裁剪（疑似 disocclusion）
    clipFactor = clamp(ma_unit - 1.0, 0.0, 1.0);

    return result;
}

// 检测当前像素是否处于深度不连续边缘（物体边缘/遮挡边界）
// 返回 0.0=内部平坦区域，1.0=深度边缘
float depthEdge()
{
    vec2 d = vec2(1.0 / ScreenWidth, 1.0 / ScreenHeight);
    float depthC = texture(currentDepth, vUV).x;

    // 采样上下左右四邻，求最大深度差
    float d0 = texture(currentDepth, vUV + vec2( d.x, 0.0)).x;
    float d1 = texture(currentDepth, vUV + vec2(-d.x, 0.0)).x;
    float d2 = texture(currentDepth, vUV + vec2(0.0,  d.y)).x;
    float d3 = texture(currentDepth, vUV + vec2(0.0, -d.y)).x;

    float maxDiff = max(max(abs(depthC - d0), abs(depthC - d1)),
                        max(abs(depthC - d2), abs(depthC - d3)));

    // 深度差超过阈值则认为是边缘；0.01 在 NDC 空间对应近处物体的典型边缘
    return clamp(maxDiff / 0.01, 0.0, 1.0);
}

void main()
{
    vec3 nowColor = texture(currentColor, vUV).rgb;
    if(frameCount == 0)
    {
        FragColor = vec4(nowColor, 1.0);
        return;
    }

    vec2 velocity = texture(velocityTexture, getClosestOffset()).rg;
    vec2 offsetUV = vUV - velocity;

    // reprojection UV 超出屏幕 → 直接用当前帧
    if (offsetUV.x < 0.0 || offsetUV.x > 1.0 || offsetUV.y < 0.0 || offsetUV.y > 1.0)
    {
        FragColor = vec4(nowColor, 1.0);
        return;
    }

    vec3 preColor = texture(previousColor, offsetUV).rgb;

    nowColor = RGB2YCoCgR(ToneMap(nowColor));
    preColor = RGB2YCoCgR(ToneMap(preColor));

    float clipFactor;
    preColor = clipAABB(nowColor, preColor, clipFactor);

    preColor = UnToneMap(YCoCgR2RGB(preColor));
    nowColor = UnToneMap(YCoCgR2RGB(nowColor));

    float velocityLen = length(velocity);

    // 基础 blend：静止 c=0.1（多用历史），运动时提高（信任当前帧）
    float c = clamp(0.1 + velocityLen * 40.0, 0.1, 0.5);

    // 深度边缘检测：边缘处大幅提高当前帧权重，防止 disocclusion 黑边累积
    // 仅在真正的几何边缘处生效，内部平坦区不受影响 → 不引起抖动
    float edge = depthEdge();
    c = mix(c, 0.8, edge);

    FragColor = vec4(clamp(c * nowColor + (1.0 - c) * preColor, 0.0, 65504.0), 1.0);
}
)";

TAAEffect::TAAEffect() {
}

TAAEffect::~TAAEffect() {
    release();
}

void TAAEffect::initFBO(int w, int h) {
    historyTex.resize(2);
    historyFBO.resize(2);
    for (int i = 0; i < 2; i++) {
        if (historyFBO[i]) glDeleteFramebuffers(1, &historyFBO[i]);
        if (historyTex[i]) glDeleteTextures(1, &historyTex[i]);

        glGenTextures(1, &historyTex[i]);
        glBindTexture(GL_TEXTURE_2D, historyTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenFramebuffers(1, &historyFBO[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, historyFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, historyTex[i], 0);
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            LOGE("TAA historyFBO[%d] incomplete: 0x%x", i, status);
        }
    }

    if (displayTex) glDeleteTextures(1, &displayTex);
    if (displayFBO) glDeleteFramebuffers(1, &displayFBO);
    glGenTextures(1, &displayTex);
    glBindTexture(GL_TEXTURE_2D, displayTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &displayFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, displayFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, displayTex, 0);
    GLenum status2 = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status2 != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("TAA displayFBO incomplete: 0x%x", status2);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    LOGI("TAA initFBO: %dx%d, hist[0]=%d hist[1]=%d, displayFBO=%d", w, h, historyFBO[0], historyFBO[1], displayFBO);
}

void TAAEffect::init(int w, int h) {
    currentWidth = w;
    currentHeight = h;

    release();

    Shader shader;
    if (!shader.init(VERTEX_SHADER, FRAGMENT_SHADER)) {
        LOGE("TAA shader compile failed");
        return;
    }
    GLint linked = 0;
    GLuint prog = shader.getProgram();
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        if (len > 0) {
            char* log = new char[len];
            glGetProgramInfoLog(prog, len, &len, log);
            LOGE("TAA program link error: %s", log);
            delete[] log;
        }
        return;
    }
    LOGI("TAA shader compiled and linked OK: prog=%d", prog);
    shader.stealProgram(program.prog);
    shader.release();

    program.loc_screenWidth = glGetUniformLocation(program.prog, "ScreenWidth");
    program.loc_screenHeight = glGetUniformLocation(program.prog, "ScreenHeight");
    program.loc_frameCount = glGetUniformLocation(program.prog, "frameCount");
    program.loc_currentColor = glGetUniformLocation(program.prog, "currentColor");
    program.loc_previousColor = glGetUniformLocation(program.prog, "previousColor");
    program.loc_velocityTexture = glGetUniformLocation(program.prog, "velocityTexture");
    program.loc_currentDepth = glGetUniformLocation(program.prog, "currentDepth");
    frameCount = 0;

    float quad[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f
    };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    initFBO(w, h);

    historyIndex = 0;

    LOGI("TAAEffect init: %dx%d", w, h);
}

RenderContext TAAEffect::render(const RenderContext& input) {
    
    if (program.prog == 0) {
        LOGD("TAA render: program.prog==0, returning input");
        return input;
    }
    if (input.colorTex == 0 || input.depthTex == 0 || input.velocityTex == 0) {
        LOGD("TAA render: input texture missing (color=%d, depth=%d, velocity=%d), returning input",
             input.colorTex, input.depthTex, input.velocityTex);
        return input;
    }

    // Save OpenGL state
    GLint prevFBO, prevVAO, prevProgram;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prevBlend = glIsEnabled(GL_BLEND);

    int outIdx = 1 - historyIndex;
    glBindFramebuffer(GL_FRAMEBUFFER, historyFBO[outIdx]);
    glViewport(0, 0, input.width, input.height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glUseProgram(program.prog);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, input.colorTex);
    glUniform1i(program.loc_currentColor, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, historyTex[historyIndex]);
    glUniform1i(program.loc_previousColor, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, input.velocityTex);
    glUniform1i(program.loc_velocityTexture, 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, input.depthTex);
    glUniform1i(program.loc_currentDepth, 3);

    glUniform1f(program.loc_screenWidth, (float)input.width);
    glUniform1f(program.loc_screenHeight, (float)input.height);
    glUniform1i(program.loc_frameCount, frameCount);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    historyIndex = outIdx;
    frameCount++;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, historyFBO[outIdx]);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, displayFBO);
    glBlitFramebuffer(0, 0, input.width, input.height,
                      0, 0, input.width, input.height,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);

    // Restore OpenGL state (except FBO - leave it bound to displayFBO for next effect)
    glBindVertexArray(prevVAO);
    glUseProgram(prevProgram);
    if (prevDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (prevBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);

    RenderContext output;
    output.colorTex = displayTex;
    output.depthTex = input.depthTex;
    output.velocityTex = 0;
    output.fbo = displayFBO;
    output.width = input.width;
    output.height = input.height;
    output.isScreen = false;

    return output;
}

void TAAEffect::release() {
    if (program.prog != 0) {
        glDeleteProgram(program.prog);
        program.prog = 0;
    }
    if (quadVAO != 0) {
        glDeleteVertexArrays(1, &quadVAO);
        quadVAO = 0;
    }
    if (quadVBO != 0) {
        glDeleteBuffers(1, &quadVBO);
        quadVBO = 0;
    }
    for (GLuint tex : historyTex) {
        if (tex != 0) glDeleteTextures(1, &tex);
    }
    historyTex.clear();
    for (GLuint fb : historyFBO) {
        if (fb != 0) glDeleteFramebuffers(1, &fb);
    }
    historyFBO.clear();
    if (displayTex) { glDeleteTextures(1, &displayTex); displayTex = 0; }
    if (displayFBO) { glDeleteFramebuffers(1, &displayFBO); displayFBO = 0; }
    historyIndex = 0;
    currentWidth = 0;
    currentHeight = 0;
}
