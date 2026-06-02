#include "../include/SimpleTextureMaterial.h"
#include "../include/Shader.h"
#include "../include/Common.h"
#include <string.h>

static const char* TEX_VERTEX = R"(
    #version 300 es
    precision mediump float;
    uniform mat4 uModelMatrix;
    uniform mat4 uViewMatrix;
    uniform mat4 uProjectionMatrix;
    uniform mat4 uPrevModelMatrix;
    uniform mat4 uPrevViewMatrix;
    uniform mat4 uPrevProjectionMatrix;
    uniform vec2 uJitterOffset;
    layout(location = 0) in vec3 aPosition;
    layout(location = 1) in vec2 aTexCoord;
    out vec2 vTexCoord;
    flat out vec4 nowScreenPos;
    flat out vec4 preScreenPos;
    void main() {
        vec4 worldPos = uModelMatrix * vec4(aPosition, 1.0);
        vTexCoord = aTexCoord;
        mat4 p = uProjectionMatrix;
        p[2][0] += uJitterOffset.x;
        p[2][1] += uJitterOffset.y;
        gl_Position = p * uViewMatrix * worldPos;
        nowScreenPos = uProjectionMatrix * uViewMatrix * worldPos;
        vec4 prevWorldPos = uPrevModelMatrix * vec4(aPosition, 1.0);
        preScreenPos = uPrevProjectionMatrix * uPrevViewMatrix * prevWorldPos;
    }
)";

static const char* TEX_FRAGMENT = R"(
    #version 300 es
    precision mediump float;
    in vec2 vTexCoord;
    flat in vec4 nowScreenPos;
    flat in vec4 preScreenPos;
    uniform sampler2D uTexture;
    layout(location = 0) out vec4 fragColor;
    layout(location = 1) out vec2 velocity;
    void main() {
        fragColor = texture(uTexture, vTexCoord);
        vec2 newPos = ((nowScreenPos.xy / nowScreenPos.w) * 0.5 + 0.5);
        vec2 prePos = ((preScreenPos.xy / preScreenPos.w) * 0.5 + 0.5);
        velocity = newPos - prePos;
    }
)";

SimpleTextureMaterial::SimpleTextureMaterial() : texId(0),
    modelLocation(0), viewLocation(0), projectionLocation(0),
    prevModelLocation(0), prevViewLocation(0), prevProjectionLocation(0),
    jitterOffsetLocation(0),
    texLocation(0), positionLocation(0), texCoordLocation(0) {
}

SimpleTextureMaterial::~SimpleTextureMaterial() {
}

void SimpleTextureMaterial::init() {
    shader = new Shader();
    shader->init(TEX_VERTEX, TEX_FRAGMENT);
    modelLocation = shader->getUniformLocation("uModelMatrix");
    viewLocation = shader->getUniformLocation("uViewMatrix");
    projectionLocation = shader->getUniformLocation("uProjectionMatrix");
    prevModelLocation = shader->getUniformLocation("uPrevModelMatrix");
    prevViewLocation = shader->getUniformLocation("uPrevViewMatrix");
    prevProjectionLocation = shader->getUniformLocation("uPrevProjectionMatrix");
    jitterOffsetLocation = shader->getUniformLocation("uJitterOffset");
    texLocation = shader->getUniformLocation("uTexture");
    positionLocation = shader->getAttributeLocation("aPosition");
    texCoordLocation = shader->getAttributeLocation("aTexCoord");
    initialized = true;
    LOGI("SimpleTextureMaterial initialized");
}

void SimpleTextureMaterial::setTransform(const float* model, const float* view, const float* projection) {
    if (!initialized || shader == nullptr) return;
    shader->use();
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model);
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, view);
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, projection);
    glUniformMatrix4fv(prevModelLocation, 1, GL_FALSE, prevModelMatrix);
    glUniformMatrix4fv(prevViewLocation, 1, GL_FALSE, prevViewMatrix);
    glUniformMatrix4fv(prevProjectionLocation, 1, GL_FALSE, prevProjMatrix);
    glUniform2fv(jitterOffsetLocation, 1, jitterOffset);
    if (texId != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texId);
        glUniform1i(texLocation, 0);
    }
}

void SimpleTextureMaterial::release() {
    if (shader != nullptr) {
        shader->release();
        delete shader;
        shader = nullptr;
    }
    initialized = false;
}

void SimpleTextureMaterial::setTexture(GLuint tex) {
    texId = tex;
}

const Material::VertexAttrib* SimpleTextureMaterial::getVertexAttribs(int& count) {
    static VertexAttrib attribs[2] = {
        {"aPosition", 3, 0, 0},
        {"aTexCoord", 2, 0, 3 * (int)sizeof(float)}
    };
    attribs[0].location = positionLocation;
    attribs[1].location = texCoordLocation;
    count = 2;
    return attribs;
}
