#pragma once

#include <GLES3/gl3.h>
#include <string>
#include <vector>
#include "glm/glm.hpp"
#include "ResourceLoader.h"

class ResourceLoader;

class IBLPipeline2 {
public:
    IBLPipeline2();
    ~IBLPipeline2();

    bool init(ResourceLoader* loader, const char* hdrPath);
    void release();
    void renderBackground(float* projection, float* view);
    void rebuildBackgroundShader();
    void setAspectRatio(float aspect) { this->aspectRatio = aspect; }

    GLuint getEnvCubemap() const { return envCubemap; }
    GLuint getIrradianceMap() const { return irradianceMap; }
    GLuint getPrefilterMap() const { return prefilterMap; }
    GLuint getBRDFLUTTexture() const { return brdfLUTTexture; }
    int getPrefilterLevels() const { return prefilterLevels; }
    bool isReady() const { return isCreated; }

private:
    bool generateEnvCubemap();
    void generateIrradianceMap();
    void generatePrefilterMap();
    void generateBRDFLUTTexture();

    GLuint createProgram(const char* vs, const char* fs);
    GLuint loadHDR(const char* path, int& w, int& h, int& ch);

    GLuint equiToCubemapProgram = 0;
    GLuint irradianceProgram = 0;
    GLuint prefilterProgram = 0;
    GLuint brdfProgram = 0;
    GLuint backgroundProgram = 0;

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

    int prefilterLevels = 5;
    bool isCreated = false;
    float aspectRatio = 1.78f;

    ResourceLoader* loader;

    static const float kCubeVertices[288];
    static const float kQuadVertices[20];
    static glm::mat4 captureProjection;
    static glm::mat4 captureViews[6];

    std::string hdrPath;
};

extern const float kCubeVertices[288];
extern const float kQuadVertices[20];
