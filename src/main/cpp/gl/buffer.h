//
// Created by BZLZHH on 2025/1/28.
//

#ifndef MOBILEGLUES_BUFFER_H

#include "../includes.h"
#include "gl.h"
#include "glcorearb.h"
#include "log.h"
#include "loader.h"
#include "../gles/loader.h"
#include "mg.h"

typedef struct {
    GLenum target;
    GLuint buffer_id;
    void* mapped_ptr;
    GLsizeiptr size;
    GLbitfield flags;
    GLboolean is_dirty;
} BufferMapping;

static BufferMapping g_active_mapping = {0};

static GLenum get_binding_query(GLenum target);
static void force_unmap(GLenum target, GLuint original_buffer);

GLAPI GLAPIENTRY GLboolean glUnmapBuffer(GLenum target);
GLAPI GLAPIENTRY void *glMapBuffer(GLenum target, GLenum access);

#define MOBILEGLUES_BUFFER_H

#endif //MOBILEGLUES_BUFFER_H
