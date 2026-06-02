#pragma once

#include "Material.h"

class ResourceLoader;

class PBRMaterial : public Material {
public:
    PBRMaterial(bool clearCoat = false);
    ~PBRMaterial() override;

    void init() override;
    void setTransform(const float* model, const float* view, const float* projection) override;
    void release() override;

    void setBaseColor(float r, float g, float b);
    void setMetallic(float metallic);
    void setRoughness(float roughness);
    void setLightPos(int index, float x, float y, float z);
    void setLightColor(int index, float r, float g, float b);
    void setClearCoat(float intensity, float roughness);
    void enableClearCoat(bool enable);
    void setIBL(float sh[9][3], GLuint envTex, GLuint prefiltered[], int levels, GLuint brdfLUT);
    void setIBLCubemap(float sh[9][3], GLuint irradianceMap, GLuint prefilterCubemap, int levels, GLuint brdfLUT);
    void setIBLFromScene(GLuint irradiance, GLuint prefilter, GLuint brdfLUT);
    void setTextureMaps(GLuint albedo, GLuint normal, GLuint metallic, GLuint roughness, GLuint ao);
    static GLuint uploadTextureFromFile(ResourceLoader* loader, const char* path, bool flipUv = true);

    float* getBaseColor() { return baseColor; }
    float getMetallic() { return metallic; }
    float getRoughness() { return roughness; }

    const VertexAttrib* getVertexAttribs(int& count) override;

private:
    bool clearCoatEnabled;
    float baseColor[3];
    float metallic;
    float roughness;
    float lightPos[3][3];
    float lightColor[3][3];
    float clearCoatIntensity;
    float clearCoatRoughness;

    float shCoeffs[9][3];
    GLuint envTexture;
    GLuint envCubemapTexture;
    GLuint irradianceMapTexture;
    GLuint prefilterCubemapTexture;
    GLuint prefilteredTextures[8];
    int prefilteredLevels;
    GLuint brdfLUTTexture;
    bool iblEnabled;
    bool useCubemapIBL;

    GLint modelLocation;
    GLint viewLocation;
    GLint projectionLocation;
    GLint prevModelLocation;
    GLint prevViewLocation;
    GLint prevProjectionLocation;
    GLint jitterOffsetLocation;
    GLint baseColorLocation;
    GLint metallicLocation;
    GLint roughnessLocation;
    GLint lightPosLocation[3];
    GLint lightColorLocation[3];
    GLint cameraPosLocation;
    GLint clearCoatLocation;
    GLint clearCoatRoughnessLocation;
    GLint positionLocation;
    GLint normalLocation;
    GLint shLocation[9];
    GLint envTextureLocation;
    GLint envCubemapLocation;
    GLint irradianceMapLocation;
    GLint prefilterCubemapLocation;
    GLint prefilteredTexLocation[8];
    GLint prefilteredLevelsLocation;
    GLint brdfLUTLocation;
    GLint useCubemapLocation;
    GLint enableDirectLightLocation;
    GLint albedoMapLocation;
    GLint normalMapLocation;
    GLint metallicMapLocation;
    GLint roughnessMapLocation;
    GLint aoMapLocation;
    GLint normalMapEnabledLocation;
    GLint albedoMapEnabledLocation;
    GLint metallicMapEnabledLocation;
    GLint roughnessMapEnabledLocation;
    GLint aoMapEnabledLocation;
    GLint debugModeLocation;
    GLuint albedoTex;
    GLuint normalTex;
    GLuint metallicTex;
    GLuint roughnessTex;
    GLuint aoTex;
};
