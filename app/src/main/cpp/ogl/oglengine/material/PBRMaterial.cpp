#include "../include/PBRMaterial.h"
#include "../include/Shader.h"
#include "../include/ShaderManager.h"
#include "../include/ResourceLoader.h"
#include "../include/Camera.h"
#include "../include/Common.h"
#include <string.h>
#include <cstdio>

PBRMaterial::PBRMaterial(bool clearCoat) : clearCoatEnabled(clearCoat),
    modelLocation(0), viewLocation(0), projectionLocation(0),
    prevModelLocation(0), prevViewLocation(0), prevProjectionLocation(0),
    jitterOffsetLocation(0),
    baseColorLocation(0), metallicLocation(0),
    roughnessLocation(0), cameraPosLocation(0),
    clearCoatLocation(0), clearCoatRoughnessLocation(0), positionLocation(0), normalLocation(0),
    envTexture(0), prefilteredLevels(0), brdfLUTTexture(0), brdfLUTLocation(0), iblEnabled(false),
    debugModeLocation(0) {
    baseColor[0] = 1.0f; baseColor[1] = 1.0f; baseColor[2] = 1.0f;
    metallic = 0.0f;
    roughness = 0.2f;

    lightPos[0][0] = 2.0f; lightPos[0][1] = 1.5f; lightPos[0][2] = 2.0f;
    lightColor[0][0] = 20.0f; lightColor[0][1] = 20.0f; lightColor[0][2] = 20.0f;

    lightPos[1][0] = -2.0f; lightPos[1][1] = 1.0f; lightPos[1][2] = 1.5f;
    lightColor[1][0] = 10.0f; lightColor[1][1] = 10.0f; lightColor[1][2] = 10.0f;

    lightPos[2][0] = 0.0f; lightPos[2][1] = 2.0f; lightPos[2][2] = -1.0f;
    lightColor[2][0] = 10.0f; lightColor[2][1] = 10.0f; lightColor[2][2] = 10.0f;

    clearCoatIntensity = 1.0f;
    clearCoatRoughness = 0.02f;

    for (int i = 0; i < 9; i++) {
        shCoeffs[i][0] = shCoeffs[i][1] = shCoeffs[i][2] = 0.0f;
    }
    for (int i = 0; i < 8; i++) {
        prefilteredTextures[i] = 0;
    }
    envTexture = 0;
    envCubemapTexture = 0;
    irradianceMapTexture = 0;
    prefilterCubemapTexture = 0;
    brdfLUTTexture = 0;
    prefilteredLevels = 0;
    iblEnabled = false;
    useCubemapIBL = false;
    albedoTex = 0;
    normalTex = 0;
    metallicTex = 0;
    roughnessTex = 0;
    aoTex = 0;
}

PBRMaterial::~PBRMaterial() {
}

void PBRMaterial::init() {
    shader = new Shader();
    if (clearCoatEnabled) {
        shader->init(ShaderManager::getPBRClearCoatIBLVertex(), ShaderManager::getPBRClearCoatIBLFragment());
    } else {
        shader->init(ShaderManager::getPBRIBLVertex(), ShaderManager::getPBRIBLFragment());
    }
    modelLocation = shader->getUniformLocation("uModelMatrix");
    viewLocation = shader->getUniformLocation("uViewMatrix");
    projectionLocation = shader->getUniformLocation("uProjectionMatrix");
    prevModelLocation = shader->getUniformLocation("uPrevModelMatrix");
    prevViewLocation = shader->getUniformLocation("uPrevViewMatrix");
    prevProjectionLocation = shader->getUniformLocation("uPrevProjectionMatrix");
    jitterOffsetLocation = shader->getUniformLocation("uJitterOffset");
    baseColorLocation = shader->getUniformLocation("uBaseColor");
    metallicLocation = shader->getUniformLocation("uMetallic");
    roughnessLocation = shader->getUniformLocation("uRoughness");
    LOGI("PBRMaterial init: program=%d modelLoc=%d viewLoc=%d projLoc=%d",
         shader->getProgram(), modelLocation, viewLocation, projectionLocation);
    positionLocation = shader->getAttributeLocation("aPosition");
    normalLocation = shader->getAttributeLocation("aNormal");
    cameraPosLocation = shader->getUniformLocation("uCameraPos");
    char name[32];
    for (int i = 0; i < 3; i++) {
        sprintf(name, "uLightPos%d", i);
        lightPosLocation[i] = shader->getUniformLocation(name);
        sprintf(name, "uLightColor%d", i);
        lightColorLocation[i] = shader->getUniformLocation(name);
    }
    for (int i = 0; i < 9; i++) {
        sprintf(name, "uSH[%d]", i);
        shLocation[i] = shader->getUniformLocation(name);
    }
    envTextureLocation = shader->getUniformLocation("uEnvTexture");
    envCubemapLocation = shader->getUniformLocation("uEnvCubemap");
    irradianceMapLocation = shader->getUniformLocation("uIrradianceMap");
    prefilterCubemapLocation = shader->getUniformLocation("uPrefilterCubemap");
    for (int i = 0; i < 8; i++) {
        sprintf(name, "uPrefilt%d", i);
        prefilteredTexLocation[i] = shader->getUniformLocation(name);
    }
    prefilteredLevelsLocation = shader->getUniformLocation("uPrefiltLevels");
    useCubemapLocation = shader->getUniformLocation("uUseCubemap");
    enableDirectLightLocation = shader->getUniformLocation("uEnableDirectLight");
    if (clearCoatEnabled) {
        clearCoatLocation = shader->getUniformLocation("uClearCoat");
        clearCoatRoughnessLocation = shader->getUniformLocation("uClearCoatRoughness");
    }
    brdfLUTLocation = shader->getUniformLocation("uBRDFLUT");
    albedoMapLocation = shader->getUniformLocation("uAlbedoMap");
    normalMapLocation = shader->getUniformLocation("uNormalMap");
    metallicMapLocation = shader->getUniformLocation("uMetallicMap");
    roughnessMapLocation = shader->getUniformLocation("uRoughnessMap");
    aoMapLocation = shader->getUniformLocation("uAOMap");
    normalMapEnabledLocation = shader->getUniformLocation("uNormalMapEnabled");
    albedoMapEnabledLocation = shader->getUniformLocation("uAlbedoMapEnabled");
    metallicMapEnabledLocation = shader->getUniformLocation("uMetallicMapEnabled");
    roughnessMapEnabledLocation = shader->getUniformLocation("uRoughnessMapEnabled");
    aoMapEnabledLocation = shader->getUniformLocation("uAOMapEnabled");
    debugModeLocation = shader->getUniformLocation("uDebugMode");
    initialized = true;
    LOGI("PBRMaterial initialized (clearCoat=%d, ibl=%d)", clearCoatEnabled, iblEnabled);
}

void PBRMaterial::setTransform(const float* model, const float* view, const float* projection) {
    if (!initialized || shader == nullptr) return;
    shader->use();
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model);
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, view);
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, projection);
    glUniformMatrix4fv(prevModelLocation, 1, GL_FALSE, prevModelMatrix);
    glUniformMatrix4fv(prevViewLocation, 1, GL_FALSE, prevViewMatrix);
    glUniformMatrix4fv(prevProjectionLocation, 1, GL_FALSE, prevProjMatrix);
    glUniform2fv(jitterOffsetLocation, 1, jitterOffset);
    glUniform3fv(baseColorLocation, 1, baseColor);
    glUniform1f(metallicLocation, metallic);
    glUniform1f(roughnessLocation, roughness);
    for (int i = 0; i < 3; i++) {
        glUniform3fv(lightPosLocation[i], 1, lightPos[i]);
        glUniform3fv(lightColorLocation[i], 1, lightColor[i]);
    }

    if (camera != nullptr) {
        float* camPos = camera->getPosition();
        glUniform3f(cameraPosLocation, camPos[0], camPos[1], camPos[2]);
    }

    if (iblEnabled) {
        glUniform3fv(shLocation[0], 9, &shCoeffs[0][0]);
        glUniform1f(useCubemapLocation, useCubemapIBL ? 1.0f : 0.0f);

        if (useCubemapIBL) {
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMapTexture);
            glUniform1i(glGetUniformLocation(shader->getProgram(), "uIrradianceMap"), 10);

            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterCubemapTexture);
            glUniform1i(glGetUniformLocation(shader->getProgram(), "uPrefilterCubemap"), 11);

            glActiveTexture(GL_TEXTURE12);
            glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
            glUniform1i(glGetUniformLocation(shader->getProgram(), "uBRDFLUT"), 12);
        } else {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, envTexture);
            glUniform1i(envTextureLocation, 0);
            for (int i = 0; i < prefilteredLevels; i++) {
                glActiveTexture(GL_TEXTURE1 + i);
                glBindTexture(GL_TEXTURE_2D, prefilteredTextures[i]);
                glUniform1i(prefilteredTexLocation[i], 1 + i);
            }
        }

        glUniform1f(prefilteredLevelsLocation, (float)prefilteredLevels);
    }

    glUniform1f(enableDirectLightLocation, 1.0f);
    glUniform1f(debugModeLocation, 0.0f);

    glUniform1f(albedoMapEnabledLocation, albedoTex != 0 ? 1.0f : 0.0f);
    glUniform1f(normalMapEnabledLocation, normalTex != 0 ? 1.0f : 0.0f);
    glUniform1f(metallicMapEnabledLocation, metallicTex != 0 ? 1.0f : 0.0f);
    glUniform1f(roughnessMapEnabledLocation, roughnessTex != 0 ? 1.0f : 0.0f);
    glUniform1f(aoMapEnabledLocation, aoTex != 0 ? 1.0f : 0.0f);

    if (albedoTex != 0) {
        glActiveTexture(GL_TEXTURE16);
        glBindTexture(GL_TEXTURE_2D, albedoTex);
        glUniform1i(albedoMapLocation, 16);
    }
    if (normalTex != 0) {
        glActiveTexture(GL_TEXTURE17);
        glBindTexture(GL_TEXTURE_2D, normalTex);
        glUniform1i(normalMapLocation, 17);
    }
    if (metallicTex != 0) {
        glActiveTexture(GL_TEXTURE18);
        glBindTexture(GL_TEXTURE_2D, metallicTex);
        glUniform1i(metallicMapLocation, 18);
    }
    if (roughnessTex != 0) {
        glActiveTexture(GL_TEXTURE19);
        glBindTexture(GL_TEXTURE_2D, roughnessTex);
        glUniform1i(roughnessMapLocation, 19);
    }
    if (aoTex != 0) {
        glActiveTexture(GL_TEXTURE20);
        glBindTexture(GL_TEXTURE_2D, aoTex);
        glUniform1i(aoMapLocation, 20);
    }

    if (clearCoatEnabled) {
        glUniform1f(clearCoatLocation, clearCoatIntensity);
        glUniform1f(clearCoatRoughnessLocation, clearCoatRoughness);
    }
}

void PBRMaterial::release() {
    if (shader != nullptr) {
        shader->release();
        delete shader;
        shader = nullptr;
    }
    initialized = false;
}

void PBRMaterial::setBaseColor(float r, float g, float b) {
    baseColor[0] = r; baseColor[1] = g; baseColor[2] = b;
}

void PBRMaterial::setMetallic(float m) {
    metallic = m;
}

void PBRMaterial::setRoughness(float r) {
    roughness = r;
}

void PBRMaterial::setLightPos(int index, float x, float y, float z) {
    if (index >= 0 && index < 3) {
        lightPos[index][0] = x; lightPos[index][1] = y; lightPos[index][2] = z;
    }
}

void PBRMaterial::setLightColor(int index, float r, float g, float b) {
    if (index >= 0 && index < 3) {
        lightColor[index][0] = r; lightColor[index][1] = g; lightColor[index][2] = b;
    }
}

void PBRMaterial::setClearCoat(float intensity, float r) {
    clearCoatIntensity = intensity;
    clearCoatRoughness = r;
}

void PBRMaterial::enableClearCoat(bool enable) {
    if (clearCoatEnabled != enable) {
        clearCoatEnabled = enable;
        if (initialized) {
            release();
            init();
        }
    }
}

void PBRMaterial::setIBL(float sh[9][3], GLuint envTex, GLuint prefiltered[], int levels, GLuint brdfLUT) {
    for (int i = 0; i < 9; i++) {
        shCoeffs[i][0] = sh[i][0];
        shCoeffs[i][1] = sh[i][1];
        shCoeffs[i][2] = sh[i][2];
    }
    envTexture = envTex;
    envCubemapTexture = 0;
    prefilterCubemapTexture = 0;
    prefilteredLevels = levels < 8 ? levels : 8;
    for (int i = 0; i < prefilteredLevels; i++) {
        prefilteredTextures[i] = prefiltered[i];
    }
    brdfLUTTexture = brdfLUT;
    iblEnabled = true;
    useCubemapIBL = false;
}

void PBRMaterial::setIBLCubemap(float sh[9][3], GLuint irradianceMap, GLuint prefilterCubemap, int levels, GLuint brdfLUT) {
    for (int i = 0; i < 9; i++) {
        shCoeffs[i][0] = sh[i][0];
        shCoeffs[i][1] = sh[i][1];
        shCoeffs[i][2] = sh[i][2];
    }
    envTexture = 0;
    irradianceMapTexture = irradianceMap;
    prefilterCubemapTexture = prefilterCubemap;
    prefilteredLevels = levels;
    brdfLUTTexture = brdfLUT;
    iblEnabled = true;
    useCubemapIBL = true;
}

void PBRMaterial::setIBLFromScene(GLuint irradiance, GLuint prefilter, GLuint brdfLUT) {
    if (!initialized || shader == nullptr) return;
    irradianceMapTexture = irradiance;
    prefilterCubemapTexture = prefilter;
    brdfLUTTexture = brdfLUT;
    prefilteredLevels = 4;
    iblEnabled = true;
    useCubemapIBL = true;

    shader->use();
    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMapTexture);
    glUniform1i(irradianceMapLocation, 10);
    glActiveTexture(GL_TEXTURE11);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterCubemapTexture);
    glUniform1i(prefilterCubemapLocation, 11);
    glActiveTexture(GL_TEXTURE12);
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glUniform1i(brdfLUTLocation, 12);
    glUniform1f(useCubemapLocation, 1.0f);
    glUniform1f(prefilteredLevelsLocation, 4.0f);
    glUniform1f(enableDirectLightLocation, 1.0f);
}

const Material::VertexAttrib* PBRMaterial::getVertexAttribs(int& count) {
    static VertexAttrib attribs[3] = {
        {"aPosition", 3, 0, 0},
        {"aNormal", 3, 0, 3 * (int)sizeof(float)},
        {"aTexCoord", 2, 0, 6 * (int)sizeof(float)}
    };
    attribs[0].location = positionLocation;
    attribs[1].location = normalLocation;
    count = 3;
    return attribs;
}

GLuint PBRMaterial::uploadTextureFromFile(ResourceLoader* loader, const char* path, bool flipUv) {
    if (loader == nullptr || path == nullptr) return 0;
    TextureData tex = loader->loadTexture(path, flipUv);
    if (!tex.isValid()) {
        LOGE("uploadTextureFromFile FAILED: %s", path);
        return 0;
    }
    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    GLenum fmt = tex.channels == 4 ? GL_RGBA : (tex.channels == 1 ? GL_RED : GL_RGB);
    GLenum type = tex.is16bit ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE;
    if (tex.is16bit) {
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, tex.width, tex.height, 0, fmt, type, tex.data16);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, tex.width, tex.height, 0, fmt, type, tex.data);
    }
    glGenerateMipmap(GL_TEXTURE_2D);
    LOGI("uploadTextureFromFile OK: %s -> tex=%d (%dx%d ch=%d 16bit=%d)", path, texId, tex.width, tex.height, tex.channels, tex.is16bit);
    loader->freeTexture(tex);
    return texId;
}

void PBRMaterial::setTextureMaps(GLuint albedo, GLuint normal, GLuint metallic, GLuint roughness, GLuint ao) {
    if (albedoTex != 0 && albedoTex != albedo) { glDeleteTextures(1, &albedoTex); }
    if (normalTex != 0 && normalTex != normal) { glDeleteTextures(1, &normalTex); }
    if (metallicTex != 0 && metallicTex != metallic) { glDeleteTextures(1, &metallicTex); }
    if (roughnessTex != 0 && roughnessTex != roughness) { glDeleteTextures(1, &roughnessTex); }
    if (aoTex != 0 && aoTex != ao) { glDeleteTextures(1, &aoTex); }
    albedoTex = albedo;
    normalTex = normal;
    metallicTex = metallic;
    roughnessTex = roughness;
    aoTex = ao;
}
