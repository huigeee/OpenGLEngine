#pragma once

#include "Object3D.h"
#include <GLES3/gl3.h>

// A flat XZ-plane quad for use with IBLPBRMaterial.
// Vertex layout: vec3 aPos | vec3 aNormal | vec2 aTexCoords  (8 floats / vertex)
class GroundPlane : public Object3D {
public:
    GroundPlane();
    void init() override;
    void release() override;
    void render() override;

    // half-size in world units (default 10 → 20m × 20m ground)
    void setHalfSize(float hs) { halfSize = hs; }
    // UV tiling repeat factor (default 10)
    void setUVTile(float tile) { uvTile = tile; }

private:
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    float halfSize = 10.0f;
    float uvTile   = 10.0f;
    int   indexCount = 0;
};
