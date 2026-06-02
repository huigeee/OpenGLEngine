#pragma once

#include <GLES3/gl3.h>
#include "ResourceLoader.h"

class IBLPipeline {
public:
    IBLPipeline(ResourceLoader* loader);
    ~IBLPipeline();

    bool init(const char* hdrPath, int prefilterLevels, int maxTexSize);
    void release();

    GLuint getEnvTexture() const { return envTexture; }
    GLuint getBRDFLUTTexture() const { return brdfLUTTexture; }
    const GLuint* getPrefilteredTextures() const { return prefilteredTextures; }
    int getPrefilteredLevels() const { return prefilteredLevels; }
    const float* getSHCoeffs() const { return &shCoeffs[0][0]; }
    bool isReady() const { return ready; }

private:
    struct V2 { float x, y; };
    struct V3 { float x, y, z; };

    void computeSH(float* pixels, int w, int h, float sh[9][3]);
    void prefilterEnv(float* pixels, int w, int h);
    uint32_t radicalInverse_VdC(uint32_t bits) const;
    V2 hammersley(uint32_t i, uint32_t N) const;
    V3 importanceSampleGGX(V2 Xi, V3 N, float roughness) const;

    ResourceLoader* resourceLoader;
    GLuint envTexture;
    GLuint prefilteredTextures[8];
    int prefilteredLevels;
    GLuint brdfLUTTexture;
    float shCoeffs[9][3];
    bool ready;
};
