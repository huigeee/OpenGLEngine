#pragma once

#include "Material.h"

class DebugIBLMaterial : public Material {
public:
    DebugIBLMaterial();
    ~DebugIBLMaterial() override;
    void init() override;
    void setTransform(const float* model, const float* view, const float* projection) override;
    void release() override;
    void setIBLCoeffs(float sh[9][3]);
    void setTextureMaps(GLuint albedo, GLuint metallic, GLuint roughness, GLuint ao);
    const VertexAttrib* getVertexAttribs(int& count) override;
protected:
    float shCoeffs[9][3];
    GLint modelLocation;
    GLint viewLocation;
    GLint projectionLocation;
    GLint prevModelLocation;
    GLint prevViewLocation;
    GLint prevProjectionLocation;
    GLint jitterOffsetLocation;
    GLint cameraPosLocation;
    GLint shLocation0;
    GLint baseColorLocation;
    GLint metallicLocation;
    GLint roughnessLocation;
    GLint albedoMapLocation;
    GLint metallicMapLocation;
    GLint roughnessMapLocation;
    GLint aoMapLocation;
    GLint albedoMapEnabledLocation;
    GLint metallicMapEnabledLocation;
    GLint roughnessMapEnabledLocation;
    GLint aoMapEnabledLocation;
    GLuint albedoTex;
    GLuint metallicTex;
    GLuint roughnessTex;
    GLuint aoTex;
};
