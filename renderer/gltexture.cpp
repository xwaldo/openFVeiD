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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "gltexture.h"
#include "lenassert.h"
#include "core/assets.h"
#include <iostream>

std::vector<bool> GlTexture::usedIDs;

GlTexture::GlTexture(unsigned char* data, int width, int height, int mode) {
    handle = 0;
    iType = 0;
    mId = getFreeID();
    if (mId >= 0)
        GlTexture::usedIDs[mId] = true;
    glActiveTexture(GL_TEXTURE0 + (mId >= 0 ? mId : 0));
    lenAssert(data != nullptr);

    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);

    GLfloat fLargest;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    if (mode == 0) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    } else if (mode == 1) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width,
                 height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 data);

    glGenerateMipmap(GL_TEXTURE_2D);

    iType = 0;
}

GlTexture::GlTexture(const char* _image, int mode) {
    handle = 0;
    iType = 0;
    mId = getFreeID();
    if (mId >= 0)
        GlTexture::usedIDs[mId] = true;
    glActiveTexture(GL_TEXTURE0 + (mId >= 0 ? mId : 0));

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = nullptr;
    const AssetData* asset = getEmbeddedAsset(_image);
    if (asset) {
        data = stbi_load_from_memory(asset->data, static_cast<int>(asset->size), &width, &height, &channels, 4);
    } else {
        data = stbi_load(_image, &width, &height, &channels, 4);
    }

    if (!data) {
        std::cerr << "Failed to load texture: " << _image << " (Reason: " << stbi_failure_reason() << ")" << std::endl;
        return;
    }

    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);
    GLfloat fLargest;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    if (mode == 0) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    } else if (mode == 1) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width,
                 height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 data);

    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    iType = 0;
}

GlTexture::GlTexture(const char* _posx, const char* _negx, const char* _posy,
                     const char* _negy, const char* _posz, const char* _negz) {
    handle = 0;
    iType = 2;
    mId = getFreeID();
    if (mId >= 0)
        GlTexture::usedIDs[mId] = true;
    glActiveTexture(GL_TEXTURE0 + (mId >= 0 ? mId : 0));
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_CUBE_MAP, handle);

    const char* files[6] = {_posx, _negx, _posy, _negy, _posz, _negz};
    GLenum targets[6] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z};

    stbi_set_flip_vertically_on_load(false);
    for (int i = 0; i < 6; i++) {
        int width, height, channels;
        unsigned char* data = nullptr;
        const AssetData* asset = getEmbeddedAsset(files[i]);
        if (asset) {
            data = stbi_load_from_memory(asset->data, static_cast<int>(asset->size), &width, &height, &channels, 4);
        } else {
            data = stbi_load(files[i], &width, &height, &channels, 4);
        }
        if (data) {
            glTexImage2D(targets[i], 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            std::cerr << "Failed to load cubemap texture: " << files[i] << " (Reason: " << stbi_failure_reason() << ")" << std::endl;
        }
    }

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    iType = 2;
}

GlTexture::GlTexture(int _width, int _height, GLuint _format,
                     GLuint _intFormat) {
    handle = 0;
    iType = 1;
    mId = getFreeID();
    if (mId >= 0)
        GlTexture::usedIDs[mId] = true;
    glActiveTexture(GL_TEXTURE0 + (mId >= 0 ? mId : 0));
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, _format, _width, _height,
                 0, _intFormat, GL_FLOAT, 0);

    glGenerateMipmap(GL_TEXTURE_2D);

    iType = 1;
}

GlTexture::~GlTexture() {
    if (mId >= 0 && mId < (int)GlTexture::usedIDs.size())
        GlTexture::usedIDs[mId] = false;
    glDeleteTextures(1, &handle);
}

void GlTexture::resize(int _width, int _height, GLuint _format,
                       GLuint _intFormat) {
    glActiveTexture(GL_TEXTURE0 + mId);
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexImage2D(GL_TEXTURE_2D, 0, _format, _width, _height,
                 0, _intFormat, GL_FLOAT, 0);
    glGenerateMipmap(GL_TEXTURE_2D);
}

void GlTexture::changeTexture(unsigned char* data, int width, int height) {
    glActiveTexture(GL_TEXTURE0 + mId);
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width,
                 height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 data);
    glGenerateMipmap(GL_TEXTURE_2D);
}

GLuint GlTexture::getId() {
    return (GLuint)mId;
}

GLuint GlTexture::getHandle() {
    return handle;
}

void GlTexture::bind() {
    glActiveTexture(GL_TEXTURE0 + mId);
    if (iType == 0 || iType == 1) {
        glBindTexture(GL_TEXTURE_2D, handle);
    } else if (iType == 2) {
        glBindTexture(GL_TEXTURE_CUBE_MAP, handle);
    }
}

int GlTexture::getFreeID() {
    for (int i = 0; i < (int)GlTexture::usedIDs.size(); ++i) {
        if (!GlTexture::usedIDs[i])
            return i;
    }
    return -1;
}

void GlTexture::initialize() {
    int maxIDs = 0;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxIDs);

    if (maxIDs <= 0) {
        std::cerr << "Warning: GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS returned 0. Using default of 32." << std::endl;
        maxIDs = 32;
    }

    GlTexture::usedIDs.clear();
    for (int i = 0; i < maxIDs; ++i) {
        GlTexture::usedIDs.push_back(false);
    }
}
