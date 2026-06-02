#pragma once

#include <GLES3/gl3.h>
#include <string>
#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "ResourceLoader.h"

class ResourceLoader;

class IBLPipeline3 {
public:
    IBLPipeline3();
    ~IBLPipeline3();

    bool init(ResourceLoader* loader, const char* hdrPath);
    void release();
    void renderBackground(float* projection, float* view);

    GLuint getEnvCubemap() const { return envCubemap; }
    GLuint getIrradianceMap() const { return irradianceMap; }
    GLuint getPrefilterMap() const { return prefilterMap; }
    GLuint getBRDFLUTTexture() const { return brdfLUTTexture; }
    int getPrefilterLevels() const { return iMaxMipLevels; }
    bool isReady() const { return isCreated; }

private:
    bool generateEnvCubemap();
    void generateIrradianceMap();
    void generatePrefilterMap();
    void generateBRDFLUTTexture();

    GLuint createProgram(const char* vs, const char* fs);
    GLuint loadHDR(const char* path, int& w, int& h, int& ch);

    static const char* SHADER_BACKGROUND_VS;
    static const char* SHADER_BACKGROUND_FS;
    static const char* SHADER_CUBEMAP_VS;
    static const char* SHADER_CUBEMAP_FS;
    static const char* SHADER_IRRADIANCE_FS;
    static const char* SHADER_PREFILTER_VS;
    static const char* SHADER_PREFILTER_FS;
    static const char* SHADER_BRDF_VS;
    static const char* SHADER_BRDF_FS;

    GLuint tBackgroundProgram = 0;
    GLuint tCubemapProgram = 0;
    GLuint tIrradianceProgram = 0;
    GLuint prefilterProgram = 0;
    GLuint brdfProgram = 0;

    GLuint VAO_Cubemap = 0;
    GLuint VBO_Cubemap = 0;
    GLuint VAO_Quad = 0;
    GLuint VBO_Quad = 0;
    GLuint captureFBO = 0;
    GLuint captureRBO = 0;

    GLuint hdrTexture = 0;
    GLuint envCubemap = 0;
    GLuint irradianceMap = 0;
    GLuint prefilterMap = 0;
    GLuint brdfLUTTexture = 0;

    unsigned int iMaxMipLevels = 5;
    bool isCreated = false;

    ResourceLoader* loader = nullptr;

    static const float kCubeVertices[288];
    static const float kQuadVertices[20];
    glm::mat4 captureProjection;
    glm::mat4 captureViews[6];
};
