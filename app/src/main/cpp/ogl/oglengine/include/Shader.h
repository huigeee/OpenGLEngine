#pragma once

#include <GLES3/gl3.h>
#include <string>
#include <cstring>

class Shader {
public:
    Shader();
    ~Shader();

    bool init(const char* vertexCode, const char* fragmentCode);
    void use();
    GLint getAttributeLocation(const char* name);
    GLint getUniformLocation(const char* name);
    void release();
    GLuint getProgram() const { return program; }
    void stealProgram(GLuint& out) { out = program; program = 0; }

    void setBool(const char* name, bool value) const {
        glUniform1i(glGetUniformLocation(program, name), (int)value);
    }
    void setInt(const char* name, int value) const {
        glUniform1i(glGetUniformLocation(program, name), value);
    }
    void setFloat(const char* name, float value) const {
        glUniform1f(glGetUniformLocation(program, name), value);
    }
    void setVec2(const char* name, const float* value) const {
        glUniform2fv(glGetUniformLocation(program, name), 1, value);
    }
    void setVec2(const char* name, float x, float y) const {
        glUniform2f(glGetUniformLocation(program, name), x, y);
    }
    void setVec3(const char* name, const float* value) const {
        glUniform3fv(glGetUniformLocation(program, name), 1, value);
    }
    void setVec3(const char* name, float x, float y, float z) const {
        glUniform3f(glGetUniformLocation(program, name), x, y, z);
    }
    void setVec4(const char* name, const float* value) const {
        glUniform4fv(glGetUniformLocation(program, name), 1, value);
    }
    void setVec4(const char* name, float x, float y, float z, float w) const {
        glUniform4f(glGetUniformLocation(program, name), x, y, z, w);
    }
    void setMat2(const char* name, const float* value) const {
        glUniformMatrix2fv(glGetUniformLocation(program, name), 1, GL_FALSE, value);
    }
    void setMat3(const char* name, const float* value) const {
        glUniformMatrix3fv(glGetUniformLocation(program, name), 1, GL_FALSE, value);
    }
    void setMat4(const char* name, const float* value) const {
        glUniformMatrix4fv(glGetUniformLocation(program, name), 1, GL_FALSE, value);
    }

private:
    GLuint program;

    bool initInternal(const char* vertexCode, const char* fragmentCode);
    GLuint loadShader(GLenum type, const char* shaderCode);
    void checkCompileErrors(GLuint shader, const char* type);
};
