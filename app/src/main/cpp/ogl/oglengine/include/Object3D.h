#pragma once

#include <GLES3/gl3.h>

class Material;

class Object3D {
public:
    Object3D();
    virtual ~Object3D();

    virtual void init() = 0;
    virtual void beforeRender();
    virtual void onPreRender() {}  // called before beforeRender — hook for physics update
    virtual void render() = 0;
    virtual void afterRender();
    virtual void release() = 0;

    void setVisible(bool visible);
    bool isVisible() const;
    void setPosition(float x, float y, float z);
    void setScale(float scale);
    // Euler angles in degrees: applied as Ry * Rx * Rz
    void setRotation(float xDeg, float yDeg, float zDeg);
    void setRotationY(float yDeg);

    // Arbitrary-axis self rotation, applied AFTER the Euler rotation
    // (i.e. inside the pivot frame, on top of Ry*Rx*Rz). Axis does not
    // need to be normalised; passing a zero axis or angle disables it.
    void setRotationAxis(float ax, float ay, float az, float angleDeg);

    // Local pivot point (in the object's local/model space, before
    // scale-up). All rotations (Euler + axis) are performed around this
    // point. Default (0,0,0) reproduces the previous behaviour.
    void setPivot(float px, float py, float pz);

    float* getPosition();
    float getScale();
    float* getRotation(); // returns rotation[3]
    void setMaterial(Material* mat);
    Material* getMaterial() { return material; }
    virtual void getModelMatrix(float* outMatrix);

protected:
    float position[3] = {0.0f, 0.0f, 0.0f};
    float rotation[3] = {0.0f, 0.0f, 0.0f}; // degrees: x, y, z
    float pivot[3]    = {0.0f, 0.0f, 0.0f}; // local-space pivot point
    float axis[3]     = {0.0f, 1.0f, 0.0f}; // arbitrary rotation axis
    float axisAngle   = 0.0f;               // degrees; 0 = disabled
    float scale = 1.0f;
    bool visible;
    Material* material;
};