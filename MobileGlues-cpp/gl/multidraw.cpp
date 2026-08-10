// MobileGlues - gl/multidraw.cpp
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

#include "multidraw.h"
#include "../config/settings.h"
#include "buffer.h"
#include "enable.h"
#include "restart.h"
#include "../egl/context.h"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#define DEBUG 0

void prepareForDraw();

// ---------------------------------------------------------------------------
// Diagnostics
//
// LOG_D/LOG_W/LOG_E and the whole CHECK_GL_ERROR family expand to nothing when
// GLOBAL_DEBUG is 0 (gl/log.h), which is the shipping configuration. Every
// fallback and every error path in this file used to be completely invisible.
// MD_WARN_ONCE goes through LOG_W_FORCE, which is unconditional, and keeps a
// per-site latch so a per-draw-call condition cannot flood the log.
// ---------------------------------------------------------------------------
#define MD_WARN_ONCE(...)                                                                                              \
    do {                                                                                                               \
        static bool mg_md_warned = false;                                                                              \
        if (!mg_md_warned) {                                                                                           \
            mg_md_warned = true;                                                                                       \
            LOG_W_FORCE(__VA_ARGS__)                                                                                   \
        }                                                                                                              \
    } while (0)

// Error probe used to drive fallback decisions. CHECK_GL_ERROR is a no-op in
// release builds, so it cannot be used for that.
//
// This only reports the error; the caller logs it through MD_WARN_ONCE. Logging
// here would be per-call and unlatched, and write_log() flushes synchronously,
// so a persistently failing driver would add a disk flush to every draw.
//
// Always pair this with mg_md_drain() immediately before the call being checked:
// otherwise an error left pending by the application would be misread as our own
// failure and push every later draw onto a slower path.
static GLenum mg_md_check() {
    return GLES.glGetError();
}

// Empty the error queue so a following mg_md_check() reports only what the
// bracketed call produced. Nothing observable is lost: this layer already
// answers glGetError from elsewhere, so these entries could never reach the
// application anyway.
static void mg_md_drain() {
    for (int i = 0; i < 16 && GLES.glGetError() != GL_NO_ERROR; ++i) {
    }
}

// ---------------------------------------------------------------------------
// Index helpers
// ---------------------------------------------------------------------------

static inline GLsizei mg_index_size(GLenum type) {
    switch (type) {
    case GL_UNSIGNED_BYTE:
        return 1;
    case GL_UNSIGNED_SHORT:
        return 2;
    case GL_UNSIGNED_INT:
        return 4;
    default:
        return 0;
    }
}


// Rebase indices of any width into a 32-bit output stream.
//
// Widening is mandatory, not an optimisation: GL 4.6 sec. 10.5 evaluates
// (index + basevertex) in the full vertex index space. Writing the sum back
// into the source width truncates it to (index + basevertex) mod 65536 (or
// mod 256), which silently renders wrong geometry for the standard
// "one large vertex buffer + 16-bit indices + large basevertex" layout.
//
// When GL_PRIMITIVE_RESTART_FIXED_INDEX is enabled the sentinel must not be
// offset by basevertex (GL 4.6 sec. 10.3.6 compares the value as read from the
// buffer, before basevertex is added), and it has to be re-emitted as the 32-bit
// sentinel because the output stream is drawn as GL_UNSIGNED_INT.
//
// When restart is disabled there is no sentinel at all: 0xFFFF in a 16-bit index
// buffer is simply vertex 65535, so it must be rebased like any other index.
// Translating it unconditionally would turn a legitimate vertex reference into an
// out-of-range one.
static void mg_rebase_indices_to_u32(GLuint* dst, const void* src, GLsizei count, GLenum type, GLint basevertex,
                                     bool restart_enabled, GLuint sentinel) {
    const GLuint bv = static_cast<GLuint>(basevertex);

#define MG_REBASE_LOOP(SRCTYPE)                                                                                        \
    do {                                                                                                               \
        const SRCTYPE* s = static_cast<const SRCTYPE*>(src);                                                           \
        if (restart_enabled) {                                                                                         \
            for (GLsizei j = 0; j < count; ++j)                                                                        \
                dst[j] = (static_cast<GLuint>(s[j]) == sentinel) ? 0xFFFFFFFFu : (static_cast<GLuint>(s[j]) + bv);     \
        } else {                                                                                                       \
            for (GLsizei j = 0; j < count; ++j)                                                                        \
                dst[j] = static_cast<GLuint>(s[j]) + bv;                                                                \
        }                                                                                                              \
    } while (0)

    switch (type) {
    case GL_UNSIGNED_INT:
        MG_REBASE_LOOP(GLuint);
        break;
    case GL_UNSIGNED_SHORT:
        MG_REBASE_LOOP(GLushort);
        break;
    case GL_UNSIGNED_BYTE:
        MG_REBASE_LOOP(GLubyte);
        break;
    default:
        break;
    }

#undef MG_REBASE_LOOP
}

// Vertices per primitive for the separable modes. 0 means "not separable, do not
// fuse"; is_strip_like_mode covers those already.
static GLsizei mg_verts_per_primitive(GLenum mode) {
    switch (mode) {
    case GL_POINTS:
        return 1;
    case GL_LINES:
        return 2;
    case GL_TRIANGLES:
        return 3;
    case GL_LINES_ADJACENCY:
        return 4;
    case GL_TRIANGLES_ADJACENCY:
        return 6;
    default:
        // GL_PATCHES depends on GL_PATCH_VERTICES, which is not tracked here.
        return 0;
    }
}

// ---------------------------------------------------------------------------
// GL_EXT_multi_draw_arrays
//
// glMultiDrawArraysEXT / glMultiDrawElementsEXT are the exact GLES equivalents of
// the GL 1.4 core commands: one driver call, no command buffer to build, no
// synthesised base vertex array. The GLES loader does not carry them and
// gles/loader.* is not ours to change, so they are resolved here, the same way
// this file already calls eglGetCurrentContext() directly.
//
// A resolved symbol is not proof of support on Android, so the callers probe the
// first call and latch the backend off if the driver rejects it.
//
// Resolution goes through the GLES library handle, NOT eglGetProcAddress: this
// layer's own eglGetProcAddress forwards to glXGetProcAddress, which does
// dlsym(RTLD_DEFAULT, ...) and would hand back MobileGlues' own exported
// glMultiDrawArraysEXT -- an alias of glMultiDrawArrays -- so calling it from
// inside glMultiDrawArrays would recurse until the stack ran out. `gles` is the
// same handle gles/loader.cpp resolves every other entry point from.
// ---------------------------------------------------------------------------

extern "C" void* gles; // defined in gles/loader.cpp

typedef void(GLAPIENTRY* mg_pfn_multi_draw_arrays_ext)(GLenum, const GLint*, const GLsizei*, GLsizei);
typedef void(GLAPIENTRY* mg_pfn_multi_draw_elements_ext)(GLenum, const GLsizei*, GLenum, const void* const*, GLsizei);

static mg_pfn_multi_draw_arrays_ext g_mda_ext = nullptr;
static mg_pfn_multi_draw_elements_ext g_mde_ext = nullptr;

static bool mg_gles_has_extension(const char* name) {
    if (!GLES.glGetStringi || !GLES.glGetIntegerv) return false;
    GLint count = 0;
    GLES.glGetIntegerv(GL_NUM_EXTENSIONS, &count);
    for (GLint i = 0; i < count; ++i) {
        const GLubyte* s = GLES.glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i));
        if (s && strcmp(reinterpret_cast<const char*>(s), name) == 0) return true;
    }
    return false;
}

bool mg_multi_draw_arrays_ext_available() {
    static bool resolved = false;
    if (!resolved) {
        resolved = true;
        // Require the extension string as well as the symbols: a bare dlsym can
        // pick up a platform wrapper stub that reports nothing.
        const bool ext = mg_gles_has_extension("GL_EXT_multi_draw_arrays");
        const bool angle = mg_gles_has_extension("GL_ANGLE_multi_draw");
        // On Apple `gles` is now a real dlopen()'d ANGLE handle (gles/loader.cpp),
        // so resolving through it reaches the backend directly and cannot recurse.
        // The pseudo-handle guard below is kept for the non-Apple dlopen'd path
        // and to be safe against a failed open (NULL).
        const bool real_handle = gles != nullptr && gles != reinterpret_cast<void*>(~(uintptr_t)0);
        if (real_handle && (ext || angle)) {
            const char* arrays_name = ext ? "glMultiDrawArraysEXT" : "glMultiDrawArraysANGLE";
            const char* elements_name = ext ? "glMultiDrawElementsEXT" : "glMultiDrawElementsANGLE";
            g_mda_ext = reinterpret_cast<mg_pfn_multi_draw_arrays_ext>(dlsym(gles, arrays_name));
            g_mde_ext = reinterpret_cast<mg_pfn_multi_draw_elements_ext>(dlsym(gles, elements_name));
        }
        LOG_D("multidraw: multi_draw_arrays ext=%d angle=%d arrays=%p elements=%p", (int)ext, (int)angle,
              (void*)g_mda_ext, (void*)g_mde_ext)
    }
    return g_mda_ext != nullptr || g_mde_ext != nullptr;
}

// glMultiDrawElementsBaseVertexEXT is not part of EXT/OES_draw_elements_base_vertex
// on its own. Both specs define the multi-draw form only when
// GL_EXT_multi_draw_arrays is *also* supported, and a driver that has the base
// vertex extension without it is an ordinary configuration -- Mali r32p1 is one.
//
// Neither of the two things this code used to rely on can see the difference.
// Android's EGL wrapper resolves the symbol whether or not the driver behind it
// implements anything, so a non-null pointer proves nothing; and such a driver
// accepts the call, draws nothing, and raises no error, so the probe-and-latch
// in mg_multi_draw_basevertex latches Working and every sub-draw is silently
// dropped for the rest of the process. Only the extension string can tell.
bool mg_multi_draw_elements_basevertex_ext_available() {
    static bool resolved = false;
    static bool available = false;
    if (!resolved) {
        resolved = true;
        available = GLES.glMultiDrawElementsBaseVertexEXT != nullptr &&
                    (g_gles_caps.GL_EXT_draw_elements_base_vertex || g_gles_caps.GL_OES_draw_elements_base_vertex) &&
                    mg_gles_has_extension("GL_EXT_multi_draw_arrays");
        LOG_D("multidraw: multibasevertex available=%d (ptr=%p bv_ext=%d/%d)", (int)available,
              (void*)GLES.glMultiDrawElementsBaseVertexEXT, g_gles_caps.GL_EXT_draw_elements_base_vertex,
              g_gles_caps.GL_OES_draw_elements_base_vertex)
    }
    return available;
}

static bool is_strip_like_mode(GLenum mode) {
    switch (mode) {
    case GL_LINE_STRIP:
    case GL_LINE_LOOP:
    case GL_TRIANGLE_STRIP:
    case GL_TRIANGLE_FAN:
    case GL_LINE_STRIP_ADJACENCY:
    case GL_TRIANGLE_STRIP_ADJACENCY:
        return true;
    default:
        return false;
    }
}

// ---------------------------------------------------------------------------
// Scratch GL objects
//
// Each name below belongs to whichever context was current when it was created.
// They used to live for the whole process, so once the application destroyed and
// recreated its EGL context the stale names were reused against the new one:
// glBindBuffer would quietly create a fresh zero-byte buffer while the cached
// capacity still claimed it was large enough, the following map returned NULL,
// and the command fill wrote through a null pointer.
// ---------------------------------------------------------------------------

// Identified by the monotonic MGContext id, not by the EGLContext pointer.
// EGLContext is a driver heap allocation, so destroying one and creating another
// very often returns the same address; comparing addresses reported "same
// context" for a context that never owned any of these objects, and the stale
// names were then used against it. 0 means "no tracked context", which is also
// what the bootstrap probe context looks like.
static unsigned long long g_owner_ctx_id = 0;

enum class md_probe_state_t { Unprobed, Working, Failed };

// indirect / multiindirect paths
static bool g_indirect_cmds_inited = false;
static GLsizei g_cmdbufsize = 0;
static GLuint g_indirectbuffer = 0;

// Arrays commands are 16 bytes, element commands 20, and the capacity counters
// below are in commands, not bytes. Sharing one buffer between the two layouts
// would let an Elements call skip its resize because an Arrays call had already
// "grown" it, and then map past the end of the store.
static GLuint g_arrays_indirectbuffer = 0;
static GLsizei g_arrays_cmdbufsize = 0;
static md_probe_state_t g_arrays_mdi_state = md_probe_state_t::Unprobed;
static md_probe_state_t g_arrays_mda_state = md_probe_state_t::Unprobed;

// drawelements path
static GLuint g_scratch_ibo = 0;

// compute path
static bool g_compute_inited = false;
static bool g_compute_failed = false;
static GLuint g_prefixsumbuffer = 0;
static GLuint g_drawcmd_ssbo = 0;
static GLuint g_outputibo = 0;
static GLuint g_compute_program = 0;
static GLint g_element_size_loc = -1;
static GLint g_max_compute_groups_x = 0;

// glMultiDraw*IndirectCount compaction
static GLuint g_count_program = 0;
static GLuint g_count_scratch = 0;
static bool g_count_inited = false;
static bool g_count_failed = false;
static GLint g_count_loc_max = -1, g_count_loc_srcwords = -1, g_count_loc_srcoff = -1, g_count_loc_cntoff = -1,
             g_count_loc_dstwords = -1;

// probe latches for the two extension-provided batched backends.
//
// One tri-state rather than a separate "probed" flag and "failed" flag: those
// were kept in two places, only one of which multidraw_check_context() reset, so
// after a context change the backend was re-enabled but never re-probed -- and an
// unprobed call reports success unconditionally, which loses the whole batch on
// a driver whose entry point is a stub.
static md_probe_state_t g_mdbv_state = md_probe_state_t::Unprobed;
static md_probe_state_t g_mda_state = md_probe_state_t::Unprobed;

// Invalidate every cached GL object name when the current context changes.
//
// Object names are actually shared across a share group rather than tied to one
// context, but EGL exposes no way to query the share group, so this compares
// context identity.
//
// Destroy-and-recreate is handled correctly. Two live contexts used alternately
// are not: each switch drops the other context's still-valid objects, so they
// are rebuilt every time and the abandoned ones are never freed (g_outputibo can
// be several MB). Deleting them here is not possible either, because a name from
// the context being left is not addressable from the one being entered. Making
// that case tidy needs a per-context map; it is not implemented.
static void multidraw_check_context() {
    const unsigned long long cur = g_current_ctx ? g_current_ctx->id : 0;
    if (cur == g_owner_ctx_id) return;

    // Deliberately no glDelete* here: if the owning context is gone its objects
    // went with it, and if it is merely not current then these names refer to
    // objects belonging to whichever context *is* current.
    g_indirect_cmds_inited = false;
    g_cmdbufsize = 0;
    g_indirectbuffer = 0;
    g_arrays_indirectbuffer = 0;
    g_arrays_cmdbufsize = 0;
    g_arrays_mdi_state = md_probe_state_t::Unprobed;
    g_arrays_mda_state = md_probe_state_t::Unprobed;
    g_count_program = 0;
    g_count_scratch = 0;
    g_count_inited = false;
    g_count_failed = false;
    g_count_loc_max = g_count_loc_srcwords = g_count_loc_srcoff = g_count_loc_cntoff = g_count_loc_dstwords = -1;
    g_scratch_ibo = 0;
    // gl/restart.cpp caches a scratch index buffer of its own, created with a
    // real driver name rather than a virtual one, so it has exactly the same
    // cross-context reuse hazard.
    mg_restart_invalidate();
    g_compute_inited = false;
    // The failure latches are per-context capability facts, so they are cleared
    // together with the objects they describe. Clearing a probe latch also has
    // to re-arm its probe, which is why it is a single tri-state.
    g_compute_failed = false;
    g_mdbv_state = md_probe_state_t::Unprobed;
    g_mda_state = md_probe_state_t::Unprobed;
    g_prefixsumbuffer = 0;
    g_drawcmd_ssbo = 0;
    g_outputibo = 0;
    g_compute_program = 0;
    g_element_size_loc = -1;
    g_max_compute_groups_x = 0;

    g_owner_ctx_id = cur;
    LOG_D("multidraw: context changed, scratch objects invalidated")
}

// ---------------------------------------------------------------------------
// Entry validation
//
// GL 4.6 sec. 10.5 requires GL_INVALID_VALUE for a negative count and
// GL_INVALID_ENUM for an unrecognised type. This layer has no way to raise a GL
// error the application can observe, so the achievable goal is consistency:
// every mode must react to the same bad input the same way. Before this, a
// negative count was silently skipped by the unrolled paths, clamped to zero by
// the compute path, and turned into a ~4-billion-index draw by the indirect
// paths.
// ---------------------------------------------------------------------------
static bool mg_validate_multidraw(const GLsizei* counts, GLenum type, GLsizei primcount);

// Shared entry gate for every mode implementation.
//
// This deliberately lives in the mode functions rather than only in the two
// dispatchers: glXGetProcAddress hands the mode-specific mg_* symbol straight to
// the application (glx/lookup.cpp handle_multidraw_func_name), so the dispatcher
// is not always on the call path, and the cached GL objects would then never be
// checked against the current context.
static bool mg_multidraw_enter(const GLsizei* counts, GLenum type, GLsizei primcount, const void* const* indices) {
    if (!mg_validate_multidraw(counts, type, primcount)) return false;
    if (!indices) {
        MD_WARN_ONCE("multidraw: indices == NULL");
        return false;
    }
    multidraw_check_context();
    return true;
}

// ---------------------------------------------------------------------------
// GL_PRIMITIVE_RESTART across a multi-draw
//
// Restart is per-index state, so it applies to every sub-draw of an indexed
// multi-draw exactly as it would to the equivalent loop of single draws. GLES
// only implements the fixed-index form, and nothing here used to account for
// either half of that, so a batch drawn with restart enabled came out with its
// strips joined end to end.
//
// Two cases, matching the two predicates gl/restart.cpp exposes:
//
//   the chosen value is the fixed one -- no rewrite is needed, but GLES still
//     has to be told, because GL_PRIMITIVE_RESTART itself is never forwarded.
//     One enable around the whole batch covers every sub-draw.
//
//   the chosen value is something else -- the index stream has to be rewritten,
//     and mg_glMultiDrawElementsBaseVertex_drawelements is the only backend that
//     rewrites. Every other backend hands the application's indices to the
//     driver untouched, so for the duration of such a draw they defer to it.
//
// Both live in the backends rather than in the dispatchers because
// glXGetProcAddress hands out the mg_* symbols directly.
// ---------------------------------------------------------------------------

namespace {
struct md_restart_scope_t {
    bool forced;
    explicit md_restart_scope_t(GLenum type) : forced(mg_restart_needs_driver_fixed(type)) {
        if (forced) GLES.glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    }
    ~md_restart_scope_t() {
        if (forced) GLES.glDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    }
    md_restart_scope_t(const md_restart_scope_t&) = delete;
    md_restart_scope_t& operator=(const md_restart_scope_t&) = delete;
};
} // namespace

// True when this call was handed to the rewriting backend and the caller is done.
static bool mg_multidraw_restart_takeover(GLenum mode, const GLsizei* counts, GLenum type, const void* const* indices,
                                          GLsizei primcount, const GLint* basevertex) {
    if (!mg_restart_needs_rewrite(type)) return false;
    mg_glMultiDrawElementsBaseVertex_drawelements(mode, const_cast<GLsizei*>(counts), type, indices, primcount,
                                                  basevertex);
    return true;
}

// ---------------------------------------------------------------------------
// Order-driven degradation
//
// Every "this backend cannot serve this call" branch below routes through one
// of the md_fall_* helpers: the next rung is whatever the user ranked after the
// failing backend for that entry point (config/settings.cpp,
// global_settings.multidraw_order), not a hard-coded neighbour. The walk is
// strictly forward through that order, so it terminates; the terminal rung for
// the three list-taking entry points is the unrolled loop, which never falls.
//
// g_md_fallback_tick lets the in-process benchmark (multidraw_bench.cpp) detect
// that a measured call was not actually served by the backend it was aimed at.
// ---------------------------------------------------------------------------

std::atomic<uint32_t> g_md_fallback_tick{0};

static void md_call_elements(md_backend_t b, GLenum mode, const GLsizei* count, GLenum type,
                             const void* const* indices, GLsizei primcount) {
    switch (b) {
    case md_backend_t::Indirect:
        mg_glMultiDrawElements_indirect(mode, count, type, indices, primcount);
        break;
    case md_backend_t::MultiIndirect:
        mg_glMultiDrawElements_multiindirect(mode, count, type, indices, primcount);
        break;
    case md_backend_t::MultiBaseVertex:
        mg_glMultiDrawElements_multibasevertex(mode, count, type, indices, primcount);
        break;
    case md_backend_t::MultiArrays:
        mg_glMultiDrawElements_multiarrays(mode, count, type, indices, primcount);
        break;
    default:
        mg_glMultiDrawElements_drawelements(mode, count, type, indices, primcount);
        break;
    }
}

static void md_fall_elements(md_backend_t cur, GLenum mode, const GLsizei* count, GLenum type,
                             const void* const* indices, GLsizei primcount) {
    g_md_fallback_tick.fetch_add(1, std::memory_order_relaxed);
    md_backend_t next = md_next_backend(md_entry_t::Elements, cur);
    if (next == cur) next = md_backend_t::Unroll; // the last rung failed; the loop always works
    md_call_elements(next, mode, count, type, indices, primcount);
}

static void md_call_elements_bv(md_backend_t b, GLenum mode, GLsizei* counts, GLenum type,
                                const void* const* indices, GLsizei primcount, const GLint* basevertex) {
    switch (b) {
    case md_backend_t::Indirect:
        mg_glMultiDrawElementsBaseVertex_indirect(mode, counts, type, indices, primcount, basevertex);
        break;
    case md_backend_t::BaseVertex:
        mg_glMultiDrawElementsBaseVertex_basevertex(mode, counts, type, indices, primcount, basevertex);
        break;
    case md_backend_t::MultiIndirect:
        mg_glMultiDrawElementsBaseVertex_multiindirect(mode, counts, type, indices, primcount, basevertex);
        break;
    case md_backend_t::MultiBaseVertex:
        mg_glMultiDrawElementsBaseVertex_multibasevertex(mode, counts, type, indices, primcount, basevertex);
        break;
    case md_backend_t::Compute:
        mg_glMultiDrawElementsBaseVertex_compute(mode, counts, type, indices, primcount, basevertex);
        break;
    default:
        mg_glMultiDrawElementsBaseVertex_drawelements(mode, counts, type, indices, primcount, basevertex);
        break;
    }
}

static void md_fall_elements_bv(md_backend_t cur, GLenum mode, GLsizei* counts, GLenum type,
                                const void* const* indices, GLsizei primcount, const GLint* basevertex) {
    g_md_fallback_tick.fetch_add(1, std::memory_order_relaxed);
    md_backend_t next = md_next_backend(md_entry_t::ElementsBaseVertex, cur);
    if (next == cur) next = md_backend_t::Unroll;
    md_call_elements_bv(next, mode, counts, type, indices, primcount, basevertex);
}

static void md_call_arrays(md_backend_t b, GLenum mode, const GLint* first, const GLsizei* count,
                           GLsizei drawcount) {
    switch (b) {
    case md_backend_t::MultiArrays:
        mg_glMultiDrawArrays_multiarrays(mode, first, count, drawcount);
        break;
    case md_backend_t::MultiIndirect:
        mg_glMultiDrawArrays_multiindirect(mode, first, count, drawcount);
        break;
    default:
        mg_glMultiDrawArrays_unroll(mode, first, count, drawcount);
        break;
    }
}

static void md_fall_arrays(md_backend_t cur, GLenum mode, const GLint* first, const GLsizei* count,
                           GLsizei drawcount) {
    g_md_fallback_tick.fetch_add(1, std::memory_order_relaxed);
    md_backend_t next = md_next_backend(md_entry_t::Arrays, cur);
    if (next == cur) next = md_backend_t::Unroll;
    md_call_arrays(next, mode, first, count, drawcount);
}

static bool mg_validate_multidraw(const GLsizei* counts, GLenum type, GLsizei primcount) {
    if (primcount < 0) {
        MD_WARN_ONCE("multidraw: negative primcount %d", primcount);
        return false;
    }
    if (primcount == 0) return false; // legal no-op
    if (!counts) {
        MD_WARN_ONCE("multidraw: counts == NULL");
        return false;
    }
    if (mg_index_size(type) == 0) {
        MD_WARN_ONCE("multidraw: invalid index type 0x%04x", type);
        return false;
    }
    for (GLsizei i = 0; i < primcount; ++i) {
        if (counts[i] < 0) {
            MD_WARN_ONCE("multidraw: negative count at sub-draw %d", i);
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Dispatchers
// ---------------------------------------------------------------------------

typedef void (*glMultiDrawElements_t)(GLenum, const GLsizei*, GLenum, const void* const*, GLsizei);

void glMultiDrawElements(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices,
                         GLsizei primcount) {
    static glMultiDrawElements_t func_ptr = nullptr;

    if (func_ptr == nullptr) {
        switch (multidraw_backend_of(md_entry_t::Elements)) {
        case md_backend_t::Indirect:
            func_ptr = mg_glMultiDrawElements_indirect;
            break;
        case md_backend_t::MultiIndirect:
            func_ptr = mg_glMultiDrawElements_multiindirect;
            break;
        case md_backend_t::MultiBaseVertex:
            func_ptr = mg_glMultiDrawElements_multibasevertex;
            break;
        case md_backend_t::MultiArrays:
            func_ptr = mg_glMultiDrawElements_multiarrays;
            break;
        default:
            // Unroll, and anything the mask should already have rejected.
            func_ptr = mg_glMultiDrawElements_drawelements;
            break;
        }
    }

    // Validation and the context check happen inside the backend implementation,
    // so they also cover the case where glXGetProcAddress handed that symbol to
    // the application directly.
    func_ptr(mode, count, type, indices, primcount);
}

typedef void (*glMultiDrawElementsBaseVertex_t)(GLenum, GLsizei*, GLenum, const void* const*, GLsizei, const GLint*);

void glMultiDrawElementsBaseVertex(GLenum mode, GLsizei* counts, GLenum type, const void* const* indices,
                                   GLsizei primcount, const GLint* basevertex) {
    static glMultiDrawElementsBaseVertex_t func_ptr = nullptr;

    if (func_ptr == nullptr) {
        switch (multidraw_backend_of(md_entry_t::ElementsBaseVertex)) {
        case md_backend_t::Indirect:
            func_ptr = mg_glMultiDrawElementsBaseVertex_indirect;
            break;
        case md_backend_t::BaseVertex:
            func_ptr = mg_glMultiDrawElementsBaseVertex_basevertex;
            break;
        case md_backend_t::MultiIndirect:
            func_ptr = mg_glMultiDrawElementsBaseVertex_multiindirect;
            break;
        case md_backend_t::Compute:
            func_ptr = mg_glMultiDrawElementsBaseVertex_compute;
            break;
        case md_backend_t::MultiBaseVertex:
            func_ptr = mg_glMultiDrawElementsBaseVertex_multibasevertex;
            break;
        default:
            func_ptr = mg_glMultiDrawElementsBaseVertex_drawelements;
            break;
        }
    }

    func_ptr(mode, counts, type, indices, primcount, basevertex);
}

// ---------------------------------------------------------------------------
// Indirect command buffer
// ---------------------------------------------------------------------------

// Returns false when the command buffer could not be prepared; the caller must
// then fall back rather than issue a draw from an unwritten buffer.
static bool prepare_indirect_buffer(const GLsizei* counts, GLenum type, const void* const* indices, GLsizei primcount,
                                    const GLint* basevertex, GLuint* out_prev_binding) {
    if (primcount <= 0) return false;

    const GLsizei elementSize = mg_index_size(type);
    if (elementSize == 0) return false;

    // firstIndex below reads indices[i] as a byte offset into the element array
    // buffer, which is only meaningful when one is bound. GLES also requires a
    // bound element array buffer for indirect draws, so without this check a
    // client-side pointer would be turned into a nonsense firstIndex and the
    // whole batch would disappear without a word. The drawelements and compute
    // paths already made this check.
    //
    // Both bindings come from the tracked state (mg_driver_bound_buffer, gl/buffer.h)
    // rather than from the driver. It answers with the driver-side name, which is
    // what `prev` must be for the restore, and both are read here at the entry of
    // the function, before it binds the command buffer over one of them.
    const GLuint bound_ibo = mg_driver_bound_buffer(GL_ELEMENT_ARRAY_BUFFER);
    if (bound_ibo == 0) {
        MD_WARN_ONCE("multidraw: indirect path needs a bound element array buffer, falling back");
        return false;
    }

    const GLuint prev = mg_driver_bound_buffer(GL_DRAW_INDIRECT_BUFFER);
    *out_prev_binding = prev;

    if (!g_indirect_cmds_inited) {
        GLES.glGenBuffers(1, &g_indirectbuffer);
        g_cmdbufsize = 0;
        g_indirect_cmds_inited = true;
    }
    GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, g_indirectbuffer);

    if (g_cmdbufsize < primcount) {
        size_t sz = g_cmdbufsize > 0 ? static_cast<size_t>(g_cmdbufsize) : 1;
        while (sz < static_cast<size_t>(primcount))
            sz *= 2;

        const size_t wanted = sz * sizeof(draw_elements_indirect_command_t);
        GLES.glBufferData(GL_DRAW_INDIRECT_BUFFER, static_cast<GLsizeiptr>(wanted), NULL, GL_DYNAMIC_DRAW);

        // Only record the new capacity once the store really grew. Assigning it
        // unconditionally made an out-of-memory permanent: the tracked size then
        // claimed more room than the buffer had, the resize branch was never
        // taken again, and every later map ran past the end of the store.
        GLint real_size = 0;
        GLES.glGetBufferParameteriv(GL_DRAW_INDIRECT_BUFFER, GL_BUFFER_SIZE, &real_size);
        if (real_size < 0 || static_cast<size_t>(real_size) < wanted) {
            MD_WARN_ONCE("multidraw: indirect buffer allocation failed (wanted %zu bytes, got %d)", wanted, real_size);
            // The failed glBufferData leaves the store in an undefined state, so
            // the previously recorded capacity no longer describes it. Clearing it
            // forces a fresh allocation attempt next time; keeping the stale value
            // would skip the resize branch forever and make every later map fail.
            g_cmdbufsize = 0;
            GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, prev);
            return false;
        }
        g_cmdbufsize = static_cast<GLsizei>(sz);
    }

    // Built on the CPU and uploaded whole, rather than written through a
    // GL_MAP_INVALIDATE_BUFFER_BIT mapping. The mapping says "all of this is
    // about to be overwritten" more precisely, and on a driver that honours it
    // there is nothing to choose between the two. What it costs is two ways to
    // fail on every single draw call: a map that returns null, and an unmap that
    // reports the contents lost. Both used to abandon the backend for the rest of
    // the process, and falling back to unrolled draws forever is far more
    // expensive than the upload that was being avoided -- a driver refusing to
    // hand out a pointer for a buffer it is still reading is a perfectly ordinary
    // thing, not a reason to give up on indirect drawing.
    //
    // thread_local for the same reason as mg_zero_basevertex: nothing in this
    // file takes a lock, and two threads can each have a current context.
    static thread_local std::vector<draw_elements_indirect_command_t> staged;
    staged.resize(static_cast<size_t>(primcount));
    draw_elements_indirect_command_t* pcmds = staged.data();

    for (GLsizei i = 0; i < primcount; ++i) {
        const uintptr_t byteOffset = reinterpret_cast<uintptr_t>(indices[i]);
        // A negative count assigned straight into the unsigned command field used
        // to become ~4.29e9 indices, i.e. a draw far past the end of every bound
        // buffer.
        const GLsizei c = counts[i] > 0 ? counts[i] : 0;

        pcmds[i].firstIndex = static_cast<GLuint>(byteOffset / static_cast<uintptr_t>(elementSize));
        pcmds[i].count = static_cast<GLuint>(c);
        pcmds[i].instanceCount = 1;
        pcmds[i].baseVertex = basevertex ? basevertex[i] : 0;
        pcmds[i].reservedMustBeZero = 0;
    }

    GLES.glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0,
                         static_cast<GLsizeiptr>(primcount * sizeof(draw_elements_indirect_command_t)), pcmds);
    return true;
}

// ---------------------------------------------------------------------------
// Mode: DrawElements (CPU rebase, no extension required)
// ---------------------------------------------------------------------------

void mg_glMultiDrawElementsBaseVertex_drawelements(GLenum mode, GLsizei* counts, GLenum type,
                                                   const void* const* indices, GLsizei primcount,
                                                   const GLint* basevertex) {
    LOG()
    if (!mg_multidraw_enter(counts, type, primcount, indices)) return;

    // Resolved once for the whole call. It used to be a switch inside the
    // per-sub-draw loop whose default branch `return`ed, abandoning the rest of
    // the multi-draw without a trace.
    const GLsizei indexSize = mg_index_size(type);

    prepareForDraw();

    // Queried once per multi-draw rather than per sub-draw: it decides whether the
    // sentinel value is special or an ordinary vertex index.
    // Read from the virtual table, not the driver: GL_PRIMITIVE_RESTART with a
    // custom index is invisible to GLES, and the driver's fixed-index flag is
    // also toggled behind the application's back by the restart emulation.
    const bool restart_enabled = mg_primitive_restart_enabled();
    const GLuint restart_value = mg_primitive_restart_index_for(type);
    // The rewritten stream carries 0xFFFFFFFF wherever a restart was, and is
    // drawn as GL_UNSIGNED_INT, so the driver's fixed-index restart has to be on
    // for these draws. Without it 0xFFFFFFFF is fetched as vertex 4294967295 and
    // every enabled attribute array is read out of bounds.
    const bool force_fixed = restart_enabled && mg_enable_get(GL_PRIMITIVE_RESTART_FIXED_INDEX, 0) != GL_TRUE;
    if (force_fixed) GLES.glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

    // Tracked rather than queried, and read before the loop below starts swapping
    // the scratch buffer in: mg_driver_bound_buffer answers with the driver-side
    // name, which is what every glBindBuffer here is handed.
    const GLuint prevElementBuffer = mg_driver_bound_buffer(GL_ELEMENT_ARRAY_BUFFER);

    // One persistent scratch buffer instead of glGenBuffers/glDeleteBuffers per
    // sub-draw.
    if (!g_scratch_ibo) GLES.glGenBuffers(1, &g_scratch_ibo);

    // Grown but never shrunk, and thread_local for the same reason `staged` above
    // is. Only the first `count` elements of any one sub-draw are written and
    // uploaded, so what a wider sub-draw left behind is never read; sizing it to
    // each count in turn would zero-fill a range mg_rebase_indices_to_u32
    // overwrites in full immediately after.
    static thread_local std::vector<GLuint> rebased;

    for (GLsizei i = 0; i < primcount; ++i) {
        const GLsizei count = counts[i];
        if (count <= 0) continue;

        const GLint bv = basevertex ? basevertex[i] : 0;

        if (rebased.size() < static_cast<size_t>(count)) rebased.resize(static_cast<size_t>(count));

        if (prevElementBuffer != 0) {
            GLES.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prevElementBuffer);
            void* srcData =
                GLES.glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLintptr>(reinterpret_cast<uintptr_t>(indices[i])),
                                      static_cast<GLsizeiptr>(count) * indexSize, GL_MAP_READ_BIT);
            if (!srcData) {
                // An index buffer created with glBufferStorage is not readable via
                // glMapBufferRange, and this used to drop the sub-draw silently.
                // Let the driver apply the base vertex instead of dropping it.
                MD_WARN_ONCE("multidraw drawelements: element buffer is not mappable for reading, "
                             "using driver base vertex");
                GLES.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prevElementBuffer);
                if (GLES.glDrawElementsBaseVertex) {
                    GLES.glDrawElementsBaseVertex(mode, count, type, indices[i], bv);
                } else if (bv == 0) {
                    // No base vertex to apply, so the unmodified stream is correct.
                    GLES.glDrawElements(mode, count, type, indices[i]);
                } else {
                    // The offset cannot be applied without either a readback or
                    // driver support. Skipping the sub-draw loses geometry, but
                    // drawing it would place it at the wrong vertices, and wrong
                    // geometry is worse than missing geometry.
                    MD_WARN_ONCE("multidraw drawelements: cannot apply base vertex %d, sub-draw skipped", bv);
                }
                continue;
            }
            mg_rebase_indices_to_u32(rebased.data(), srcData, count, type, bv, restart_enabled, restart_value);
            GLES.glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
        } else if (indices[i] != nullptr) {
            mg_rebase_indices_to_u32(rebased.data(), indices[i], count, type, bv, restart_enabled, restart_value);
        } else {
            // No element buffer bound and a null client pointer: there is nothing
            // to read. GL leaves this undefined, and reading it is a segfault at
            // address zero rather than a wrong picture -- which is what it was,
            // reachable from the in-process benchmark the moment borrowing ANGLE
            // started working, because a sub-draw's `indices` there is a buffer
            // offset and offset zero is a null pointer.
            //
            // The binding is what decides which of the two `indices` means, so a
            // zero binding with offset-shaped indices is a caller-side mistake
            // this cannot repair. Say so once and skip: missing geometry beats a
            // crash, and beats reading whatever happens to be at address zero.
            MD_WARN_ONCE("multidraw drawelements: no element buffer bound and indices[%d] is null; "
                         "sub-draw skipped",
                         i);
            continue;
        }

        GLES.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_scratch_ibo);
        GLES.glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(count) * sizeof(GLuint), rebased.data(),
                          GL_STREAM_DRAW);
        // The rebased stream is 32-bit regardless of the source width.
        GLES.glDrawElements(mode, count, GL_UNSIGNED_INT, nullptr);
    }

    if (force_fixed) GLES.glDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    GLES.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prevElementBuffer);

    CHECK_GL_ERROR
}

void mg_glMultiDrawElements_drawelements(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices,
                                         GLsizei primcount) {
    LOG()
    if (!mg_multidraw_enter(count, type, primcount, indices)) return;

    if (mg_multidraw_restart_takeover(mode, count, type, indices, primcount, nullptr)) return;
    md_restart_scope_t restart_scope(type);

    prepareForDraw();

    // GL 4.6 sec. 10.5 defines glMultiDrawElements as exactly this loop; there is
    // no base vertex component on this entry point.
    for (GLsizei i = 0; i < primcount; ++i) {
        const GLsizei c = count[i];
        if (c > 0) {
            GLES.glDrawElements(mode, c, type, indices[i]);
        }
    }

    CHECK_GL_ERROR
}

// ---------------------------------------------------------------------------
// Mode: PreferIndirect (one glDrawElementsIndirect per sub-draw)
// ---------------------------------------------------------------------------

void mg_glMultiDrawElementsBaseVertex_indirect(GLenum mode, GLsizei* counts, GLenum type, const void* const* indices,
                                               GLsizei primcount, const GLint* basevertex) {
    LOG()
    if (!mg_multidraw_enter(counts, type, primcount, indices)) return;

    if (mg_multidraw_restart_takeover(mode, counts, type, indices, primcount, basevertex)) return;
    md_restart_scope_t restart_scope(type);

    if (!GLES.glDrawElementsIndirect) {
        MD_WARN_ONCE("multidraw indirect: unavailable, falling back");
        md_fall_elements_bv(md_backend_t::Indirect, mode, counts, type, indices, primcount, basevertex);
        return;
    }

    prepareForDraw();

    GLuint prevIndirectBuffer = 0;
    if (!prepare_indirect_buffer(counts, type, indices, primcount, basevertex, &prevIndirectBuffer)) {
        md_fall_elements_bv(md_backend_t::Indirect, mode, counts, type, indices, primcount, basevertex);
        return;
    }

    for (GLsizei i = 0; i < primcount; ++i) {
        if (counts[i] <= 0) continue;
        const GLvoid* offset = reinterpret_cast<GLvoid*>(i * sizeof(draw_elements_indirect_command_t));
        GLES.glDrawElementsIndirect(mode, type, offset);
    }

    GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, prevIndirectBuffer);

    CHECK_GL_ERROR
}

void mg_glMultiDrawElements_indirect(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices,
                                     GLsizei primcount) {
    LOG()
    if (!mg_multidraw_enter(count, type, primcount, indices)) return;

    if (mg_multidraw_restart_takeover(mode, count, type, indices, primcount, nullptr)) return;
    md_restart_scope_t restart_scope(type);

    if (!GLES.glDrawElementsIndirect) {
        MD_WARN_ONCE("multidraw indirect: unavailable, falling back");
        md_fall_elements(md_backend_t::Indirect, mode, count, type, indices, primcount);
        return;
    }

    prepareForDraw();

    GLuint prevIndirectBuffer = 0;
    if (!prepare_indirect_buffer(count, type, indices, primcount, nullptr, &prevIndirectBuffer)) {
        md_fall_elements(md_backend_t::Indirect, mode, count, type, indices, primcount);
        return;
    }

    for (GLsizei i = 0; i < primcount; ++i) {
        if (count[i] <= 0) continue;
        const GLvoid* offset = reinterpret_cast<GLvoid*>(i * sizeof(draw_elements_indirect_command_t));
        GLES.glDrawElementsIndirect(mode, type, offset);
    }

    GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, prevIndirectBuffer);
    CHECK_GL_ERROR
}

// ---------------------------------------------------------------------------
// Mode: PreferMultidrawIndirect (single GLES multi-draw call)
// ---------------------------------------------------------------------------

void mg_glMultiDrawElementsBaseVertex_multiindirect(GLenum mode, GLsizei* counts, GLenum type,
                                                    const void* const* indices, GLsizei primcount,
                                                    const GLint* basevertex) {
    LOG()
    if (!mg_multidraw_enter(counts, type, primcount, indices)) return;

    if (mg_multidraw_restart_takeover(mode, counts, type, indices, primcount, basevertex)) return;
    md_restart_scope_t restart_scope(type);

    // Same pair of conditions resolution used (config/settings.cpp): the
    // extension string as well as the entry point. This function is also reached
    // as a fallback from the batched backends, so it cannot assume resolution
    // already vetted it.
    if (!GLES.glMultiDrawElementsIndirectEXT || !g_gles_caps.GL_EXT_multi_draw_indirect) {
        MD_WARN_ONCE("multidraw multiindirect: unavailable, falling back");
        md_fall_elements_bv(md_backend_t::MultiIndirect, mode, counts, type, indices, primcount, basevertex);
        return;
    }

    prepareForDraw();

    GLuint prevIndirectBuffer = 0;
    if (!prepare_indirect_buffer(counts, type, indices, primcount, basevertex, &prevIndirectBuffer)) {
        md_fall_elements_bv(md_backend_t::MultiIndirect, mode, counts, type, indices, primcount, basevertex);
        return;
    }

    GLES.glMultiDrawElementsIndirectEXT(mode, type, 0, primcount, 0);

    GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, prevIndirectBuffer);

    CHECK_GL_ERROR
}

void mg_glMultiDrawElements_multiindirect(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices,
                                          GLsizei primcount) {
    LOG()
    if (!mg_multidraw_enter(count, type, primcount, indices)) return;

    if (mg_multidraw_restart_takeover(mode, count, type, indices, primcount, nullptr)) return;
    md_restart_scope_t restart_scope(type);

    if (!GLES.glMultiDrawElementsIndirectEXT || !g_gles_caps.GL_EXT_multi_draw_indirect) {
        MD_WARN_ONCE("multidraw multiindirect: unavailable, falling back");
        md_fall_elements(md_backend_t::MultiIndirect, mode, count, type, indices, primcount);
        return;
    }

    prepareForDraw();

    GLuint prevIndirectBuffer = 0;
    if (!prepare_indirect_buffer(count, type, indices, primcount, nullptr, &prevIndirectBuffer)) {
        md_fall_elements(md_backend_t::MultiIndirect, mode, count, type, indices, primcount);
        return;
    }

    GLES.glMultiDrawElementsIndirectEXT(mode, type, 0, primcount, 0);

    GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, prevIndirectBuffer);

    CHECK_GL_ERROR
}

// ---------------------------------------------------------------------------
// Mode: PreferBaseVertex (one glDrawElementsBaseVertex per sub-draw)
// ---------------------------------------------------------------------------

void mg_glMultiDrawElementsBaseVertex_basevertex(GLenum mode, GLsizei* counts, GLenum type, const void* const* indices,
                                                 GLsizei primcount, const GLint* basevertex) {
    LOG()
    if (!mg_multidraw_enter(counts, type, primcount, indices)) return;

    if (mg_multidraw_restart_takeover(mode, counts, type, indices, primcount, basevertex)) return;
    md_restart_scope_t restart_scope(type);

    if (!GLES.glDrawElementsBaseVertex) {
        MD_WARN_ONCE("multidraw basevertex: unavailable, falling back");
        md_fall_elements_bv(md_backend_t::BaseVertex, mode, counts, type, indices, primcount, basevertex);
        return;
    }

    prepareForDraw();

    for (GLsizei i = 0; i < primcount; ++i) {
        const GLsizei count = counts[i];
        if (count > 0) {
            LOG_D("GLES.glDrawElementsBaseVertex, mode = %s, count = %d, type = %s, indices[i] = %p, basevertex[i] = "
                  "%d",
                  glEnumToString(mode), count, glEnumToString(type), indices[i], basevertex ? basevertex[i] : 0)
            GLES.glDrawElementsBaseVertex(mode, count, type, indices[i], basevertex ? basevertex[i] : 0);
        }
    }
    CHECK_GL_ERROR
}

void mg_glMultiDrawElements_basevertex(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices,
                                       GLsizei primcount) {
    LOG()
    if (!mg_multidraw_enter(count, type, primcount, indices)) return;

    if (mg_multidraw_restart_takeover(mode, count, type, indices, primcount, nullptr)) return;
    md_restart_scope_t restart_scope(type);

    prepareForDraw();

    // No base vertex component exists on this entry point, so this is the plain
    // spec-defined loop.
    for (GLsizei i = 0; i < primcount; ++i) {
        const GLsizei c = count[i];
        if (c > 0) {
            GLES.glDrawElements(mode, c, type, indices[i]);
        }
    }

    CHECK_GL_ERROR
}

// ---------------------------------------------------------------------------
// Backend: MultiBaseVertex (GL_EXT/OES_draw_elements_base_vertex)
//
// GL_EXT_draw_elements_base_vertex provides glMultiDrawElementsBaseVertexEXT
// with exactly the GL 3.2 core signature. It was already being loaded but had no
// call site anywhere in the tree.
//
// That extension only defines the multi-draw form when EXT_multi_draw_arrays is
// also present, and g_gles_caps does not track that string. A resolved,
// non-null entry point is therefore not proof that the driver implements it --
// on Android dlsym can hand back a wrapper stub. The first call is probed for a
// GL error and the mode is latched off if it fails, so a bad driver costs one
// draw rather than every draw.
// ---------------------------------------------------------------------------

// glMultiDrawElements has no base vertex, but the extension takes a real array
// with no "all zero" shorthand. The contents never change, so the buffer only has
// to grow. It is thread_local because nothing in this file takes a lock and two
// threads can each hold a current context; it holds no GL object name, so unlike
// the scratch buffers it deliberately survives a context change.
static const GLint* mg_zero_basevertex(GLsizei primcount) {
    static thread_local std::vector<GLint> zeros;
    if (zeros.size() < static_cast<size_t>(primcount)) zeros.resize(static_cast<size_t>(primcount), 0);
    return zeros.data();
}

static bool mg_multi_draw_basevertex(GLenum mode, const GLsizei* counts, GLenum type, const void* const* indices,
                                GLsizei primcount, const GLint* basevertex) {
    if (g_mdbv_state == md_probe_state_t::Failed || !mg_multi_draw_elements_basevertex_ext_available()) return false;

    const bool probing = (g_mdbv_state == md_probe_state_t::Unprobed);
    if (probing) mg_md_drain();

    GLES.glMultiDrawElementsBaseVertexEXT(mode, counts, type, indices, primcount,
                                          basevertex ? basevertex : mg_zero_basevertex(primcount));

    if (probing) {
        const GLenum err = mg_md_check();
        if (err != GL_NO_ERROR) {
            MD_WARN_ONCE("multidraw multibasevertex: glMultiDrawElementsBaseVertexEXT failed with 0x%04x, "
                         "disabling it",
                         err);
            g_mdbv_state = md_probe_state_t::Failed;
            return false;
        }
        g_mdbv_state = md_probe_state_t::Working;
    }
    return true;
}

void mg_glMultiDrawElementsBaseVertex_multibasevertex(GLenum mode, GLsizei* counts, GLenum type, const void* const* indices,
                                             GLsizei primcount, const GLint* basevertex) {
    LOG()
    // The latch read just below is per-context, so re-arm it first.
    multidraw_check_context();

    // Hand over before doing any work once this backend is known to be unusable,
    // otherwise every later call would validate and prepare the batch twice. The
    // next rung comes from the user's order, so the degradation lands on
    // whatever they ranked directly below this backend.
    if (g_mdbv_state == md_probe_state_t::Failed || !mg_multi_draw_elements_basevertex_ext_available()) {
        md_fall_elements_bv(md_backend_t::MultiBaseVertex, mode, counts, type, indices, primcount, basevertex);
        return;
    }

    if (!mg_multidraw_enter(counts, type, primcount, indices)) return;

    if (mg_multidraw_restart_takeover(mode, counts, type, indices, primcount, basevertex)) return;
    md_restart_scope_t restart_scope(type);

    prepareForDraw();

    if (!mg_multi_draw_basevertex(mode, counts, type, indices, primcount, basevertex)) {
        // Only reachable on the single call whose probe failed.
        md_fall_elements_bv(md_backend_t::MultiBaseVertex, mode, counts, type, indices, primcount, basevertex);
        return;
    }

    CHECK_GL_ERROR
}

// ---------------------------------------------------------------------------
// Backend: MultiArrays (GL_EXT_multi_draw_arrays / GL_ANGLE_multi_draw)
//
// glMultiDrawElementsEXT has exactly the signature and semantics of the GL 1.4
// core command, so this is one driver call with no command buffer to build and
// no synthesised base vertex array. Same probe-and-latch as MultiBaseVertex,
// for the same reason: a resolved symbol is not proof the driver implements it.
// ---------------------------------------------------------------------------

void mg_glMultiDrawElements_multiarrays(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices,
                                      GLsizei primcount) {
    LOG()
    multidraw_check_context();

    if (g_mda_state == md_probe_state_t::Failed || !g_mde_ext) {
        md_fall_elements(md_backend_t::MultiArrays, mode, count, type, indices, primcount);
        return;
    }

    if (!mg_multidraw_enter(count, type, primcount, indices)) return;

    if (mg_multidraw_restart_takeover(mode, count, type, indices, primcount, nullptr)) return;
    md_restart_scope_t restart_scope(type);

    prepareForDraw();

    const bool probing = (g_mda_state == md_probe_state_t::Unprobed);
    if (probing) mg_md_drain();

    g_mde_ext(mode, count, type, indices, primcount);

    if (probing) {
        const GLenum err = mg_md_check();
        if (err != GL_NO_ERROR) {
            MD_WARN_ONCE("multidraw multiarrays: glMultiDrawElementsEXT failed with 0x%04x, disabling it", err);
            g_mda_state = md_probe_state_t::Failed;
            md_fall_elements(md_backend_t::MultiArrays, mode, count, type, indices, primcount);
            return;
        }
        g_mda_state = md_probe_state_t::Working;
    }

    CHECK_GL_ERROR
}

void mg_glMultiDrawElements_multibasevertex(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices,
                                   GLsizei primcount) {
    LOG()
    // The latch read just below is per-context, so re-arm it first.
    multidraw_check_context();

    if (g_mdbv_state == md_probe_state_t::Failed || !mg_multi_draw_elements_basevertex_ext_available()) {
        md_fall_elements(md_backend_t::MultiBaseVertex, mode, count, type, indices, primcount);
        return;
    }

    if (!mg_multidraw_enter(count, type, primcount, indices)) return;

    if (mg_multidraw_restart_takeover(mode, count, type, indices, primcount, nullptr)) return;
    md_restart_scope_t restart_scope(type);

    prepareForDraw();

    // glMultiDrawElements is glMultiDrawElementsBaseVertex with an all-zero base
    // vertex array, so this entry point gets the same single driver call.
    if (mg_multi_draw_basevertex(mode, count, type, indices, primcount, nullptr)) {
        CHECK_GL_ERROR
        return;
    }

    md_fall_elements(md_backend_t::MultiBaseVertex, mode, count, type, indices, primcount);
}

// ---------------------------------------------------------------------------
// Mode: Compute (fuse every sub-draw into one rebased 32-bit index stream)
// ---------------------------------------------------------------------------

const std::string multidraw_comp_shader =
    R"(#version 310 es

layout(local_size_x = 64) in;

layout(location = 0) uniform uint uElementSize;

layout(std430, binding = 0) readonly buffer Input { uint in_indices[]; };
// .x = firstIndex (element offset of the sub-draw), .y = baseVertex.
// Kept in one block so the shader needs only four shader storage blocks, the
// minimum GLES 3.1 guarantees.
layout(std430, binding = 1) readonly buffer DrawCmd { ivec2 drawCmd[]; };
layout(std430, binding = 2) readonly buffer Prefix { uint prefixSums[]; };
layout(std430, binding = 3) writeonly buffer Output { uint out_indices[]; };

uint read_index(uint elementIndex) {
    if (uElementSize == 4u) {
        return in_indices[elementIndex];
    }
    if (uElementSize == 2u) {
        uint word = in_indices[elementIndex >> 1u];
        uint shift = (elementIndex & 1u) * 16u;
        return (word >> shift) & 0xFFFFu;
    }
    uint word = in_indices[elementIndex >> 2u];
    uint shift = (elementIndex & 3u) * 8u;
    return (word >> shift) & 0xFFu;
}

void main() {
    uint outIdx = gl_GlobalInvocationID.x;
    uint drawCount = uint(prefixSums.length());
    if (drawCount == 0u) {
        return;
    }
    uint total = prefixSums[drawCount - 1u];
    if (outIdx >= total) {
        return;
    }

    int low = 0;
    int high = int(drawCount) - 1;
    while (low < high) {
        int mid = low + (high - low) / 2;
        if (prefixSums[mid] > outIdx) {
            high = mid; // next [low, mid)
        } else {
            low = mid + 1; // next [mid + 1, high)
        }
    }

    uint localIdx = outIdx - ((low == 0) ? 0u : (prefixSums[low - 1]));
    uint inIndex = localIdx + uint(drawCmd[low].x);

    int idx = int(read_index(inIndex));
    out_indices[outIdx] = uint(idx + drawCmd[low].y);
}

)";

static GLuint compile_compute_program(const std::string& src, const char* what) {
    char compile_info[1024] = {};

    auto program = GLES.glCreateProgram();
    GLuint shader = GLES.glCreateShader(GL_COMPUTE_SHADER);
    if (program == 0 || shader == 0) {
        LOG_W_FORCE("multidraw %s: compute shaders are unavailable on this context", what)
        if (shader) GLES.glDeleteShader(shader);
        if (program) GLES.glDeleteProgram(program);
        return 0;
    }

    const char* s[] = {src.c_str()};
    const GLint length[] = {static_cast<GLint>(src.length())};
    GLES.glShaderSource(shader, 1, s, length);
    GLES.glCompileShader(shader);

    int success = 0;
    GLES.glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLES.glGetShaderInfoLog(shader, sizeof(compile_info), NULL, compile_info);
        LOG_W_FORCE("multidraw %s: shader compile error: %s", what, compile_info)
#if DEBUG || GLOBAL_DEBUG
        abort();
#endif
        GLES.glDeleteShader(shader);
        GLES.glDeleteProgram(program);
        return 0;
    }

    GLES.glAttachShader(program, shader);
    GLES.glLinkProgram(program);

    GLES.glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLES.glGetProgramInfoLog(program, sizeof(compile_info), NULL, compile_info);
        LOG_W_FORCE("multidraw %s: program link error: %s", what, compile_info)
#if DEBUG || GLOBAL_DEBUG
        abort();
#endif
        GLES.glDeleteShader(shader);
        GLES.glDeleteProgram(program);
        return 0;
    }

    GLES.glDeleteShader(shader);
    // Deliberately generic: the caller looks up whatever uniforms its own shader
    // declares. This builds more than one program now.
    return program;
}

GLAPI GLAPIENTRY void mg_glMultiDrawElementsBaseVertex_compute(GLenum mode, GLsizei* counts, GLenum type,
                                                               const void* const* indices, GLsizei primcount,
                                                               const GLint* basevertex) {
    LOG()
    if (!mg_multidraw_enter(counts, type, primcount, indices)) return;

    // Latched: without this a context that cannot compile the program used to
    // re-run glCreateShader/glCompileShader/glLinkProgram on every single call.
    if (g_compute_failed) {
        md_fall_elements_bv(md_backend_t::Compute, mode, counts, type, indices, primcount, basevertex);
        return;
    }

    const GLuint elementSize = static_cast<GLuint>(mg_index_size(type));

    if (is_strip_like_mode(mode)) {
        LOG_D("multidraw compute: strip/loop mode, fallback")
        md_fall_elements_bv(md_backend_t::Compute, mode, counts, type, indices, primcount, basevertex);
        return;
    }

    // Every sub-draw is concatenated into one glDrawElements, which loses the rule
    // that each draw command discards the leftover vertices at its end (GL 4.6
    // sec. 10.1). That only stays invisible while every count is a whole number of
    // primitives; otherwise the tail of one sub-draw is joined with the head of the
    // next -- across two different base vertices.
    const GLsizei verts_per_prim = mg_verts_per_primitive(mode);
    if (verts_per_prim == 0) {
        // Unknown or non-separable mode (GL_PATCHES, anything new): fusing is not
        // provably safe, so do not.
        MD_WARN_ONCE("multidraw compute: mode 0x%04x cannot be fused safely, falling back", mode);
        md_fall_elements_bv(md_backend_t::Compute, mode, counts, type, indices, primcount, basevertex);
        return;
    }
    for (GLsizei i = 0; i < primcount; ++i) {
        if (counts[i] % verts_per_prim != 0) {
            MD_WARN_ONCE("multidraw compute: sub-draw count is not a whole number of primitives, falling back");
            md_fall_elements_bv(md_backend_t::Compute, mode, counts, type, indices, primcount, basevertex);
            return;
        }
    }

    // Every sub-draw ends up in a single fused stream, so an 8- or 16-bit restart
    // sentinel would be widened into an ordinary 32-bit value and offset by
    // baseVertex, silently disabling restart. The CPU path handles sentinels.
    if (mg_primitive_restart_enabled()) {
        LOG_D("multidraw compute: primitive restart enabled, fallback")
        md_fall_elements_bv(md_backend_t::Compute, mode, counts, type, indices, primcount, basevertex);
        return;
    }

    if (!g_compute_inited) {
        LOG_D("Initializing multidraw compute pipeline...")
        GLES.glGenBuffers(1, &g_prefixsumbuffer);
        GLES.glGenBuffers(1, &g_drawcmd_ssbo);
        GLES.glGenBuffers(1, &g_outputibo);

        g_compute_program = compile_compute_program(multidraw_comp_shader, "index fusion");
        if (g_compute_program != 0) {
            g_element_size_loc = GLES.glGetUniformLocation(g_compute_program, "uElementSize");
            if (g_element_size_loc < 0) {
                // uElementSize drives read_index()'s 8/16/32-bit unpacking. Without
                // it the uniform stays 0, every index is decoded as if it were
                // 8-bit, and the draw silently renders garbage.
                MD_WARN_ONCE("multidraw compute: uElementSize uniform not found, disabling compute mode");
                GLES.glDeleteProgram(g_compute_program);
                g_compute_program = 0;
            }
        }
        if (g_compute_program == 0) {
            MD_WARN_ONCE("multidraw compute: pipeline init failed, falling back for the rest of this context");
            GLES.glDeleteBuffers(1, &g_prefixsumbuffer);
            GLES.glDeleteBuffers(1, &g_drawcmd_ssbo);
            GLES.glDeleteBuffers(1, &g_outputibo);
            g_prefixsumbuffer = 0;
            g_drawcmd_ssbo = 0;
            g_outputibo = 0;
            g_compute_failed = true;
            md_fall_elements_bv(md_backend_t::Compute, mode, counts, type, indices, primcount, basevertex);
            return;
        }

        g_max_compute_groups_x = 0;
        GLES.glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &g_max_compute_groups_x);
        if (g_max_compute_groups_x <= 0) g_max_compute_groups_x = 65535; // GLES 3.1 guaranteed minimum
        LOG_D("multidraw compute: max work group count x = %d", g_max_compute_groups_x)

        g_compute_inited = true;
    }

    // Tracked rather than queried. This is read at the point the pipeline objects
    // have only just been created and nothing has been bound yet, and the value is
    // both a shader storage source and the binding restored at the very end, so it
    // has to be the driver-side name -- which is what mg_driver_bound_buffer gives.
    const GLuint ibo = mg_driver_bound_buffer(GL_ELEMENT_ARRAY_BUFFER);
    if (ibo == 0) {
        LOG_D("multidraw compute: no element array buffer bound, fallback")
        md_fall_elements_bv(md_backend_t::Compute, mode, counts, type, indices, primcount, basevertex);
        return;
    }
    GLint ibo_size = 0;
    GLES.glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &ibo_size);
    if (ibo_size <= 0) {
        MD_WARN_ONCE("multidraw compute: invalid index buffer size, falling back");
        md_fall_elements_bv(md_backend_t::Compute, mode, counts, type, indices, primcount, basevertex);
        return;
    }
    if (elementSize < 4 && (ibo_size % 4) != 0) {
        MD_WARN_ONCE("multidraw compute: index buffer size is not 4-byte aligned, falling back");
        md_fall_elements_bv(md_backend_t::Compute, mode, counts, type, indices, primcount, basevertex);
        return;
    }

    // The fused stream is drawn as 32-bit indices with a GLsizei count, and one
    // dispatch has to cover it, so bound the total by both limits up front.
    const uint64_t max_total =
        std::min<uint64_t>(static_cast<uint64_t>(std::numeric_limits<GLint>::max()) / sizeof(GLuint),
                           static_cast<uint64_t>(g_max_compute_groups_x) * 64ull);

    std::vector<GLuint> prefix_sum(static_cast<size_t>(primcount));
    std::vector<drawcmd_compute_t> drawcmds(static_cast<size_t>(primcount));

    uint64_t running = 0;
    for (GLsizei i = 0; i < primcount; ++i) {
        const GLsizei c = counts[i] > 0 ? counts[i] : 0;
        running += static_cast<uint64_t>(c);
        if (running > max_total) {
            MD_WARN_ONCE("multidraw compute: fused index count exceeds the dispatch limit, falling back");
            md_fall_elements_bv(md_backend_t::Compute, mode, counts, type, indices, primcount, basevertex);
            return;
        }
        prefix_sum[i] = static_cast<GLuint>(running);

        drawcmds[i].firstIndex = 0;
        drawcmds[i].baseVertex = basevertex ? basevertex[i] : 0;

        if (c > 0) {
            // indices[i] == 0 is a perfectly legal byte offset here: an element
            // array buffer is bound (checked above), so this is never a client
            // pointer. Rejecting it used to push every arena-style batch whose
            // first sub-draw starts at offset 0 onto the slowest path.
            const uint64_t byteOffset = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(indices[i]));
            if ((byteOffset % elementSize) != 0) {
                // Truncating the division here would shift the whole sub-draw onto
                // the wrong indices, which looks plausible but is wrong.
                MD_WARN_ONCE("multidraw compute: misaligned index offset, falling back");
                md_fall_elements_bv(md_backend_t::Compute, mode, counts, type, indices, primcount, basevertex);
                return;
            }
            const uint64_t byteEnd = byteOffset + static_cast<uint64_t>(c) * elementSize;
            if (byteEnd > static_cast<uint64_t>(ibo_size)) {
                MD_WARN_ONCE("multidraw compute: index range out of bounds, falling back");
                md_fall_elements_bv(md_backend_t::Compute, mode, counts, type, indices, primcount, basevertex);
                return;
            }
            const uint64_t elementOffset = byteOffset / elementSize;
            if (elementOffset > static_cast<uint64_t>(std::numeric_limits<GLint>::max())) {
                MD_WARN_ONCE("multidraw compute: index offset overflow, falling back");
                md_fall_elements_bv(md_backend_t::Compute, mode, counts, type, indices, primcount, basevertex);
                return;
            }
            drawcmds[i].firstIndex = static_cast<GLuint>(elementOffset);
        }
    }

    const GLuint total_indices = prefix_sum[static_cast<size_t>(primcount) - 1];
    if (total_indices == 0) return;

    prepareForDraw();

    // Deliberately a driver query, unlike the element array binding above. GL
    // makes glBindBufferBase/Range set the generic binding as well as the indexed
    // one, and gl/buffer.cpp's glBindBufferBase does not record that, so the
    // tracked GL_SHADER_STORAGE_BUFFER slot is stale for any application that uses
    // indexed shader storage bindings at all -- restoring it would clobber the
    // binding rather than put it back.
    GLint prev_ssbo_binding = 0;
    GLES.glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &prev_ssbo_binding);

    // Both stores are verified by query, like the output buffer below. The prefix
    // store especially: the shader derives drawCount from prefixSums.length(), so
    // a short allocation makes every invocation return early, nothing is written,
    // and the draw below would read an uninitialised index buffer.
    auto upload_ssbo = [](GLuint buf, size_t bytes, const void* data, const char* what) -> bool {
        GLES.glBindBuffer(GL_SHADER_STORAGE_BUFFER, buf);
        GLES.glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(bytes), data, GL_DYNAMIC_DRAW);
        GLint got = 0;
        GLES.glGetBufferParameteriv(GL_SHADER_STORAGE_BUFFER, GL_BUFFER_SIZE, &got);
        if (got < 0 || static_cast<size_t>(got) < bytes) {
            LOG_W_FORCE("multidraw compute: %s allocation failed (wanted %zu bytes, got %d)", what, bytes, got)
            return false;
        }
        return true;
    };

    if (!upload_ssbo(g_drawcmd_ssbo, sizeof(drawcmd_compute_t) * static_cast<size_t>(primcount), drawcmds.data(),
                     "draw command buffer") ||
        !upload_ssbo(g_prefixsumbuffer, sizeof(GLuint) * static_cast<size_t>(primcount), prefix_sum.data(),
                     "prefix sum buffer")) {
        GLES.glBindBuffer(GL_SHADER_STORAGE_BUFFER, prev_ssbo_binding);
        md_fall_elements_bv(md_backend_t::Compute, mode, counts, type, indices, primcount, basevertex);
        return;
    }

    // Reallocating rather than updating is also what keeps the previous frame's
    // draw from racing this dispatch (write-after-read); do not turn this into
    // glBufferSubData without adding a fence.
    const size_t output_bytes = sizeof(GLuint) * static_cast<size_t>(total_indices);
    GLES.glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_outputibo);
    GLES.glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(output_bytes), nullptr, GL_DYNAMIC_DRAW);

    // Verify by query rather than by glGetError: the output buffer is about to be
    // both the compute target and the index source, so drawing from a store that
    // was never allocated would render garbage.
    GLint output_size = 0;
    GLES.glGetBufferParameteriv(GL_SHADER_STORAGE_BUFFER, GL_BUFFER_SIZE, &output_size);
    GLES.glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    if (output_size < 0 || static_cast<size_t>(output_size) < output_bytes) {
        MD_WARN_ONCE("multidraw compute: output buffer allocation failed (wanted %zu bytes, got %d), falling back",
                     output_bytes, output_size);
        GLES.glBindBuffer(GL_SHADER_STORAGE_BUFFER, prev_ssbo_binding);
        md_fall_elements_bv(md_backend_t::Compute, mode, counts, type, indices, primcount, basevertex);
        return;
    }

    // Indexed shader storage bindings are context state, not program state.
    GLint prev_ssbo_base[4] = {};
    GLint64 prev_ssbo_start[4] = {};
    GLint64 prev_ssbo_size[4] = {};
    for (int i = 0; i < 4; ++i) {
        GLES.glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, i, &prev_ssbo_base[i]);
        if (GLES.glGetInteger64i_v) {
            GLES.glGetInteger64i_v(GL_SHADER_STORAGE_BUFFER_START, i, &prev_ssbo_start[i]);
            GLES.glGetInteger64i_v(GL_SHADER_STORAGE_BUFFER_SIZE, i, &prev_ssbo_size[i]);
        }
    }

    auto restore_ssbo_bindings = [&]() {
        for (int i = 0; i < 4; ++i) {
            // glBindBufferBase is equivalent to binding the whole buffer, so a
            // sub-range binding has to be restored as a range or it silently
            // widens to the entire buffer.
            if (prev_ssbo_base[i] != 0 && prev_ssbo_size[i] > 0) {
                GLES.glBindBufferRange(GL_SHADER_STORAGE_BUFFER, i, static_cast<GLuint>(prev_ssbo_base[i]),
                                       static_cast<GLintptr>(prev_ssbo_start[i]),
                                       static_cast<GLsizeiptr>(prev_ssbo_size[i]));
            } else {
                GLES.glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, static_cast<GLuint>(prev_ssbo_base[i]));
            }
        }
        GLES.glBindBuffer(GL_SHADER_STORAGE_BUFFER, prev_ssbo_binding);
    };

    GLES.glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ibo);
    GLES.glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, g_drawcmd_ssbo);
    GLES.glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, g_prefixsumbuffer);
    GLES.glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, g_outputibo);

    // Both stay driver queries. gl/gl.cpp's ANGLE depth-clear triangle leaves the
    // driver on program 0 and, the first time it runs in a context, on
    // GL_ARRAY_BUFFER 0, without putting the application's back -- so the tracked
    // program and array buffer can both name something the driver is not holding,
    // and these two values are written back to the driver rather than merely read.
    GLint prev_program = 0;
    GLES.glGetIntegerv(GL_CURRENT_PROGRAM, &prev_program);
    GLint prev_vb = 0;
    GLES.glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prev_vb);

    LOG_D("Using compute program = %d", g_compute_program)
    GLES.glUseProgram(g_compute_program);
    if (g_element_size_loc >= 0) {
        GLES.glUniform1ui(g_element_size_loc, elementSize);
    }

    const uint64_t groups = (static_cast<uint64_t>(total_indices) + 63ull) / 64ull;
    if (groups > static_cast<uint64_t>(g_max_compute_groups_x)) {
        MD_WARN_ONCE("multidraw compute: work group count exceeds the limit, falling back");
        restore_ssbo_bindings();
        GLES.glUseProgram(static_cast<GLuint>(prev_program));
        md_fall_elements_bv(md_backend_t::Compute, mode, counts, type, indices, primcount, basevertex);
        return;
    }

    LOG_D("Dispatch compute")
    mg_md_drain();
    GLES.glDispatchCompute(static_cast<GLuint>(groups), 1, 1);
    const GLenum dispatch_err = mg_md_check();
    if (dispatch_err != GL_NO_ERROR) {
        MD_WARN_ONCE("multidraw compute: glDispatchCompute failed with 0x%04x, disabling compute mode",
                     dispatch_err);
        // A driver that rejects this dispatch will reject the next one too, and
        // reaching this point costs three buffer re-specifications plus the whole
        // binding dance. Latch it so the rest of this context uses the CPU path.
        g_compute_failed = true;
        // The output buffer was just reallocated, so its contents are undefined:
        // drawing from it would render garbage rather than nothing.
        restore_ssbo_bindings();
        GLES.glUseProgram(static_cast<GLuint>(prev_program));
        md_fall_elements_bv(md_backend_t::Compute, mode, counts, type, indices, primcount, basevertex);
        return;
    }

    LOG_D("memory barrier")
    GLES.glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ELEMENT_ARRAY_BARRIER_BIT);

    // Restore the shader storage bindings *before* the application's draw. They
    // are context state, so leaving the scratch buffers on bindings 0..3 hands
    // them to the application's own shader, and a writable block declared there
    // would corrupt the very index buffer this draw is consuming.
    restore_ssbo_bindings();

    LOG_D("draw")
    GLES.glUseProgram(static_cast<GLuint>(prev_program));
    GLES.glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(prev_vb));
    GLES.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_outputibo);
    GLES.glDrawElements(mode, static_cast<GLsizei>(total_indices), GL_UNSIGNED_INT, nullptr);

    GLES.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
}

void mg_glMultiDrawElements_compute(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices,
                                    GLsizei primcount) {
    LOG()
    if (!mg_multidraw_enter(count, type, primcount, indices)) return;

    prepareForDraw();

    // Nothing to rebase without a base vertex component, so the compute pipeline
    // has no work to do on this entry point.
    for (GLsizei i = 0; i < primcount; ++i) {
        const GLsizei c = count[i];
        if (c > 0) {
            GLES.glDrawElements(mode, c, type, indices[i]);
        }
    }

    CHECK_GL_ERROR
}

// ---------------------------------------------------------------------------
// GL 4.6 core multi-draw entry points
//
// These were silent no-op stubs in gl_stub.cpp: exported with default
// visibility, so glXGetProcAddress resolved them, and compiled to an empty body
// in release builds. An application calling them lost its geometry and saw no
// error at all.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// glMultiDrawArrays
//
// lookup.cpp does not mangle this name: there is one exported definition that
// picks its own backend, so the mg_glMultiDrawArrays_* symbols are internal and
// their suffixes do not have to match md_backend_suffix().
// ---------------------------------------------------------------------------

// A negative first matters more than it looks: DrawArraysIndirectCommand::first
// is a GLuint, so it would become a ~4.29e9 vertex offset instead of an error.
static bool mg_validate_multidraw_arrays(const GLint* first, const GLsizei* count, GLsizei drawcount) {
    if (drawcount <= 0) return false;
    if (!first || !count) {
        MD_WARN_ONCE("glMultiDrawArrays: first/count is NULL");
        return false;
    }
    for (GLsizei i = 0; i < drawcount; ++i) {
        if (count[i] < 0) {
            MD_WARN_ONCE("glMultiDrawArrays: negative count at sub-draw %d", i);
            return false;
        }
        if (first[i] < 0) {
            MD_WARN_ONCE("glMultiDrawArrays: negative first at sub-draw %d", i);
            return false;
        }
    }
    return true;
}

void mg_glMultiDrawArrays_unroll(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount) {
    LOG()
    if (!mg_validate_multidraw_arrays(first, count, drawcount)) return;

    prepareForDraw();
    for (GLsizei i = 0; i < drawcount; ++i) {
        if (count[i] > 0) GLES.glDrawArrays(mode, first[i], count[i]);
    }
    CHECK_GL_ERROR
}

void mg_glMultiDrawArrays_multiarrays(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount) {
    LOG()
    // The latch below is per-context, so it has to be re-armed before it is read.
    multidraw_check_context();

    if (g_arrays_mda_state == md_probe_state_t::Failed || !g_mda_ext) {
        md_fall_arrays(md_backend_t::MultiArrays, mode, first, count, drawcount);
        return;
    }
    if (!mg_validate_multidraw_arrays(first, count, drawcount)) return;

    prepareForDraw();

    const bool probing = (g_arrays_mda_state == md_probe_state_t::Unprobed);
    if (probing) mg_md_drain();

    g_mda_ext(mode, first, count, drawcount);

    if (probing) {
        const GLenum err = mg_md_check();
        if (err != GL_NO_ERROR) {
            MD_WARN_ONCE("multidraw multiarrays: glMultiDrawArraysEXT failed with 0x%04x, disabling it", err);
            g_arrays_mda_state = md_probe_state_t::Failed;
            md_fall_arrays(md_backend_t::MultiArrays, mode, first, count, drawcount);
            return;
        }
        g_arrays_mda_state = md_probe_state_t::Working;
    }
    CHECK_GL_ERROR
}

// Folds the batch into one glMultiDrawArraysIndirectEXT by building a command
// buffer. Returns false when the buffer could not be prepared.
static bool prepare_arrays_indirect_buffer(const GLint* first, const GLsizei* count, GLsizei drawcount,
                                           GLuint* out_prev_binding) {
    if (drawcount <= 0) return false;

    // Tracked, and read at entry before the command buffer is bound over it.
    // Nothing outside gl/buffer.cpp ever binds GL_DRAW_INDIRECT_BUFFER except this
    // file, and every one of those binds is restored, so the tracked name and the
    // driver's cannot drift apart for this target.
    const GLuint prev = mg_driver_bound_buffer(GL_DRAW_INDIRECT_BUFFER);
    *out_prev_binding = prev;

    if (!g_arrays_indirectbuffer) {
        GLES.glGenBuffers(1, &g_arrays_indirectbuffer);
        g_arrays_cmdbufsize = 0;
    }
    GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, g_arrays_indirectbuffer);

    if (g_arrays_cmdbufsize < drawcount) {
        size_t sz = g_arrays_cmdbufsize > 0 ? static_cast<size_t>(g_arrays_cmdbufsize) : 1;
        while (sz < static_cast<size_t>(drawcount))
            sz *= 2;

        const size_t wanted = sz * sizeof(draw_arrays_indirect_command_t);
        GLES.glBufferData(GL_DRAW_INDIRECT_BUFFER, static_cast<GLsizeiptr>(wanted), NULL, GL_DYNAMIC_DRAW);

        GLint real_size = 0;
        GLES.glGetBufferParameteriv(GL_DRAW_INDIRECT_BUFFER, GL_BUFFER_SIZE, &real_size);
        if (real_size < 0 || static_cast<size_t>(real_size) < wanted) {
            MD_WARN_ONCE("multidraw arrays: indirect buffer allocation failed (wanted %zu bytes, got %d)", wanted,
                         real_size);
            g_arrays_cmdbufsize = 0;
            GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, prev);
            return false;
        }
        g_arrays_cmdbufsize = static_cast<GLsizei>(sz);
    }

    auto* cmds = static_cast<draw_arrays_indirect_command_t*>(GLES.glMapBufferRange(
        GL_DRAW_INDIRECT_BUFFER, 0, static_cast<GLsizeiptr>(drawcount * sizeof(draw_arrays_indirect_command_t)),
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
    if (!cmds) {
        MD_WARN_ONCE("multidraw arrays: failed to map the indirect command buffer");
        GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, prev);
        return false;
    }

    for (GLsizei i = 0; i < drawcount; ++i) {
        cmds[i].count = static_cast<GLuint>(count[i] > 0 ? count[i] : 0);
        cmds[i].instanceCount = 1;
        cmds[i].first = static_cast<GLuint>(first[i]); // validated non-negative
        cmds[i].baseInstanceOrReserved = 0;
    }

    if (GLES.glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER) == GL_FALSE) {
        MD_WARN_ONCE("multidraw arrays: indirect command buffer contents lost on unmap");
        GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, prev);
        return false;
    }
    return true;
}

void mg_glMultiDrawArrays_multiindirect(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount) {
    LOG()
    multidraw_check_context();

    if (g_arrays_mdi_state == md_probe_state_t::Failed || !GLES.glMultiDrawArraysIndirectEXT) {
        md_fall_arrays(md_backend_t::MultiIndirect, mode, first, count, drawcount);
        return;
    }
    if (!mg_validate_multidraw_arrays(first, count, drawcount)) return;

    // GLES requires a vertex array object for an indirect draw, and requires
    // every enabled array to be buffer-backed. The next rung in the user's order
    // (multiarrays or the unrolled loop) is legal with VAO 0, so this check
    // decides between them rather than reporting an error.
    //
    // Asked of the driver on purpose: the question is whether the *driver* is in a
    // state that permits an indirect draw, and gl/gl.cpp's depth-clear triangle
    // leaves it on vertex array 0 while find_bound_array() still names the
    // application's. Trusting the tracked name there would issue a draw GLES
    // rejects and take the fallback away.
    GLint vao = 0;
    GLES.glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    if (vao == 0) {
        LOG_D("multidraw arrays: no vertex array object bound, falling back")
        md_fall_arrays(md_backend_t::MultiIndirect, mode, first, count, drawcount);
        return;
    }

    // Indirect draws are not allowed while transform feedback is active and not
    // paused; the other backends are.
    {
        GLint tf_active = 0, tf_paused = 0;
        GLES.glGetIntegerv(GL_TRANSFORM_FEEDBACK_ACTIVE, &tf_active);
        GLES.glGetIntegerv(GL_TRANSFORM_FEEDBACK_PAUSED, &tf_paused);
        if (tf_active && !tf_paused) {
            LOG_D("multidraw arrays: transform feedback active, falling back")
            md_fall_arrays(md_backend_t::MultiIndirect, mode, first, count, drawcount);
            return;
        }
    }

    prepareForDraw();

    GLuint prev_indirect = 0;
    if (!prepare_arrays_indirect_buffer(first, count, drawcount, &prev_indirect)) {
        md_fall_arrays(md_backend_t::MultiIndirect, mode, first, count, drawcount);
        return;
    }

    const bool probing = (g_arrays_mdi_state == md_probe_state_t::Unprobed);
    if (probing) mg_md_drain();

    GLES.glMultiDrawArraysIndirectEXT(mode, 0, drawcount, 0);

    if (probing) {
        const GLenum err = mg_md_check();
        if (err != GL_NO_ERROR) {
            MD_WARN_ONCE("multidraw arrays: glMultiDrawArraysIndirectEXT failed with 0x%04x, disabling", err);
            g_arrays_mdi_state = md_probe_state_t::Failed;
            GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, prev_indirect);
            md_fall_arrays(md_backend_t::MultiIndirect, mode, first, count, drawcount);
            return;
        }
        g_arrays_mdi_state = md_probe_state_t::Working;
    }

    GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, prev_indirect);
    CHECK_GL_ERROR
}

typedef void (*glMultiDrawArrays_t)(GLenum, const GLint*, const GLsizei*, GLsizei);

void glMultiDrawArrays(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount) {
    static glMultiDrawArrays_t func_ptr = nullptr;

    if (func_ptr == nullptr) {
        switch (multidraw_backend_of(md_entry_t::Arrays)) {
        case md_backend_t::MultiArrays:
            func_ptr = mg_glMultiDrawArrays_multiarrays;
            break;
        case md_backend_t::MultiIndirect:
            func_ptr = mg_glMultiDrawArrays_multiindirect;
            break;
        default:
            func_ptr = mg_glMultiDrawArrays_unroll;
            break;
        }
    }

    func_ptr(mode, first, count, drawcount);
}

void glMultiDrawArraysIndirect(GLenum mode, const void* indirect, GLsizei drawcount, GLsizei stride) {
    LOG()
    if (drawcount <= 0) return;
    if (stride < 0) {
        MD_WARN_ONCE("glMultiDrawArraysIndirect: negative stride %d", stride);
        return;
    }

    prepareForDraw();

    // The application already supplies the commands, so the only choice is
    // whether to hand the whole batch to the driver or walk it one command at a
    // time. The runtime checks stay as a safety net: resolution only picks
    // multiindirect when it is available, but the entry point is also reachable
    // by direct dlsym.
    const bool want_batch = multidraw_backend_of(md_entry_t::ArraysIndirect) == md_backend_t::MultiIndirect;

    if (want_batch && g_gles_caps.GL_EXT_multi_draw_indirect && GLES.glMultiDrawArraysIndirectEXT) {
        GLES.glMultiDrawArraysIndirectEXT(mode, indirect, drawcount, stride);
    } else if (GLES.glDrawArraysIndirect) {
        // GL 4.6 sec. 10.5: stride 0 means the commands are tightly packed.
        const GLsizei s = stride ? stride : static_cast<GLsizei>(sizeof(draw_arrays_indirect_command_t));
        const uintptr_t base = reinterpret_cast<uintptr_t>(indirect);
        for (GLsizei i = 0; i < drawcount; ++i) {
            GLES.glDrawArraysIndirect(
                mode, reinterpret_cast<const void*>(base + static_cast<uintptr_t>(i) * static_cast<uintptr_t>(s)));
        }
    } else {
        MD_WARN_ONCE("glMultiDrawArraysIndirect: no indirect draw support on this context");
    }
    CHECK_GL_ERROR
}

void glMultiDrawElementsIndirect(GLenum mode, GLenum type, const void* indirect, GLsizei drawcount,
                                 GLsizei stride) {
    LOG()
    if (drawcount <= 0) return;
    if (stride < 0) {
        MD_WARN_ONCE("glMultiDrawElementsIndirect: negative stride %d", stride);
        return;
    }

    prepareForDraw();

    // Indexed, so restart applies here too. The commands live in a GPU buffer and
    // may have been written by the GPU, so the index stream cannot be rewritten
    // on the way past; the fixed-index form is all that can be honoured.
    md_restart_scope_t restart_scope(type);
    if (mg_restart_needs_rewrite(type)) {
        MD_WARN_ONCE("glMultiDrawElementsIndirect: GL_PRIMITIVE_RESTART with a custom index cannot be emulated "
                     "on an indirect draw; restarts will be ignored");
    }

    const bool want_batch = multidraw_backend_of(md_entry_t::ElementsIndirect) == md_backend_t::MultiIndirect;

    if (want_batch && g_gles_caps.GL_EXT_multi_draw_indirect && GLES.glMultiDrawElementsIndirectEXT) {
        GLES.glMultiDrawElementsIndirectEXT(mode, type, indirect, drawcount, stride);
    } else if (GLES.glDrawElementsIndirect) {
        const GLsizei s = stride ? stride : static_cast<GLsizei>(sizeof(draw_elements_indirect_command_t));
        const uintptr_t base = reinterpret_cast<uintptr_t>(indirect);
        for (GLsizei i = 0; i < drawcount; ++i) {
            GLES.glDrawElementsIndirect(
                mode, type,
                reinterpret_cast<const void*>(base + static_cast<uintptr_t>(i) * static_cast<uintptr_t>(s)));
        }
    } else {
        MD_WARN_ONCE("glMultiDrawElementsIndirect: no indirect draw support on this context");
    }
    CHECK_GL_ERROR
}

// ---------------------------------------------------------------------------
// glMultiDraw{Arrays,Elements}IndirectCount
//
// The draw count lives in the buffer bound to GL_PARAMETER_BUFFER, which is the
// whole point of the command: it is normally written by the GPU, so reading it
// back on the CPU would mean a full pipeline stall every frame -- and reading it
// without a stall would mean reading a stale value and drawing the wrong number
// of commands.
//
// Instead the commands are compacted on the GPU. A compute shader copies
// maxdrawcount commands into a scratch buffer and sets instanceCount to 0 on
// every command at or past the real count; a command with instanceCount 0 draws
// nothing. One glMultiDraw*IndirectEXT over the scratch buffer then produces
// exactly the requested draws with no readback and no stall.
//
// Drawing maxdrawcount commands unchanged would NOT be a valid shortcut: the
// slots past the count hold stale or zeroed commands, and rendering them puts
// back the geometry the application just culled.
// ---------------------------------------------------------------------------

static const std::string multidraw_count_shader =
    R"(#version 310 es

layout(local_size_x = 64) in;

layout(location = 0) uniform uint uMaxDrawCount;
layout(location = 1) uniform uint uSrcWords;   // stride in words between source commands
layout(location = 2) uniform uint uSrcOffset;  // byte offset of the first command, in words
layout(location = 3) uniform uint uCountWord;  // byte offset of the count, in words
layout(location = 4) uniform uint uDstWords;   // words per command (4 arrays, 5 elements)

layout(std430, binding = 0) readonly buffer Src { uint src[]; };
layout(std430, binding = 1) readonly buffer Param { uint param[]; };
layout(std430, binding = 2) writeonly buffer Dst { uint dst[]; };

void main() {
    uint i = gl_GlobalInvocationID.x;
    if (i >= uMaxDrawCount) {
        return;
    }

    // Only decides whether this command is kept. The read bounds come from the
    // early return above and from the host-side check that maxdrawcount commands
    // fit inside the source buffer, so a corrupt count cannot widen them.
    uint realCount = param[uCountWord];

    uint sbase = uSrcOffset + i * uSrcWords;
    uint dbase = i * uDstWords;
    for (uint w = 0u; w < uDstWords; ++w) {
        dst[dbase + w] = src[sbase + w];
    }
    if (i >= realCount) {
        dst[dbase + 1u] = 0u; // instanceCount, word 1 of both command layouts
    }
}

)";

static bool mg_count_init() {
    if (g_count_failed) return false;
    if (g_count_inited) return true;

    if (!GLES.glDispatchCompute) {
        MD_WARN_ONCE("multidraw count: compute shaders are unavailable, cannot honour *IndirectCount");
        g_count_failed = true;
        return false;
    }

    g_count_program = compile_compute_program(multidraw_count_shader, "count compaction");
    if (g_count_program == 0) {
        MD_WARN_ONCE("multidraw count: compaction shader failed to build");
        g_count_failed = true;
        return false;
    }
    g_count_loc_max = GLES.glGetUniformLocation(g_count_program, "uMaxDrawCount");
    g_count_loc_srcwords = GLES.glGetUniformLocation(g_count_program, "uSrcWords");
    g_count_loc_srcoff = GLES.glGetUniformLocation(g_count_program, "uSrcOffset");
    g_count_loc_cntoff = GLES.glGetUniformLocation(g_count_program, "uCountWord");
    g_count_loc_dstwords = GLES.glGetUniformLocation(g_count_program, "uDstWords");
    if (g_count_loc_max < 0 || g_count_loc_srcwords < 0 || g_count_loc_srcoff < 0 || g_count_loc_cntoff < 0 ||
        g_count_loc_dstwords < 0) {
        MD_WARN_ONCE("multidraw count: compaction shader is missing uniforms");
        GLES.glDeleteProgram(g_count_program);
        g_count_program = 0;
        g_count_failed = true;
        return false;
    }

    GLES.glGenBuffers(1, &g_count_scratch);
    g_count_inited = true;
    return true;
}

// Returns true when the compacted draw was issued.
static bool mg_indirect_count(GLenum mode, GLenum type, bool is_elements, const void* indirect, GLintptr drawcount,
                              GLsizei maxdrawcount, GLsizei stride) {
    if (maxdrawcount <= 0) return true; // nothing to draw, and that is not an error

    const GLsizei cmd_bytes = is_elements ? static_cast<GLsizei>(sizeof(draw_elements_indirect_command_t))
                                          : static_cast<GLsizei>(sizeof(draw_arrays_indirect_command_t));
    const GLsizei src_stride = stride ? stride : cmd_bytes;
    const uintptr_t src_off = reinterpret_cast<uintptr_t>(indirect);

    if (stride < 0 || (src_stride % 4) != 0 || (src_off % 4) != 0 || (drawcount % 4) != 0 || drawcount < 0) {
        MD_WARN_ONCE("multidraw count: stride/offsets must be non-negative multiples of 4");
        return false;
    }
    if (src_stride < cmd_bytes) {
        MD_WARN_ONCE("multidraw count: stride %d is smaller than one command", src_stride);
        return false;
    }

    multidraw_check_context();
    if (!mg_count_init()) return false;

    const GLuint param_real = find_real_buffer(find_bound_buffer(GL_PARAMETER_BUFFER_BINDING));
    if (param_real == 0) {
        MD_WARN_ONCE("multidraw count: no GL_PARAMETER_BUFFER bound, nothing drawn");
        return false;
    }
    // Tracked, like the parameter buffer above: this ends up as a shader storage
    // source and as a bind target, so it has to be the driver-side name, and
    // nothing has been bound yet at this point.
    const GLuint src_bound = mg_driver_bound_buffer(GL_DRAW_INDIRECT_BUFFER);
    if (src_bound == 0) {
        MD_WARN_ONCE("multidraw count: no GL_DRAW_INDIRECT_BUFFER bound, nothing drawn");
        return false;
    }

    // Save everything the dispatch is about to take over BEFORE touching any of
    // it. Reading GL_SHADER_STORAGE_BUFFER_BINDING after the scratch sizing block
    // had already bound and unbound the scratch buffer would have recorded 0 and
    // then "restored" that, silently clearing the application's binding.
    //
    // The first two stay driver queries for the reasons given in the compute path:
    // the tracked generic shader storage binding does not see glBindBufferBase, and
    // the tracked current program does not see gl/gl.cpp's depth-clear triangle.
    GLint prev_ssbo = 0, prev_program = 0;
    GLES.glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &prev_ssbo);
    GLES.glGetIntegerv(GL_CURRENT_PROGRAM, &prev_program);
    const GLuint prev_indirect = mg_driver_bound_buffer(GL_DRAW_INDIRECT_BUFFER);
    GLint prev_base[3] = {};
    GLint64 prev_start[3] = {}, prev_size[3] = {};
    for (int i = 0; i < 3; ++i) {
        GLES.glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, i, &prev_base[i]);
        if (GLES.glGetInteger64i_v) {
            GLES.glGetInteger64i_v(GL_SHADER_STORAGE_BUFFER_START, i, &prev_start[i]);
            GLES.glGetInteger64i_v(GL_SHADER_STORAGE_BUFFER_SIZE, i, &prev_size[i]);
        }
    }
    auto restore_ssbo = [&]() {
        for (int i = 0; i < 3; ++i) {
            if (prev_base[i] != 0 && prev_size[i] > 0)
                GLES.glBindBufferRange(GL_SHADER_STORAGE_BUFFER, i, static_cast<GLuint>(prev_base[i]),
                                       static_cast<GLintptr>(prev_start[i]), static_cast<GLsizeiptr>(prev_size[i]));
            else
                GLES.glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, static_cast<GLuint>(prev_base[i]));
        }
        GLES.glBindBuffer(GL_SHADER_STORAGE_BUFFER, static_cast<GLuint>(prev_ssbo));
    };

    // Every source command must be inside the bound indirect buffer, and the
    // count must be inside the parameter buffer. The shader indexes both as raw
    // uint arrays, and an out-of-range SSBO access is undefined in GLES, not an
    // error. maxdrawcount is allowed to exceed the number of commands actually
    // stored, so this cannot be assumed.
    const uint64_t src_span = static_cast<uint64_t>(src_off) +
                              static_cast<uint64_t>(maxdrawcount - 1) * static_cast<uint64_t>(src_stride) +
                              static_cast<uint64_t>(cmd_bytes);
    GLint src_size = 0;
    GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, src_bound);
    GLES.glGetBufferParameteriv(GL_DRAW_INDIRECT_BUFFER, GL_BUFFER_SIZE, &src_size);
    if (src_size < 0 || src_span > static_cast<uint64_t>(src_size)) {
        MD_WARN_ONCE("multidraw count: commands run past the end of the indirect buffer (%llu > %d), nothing drawn",
                     static_cast<unsigned long long>(src_span), src_size);
        return false;
    }

    GLint param_size = 0;
    GLES.glBindBuffer(GL_SHADER_STORAGE_BUFFER, param_real);
    GLES.glGetBufferParameteriv(GL_SHADER_STORAGE_BUFFER, GL_BUFFER_SIZE, &param_size);
    if (param_size < 0 || static_cast<uint64_t>(drawcount) + 4ull > static_cast<uint64_t>(param_size)) {
        MD_WARN_ONCE("multidraw count: the draw count lies past the end of the parameter buffer, nothing drawn");
        GLES.glBindBuffer(GL_SHADER_STORAGE_BUFFER, static_cast<GLuint>(prev_ssbo));
        return false;
    }

    // Accounted in size_t: truncating to GLsizei could wrap negative and make the
    // comparison skip the allocation entirely.
    const size_t dst_bytes = static_cast<size_t>(maxdrawcount) * static_cast<size_t>(cmd_bytes);
    if (dst_bytes > static_cast<size_t>(std::numeric_limits<GLint>::max())) {
        MD_WARN_ONCE("multidraw count: %zu bytes of commands is too large, nothing drawn", dst_bytes);
        GLES.glBindBuffer(GL_SHADER_STORAGE_BUFFER, static_cast<GLuint>(prev_ssbo));
        return false;
    }

    // Re-specified unconditionally rather than reused. That is what keeps the
    // previous frame's draw, which may still be fetching commands from this
    // buffer, from racing this dispatch's writes -- the same invariant the index
    // fusion path relies on. Do not turn this into a size check plus reuse
    // without adding a fence.
    GLES.glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_count_scratch);
    GLES.glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(dst_bytes), nullptr, GL_DYNAMIC_DRAW);
    GLint got = 0;
    GLES.glGetBufferParameteriv(GL_SHADER_STORAGE_BUFFER, GL_BUFFER_SIZE, &got);
    if (got < 0 || static_cast<size_t>(got) < dst_bytes) {
        MD_WARN_ONCE("multidraw count: scratch allocation failed (wanted %zu bytes, got %d)", dst_bytes, got);
        GLES.glBindBuffer(GL_SHADER_STORAGE_BUFFER, static_cast<GLuint>(prev_ssbo));
        return false;
    }
    GLES.glBindBuffer(GL_SHADER_STORAGE_BUFFER, static_cast<GLuint>(prev_ssbo));

    GLES.glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, src_bound);
    GLES.glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, param_real);
    GLES.glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, g_count_scratch);

    GLES.glUseProgram(g_count_program);
    GLES.glUniform1ui(g_count_loc_max, static_cast<GLuint>(maxdrawcount));
    GLES.glUniform1ui(g_count_loc_srcwords, static_cast<GLuint>(src_stride / 4));
    GLES.glUniform1ui(g_count_loc_srcoff, static_cast<GLuint>(src_off / 4));
    GLES.glUniform1ui(g_count_loc_cntoff, static_cast<GLuint>(drawcount / 4));
    GLES.glUniform1ui(g_count_loc_dstwords, static_cast<GLuint>(cmd_bytes / 4));

    mg_md_drain();
    GLES.glDispatchCompute(static_cast<GLuint>((maxdrawcount + 63) / 64), 1, 1);
    const GLenum err = mg_md_check();
    if (err != GL_NO_ERROR) {
        MD_WARN_ONCE("multidraw count: compaction dispatch failed with 0x%04x, nothing drawn", err);
        restore_ssbo();
        GLES.glUseProgram(static_cast<GLuint>(prev_program));
        return false;
    }

    GLES.glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);

    restore_ssbo();
    GLES.glUseProgram(static_cast<GLuint>(prev_program));

    prepareForDraw();
    GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, g_count_scratch);

    const bool have_mdi = g_gles_caps.GL_EXT_multi_draw_indirect &&
                          (is_elements ? GLES.glMultiDrawElementsIndirectEXT != nullptr
                                       : GLES.glMultiDrawArraysIndirectEXT != nullptr);
    if (have_mdi) {
        if (is_elements)
            GLES.glMultiDrawElementsIndirectEXT(mode, type, 0, maxdrawcount, 0);
        else
            GLES.glMultiDrawArraysIndirectEXT(mode, 0, maxdrawcount, 0);
    } else if (is_elements ? GLES.glDrawElementsIndirect != nullptr : GLES.glDrawArraysIndirect != nullptr) {
        // Commands past the count carry instanceCount 0, so walking all of them
        // draws exactly the same thing, one call at a time.
        for (GLsizei i = 0; i < maxdrawcount; ++i) {
            const void* off = reinterpret_cast<const void*>(static_cast<uintptr_t>(i) * cmd_bytes);
            if (is_elements)
                GLES.glDrawElementsIndirect(mode, type, off);
            else
                GLES.glDrawArraysIndirect(mode, off);
        }
    } else {
        MD_WARN_ONCE("multidraw count: no indirect draw entry point, nothing drawn");
        GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, prev_indirect);
        return false;
    }

    GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, prev_indirect);
    return true;
}

void glMultiDrawArraysIndirectCount(GLenum mode, const void* indirect, GLintptr drawcount, GLsizei maxdrawcount,
                                    GLsizei stride) {
    LOG()
    mg_indirect_count(mode, 0, false, indirect, drawcount, maxdrawcount, stride);
}

void glMultiDrawElementsIndirectCount(GLenum mode, GLenum type, const void* indirect, GLintptr drawcount,
                                      GLsizei maxdrawcount, GLsizei stride) {
    LOG()
    // Same as glMultiDrawElementsIndirect: indexed, so restart applies, but the
    // stream is not reachable for rewriting.
    md_restart_scope_t restart_scope(type);
    if (mg_restart_needs_rewrite(type)) {
        MD_WARN_ONCE("glMultiDrawElementsIndirectCount: GL_PRIMITIVE_RESTART with a custom index cannot be emulated "
                     "on an indirect draw; restarts will be ignored");
    }
    mg_indirect_count(mode, type, true, indirect, drawcount, maxdrawcount, stride);
}

#ifndef __APPLE__
extern "C"
{
    GLAPI GLAPIENTRY void glMultiDrawArraysEXT(GLenum mode, const GLint* first, const GLsizei* count, GLsizei primcount)
        __attribute__((alias("glMultiDrawArrays")));
    GLAPI GLAPIENTRY void glMultiDrawElementsEXT(GLenum mode, const GLsizei* count, GLenum type,
                                                 const void* const* indices, GLsizei primcount)
        __attribute__((alias("glMultiDrawElements")));
    GLAPI GLAPIENTRY void glMultiDrawArraysIndirectARB(GLenum mode, const void* indirect, GLsizei drawcount,
                                                       GLsizei stride)
        __attribute__((alias("glMultiDrawArraysIndirect")));
    GLAPI GLAPIENTRY void glMultiDrawElementsIndirectARB(GLenum mode, GLenum type, const void* indirect,
                                                         GLsizei drawcount, GLsizei stride)
        __attribute__((alias("glMultiDrawElementsIndirect")));
    GLAPI GLAPIENTRY void glMultiDrawArraysIndirectCountARB(GLenum mode, const void* indirect, GLintptr drawcount,
                                                            GLsizei maxdrawcount, GLsizei stride)
        __attribute__((alias("glMultiDrawArraysIndirectCount")));
    GLAPI GLAPIENTRY void glMultiDrawElementsIndirectCountARB(GLenum mode, GLenum type, const void* indirect,
                                                              GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride)
        __attribute__((alias("glMultiDrawElementsIndirectCount")));
}
#else
// Mach-O does not support alias attributes across translation units the way ELF
// does, so the same six names are provided as forwarding definitions. They must
// exist here: the gl_stub.cpp entries they replace were removed unconditionally,
// and without these the Apple build would export nothing at all for them and
// glXGetProcAddress would start returning nullptr where it used to return a stub.
extern "C"
{
    GLAPI GLAPIENTRY void glMultiDrawArraysEXT(GLenum mode, const GLint* first, const GLsizei* count,
                                               GLsizei primcount) {
        glMultiDrawArrays(mode, first, count, primcount);
    }
    GLAPI GLAPIENTRY void glMultiDrawElementsEXT(GLenum mode, const GLsizei* count, GLenum type,
                                                 const void* const* indices, GLsizei primcount) {
        glMultiDrawElements(mode, count, type, indices, primcount);
    }
    GLAPI GLAPIENTRY void glMultiDrawArraysIndirectARB(GLenum mode, const void* indirect, GLsizei drawcount,
                                                       GLsizei stride) {
        glMultiDrawArraysIndirect(mode, indirect, drawcount, stride);
    }
    GLAPI GLAPIENTRY void glMultiDrawElementsIndirectARB(GLenum mode, GLenum type, const void* indirect,
                                                         GLsizei drawcount, GLsizei stride) {
        glMultiDrawElementsIndirect(mode, type, indirect, drawcount, stride);
    }
    GLAPI GLAPIENTRY void glMultiDrawArraysIndirectCountARB(GLenum mode, const void* indirect, GLintptr drawcount,
                                                            GLsizei maxdrawcount, GLsizei stride) {
        glMultiDrawArraysIndirectCount(mode, indirect, drawcount, maxdrawcount, stride);
    }
    GLAPI GLAPIENTRY void glMultiDrawElementsIndirectCountARB(GLenum mode, GLenum type, const void* indirect,
                                                              GLintptr drawcount, GLsizei maxdrawcount,
                                                              GLsizei stride) {
        glMultiDrawElementsIndirectCount(mode, type, indirect, drawcount, maxdrawcount, stride);
    }
}
#endif
