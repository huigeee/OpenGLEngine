#include "../include/DebugIBLMaterial.h"
#include "../include/Shader.h"
#include "../include/ShaderManager.h"
#include "../include/Camera.h"
#include "../include/Common.h"

DebugIBLMaterial::DebugIBLMaterial() : cameraPosLocation(0), shLocation0(0), baseColorLocation(0),
    metallicLocation(0), roughnessLocation(0), albedoMapLocation(0), metallicMapLocation(0),
    roughnessMapLocation(0), aoMapLocation(0), albedoTex(0), metallicTex(0), roughnessTex(0), aoTex(0) {
    for (int i = 0; i < 9; i++) {
        shCoeffs[i][0] = shCoeffs[i][1] = shCoeffs[i][2] = 0.0f;
    }
}

DebugIBLMaterial::~DebugIBLMaterial() {
}

void DebugIBLMaterial::init() {
    shader = new Shader();
    shader->init(ShaderManager::getDebugIBLVertex(), ShaderManager::getDebugIBLFragment());
    modelLocation = shader->getUniformLocation("uModelMatrix");
    viewLocation = shader->getUniformLocation("uViewMatrix");
    projectionLocation = shader->getUniformLocation("uProjectionMatrix");
    prevModelLocation = shader->getUniformLocation("uPrevModelMatrix");
    prevViewLocation = shader->getUniformLocation("uPrevViewMatrix");
    prevProjectionLocation = shader->getUniformLocation("uPrevProjectionMatrix");
    jitterOffsetLocation = shader->getUniformLocation("uJitterOffset");
    cameraPosLocation = shader->getUniformLocation("uCameraPos");
    shLocation0 = shader->getUniformLocation("uSH[0]");
    baseColorLocation = shader->getUniformLocation("uBaseColor");
    metallicLocation = shader->getUniformLocation("uMetallic");
    roughnessLocation = shader->getUniformLocation("uRoughness");
    albedoMapLocation = shader->getUniformLocation("uAlbedoMap");
    metallicMapLocation = shader->getUniformLocation("uMetallicMap");
    roughnessMapLocation = shader->getUniformLocation("uRoughnessMap");
    aoMapLocation = shader->getUniformLocation("uAOMap");
    albedoMapEnabledLocation = shader->getUniformLocation("uAlbedoMapEnabled");
    metallicMapEnabledLocation = shader->getUniformLocation("uMetallicMapEnabled");
    roughnessMapEnabledLocation = shader->getUniformLocation("uRoughnessMapEnabled");
    aoMapEnabledLocation = shader->getUniformLocation("uAOMapEnabled");
    initialized = true;
    LOGI("DebugIBLMaterial initialized, program=%d", shader->getProgram());
}

void DebugIBLMaterial::setTransform(const float* model, const float* view, const float* projection) {
    if (!initialized || shader == nullptr) return;
    shader->use();
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model);
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, view);
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, projection);
    glUniformMatrix4fv(prevModelLocation, 1, GL_FALSE, prevModelMatrix);
    glUniformMatrix4fv(prevViewLocation, 1, GL_FALSE, prevViewMatrix);
    glUniformMatrix4fv(prevProjectionLocation, 1, GL_FALSE, prevProjMatrix);
    glUniform2fv(jitterOffsetLocation, 1, jitterOffset);
    glUniform3fv(shLocation0, 9, &shCoeffs[0][0]);
    if (camera != nullptr) {
        float* camPos = camera->getPosition();
        glUniform3f(cameraPosLocation, camPos[0], camPos[1], camPos[2]);
    }
    glUniform3f(baseColorLocation, 1.0f, 1.0f, 1.0f);
    glUniform1f(metallicLocation, 0.0f);
    glUniform1f(roughnessLocation, 0.2f);

    glUniform1f(albedoMapEnabledLocation, albedoTex != 0 ? 1.0f : 0.0f);
    glUniform1f(metallicMapEnabledLocation, metallicTex != 0 ? 1.0f : 0.0f);
    glUniform1f(roughnessMapEnabledLocation, roughnessTex != 0 ? 1.0f : 0.0f);
    glUniform1f(aoMapEnabledLocation, aoTex != 0 ? 1.0f : 0.0f);

    if (albedoTex != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, albedoTex);
        glUniform1i(albedoMapLocation, 0);
    }
    if (metallicTex != 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, metallicTex);
        glUniform1i(metallicMapLocation, 1);
    }
    if (roughnessTex != 0) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, roughnessTex);
        glUniform1i(roughnessMapLocation, 2);
    }
    if (aoTex != 0) {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, aoTex);
        glUniform1i(aoMapLocation, 3);
    }
}

void DebugIBLMaterial::release() {
    if (shader != nullptr) {
        shader->release();
        delete shader;
        shader = nullptr;
    }
    initialized = false;
}

const Material::VertexAttrib* DebugIBLMaterial::getVertexAttribs(int& count) {
    static VertexAttrib attribs[3] = {
        {"aPosition", 3, 0, 0},
        {"aNormal", 3, 0, 12},
        {"aTexCoord", 2, 0, 24}
    };
    count = 3;
    return attribs;
}

void DebugIBLMaterial::setIBLCoeffs(float sh[9][3]) {
    for (int i = 0; i < 9; i++) {
        shCoeffs[i][0] = sh[i][0];
        shCoeffs[i][1] = sh[i][1];
        shCoeffs[i][2] = sh[i][2];
    }
}

void DebugIBLMaterial::setTextureMaps(GLuint albedo, GLuint metallic, GLuint roughness, GLuint ao) {
    albedoTex = albedo;
    metallicTex = metallic;
    roughnessTex = roughness;
    aoTex = ao;
}
