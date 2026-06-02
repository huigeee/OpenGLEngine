#include "../include/Shader.h"
#include "../include/Common.h"
#include <cstdio>

Shader::Shader() : program(0) {
}

Shader::~Shader() {
    release();
}

bool Shader::init(const char* vertexCode, const char* fragmentCode) {
    return initInternal(vertexCode, fragmentCode);
}

void Shader::use() {
    if (program != 0) {
        glUseProgram(program);
    }
}

GLint Shader::getAttributeLocation(const char* name) {
    return glGetAttribLocation(program, name);
}

GLint Shader::getUniformLocation(const char* name) {
    return glGetUniformLocation(program, name);
}

void Shader::release() {
    if (program != 0) {
        glDeleteProgram(program);
        program = 0;
    }
}

bool Shader::initInternal(const char* vertexCode, const char* fragmentCode) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexCode);
    if (vertexShader == 0) {
        LOGE("Failed to load vertex shader");
        return false;
    }

    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentCode);
    if (fragmentShader == 0) {
        LOGE("Failed to load fragment shader");
        glDeleteShader(vertexShader);
        return false;
    }

    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (!linkStatus) {
        LOGE("Failed to link shader program");
        GLint infoLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char* infoLog = new char[infoLen];
            glGetProgramInfoLog(program, infoLen, nullptr, infoLog);
            LOGE("Program link error: %s", infoLog);
            delete[] infoLog;
        }
        glDeleteProgram(program);
        program = 0;
        return false;
    }

    LOGI("Shader initialized successfully, program=%d", program);
    return true;
}

GLuint Shader::loadShader(GLenum type, const char* shaderCode) {
    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        LOGE("Failed to create shader");
        return 0;
    }

    glShaderSource(shader, 1, &shaderCode, nullptr);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        checkCompileErrors(shader, "SHADER");
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

void Shader::checkCompileErrors(GLuint shader, const char* type) {
    GLint success;
    GLchar infoLog[1024];
    if (strcmp(type, "SHADER") == 0) {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            LOGE("ERROR::SHADER_COMPILATION_ERROR\n%s", infoLog);
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            LOGE("ERROR::PROGRAM_LINKING_ERROR\n%s", infoLog);
        }
    }
}
