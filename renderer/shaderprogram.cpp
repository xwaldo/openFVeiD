/*
#    FVD++, an advanced coaster design tool for NoLimits
#    Copyright (C) 2012-2015, Stephan "Lenny" Alt <alt.stephan@web.de>
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "shaderprogram.h"
#include "shaders.h"
#include "core/assets.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

static std::string readAll(const char* path) {
    const AssetData* asset = getEmbeddedAsset(path);
    if (asset) {
        return std::string((const char*)asset->data, asset->size);
    }

    std::string sPath(path);
    if (sPath == "shaders/simple_shadow.vert") {
        return "#version 430 core\n"
               "layout (location = 0) in vec3 aPosition;\n"
               "layout (location = 3) in mat4 aInstanceMatrix;\n"
               "layout (location = 7) in vec4 aAttributes1;\n"
               "layout (location = 8) in vec4 aAttributes2;\n"
               "uniform mat4 projectionMatrix; uniform mat4 modelMatrix; uniform mat4 anchorBase; uniform mat4 shadowMatrix;\n"
               "uniform int isInstanced; uniform float uTrackLength;\n"
               "struct SplineNode { vec4 pos; vec4 lat; vec4 norm; vec4 dir; };\n"
               "layout(std430, binding = 0) buffer SplineData { SplineNode nodes[]; };\n"
               "void main() {\n"
               "  vec4 worldPos;\n"
               "  if (isInstanced == 1) {\n"
               "    float startDist = aInstanceMatrix[3].x; float minZ = aInstanceMatrix[3].z;\n"
               "    float dist = clamp(startDist + (aPosition.z - minZ), 0.0, uTrackLength);\n"
               "    float fIndex = dist / 0.1; int i0 = int(floor(fIndex)); int i1 = i0 + 1; float t = fract(fIndex);\n"
               "    vec3 iPos = mix(nodes[i0].pos.xyz, nodes[i1].pos.xyz, t);\n"
               "    vec3 iLat = normalize(mix(nodes[i0].lat.xyz, nodes[i1].lat.xyz, t));\n"
               "    vec3 iNorm = normalize(mix(nodes[i0].norm.xyz, nodes[i1].norm.xyz, t));\n"
               "    float totalX = (aPosition.x * aInstanceMatrix[0][0]) + aAttributes2.z;\n"
               "    float totalY = (aPosition.y * aInstanceMatrix[1][1]) + aAttributes2.w;\n"
               "    worldPos = anchorBase * vec4(iPos - (iLat * totalX) - (iNorm * totalY), 1.0);\n"
               "  } else { worldPos = anchorBase * vec4(aPosition, 1.0); }\n"
               "  gl_Position = projectionMatrix * modelMatrix * shadowMatrix * worldPos;\n"
               "}";
    }
    if (sPath == "shaders/simple_shadow.frag") {
        return "#version 330 core\n"
               "out vec4 oFragColor;\n"
               "void main() { oFragColor = vec4(0.2, 0.2, 0.2, 0.6); }";
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

ShaderProgram::ShaderProgram(const char* _vertex, const char* _fragment) {
    std::string v = readAll(_vertex);
    std::string f = readAll(_fragment);

    sources[0] = glCreateShader(GL_VERTEX_SHADER);
    sources[1] = glCreateShader(GL_FRAGMENT_SHADER);
    program = glCreateProgram();

    const char* v_ptr = v.c_str();
    const char* f_ptr = f.c_str();

    glShaderSource(sources[0], 1, &v_ptr, 0);
    glShaderSource(sources[1], 1, &f_ptr, 0);

    glCompileShader(sources[0]);
    printGLSLCompileLog(sources[0]);
    glCompileShader(sources[1]);
    printGLSLCompileLog(sources[1]);
}

ShaderProgram::~ShaderProgram() {}

void ShaderProgram::useAttribute(GLuint _index, const GLchar* _name) {
    glBindAttribLocation(program, _index, _name);
}

void ShaderProgram::setOutput(GLuint _index, const GLchar* _name) {
    glBindFragDataLocation(program, _index, _name);
}

void ShaderProgram::useUniform(const GLchar* _name, glm::mat4* _mat4) {
    glUniformMatrix4fv(glGetUniformLocation(program, _name), 1, GL_FALSE,
                       glm::value_ptr(*_mat4));
}

void ShaderProgram::useUniform(const GLchar* _name, glm::vec4* _vec4) {
    glUniform4f(glGetUniformLocation(program, _name), _vec4->x, _vec4->y,
                _vec4->z, _vec4->w);
}

void ShaderProgram::useUniform(const GLchar* _name, glm::vec3* _vec3) {
    glUniform3f(glGetUniformLocation(program, _name), _vec3->x, _vec3->y,
                _vec3->z);
}

void ShaderProgram::useUniform(const GLchar* _name, float f1, float f2, float f3) {
    glUniform3f(glGetUniformLocation(program, _name), f1, f2, f3);
}

void ShaderProgram::useUniform(const GLchar* _name, GLuint _int) {
    glUniform1i(glGetUniformLocation(program, _name), _int);
}

void ShaderProgram::useUniform(const GLchar* _name, float _float) {
    glUniform1f(glGetUniformLocation(program, _name), _float);
}

void ShaderProgram::linkProgram() {
    glAttachShader(program, sources[0]);
    glAttachShader(program, sources[1]);

    glLinkProgram(program);
    printGLSLLinkLog(program);
}

void ShaderProgram::bind() {
    glUseProgram(program);
}
