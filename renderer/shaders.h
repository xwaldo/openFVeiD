/*
#    Copyright (C) 2026 Veia <h27ck@proton.me>
*/

#ifndef SHADERS_H
#define SHADERS_H

#include <GL/glew.h>
#include <iostream>

static void printGLSLCompileLog(GLuint shaderHandle) {
    GLint shaderError;
    glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &shaderError);
    if (shaderError != GL_TRUE) {
        std::cerr << "Shader compile error: " << std::endl;
    }

    GLsizei length = 0;
    glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &length);
    if (length > 1) {
        GLchar* pInfo = new char[length + 1];
        glGetShaderInfoLog(shaderHandle, length, &length, pInfo);
        std::cout << "Compile log: " << pInfo << std::endl;
        delete[] pInfo;
    }
}

static void printGLSLLinkLog(GLuint progHandle) {
    GLint programError;
    glGetProgramiv(progHandle, GL_LINK_STATUS, &programError);

    if (programError != GL_TRUE) {
        std::cerr << "Program could not get linked:" << std::endl;
    }

    GLsizei length = 0;
    glGetProgramiv(progHandle, GL_INFO_LOG_LENGTH, &length);
    if (length > 1) {
        GLchar* pInfo = new char[length + 1];
        glGetProgramInfoLog(progHandle, length, &length, pInfo);
        std::cout << "Linker log: " << pInfo << std::endl;
        delete[] pInfo;
    }
}

#endif // SHADERS_H
