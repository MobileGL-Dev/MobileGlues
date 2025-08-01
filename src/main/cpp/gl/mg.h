//
// Created by BZLZHH on 2025/1/27.
//

#ifndef MOBILEGLUES_MG_H
#define MOBILEGLUES_MG_H

typedef unsigned int uint;

#include <cstring>
#include <cstdlib>

#ifndef __APPLE__
#include <malloc.h>
#include <android/log.h>
#endif

#include <GL/gl.h>
#include "../gles/gles.h"
#include "log.h"
#include "../gles/loader.h"
#include "../includes.h"
#include "glsl/glsl_for_es.h"
#include "../config/config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FUNC_GL_STATE_SIZEI(name) \
void set_gl_state_##name (GLsizei value) { \
    gl_state->name=value; \
    LOG_D(" -> gl_state: %s is %d",#name,value); \
}
#define FUNC_GL_STATE_ENUM(name) \
void set_gl_state_##name (GLenum value) { \
    gl_state->name=value; \
    LOG_D(" -> gl_state: %s is %d",#name,value); \
}
#define FUNC_GL_STATE_UINT(name) \
void set_gl_state_##name (GLuint value) { \
    gl_state->name=value; \
    LOG_D(" -> gl_state: %s is %d",#name,value); \
}
#define FUNC_GL_STATE_SIZEI_DECLARATION(name) void set_gl_state_##name (GLsizei value);
#define FUNC_GL_STATE_ENUM_DECLARATION(name) void set_gl_state_##name (GLenum value);
#define FUNC_GL_STATE_UINT_DECLARATION(name) void set_gl_state_##name (GLuint value);

FUNC_GL_STATE_SIZEI_DECLARATION(proxy_width)
FUNC_GL_STATE_SIZEI_DECLARATION(proxy_height)
FUNC_GL_STATE_ENUM_DECLARATION(proxy_intformat)
FUNC_GL_STATE_UINT_DECLARATION(current_program)
FUNC_GL_STATE_UINT_DECLARATION(current_tex_unit)
FUNC_GL_STATE_UINT_DECLARATION(current_draw_fbo)

struct hardware_s {
    unsigned int es_version;
    bool emulate_texture_buffer;
};
typedef struct hardware_s *hardware_t;
extern hardware_t hardware;

struct gl_state_s {
    GLsizei proxy_width;
    GLsizei proxy_height;
    GLenum proxy_intformat;
    
	GLuint current_program;
	GLuint current_tex_unit;
	GLuint current_draw_fbo;
};
typedef struct gl_state_s *gl_state_t;
extern gl_state_t gl_state;

GLenum pname_convert(GLenum pname);
GLenum map_tex_target(GLenum target);
void start_log();
void write_log(const char *format, ...);
void write_log_n(const char *format, ...);
void clear_log();

#ifdef __cplusplus
}
#endif

void prepareForDraw();

#endif //MOBILEGLUES_MG_H
