//
// Created by BZLZHH on 2025/7/30.
//

#include "MultiBindWrapper.h"
#include <cassert>
#include "../texture.h"

#define DEBUG 0

void glBindTextures(GLuint first, GLsizei count, const GLuint* textures) {
    GLuint prevUnit;
    glGetIntegerv(GL_ACTIVE_TEXTURE, reinterpret_cast<GLint*>(&prevUnit));
    for (GLsizei i = 0; i < count; ++i) {
		GLenum target = ConvertTextureTargetToGLEnum(mgGetTexObjectByID(textures[i])->target);
        glActiveTexture(GL_TEXTURE0 + first + i);
        glBindTexture(target, textures[i]);
    }
    glActiveTexture(prevUnit);
}

void glBindSamplers(GLuint first, GLsizei count, const GLuint* samplers) {
    for (GLsizei i = 0; i < count; ++i) {
        glBindSampler(first + i, samplers[i]);
    }
}

void glBindImageTextures(GLuint first, GLsizei count, const GLuint* textures) {
    for (int i = 0; i < count; i++) {
        if (textures == nullptr || textures[i] == 0) {
            glBindImageTexture(first + i, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);
        }
        else {
            glBindImageTexture(first + i, textures[i], 0, GL_TRUE, 0, GL_READ_WRITE, mgGetTexObjectByID(textures[i])->internal_format);
        }
    }
}

void glBindVertexBuffers(GLuint first, GLsizei count, const GLuint* buffers,
    const GLintptr* offsets, const GLsizei* strides)
{
    for (GLsizei i = 0; i < count; ++i) {
        glBindVertexBuffer(
            first + i,
            buffers[i],
            offsets[i],
            strides[i]
        );
    }
}
