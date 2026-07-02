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

#include "glframebuffer.h"
#include <iostream>
#include "gltexture.h"

GlFramebuffer::GlFramebuffer(int _width, int _height, GLuint _format,
                             GLuint _intFormat, bool _useRenderbuffer) {
    width = _width;
    height = _height;
    format = _format;
    intFormat = _intFormat;
    clearColor = glm::vec3(1, 1, 1);

    glGenFramebuffers(1, &handle);
    glBindFramebuffer(GL_FRAMEBUFFER, handle);

    target = new GlTexture(_width, _height, format, intFormat);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           target->getHandle(), 0);

    if (_useRenderbuffer) {
        glGenRenderbuffers(1, &renderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_STENCIL, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, renderbuffer);
    }

    GLuint attachments[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printFBStatus();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GlFramebuffer::~GlFramebuffer() {
    glDeleteFramebuffers(1, &handle);
    delete target;
    glDeleteRenderbuffers(1, &renderbuffer);
}

void GlFramebuffer::resize(int _width, int _height) {
    width = _width;
    height = _height;

    glBindFramebuffer(GL_FRAMEBUFFER, handle);
    target->resize(width, height, format, intFormat);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           target->getHandle(), 0);

    if (renderbuffer) {
        glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_STENCIL, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, renderbuffer);
    }

    GLuint attachments[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printFBStatus();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint GlFramebuffer::getHandle() {
    return handle;
}

GLuint GlFramebuffer::getTexture() {
    return target->getId();
}

GLuint GlFramebuffer::getTextureUnit() {
    return target->getId();
}

GLuint GlFramebuffer::getTextureHandle() {
    return target->getHandle();
}

void GlFramebuffer::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, handle);
}

void GlFramebuffer::clear() {
    glBindFramebuffer(GL_FRAMEBUFFER, handle);
    glClearColor(clearColor.x, clearColor.y, clearColor.z, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GlFramebuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GlFramebuffer::bindTexture() {
    if (target)
        target->bind();
}

void GlFramebuffer::printFBStatus() {
    GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch (status) {
    case GL_FRAMEBUFFER_UNDEFINED:
        std::cerr << "Target is the default framebuffer, but the default framebuffer "
                     "does not exist."
                  << std::endl;
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        std::cerr << "Any of the framebuffer attachment points are framebuffer incomplete." << std::endl;
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        std::cerr << "The framebuffer does not have at least one image attached to it." << std::endl;
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        std::cerr << "The value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE "
                     "for any color attachment point(s) named by GL_DRAW_BUFFERi."
                  << std::endl;
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        std::cerr << "GL_READ_BUFFER is not GL_NONE and the value of "
                     "GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for the color "
                     "attachment point named by GL_READ_BUFFER."
                  << std::endl;
        break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
        std::cerr << "The combination of internal formats of the attached images "
                     "violates an implementation-dependent set of restrictions."
                  << std::endl;
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        std::cerr << "The value of GL_RENDERBUFFER_SAMPLES is not the same for all attached "
                     "renderbuffers; if the value of GL_TEXTURE_SAMPLES is the not same for "
                     "all attached textures; or, if the attached images are a mix of "
                     "renderbuffers and textures, the value of GL_RENDERBUFFER_SAMPLES does "
                     "not match the value of GL_TEXTURE_SAMPLES."
                  << std::endl;
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        std::cerr << "is returned if any framebuffer attachment is layered, and any "
                     "populated attachment is not layered, or if all populated color "
                     "attachments are not from textures of the same target."
                  << std::endl;
        break;
    }
}

void GlFramebuffer::setClearColor(float f1, float f2, float f3) {
    clearColor = glm::vec3(f1, f2, f3);
}
