// MobileGlues - gl/mg.h
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

#ifndef MOBILEGLUES_MG_H
#define MOBILEGLUES_MG_H

typedef unsigned int uint;

#include <cstring>
#include <cstdlib>

#ifndef __APPLE__
#include <malloc.h>
#endif

#ifdef __ANDROID__
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
extern "C"
{
#endif

#define FUNC_GL_STATE_SIZEI(name)                                                                                      \
    void set_gl_state_##name(GLsizei value) {                                                                          \
        gl_state->name = value;                                                                                        \
        LOG_D(" -> gl_state: %s is %d", #name, value);                                                                 \
    }
#define FUNC_GL_STATE_ENUM(name)                                                                                       \
    void set_gl_state_##name(GLenum value) {                                                                           \
        gl_state->name = value;                                                                                        \
        LOG_D(" -> gl_state: %s is %d", #name, value);                                                                 \
    }
#define FUNC_GL_STATE_UINT(name)                                                                                       \
    void set_gl_state_##name(GLuint value) {                                                                           \
        gl_state->name = value;                                                                                        \
        LOG_D(" -> gl_state: %s is %d", #name, value);                                                                 \
    }
#define FUNC_GL_STATE_SIZEI_DECLARATION(name) void set_gl_state_##name(GLsizei value);
#define FUNC_GL_STATE_ENUM_DECLARATION(name) void set_gl_state_##name(GLenum value);
#define FUNC_GL_STATE_UINT_DECLARATION(name) void set_gl_state_##name(GLuint value);

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
    typedef struct hardware_s* hardware_t;
    extern hardware_t hardware;

    struct gl_state_s {
        GLsizei proxy_width;
        GLsizei proxy_height;
        GLenum proxy_intformat;

        GLuint current_program;
        GLuint current_tex_unit;
        GLuint current_draw_fbo;

        // The pixel-store parameters desktop GL has and GLES does not.
        //
        // GLES answers GL_INVALID_ENUM for all six, in both directions, so before
        // this they could neither be set nor read: glGetIntegerv left the
        // application's variable exactly as it found it, which for the usual
        // stack local is whatever happened to be there. State that cannot be read
        // back is not state. Kept here so it is per context, like the rest of the
        // pixel-store block the driver owns.
        //
        // Only the two SWAP_BYTES are acted on (gl/transfer.cpp, on the paths that
        // already repack on the CPU). LSB_FIRST orders bits within a byte for
        // GL_BITMAP and colour-index transfers, neither of which exists in a core
        // profile; PACK_IMAGE_HEIGHT and PACK_SKIP_IMAGES describe a
        // three-dimensional readback this layer does not implement.
        GLint unpack_swap_bytes;
        GLint unpack_lsb_first;
        GLint pack_swap_bytes;
        GLint pack_lsb_first;
        GLint pack_image_height;
        GLint pack_skip_images;
    };
    typedef struct gl_state_s* gl_state_t;
    // Where gl_state points when no tracked context is current. Every member is a
    // scalar, so this is constant-initialised and is already usable while the
    // library's own constructors run.
    extern gl_state_s g_default_gl_state;
    // thread_local: EGL scopes the current context per thread, so two threads
    // each holding a context must not share this pointer.
    //
    // It is initialised to the fallback rather than left null. Of the per-context
    // pointers this layer swaps on eglMakeCurrent -- the buffer, texture,
    // framebuffer and FSR1 ones -- this was the only one that started as a bare
    // null, and the fallback was only ever installed from inside
    // mg_context_make_current. A thread whose current context this layer never saw
    // created therefore reached glUseProgram with a null gl_state and dereferenced
    // address zero.
    extern thread_local gl_state_t gl_state;

    // Raise one of this layer's own GL errors, latched until the next glGetError.
    //
    // A call this layer rejects by itself -- a readback target it cannot emulate,
    // a texture name it has no record of, a buffer size that would overflow --
    // never reaches the driver, so there is no backend error for glGetError to
    // find. Those paths used to just return, handing the application back an
    // untouched buffer and GL_NO_ERROR with no way to tell the two apart. This is
    // where they say what went wrong instead.
    //
    // One slot per thread and the first error wins, which is what GL 4.6 sec 2.3.1
    // asks for. Per thread rather than per context because several of these paths
    // run with no context current at all.
    void mg_set_gl_error(GLenum error);

    GLenum pname_convert(GLenum pname);
    GLenum map_tex_target(GLenum target);
    void start_log();
    void write_log(const char* format, ...);
    void write_log_n(const char* format, ...);
    void clear_log();

#ifdef __cplusplus
}
#endif

// Built by prepareForDraw() before an entry point hands off to the driver
// and destroyed when it returns, so the depth-completeness rewrite spans
// exactly the draws it must (multidraw loops included). The rule being
// patched around is in gl/depth_filter.h; the scan that fills this runs in
// gl/texture.cpp. Move-only: the destructor owns driver state.
struct mg_depth_draw_guard {
    // Same bound as MAX_TEXTURE_IMAGE_UNITS in gl/texture.cpp; the scan
    // fills at most mg_max_texture_units() entries.
    static constexpr int kMaxUnits = 128;
    struct Moved {
        int unit;
        GLuint sampler;
        // -1 while nothing on the texture object was rewritten,
        // otherwise its GL_TEXTURE_COMPARE_MODE, restored on destruction.
        int saved_compare;
    };
    Moved moved[kMaxUnits] = {};
    int moved_count = 0;

    mg_depth_draw_guard();
    ~mg_depth_draw_guard();
    mg_depth_draw_guard(const mg_depth_draw_guard&) = delete;
    mg_depth_draw_guard& operator=(const mg_depth_draw_guard&) = delete;
};

mg_depth_draw_guard prepareForDraw();

#endif // MOBILEGLUES_MG_H
