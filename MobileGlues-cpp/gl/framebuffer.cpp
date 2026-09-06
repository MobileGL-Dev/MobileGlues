// MobileGlues - gl/framebuffer.cpp
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

#include "framebuffer.h"
#include "../egl/context.h"
#include <mutex>
#include <memory>
#include <ska/flat_hash_map.hpp>
#include "log.h"
#include "../config/settings.h"
#include "FSR1/FSR1.h"

#define DEBUG 0

// One line per site, not one per call. LOG_W_FORCE is unconditional -- that is
// what it is for, LOG_W compiles out in release -- and each line costs an
// __android_log_print, a printf and an fflush'd write to the log file. The only
// site is in glDrawBuffers, whose condition is a property of the framebuffer's
// attachment layout: it holds for as long as the application keeps that layout,
// so it fires on every pass of every frame and says nothing new after the first.
#define FB_WARN_ONCE(...)                                                                                              \
    do {                                                                                                               \
        static bool mg_fb_warned = false;                                                                              \
        if (!mg_fb_warned) {                                                                                           \
            mg_fb_warned = true;                                                                                       \
            LOG_W_FORCE(__VA_ARGS__)                                                                                   \
        }                                                                                                              \
    } while (0)

static GLint MAX_COLOR_ATTACHMENTS = 0;
static GLint MAX_DRAW_BUFFERS = 0;
// Framebuffer objects are container state: GL does not share them across a share
// group, so they belong to one context. See gl/buffer.cpp for the pattern.
//
// The two current bindings live in this record rather than at file scope. They
// used to be process-global while the table they index was already per-context,
// so a context switch left a name that was only valid in the old context
// indexing the new context's table -- and every index site in this file is an
// unchecked operator[]. The new table is freshly default-constructed, so the
// read landed either on a null data() or, worse, inside the reserved capacity of
// g_fbo_default, which yields an unconstructed framebuffer_t whose
// color_attachments is an uninitialised pointer that update_attachment writes
// twelve bytes through.
namespace {
// Keyed by name rather than indexed by it. The vector this replaces was resized
// to id + 10 on every miss, so one framebuffer name out of the usual small
// sequential run cost a record for every name below it -- and framebuffer_t
// carries two vectors now, which made that worse. The records are held by
// pointer: the map moves its elements when it grows, and several functions
// here hold a framebuffer_t& across calls that can insert another name.
struct fbo_ctx_state_t {
    ska::flat_hash_map<GLuint, std::unique_ptr<framebuffer_t>> table;
    GLuint draw = 0;
    GLuint read = 0;
};
std::mutex g_fbo_mutex;
// By pointer for the same reason: g_fc is a thread_local into an entry.
ska::flat_hash_map<unsigned long long, std::unique_ptr<fbo_ctx_state_t>> g_fbo_ctxs;
// Per thread, not one shared instance. This is where a context this layer never
// saw created lands, and every such thread used to read and write the same
// tables and the same two bindings with no lock between them. A context is
// current on one thread at a time, so a thread-local fallback is also the more
// accurate model of what it stands for.
thread_local fbo_ctx_state_t g_fbo_default;
thread_local fbo_ctx_state_t* g_fc = &g_fbo_default;
} // namespace

void mg_framebuffer_bind_context(unsigned long long ctx_id) {
    if (ctx_id == 0) {
        g_fc = &g_fbo_default;
        return;
    }
    std::lock_guard<std::mutex> lock(g_fbo_mutex);
    std::unique_ptr<fbo_ctx_state_t>& slot = g_fbo_ctxs[ctx_id];
    if (!slot) slot = std::make_unique<fbo_ctx_state_t>();
    g_fc = slot.get();
}

void mg_framebuffer_forget_context(unsigned long long ctx_id) {
    if (ctx_id == 0) return;
    std::lock_guard<std::mutex> lock(g_fbo_mutex);
    const auto it = g_fbo_ctxs.find(ctx_id);
    if (it == g_fbo_ctxs.end()) return;
    if (g_fc == it->second.get()) g_fc = &g_fbo_default;
    g_fbo_ctxs.erase(it);
}

// The bodies below are unchanged: the names now resolve into the per-context
// record instead of to file-scope globals.
#define framebuffers (g_fc->table)
#define current_draw_fbo (g_fc->draw)
#define current_read_fbo (g_fc->read)
// gl/gl.cpp used to reach in with mg_framebuffers()[current_draw_fbo], which is
// the same unchecked index from another translation unit and could not see the
// table's size. It gets the predicate instead.
bool mg_draw_framebuffer_all_none() {
    const auto it = framebuffers.find(current_draw_fbo);
    return it != framebuffers.end() && it->second->color_attachments_all_none;
}
void ensure_max_attachments() {
    // The fallback is used but no longer cached. A query made with no context
    // current answers nothing, and writing 8 into the static then meant 8 for the
    // life of the process -- on four-attachment hardware the draw-buffer shuffle
    // would attach past the end of what the driver has. Leaving the static at
    // zero makes the next call, once a context exists, ask again.
    if (MAX_COLOR_ATTACHMENTS == 0) {
        GLint v = 0;
        GLES.glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &v);
        if (v > 0) MAX_COLOR_ATTACHMENTS = v;
    }
    if (MAX_DRAW_BUFFERS == 0) {
        GLint v = 0;
        GLES.glGetIntegerv(GL_MAX_DRAW_BUFFERS, &v);
        if (v > 0) MAX_DRAW_BUFFERS = v;
    }
}

// What the tables are sized against while the query has not answered yet.
static GLint max_color_attachments_or_default() {
    return MAX_COLOR_ATTACHMENTS > 0 ? MAX_COLOR_ATTACHMENTS : 8;
}
framebuffer_t& get_framebuffer(GLuint id) {
    // Created on first sight of the name, as the old default-construct-on-index
    // did. The reference is what callers keep across further calls to this
    // function, so the record is what must not move when the map grows -- which
    // is why the table holds framebuffer_t by unique_ptr and why the lookup below
    // must keep returning the pointee rather than anything stored inline.
    //
    // A hit is the overwhelmingly common case: every glBindFramebuffer, every
    // attachment change and every draw-buffer call routes through here, and a
    // name is new only once. operator[] is the insertion path -- it drags the
    // rehash-and-grow tail in at the call site and only then discovers there was
    // nothing to insert -- so the hit is answered by find() and operator[] is
    // reached only on a genuine miss.
    const auto it = framebuffers.find(id);
    if (it != framebuffers.end() && it->second) return *it->second;
    // The null-pointer half of that test is not dead: a throwing make_unique
    // leaves an inserted-but-empty slot behind, and this function has always
    // filled such a slot rather than dereferencing it.
    std::unique_ptr<framebuffer_t>& slot = framebuffers[id];
    if (!slot) slot = std::make_unique<framebuffer_t>();
    return *slot;
}
void InitFramebufferMap(size_t expectedSize) {
    framebuffers.reserve(expectedSize);
}
void init_framebuffer(framebuffer_t& fbo) {
    ensure_max_attachments();
    const size_t want = static_cast<size_t>(max_color_attachments_or_default());
    if (!fbo.initialized) {
        fbo.color_attachments.assign(want, attachment_t{});
        fbo.initialized = true;
        return;
    }
    // The limit can go up after a record was sized. ensure_max_attachments
    // deliberately stopped latching its 8-slot fallback -- caching it from a query
    // made with no context current was its own bug -- so a framebuffer first
    // touched before the driver could answer holds eight slots while every bounds
    // check now uses the driver's real, larger number. Indexing the difference is
    // a heap write past the end.
    if (fbo.color_attachments.size() < want) fbo.color_attachments.resize(want, attachment_t{});
}
void glBindFramebuffer(GLenum target, GLuint framebuffer) {
    LOG()
    LOG_D("glBindFramebuffer, target = %s, framebuffer = %u", glEnumToString(target), framebuffer)
    ensure_max_attachments();

    // Resolve the redirect before touching the table: this used to take the
    // reference for the id the application passed and then initialise that
    // record, while the id that actually became current was g_renderFBO -- so
    // framebuffers[g_renderFBO].color_attachments stayed null and the per-fbo
    // state written later landed on the wrong record.
    //
    // The redirect is the draw binding only. GL_READ_FRAMEBUFFER was already left
    // alone, but GL_FRAMEBUFFER is two bindings in one call, so the target test
    // moved the read binding to g_renderFBO as well: the two ways of saying "read
    // framebuffer 0" then meant different framebuffers, and restoring a saved read
    // binding of 0 -- which gl/texture.cpp does around its blits -- landed on the
    // FSR1 target or not depending on which spelling the caller used.
    GLuint draw_fb = framebuffer;
    if (framebuffer == 0 && target != GL_READ_FRAMEBUFFER) {
        draw_fb = FSR1_Context::g_renderFBO;
        FSR1_Context::g_dirty = true;
    }

    if (draw_fb != 0) {
        init_framebuffer(get_framebuffer(draw_fb));
    }

    if (target != GL_READ_FRAMEBUFFER) {
        set_gl_state_current_draw_fbo(draw_fb);
    }

    if (target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER) {
        current_draw_fbo = draw_fb;
    }
    if (target == GL_READ_FRAMEBUFFER || target == GL_FRAMEBUFFER) {
        current_read_fbo = framebuffer;
    }

    if (target == GL_FRAMEBUFFER && draw_fb != framebuffer) {
        GLES.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_fb);
        GLES.glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    } else {
        GLES.glBindFramebuffer(target, draw_fb);
    }
}
// Only the outermost scope on a thread touches the binding.
//
// The tracked read binding stays 0 the whole time one of these is active -- the
// redirect goes straight to the backend and deliberately does not change what the
// application asked for -- so a nested scope would see the same "needs
// redirecting" state, activate as well, and then undo the outer one's redirect on
// its way out while the outer scope still had work to do.
static thread_local int g_fsr_read_depth = 0;

mg_fsr_read_scope_t::mg_fsr_read_scope_t() {
    // g_renderFBO is nonzero only while FSR1 is on, so this is also the FSR1 test.
    if (FSR1_Context::g_renderFBO == 0 || current_read_fbo != 0) return;
    counted = true;
    if (g_fsr_read_depth++ > 0) return; // an outer scope already holds it
    active = true;
    GLES.glBindFramebuffer(GL_READ_FRAMEBUFFER, FSR1_Context::g_renderFBO);
}
mg_fsr_read_scope_t::~mg_fsr_read_scope_t() {
    if (!counted) return;
    --g_fsr_read_depth;
    // Back to the raw surface, which is what the tracked binding of 0 means for
    // the read target -- glBindFramebuffer never redirects that one.
    if (active) GLES.glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

// Record what is now attached at `attachment`. Only colour attachments are kept:
// nothing reads depth or stencil back out of here.
void update_attachment(GLenum target, GLenum attachment, const attachment_t& what) {
    GLuint current_fbo = (target == GL_READ_FRAMEBUFFER) ? current_read_fbo : current_draw_fbo;
    if (current_fbo == 0) return;
    if (attachment < GL_COLOR_ATTACHMENT0 || attachment >= GL_COLOR_ATTACHMENT0 + (GLenum)max_color_attachments_or_default()) {
        return;
    }
    framebuffer_t& fbo = get_framebuffer(current_fbo);
    init_framebuffer(fbo);
    const size_t index = attachment - GL_COLOR_ATTACHMENT0;
    if (index >= fbo.color_attachments.size()) return;
    fbo.color_attachments[index] = what;
    // If a shuffle had moved this attachment, that record now describes where the
    // PREVIOUS texture went; glReadBuffer would send the application there instead
    // of to what was just attached. Only this entry is dropped -- the others are
    // still where the shuffle put them, and forgetting that would strand them.
    if (index < fbo.draw_buffer_map.size()) fbo.draw_buffer_map[index] = 0;
}

// Undo a shuffle: put every attachment glDrawBuffers moved back on its own
// attachment point.
//
// Needed because leaving the shuffled state is not free. After
// glDrawBuffers({ATTACHMENT1, ATTACHMENT0}) the texture the application calls
// attachment 0 physically sits on GL_COLOR_ATTACHMENT1, so a later
// glDrawBuffers({ATTACHMENT0}) -- identity order, nothing to move by itself --
// would draw into whatever is on physical attachment 0, which is the other
// texture. The old unconditional re-attach happened to fix this up on every call;
// anything that skips it has to put things back first.
void restore_home_attachments(framebuffer_t& fbo);

// Put a recorded attachment onto a (possibly different) attachment point, the
// same way it originally arrived.
//
// Deliberately calls GLES directly: the only caller is the glDrawBuffers shuffle,
// which is in the middle of building draw_buffer_map, and going back through the
// wrappers above would clear it.
void reattach(GLenum target, GLenum attachment, const attachment_t& a) {
    switch (a.kind) {
    case attach_kind_t::Texture2D:
        GLES.glFramebufferTexture2D(target, attachment, a.textarget, a.texture, a.level);
        break;
    case attach_kind_t::TextureLayer:
        GLES.glFramebufferTextureLayer(target, attachment, a.texture, a.level, a.layer);
        break;
    case attach_kind_t::TextureAll:
        GLES.glFramebufferTexture(target, attachment, a.texture, a.level);
        break;
    case attach_kind_t::Renderbuffer:
        GLES.glFramebufferRenderbuffer(target, attachment, a.textarget, a.texture);
        break;
    case attach_kind_t::None:
        // Never reached: the caller checks for it, because "re-attach nothing" is
        // a detach and that was the bug.
        break;
    }
}

void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    update_attachment(target, attachment, {attach_kind_t::Texture2D, textarget, texture, level, 0});
    GLES.glFramebufferTexture2D(target, attachment, textarget, texture, level);
}
void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level) {
    // Kind rather than a made-up GL_TEXTURE_2D. This entry point attaches the
    // whole texture, whatever its target is, and recording it as a 2D attachment
    // meant a replay re-attached an array or 3D texture as if it were flat.
    update_attachment(target, attachment, {attach_kind_t::TextureAll, 0, texture, level, 0});
    GLES.glFramebufferTexture(target, attachment, texture, level);
}
// Wrapped rather than passed straight through, so the record knows about them.
// While these bypassed the table, the record for an attachment they wrote stayed
// all-zero and the glDrawBuffers shuffle detached it.
void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {
    update_attachment(target, attachment, {attach_kind_t::TextureLayer, 0, texture, level, layer});
    GLES.glFramebufferTextureLayer(target, attachment, texture, level, layer);
}
void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    update_attachment(target, attachment, {attach_kind_t::Renderbuffer, renderbuffertarget, renderbuffer, 0, 0});
    GLES.glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}

void restore_home_attachments(framebuffer_t& fbo) {
    if (fbo.draw_buffer_map.empty()) return;
    // Every recorded attachment, not just the ones the map says were moved.
    //
    // The map answers "where did logical i go", which is the wrong direction for
    // undoing. A slot that was only ever a DESTINATION -- something else was moved
    // on top of it, detaching what lived there -- has a zero entry, so walking the
    // map skipped exactly the slot that needs repairing. With colortex0 on
    // attachment 0 and colortex1 on attachment 1, glDrawBuffers({ATTACHMENT1})
    // moves colortex1 onto physical 0; a following glDrawBuffers({ATTACHMENT0})
    // then found map[0] == 0, left physical 0 holding colortex1, and the pass
    // wrote its output into the wrong texture.
    //
    // color_attachments is the layout the application actually asked for, so
    // putting every entry back on its own point is both sufficient and
    // idempotent: re-attaching something already in place costs one call and
    // changes nothing.
    for (size_t idx = 0; idx < fbo.color_attachments.size(); ++idx) {
        const attachment_t& a = fbo.color_attachments[idx];
        if (a.kind == attach_kind_t::None) continue;
        reattach(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (GLenum)idx, a);
    }
    fbo.draw_buffer_map.clear();
}

// The ARB spellings, which these three had while they were NATIVE_FUNCTION_HEAD
// entries -- that macro emits a `name##ARB` alias next to every function it
// defines, and moving them here quietly dropped three of the 500-odd aliases the
// library exports. An application that resolves glDeleteFramebuffersARB, as
// anything written against EXT_framebuffer_object does, got a null pointer.
//
// Not on Apple: Mach-O has no symbol aliases and clang refuses the attribute
// there ("aliases are not supported on darwin"), which is why
// NATIVE_FUNCTION_HEAD leaves the ARB names out on that platform as well.
#ifndef __APPLE__
extern "C" {
GLAPI GLAPIENTRY void glDeleteFramebuffersARB(GLsizei n, const GLuint* names) __attribute__((alias("glDeleteFramebuffers")));
GLAPI GLAPIENTRY void glFramebufferRenderbufferARB(GLenum target, GLenum attachment, GLenum renderbuffertarget,
                                                   GLuint renderbuffer) __attribute__((alias("glFramebufferRenderbuffer")));
GLAPI GLAPIENTRY void glFramebufferTextureLayerARB(GLenum target, GLenum attachment, GLuint texture, GLint level,
                                                   GLint layer) __attribute__((alias("glFramebufferTextureLayer")));
}
#endif

// Wrapped for the same reason glReadPixels is: the source is the read framebuffer,
// and while FSR1 is on the application's framebuffer 0 is not where its frame is.
// The commit that added mg_fsr_read_scope_t covered this layer's own internal
// blits and left the application-facing entry point a raw passthrough, so a host
// blitting from framebuffer 0 -- which is how most post-processing gets its
// input, and the only way to reach the depth buffer that lives on the FSR target
// and never on the surface -- still read the window.
void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1,
                       GLint dstY1, GLbitfield mask, GLenum filter) {
    LOG()
    mg_fsr_read_scope_t fsr_read;
    GLES.glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    CHECK_GL_ERROR
}

void glDeleteFramebuffers(GLsizei n, const GLuint* names) {
    // The record has to die with the name. Drivers hand deleted framebuffer names
    // straight back out of the next glGenFramebuffers, and this table never
    // dropped anything -- so a recycled name inherited the previous framebuffer's
    // attachments (which the draw-buffer shuffle would then re-attach over
    // whatever the application had just bound), its draw_buffer_map (silently
    // redirecting glReadBuffer) and its all-none flag (misfiring the ANGLE
    // depth-clear workaround in gl/gl.cpp). The pixel helpers in gl/texture.cpp
    // create and delete temporary framebuffers constantly, so this recycling is
    // the common case rather than a corner one.
    if (names != nullptr) {
        for (GLsizei i = 0; i < n; ++i) {
            const GLuint name = names[i];
            if (name == 0) continue; // silently ignored, per spec
            framebuffers.erase(name);
            // "If a framebuffer object that is currently bound is deleted, the
            // binding reverts to 0" -- through this layer's own entry point, so the
            // FSR1 redirect and the tracked bindings stay in step.
            if (current_draw_fbo == name) glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            if (current_read_fbo == name) glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        }
    }
    GLES.glDeleteFramebuffers(n, names);
}
void glDrawBuffer(GLenum buffer) {
    LOG()
    LOG_D("glDrawBuffer %d", buffer)

    //    GLint currentFBO;
    //    GLES.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
    if (current_draw_fbo == 0) {
        GLenum buffers[] = {buffer};
        glDrawBuffers(1, buffers);
    } else {
        // Under the FSR1 redirect the application still believes it is drawing to
        // the window, so it names the window's buffers -- but the binding really
        // is an FBO, whose only colour buffer is attachment 0. GL_BACK matched
        // neither GL_NONE nor the attachment range below, so the call emitted
        // nothing whatsoever and an earlier glDrawBuffer(GL_NONE) could never be
        // undone.
        if (current_draw_fbo == FSR1_Context::g_renderFBO &&
            (buffer == GL_BACK || buffer == GL_FRONT || buffer == GL_FRONT_AND_BACK || buffer == GL_LEFT ||
             buffer == GL_BACK_LEFT || buffer == GL_FRONT_LEFT)) {
            LOG_D("glDrawBuffer 0x%x on the FSR1 target -> GL_COLOR_ATTACHMENT0", buffer)
            buffer = GL_COLOR_ATTACHMENT0;
        }
        // The cached value, not a driver round trip on every call -- and the same
        // one the rest of this file uses, so the two cannot disagree about how
        // many attachments there are.
        const GLint maxAttachments = max_color_attachments_or_default();

        if (buffer == GL_NONE) {
            get_framebuffer(current_draw_fbo).color_attachments_all_none = true;
            std::vector<GLenum> buffers(maxAttachments, GL_NONE);
            glDrawBuffers(maxAttachments, buffers.data());
        } else if (buffer >= GL_COLOR_ATTACHMENT0 && buffer < GL_COLOR_ATTACHMENT0 + maxAttachments) {
            get_framebuffer(current_draw_fbo).color_attachments_all_none = false;
            std::vector<GLenum> buffers(maxAttachments, GL_NONE);
            buffers[buffer - GL_COLOR_ATTACHMENT0] = buffer;
            glDrawBuffers(maxAttachments, buffers.data());
        }
    }
    CHECK_GL_ERROR;
}
void glDrawBuffers(GLsizei n, const GLenum* bufs) {
    LOG()
    if (current_draw_fbo == 0) {
        GLES.glDrawBuffers(n, bufs);
        return;
    }

    framebuffer_t& fbo = get_framebuffer(current_draw_fbo);
    init_framebuffer(fbo);

    bool all_none = true;
    for (int i = 0; i < n; ++i) {
        if (bufs[i] != GL_NONE) {
            all_none = false;
            break;
        }
    }

    if (all_none) {
        LOG_D("glDrawBuffers, fb %d all_none true", current_draw_fbo)
        fbo.color_attachments_all_none = true;
        GLES.glDrawBuffers(n, bufs);
        return;
    } else {
        LOG_D("glDrawBuffers, fb %d all_none false", current_draw_fbo)
        fbo.color_attachments_all_none = false;
    }

    // Attachment i already in slot i is the only arrangement GLES accepts, so
    // there is nothing to move -- and this is what applications ask for almost
    // every time. Moving it anyway is how a renderbuffer or layered attachment,
    // which this table cannot describe, used to get detached by a call that
    // should have been a no-op.
    bool identity = true;
    for (int i = 0; i < n; i++) {
        if (bufs[i] != GL_COLOR_ATTACHMENT0 + (GLenum)i) {
            identity = false;
            break;
        }
    }
    if (identity) {
        LOG_D("glDrawBuffers, fb %d identity order, nothing to move", current_draw_fbo)
        restore_home_attachments(fbo);
        GLES.glDrawBuffers(n, bufs);
        return;
    }

    // A real shuffle. Every attachment it has to move must be one this layer
    // recorded -- an unrecorded one is either genuinely empty or attached through
    // a path that does not reach update_attachment, and "re-attaching" its zeroed
    // record would detach whatever is really there.
    for (int i = 0; i < n; i++) {
        if (bufs[i] < GL_COLOR_ATTACHMENT0 || bufs[i] >= GL_COLOR_ATTACHMENT0 + (GLenum)max_color_attachments_or_default()) continue;
        if (bufs[i] - GL_COLOR_ATTACHMENT0 >= fbo.color_attachments.size()) continue;
        if (fbo.color_attachments[bufs[i] - GL_COLOR_ATTACHMENT0].kind != attach_kind_t::None) continue;
        // Passed through unchanged instead. GLES rejects a non-identity draw
        // buffer list, so the application gets GL_INVALID_OPERATION -- which is
        // both true and something it can now see -- rather than a framebuffer that
        // quietly lost an attachment.
        FB_WARN_ONCE("glDrawBuffers: fb %u wants attachment %u in slot %d but nothing is recorded there; "
                     "passing the list through rather than detaching it",
                     current_draw_fbo, bufs[i] - GL_COLOR_ATTACHMENT0, i);
        restore_home_attachments(fbo);
        GLES.glDrawBuffers(n, bufs);
        return;
    }

    // Undo whatever the last shuffle did before arranging a new one. Overwriting
    // the map with assign() below discards the record of where the previous
    // shuffle put things while the driver still has them there, so any slot the
    // old shuffle moved and the new one does not name would be stranded with no
    // way left to find it.
    restore_home_attachments(fbo);

    std::vector<GLenum> new_bufs(n);
    fbo.draw_buffer_map.assign(max_color_attachments_or_default(), 0);
    for (int i = 0; i < n; i++) {
        if (bufs[i] >= GL_COLOR_ATTACHMENT0 && bufs[i] < GL_COLOR_ATTACHMENT0 + (GLenum)max_color_attachments_or_default()) {
            GLenum logical_attachment = bufs[i];
            GLenum physical_attachment = GL_COLOR_ATTACHMENT0 + i;
            new_bufs[i] = physical_attachment;
            size_t index = logical_attachment - GL_COLOR_ATTACHMENT0;
            if (index >= fbo.color_attachments.size() || index >= fbo.draw_buffer_map.size()) {
                new_bufs[i] = bufs[i];
                continue;
            }
            reattach(GL_DRAW_FRAMEBUFFER, physical_attachment, fbo.color_attachments[index]);
            // Remember where it went, so glReadBuffer can read it where it is
            // rather than moving it a second time.
            fbo.draw_buffer_map[index] = physical_attachment;
        } else {
            new_bufs[i] = bufs[i];
        }
    }
    GLES.glDrawBuffers(n, new_bufs.data());
}
void glReadBuffer(GLenum src) {
    if (current_read_fbo != 0 && src >= GL_COLOR_ATTACHMENT0 && src < GL_COLOR_ATTACHMENT0 + max_color_attachments_or_default()) {
        framebuffer_t& fbo = get_framebuffer(current_read_fbo);
        init_framebuffer(fbo);
        const size_t index = static_cast<size_t>(src - GL_COLOR_ATTACHMENT0);
        // glDrawBuffers has to move logical attachment i onto physical
        // GL_COLOR_ATTACHMENTi, because GLES only accepts COLOR_ATTACHMENTi in slot
        // i of the draw buffer list. After such a shuffle the texture the
        // application calls attachment n is somewhere else, so read it there.
        //
        // This used to re-attach it onto GL_COLOR_ATTACHMENT0 instead, which
        // destroyed whatever was on attachment 0 -- and did so even for a
        // framebuffer that had never been shuffled, and even when the record was
        // empty because the application had attached with glFramebufferRenderbuffer
        // or one of the layered entry points, which do not reach update_attachment.
        // Reading where the texture already is moves nothing and cannot clobber.
        if (index < fbo.draw_buffer_map.size() && fbo.draw_buffer_map[index] != 0) {
            GLES.glReadBuffer(fbo.draw_buffer_map[index]);
            return;
        }
    }
    GLES.glReadBuffer(src);
}
GLenum glCheckFramebufferStatus(GLenum target) {
    GLenum status = GLES.glCheckFramebufferStatus(target);
    if (global_settings.ignore_error == IgnoreErrorLevel::Full && status != GL_FRAMEBUFFER_COMPLETE) {
        return GL_FRAMEBUFFER_COMPLETE;
    }
    return status;
}