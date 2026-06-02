#pragma once

#include <GLES3/gl3.h>
#include "../include/PostEffectBase.h"
#include "../include/RenderContext.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Forward declare
class Scene;

/**
 * Shadow Mapping Effect
 * 实现实时阴影映射
 * 
 * 工作流程：
 * 1. 从光源视角渲染场景深度图（Shadow Pass）
 * 2. 在主渲染 pass 中采样阴影贴图计算阴影
 */
class ShadowEffect : public PostEffectBase {
public:
    ShadowEffect();
    ~ShadowEffect() override;
    
    // PostEffectBase interface
    void init(int w, int h) override;
    RenderContext render(const RenderContext& input) override;
    void release() override;
    void onSurfaceChanged(int w, int h) override;
    PostEffectType getType() const override { return PostEffectType::Shadow; }
    
    // Shadow specific
    void setLightPosition(float x, float y, float z);
    void setShadowMapSize(int size) { shadowMapSize = size; }
    GLuint getShadowMapTexture() const { return shadowMapTexture; }
    float* getLightSpaceMatrix() { return lightSpaceMatrix; }
    bool isShadowEnabled() const { return enabled; }
    GLint getShadowProgramLocModel() const { return loc_shadowModel; }
    
    // Shadow quality settings
    void setShadowBias(float bias) { shadowBias = bias; }
    void setShadowSoftness(float softness) { shadowSoftness = softness; }
    
    // Scene integration
    void setSceneRef(void* scene) { sceneRef = scene; }
    void renderShadowPass();
    
private:
    void buildShadowMap(int w, int h);
    void updateLightSpaceMatrix();
    void createShadowShader();
    
    // Shadow map FBO
    GLuint shadowFBO = 0;
    GLuint shadowMapTexture = 0;
    int shadowMapSize = 2048;
    
    // Light setup (directional light)
    float lightPos[3] = {5.0f, 10.0f, 5.0f};
    float lightSpaceMatrix[16];  // lightProjection * lightView
    float lightView[16];
    float lightProjection[16];
    
    // Shadow quality
    float shadowBias = 0.005f;  // Increased to prevent shadow acne
    float shadowSoftness = 0.5f;
    
    // Shadow cascade (simple single cascade for now)
    // Adjust these values based on your scene size
    float shadowDistance = 30.0f;   // Half width/height of shadow ortho box
    float shadowNearPlane = 0.1f;   // Near plane of shadow frustum
    float shadowFarPlane = 80.0f;   // Far plane of shadow frustum
    
    // Shadow shader
    GLuint shadowProgram = 0;
    GLint loc_shadowModel = -1;
    GLint loc_shadowLightSpace = -1;
    
    // For rendering scene objects in shadow pass
    void* sceneRef = nullptr;  // pointer to Scene for rendering objects
};
