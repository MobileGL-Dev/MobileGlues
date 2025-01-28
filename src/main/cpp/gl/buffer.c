//
// Created by BZLZHH on 2025/1/28.
//

#include "buffer.h"

static GLenum get_binding_query(GLenum target) {
    switch(target) {
        case GL_ARRAY_BUFFER:          return GL_ARRAY_BUFFER_BINDING;
        case GL_ELEMENT_ARRAY_BUFFER:  return GL_ELEMENT_ARRAY_BUFFER_BINDING;
        case GL_PIXEL_PACK_BUFFER:     return GL_PIXEL_PACK_BUFFER_BINDING;
        case GL_PIXEL_UNPACK_BUFFER:   return GL_PIXEL_UNPACK_BUFFER_BINDING;
        default:                       return 0;
    }
}

void* glMapBuffer(GLenum target, GLenum access) {
    if (get_binding_query(target) == 0) {
        return NULL;
    }
    GLint current_buffer;
    glGetIntegerv(get_binding_query(target), &current_buffer);
    if (current_buffer == 0) {
        return NULL;
    }
    if (g_active_mapping.mapped_ptr != NULL) {
        return NULL;
    }
    GLint buffer_size;
    glGetBufferParameteriv(target, GL_BUFFER_SIZE, &buffer_size);
    if (buffer_size <= 0 || glGetError() != GL_NO_ERROR) {
        return NULL;
    }
    GLbitfield flags = 0;
    switch (access) {
        case GL_READ_ONLY:  flags = GL_MAP_READ_BIT; break;
        case GL_WRITE_ONLY: flags = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT; break;
        case GL_READ_WRITE: flags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT; break;
        default:  
            return NULL;
    }
    void* ptr = glMapBufferRange(target, 0, buffer_size, flags);
    if (!ptr) return NULL;
    g_active_mapping.target = target;
    g_active_mapping.buffer_id = (GLuint)current_buffer;
    g_active_mapping.mapped_ptr = ptr;
    g_active_mapping.size = buffer_size;
    g_active_mapping.flags = flags;
    g_active_mapping.is_dirty = (flags & GL_MAP_WRITE_BIT) ? GL_TRUE : GL_FALSE;

    return ptr;
}

static void force_unmap(GLenum target, GLuint original_buffer) {
    GLint prev_buffer;
    glGetIntegerv(get_binding_query(target), &prev_buffer);
    GLuint temp_buffer;
    glGenBuffers(1, &temp_buffer);
    glBindBuffer(target, temp_buffer);
    glBindBuffer(target, 0);
    glDeleteBuffers(1, &temp_buffer);
    if (target == GL_ARRAY_BUFFER) {
        GLint prev_element_array;
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &prev_element_array);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prev_element_array);
    } else {
        GLint prev_array;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prev_array);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, prev_array);
    }
    glBindBuffer(target, original_buffer);
}

GLboolean glUnmapBuffer(GLenum target) {
    if (g_active_mapping.mapped_ptr == NULL ||
        g_active_mapping.target != target ||
        g_active_mapping.buffer_id == 0)
    {
        return GL_FALSE;
    }
    GLint buffer_size;
    glGetBufferParameteriv(target, GL_BUFFER_SIZE, &buffer_size);
    if (glGetError() != GL_NO_ERROR || buffer_size <= 0) {
        memset(&g_active_mapping, 0, sizeof(BufferMapping));
        return GL_FALSE;
    }
    glBindBuffer(target, g_active_mapping.buffer_id);
    force_unmap(target, g_active_mapping.buffer_id);
    if (g_active_mapping.is_dirty) {
        GLuint temp_pbo;
        glGenBuffers(1, &temp_pbo);
        glBindBuffer(GL_COPY_WRITE_BUFFER, temp_pbo);
        glBufferData(GL_COPY_WRITE_BUFFER, g_active_mapping.size,
                     g_active_mapping.mapped_ptr, GL_STREAM_COPY);
        glBindBuffer(target, g_active_mapping.buffer_id);
        GLint is_mapped;
        glGetBufferParameteriv(target, GL_BUFFER_MAPPED, &is_mapped);
        if (is_mapped) {
            glBufferData(target, g_active_mapping.size, NULL, GL_STATIC_DRAW);
        } else {
            glBindBuffer(GL_COPY_READ_BUFFER, temp_pbo);
            glCopyBufferSubData(GL_COPY_READ_BUFFER, target, 0, 0, g_active_mapping.size);
        }

        glDeleteBuffers(1, &temp_pbo);
    }
    memset(&g_active_mapping, 0, sizeof(BufferMapping));
    return GL_TRUE;
}