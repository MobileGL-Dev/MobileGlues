//
// Created by BZLZHH on 2025/1/28.
//

#include "buffer.h"
#include <unordered_map>

#define DEBUG 0

std::unordered_map<GLuint, BufferMapping> g_active_mappings;

static GLenum get_binding_query(GLenum target) {
    switch(target) {
        case GL_ARRAY_BUFFER:          return GL_ARRAY_BUFFER_BINDING;
        case GL_ELEMENT_ARRAY_BUFFER:  return GL_ELEMENT_ARRAY_BUFFER_BINDING;
        case GL_PIXEL_PACK_BUFFER:     return GL_PIXEL_PACK_BUFFER_BINDING;
        case GL_PIXEL_UNPACK_BUFFER:   return GL_PIXEL_UNPACK_BUFFER_BINDING;
        default:                       return 0;
    }
}

void glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
    LOG()
    LOG_D("glBufferData, target = %s, size = %d, data = 0x%x, usage = %s",
          glEnumToString(target), size, data, glEnumToString(usage))
    LOAD_GLES_FUNC(glBufferData)
    gles_glBufferData(target, size, data, usage);
    CHECK_GL_ERROR
}

void* glMapBuffer(GLenum target, GLenum access) {
    LOG()
    LOG_D("glMapBuffer, target = %s, access = %s", glEnumToString(target), glEnumToString(access))
    if(g_gles_caps.GL_OES_mapbuffer) {
        LOAD_GLES_FUNC(glMapBufferOES)
        return gles_glMapBufferOES(target, access);
    }
    if (get_binding_query(target) == 0) {
        return nullptr;
    }
    GLint current_buffer;
    LOAD_GLES_FUNC(glGetIntegerv)
    LOAD_GLES_FUNC(glGetBufferParameteriv)
    LOAD_GLES_FUNC(glMapBufferRange)
    gles_glGetIntegerv(get_binding_query(target), &current_buffer);
    if (current_buffer == 0) {
        return nullptr;
    }
    if (g_active_mappings[current_buffer].mapped_ptr != nullptr) {
        return nullptr;
    }
    GLint buffer_size;
    gles_glGetBufferParameteriv(target, GL_BUFFER_SIZE, &buffer_size);
    if (buffer_size <= 0 || glGetError() != GL_NO_ERROR) {
        return nullptr;
    }
    GLbitfield flags = 0;
    switch (access) {
        case GL_READ_ONLY:  flags = GL_MAP_READ_BIT; break;
        case GL_WRITE_ONLY: flags = GL_MAP_WRITE_BIT /*| GL_MAP_INVALIDATE_BUFFER_BIT*/; break;
        case GL_READ_WRITE: flags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT; break;
        default:  
            return nullptr;
    }
    void* ptr = gles_glMapBufferRange(target, 0, buffer_size, flags);
    if (!ptr) return nullptr;
    BufferMapping mapping;
    mapping.target = target;
    mapping.buffer_id = (GLuint)current_buffer;
    mapping.mapped_ptr = ptr;
#if GLOBAL_DEBUG || DEBUG
    if (target == GL_PIXEL_UNPACK_BUFFER) {
        mapping.client_ptr = malloc(buffer_size);
        memset(mapping.client_ptr, 0xFF, buffer_size);
    }
#endif
    mapping.size = buffer_size;
    mapping.flags = flags;
    mapping.is_dirty = (flags & GL_MAP_WRITE_BIT) ? GL_TRUE : GL_FALSE;
    g_active_mappings[current_buffer] = mapping;
    CHECK_GL_ERROR
#if GLOBAL_DEBUG || DEBUG
    if (target == GL_PIXEL_UNPACK_BUFFER)
        return mapping.client_ptr;
    else
        return ptr;
#else
    return ptr;
#endif
}

#if GLOBAL_DEBUG || DEBUG
#include <fstream>
#define BIN_FILE_PREFIX "/sdcard/MG/buf/"
#endif

#ifdef __cplusplus
extern "C" {
#endif
    
#if !defined(__APPLE__)
GLAPI GLAPIENTRY void *glMapBufferARB(GLenum target, GLenum access) __attribute__((alias("glMapBuffer")));
GLAPI GLAPIENTRY void *glBufferDataARB(GLenum target, GLenum access) __attribute__((alias("glBufferData")));
GLAPI GLAPIENTRY GLboolean glUnmapBufferARB(GLenum target) __attribute__((alias("glUnmapBuffer")));
GLAPI GLAPIENTRY void glBufferStorageARB(GLenum target, GLsizeiptr size, const void* data, GLbitfield flags) __attribute__((alias("glBufferStorage")));
GLAPI GLAPIENTRY void glBindBufferARB(GLenum target, GLuint buffer) __attribute__((alias("glBindBuffer")));
#endif

#ifdef __cplusplus
}
#endif

GLboolean glUnmapBuffer(GLenum target) {
    LOG()
    LOAD_GLES_FUNC(glUnmapBuffer)
    if(g_gles_caps.GL_OES_mapbuffer)
        return gles_glUnmapBuffer(target);
    
    GLint buffer;
    GLenum binding_query = get_binding_query(target);
    LOAD_GLES_FUNC(glGetIntegerv)
    gles_glGetIntegerv(binding_query, &buffer);

    if (buffer == 0)
        return GL_FALSE;

#if GLOBAL_DEBUG || DEBUG
//     Blit data from client side to OpenGL here
    if (target == GL_PIXEL_UNPACK_BUFFER) {
        auto &mapping = g_active_mappings[buffer];

        std::fstream fs(std::string(BIN_FILE_PREFIX) + "buf" + std::to_string(buffer) + ".bin", std::ios::out | std::ios::binary | std::ios::trunc);
        fs.write((const char*)mapping.client_ptr, mapping.size);
        fs.close();

//        memset(mapping.mapped_ptr, 0xFF, mapping.size);
        memcpy(mapping.mapped_ptr, mapping.client_ptr, mapping.size);
        free(mapping.client_ptr);
        mapping.client_ptr = nullptr;
    }
#endif

    GLboolean result = gles_glUnmapBuffer(target);
    g_active_mappings.erase(buffer);
    CHECK_GL_ERROR
    return result;
}

void glBufferStorage(GLenum target, GLsizeiptr size, const void* data, GLbitfield flags) {
    LOG()
    LOAD_GLES_FUNC(glBufferStorageEXT)
    if(gles_glBufferStorageEXT)
        gles_glBufferStorageEXT(target,size,data,flags);
    CHECK_GL_ERROR
}

void glBindBuffer(GLenum target, GLuint buffer) {
    LOG()
    LOG_D("glBindBuffer, target = %s, buffer = %d", glEnumToString(target), buffer)
    LOAD_GLES_FUNC(glBindBuffer)
    gles_glBindBuffer(target, buffer);
}