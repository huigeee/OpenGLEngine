#include "../include/IBLPipeline3.h"
#include "../include/Common.h"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

bool IBLPipeline3::generateEnvCubemap() {
    glGenTextures(1, &envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, 512, 512, 0, GL_RGBA, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glUseProgram(tCubemapProgram);
    glUniformMatrix4fv(glGetUniformLocation(tCubemapProgram, "projection"), 1, GL_FALSE, &captureProjection[0][0]);

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(tCubemapProgram, "texture_background"), 0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    glViewport(0, 0, 512, 512);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    for (unsigned int i = 0; i < 6; ++i) {
        glUniformMatrix4fv(glGetUniformLocation(tCubemapProgram, "view"), 1, GL_FALSE, &captureViews[i][0][0]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(VAO_Cubemap);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("EnvCubemap FBO incomplete: 0x%x", fboStatus);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    LOGI("generateEnvCubemap done, fbo=0x%x", fboStatus);
    return true;
}

void IBLPipeline3::generateIrradianceMap() {
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("IrradianceMap FBO incomplete: 0x%x", fboStatus);
    } else {
        LOGI("IrradianceMap FBO COMPLETE");
    }

    glUseProgram(tIrradianceProgram);
    glUniformMatrix4fv(glGetUniformLocation(tIrradianceProgram, "projection"), 1, GL_FALSE, &captureProjection[0][0]);
    glUniform1i(glGetUniformLocation(tIrradianceProgram, "texture_background"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glViewport(0, 0, 32, 32);

    for (unsigned int i = 0; i < 6; ++i) {
        glUniformMatrix4fv(glGetUniformLocation(tIrradianceProgram, "view"), 1, GL_FALSE, &captureViews[i][0][0]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(VAO_Cubemap);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    LOGI("generateIrradianceMap done");
}

void IBLPipeline3::generatePrefilterMap() {
    int mapWidth = 128;
    int mapHeight = 128;

    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, mapWidth, mapHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glUseProgram(prefilterProgram);
    glUniformMatrix4fv(glGetUniformLocation(prefilterProgram, "projection"), 1, GL_FALSE, &captureProjection[0][0]);
    glUniform1i(glGetUniformLocation(prefilterProgram, "environmentMap"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("PrefilterMap FBO incomplete: 0x%x", fboStatus);
    }
    unsigned int maxMipLevels = iMaxMipLevels;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
        unsigned int mipWidth = static_cast<unsigned int>(mapWidth * std::pow(0.5, mip));
        unsigned int mipHeight = static_cast<unsigned int>(mapHeight * std::pow(0.5, mip));
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        glUniform1f(glGetUniformLocation(prefilterProgram, "roughness"), roughness);
        for (unsigned int i = 0; i < 6; ++i) {
            glUniformMatrix4fv(glGetUniformLocation(prefilterProgram, "view"), 1, GL_FALSE, &captureViews[i][0][0]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glBindVertexArray(VAO_Cubemap);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    LOGI("generatePrefilterMap done");
}

void IBLPipeline3::generateBRDFLUTTexture() {
    glGenTextures(1, &brdfLUTTexture);
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);
    GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("BRDFLUT FBO incomplete: 0x%x", fboStatus);
    }

    glViewport(0, 0, 512, 512);
    glUseProgram(brdfProgram);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(VAO_Quad);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    LOGI("generateBRDFLUTTexture done");
}

void IBLPipeline3::renderBackground(float* projection, float* view) {
    if (!isCreated || envCubemap == 0 || tBackgroundProgram == 0) {
        return;
    }

    GLboolean cullEnabled;
    glGetBooleanv(GL_CULL_FACE, &cullEnabled);
    if (cullEnabled) glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glUseProgram(tBackgroundProgram);

    glm::mat4 viewMat = glm::make_mat4(view);
    glm::mat4 projMat = glm::make_mat4(projection);
    glm::mat4 modelMat(1.0f);

    glUniformMatrix4fv(glGetUniformLocation(tBackgroundProgram, "projection"), 1, GL_FALSE, &projMat[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(tBackgroundProgram, "view"), 1, GL_FALSE, &viewMat[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(tBackgroundProgram, "model"), 1, GL_FALSE, &modelMat[0][0]);
    glUniform1i(glGetUniformLocation(tBackgroundProgram, "texture_background"), 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glBindVertexArray(VAO_Cubemap);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    if (cullEnabled) glEnable(GL_CULL_FACE);
}

void IBLPipeline3::release() {
    if (tBackgroundProgram) glDeleteProgram(tBackgroundProgram);
    if (tCubemapProgram) glDeleteProgram(tCubemapProgram);
    if (tIrradianceProgram) glDeleteProgram(tIrradianceProgram);
    if (prefilterProgram) glDeleteProgram(prefilterProgram);
    if (brdfProgram) glDeleteProgram(brdfProgram);
    if (VAO_Cubemap) glDeleteVertexArrays(1, &VAO_Cubemap);
    if (VBO_Cubemap) glDeleteBuffers(1, &VBO_Cubemap);
    if (VAO_Quad) glDeleteVertexArrays(1, &VAO_Quad);
    if (VBO_Quad) glDeleteBuffers(1, &VBO_Quad);
    if (captureFBO) glDeleteFramebuffers(1, &captureFBO);
    if (captureRBO) glDeleteRenderbuffers(1, &captureRBO);
    if (hdrTexture) glDeleteTextures(1, &hdrTexture);
    if (envCubemap) glDeleteTextures(1, &envCubemap);
    if (irradianceMap) glDeleteTextures(1, &irradianceMap);
    if (prefilterMap) glDeleteTextures(1, &prefilterMap);
    if (brdfLUTTexture) glDeleteTextures(1, &brdfLUTTexture);

    tBackgroundProgram = tCubemapProgram = tIrradianceProgram = prefilterProgram = brdfProgram = 0;
    VAO_Cubemap = VBO_Cubemap = VAO_Quad = VBO_Quad = captureFBO = captureRBO = 0;
    hdrTexture = envCubemap = irradianceMap = prefilterMap = brdfLUTTexture = 0;
    isCreated = false;
    LOGI("IBLPipeline3 released");
}
