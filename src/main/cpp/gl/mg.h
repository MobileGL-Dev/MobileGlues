//
// Created by BZLZHH on 2025/1/27.
//

#ifndef MOBILEGLUES_MG_H
#define MOBILEGLUES_MG_H

#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <android/log.h>

#include "gl.h"
#include "../gles/gles.h"
#include "log.h"
#include "../gles/loader.h"
#include "../includes.h"
#include "glsl/glsl_for_es.h"

#define FUNC_GL_STATE_SIZEI(name) \
static inline void set_gl_state_##name (GLsizei value) { \
    gl_state->name=value; \
    LOG_D(" -> gl_state: %s is %d",#name,value); \
}
#define FUNC_GL_STATE_ENUM(name) \
static inline void set_gl_state_##name (GLenum value) { \
    gl_state->name=value; \
    LOG_D(" -> gl_state: %s is %d",#name,value); \
}

struct hard_ext_s {
    GLint maxsize;
};
typedef struct hard_ext_s* hard_ext_t;
extern hard_ext_t hard_ext;

struct gl_state_s {
    GLsizei proxy_width;
    GLsizei proxy_height;
    GLenum proxy_intformat;
};
typedef struct gl_state_s* gl_state_t;
extern gl_state_t gl_state;
FUNC_GL_STATE_SIZEI(proxy_width)
FUNC_GL_STATE_SIZEI(proxy_height)
FUNC_GL_STATE_ENUM(proxy_intformat)

GLenum pname_convert(GLenum pname);
GLenum map_tex_target(GLenum target);

#endif //MOBILEGLUES_MG_H
