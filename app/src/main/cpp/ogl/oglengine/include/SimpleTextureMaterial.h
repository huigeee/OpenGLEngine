#pragma once

#include "Material.h"

class SimpleTextureMaterial : public Material {
public:
    SimpleTextureMaterial();
    ~SimpleTextureMaterial() override;
    void init() override;
    void setTransform(const float* model, const float* view, const float* projection) override;
    void release() override;
    void setTexture(GLuint tex);
    GLuint getTexture() { return texId; }
    const VertexAttrib* getVertexAttribs(int& count) override;
protected:
    GLuint texId;
    GLint modelLocation;
    GLint viewLocation;
    GLint projectionLocation;
    GLint prevModelLocation;
    GLint prevViewLocation;
    GLint prevProjectionLocation;
    GLint jitterOffsetLocation;
    GLint texLocation;
    GLint positionLocation;
    GLint texCoordLocation;
};
