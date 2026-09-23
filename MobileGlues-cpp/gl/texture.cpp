// MobileGlues - gl/texture.cpp
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

#include "texture.h"
#include "depth_filter.h"
#include "mg.h"
#include "../config/settings.h"
#include "../egl/context.h"
#include <mutex>
#include <memory>
#include <ska/flat_hash_map.hpp>
#include "GLES3/gl32.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

#ifdef __ANDROID__
#include <android/log.h>
#endif
#ifndef __APPLE__
#include <malloc.h>
#endif

#include "../gles/gles.h"
#include "../gles/loader.h"
#include "framebuffer.h"
#include "log.h"
#include "transfer.h"
#include "pixel.h"
#include "mg.h"
#include <GL/gl.h>

#define DEBUG 0

#define TX_WARN_ONCE(...)                                                                                              \
    do {                                                                                                               \
        static bool mg_tx_warned = false;                                                                              \
        if (!mg_tx_warned) {                                                                                           \
            mg_tx_warned = true;                                                                                       \
            LOG_W_FORCE(__VA_ARGS__)                                                                                   \
        }                                                                                                              \
    } while (0)

int nlevel(int size, int level) {
    if (size) {
        size >>= level;
        if (!size) size = 1;
    }
    return size;
}

GLenum ConvertTextureTargetToGLEnum(TextureTarget target) {
    switch (target) {
    case TextureTarget::TEXTURE_1D:
        return GL_TEXTURE_1D;
    case TextureTarget::PROXY_TEXTURE_1D:
        return GL_PROXY_TEXTURE_1D;
    case TextureTarget::TEXTURE_1D_ARRAY:
        return GL_TEXTURE_1D_ARRAY;
    case TextureTarget::PROXY_TEXTURE_1D_ARRAY:
        return GL_PROXY_TEXTURE_1D_ARRAY;
    case TextureTarget::TEXTURE_2D:
        return GL_TEXTURE_2D;
    case TextureTarget::PROXY_TEXTURE_2D:
        return GL_PROXY_TEXTURE_2D;
    case TextureTarget::TEXTURE_2D_ARRAY:
        return GL_TEXTURE_2D_ARRAY;
    case TextureTarget::PROXY_TEXTURE_2D_ARRAY:
        return GL_PROXY_TEXTURE_2D_ARRAY;
    case TextureTarget::TEXTURE_2D_MULTISAMPLE:
        return GL_TEXTURE_2D_MULTISAMPLE;
    case TextureTarget::PROXY_TEXTURE_2D_MULTISAMPLE:
        return GL_PROXY_TEXTURE_2D_MULTISAMPLE;
    case TextureTarget::TEXTURE_2D_MULTISAMPLE_ARRAY:
        return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
    case TextureTarget::PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY:
        return GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY;
    case TextureTarget::TEXTURE_3D:
        return GL_TEXTURE_3D;
    case TextureTarget::PROXY_TEXTURE_3D:
        return GL_PROXY_TEXTURE_3D;
    case TextureTarget::TEXTURE_RECTANGLE:
        return GL_TEXTURE_RECTANGLE;
    case TextureTarget::PROXY_TEXTURE_RECTANGLE:
        return GL_PROXY_TEXTURE_RECTANGLE;
    case TextureTarget::TEXTURE_CUBE_MAP:
        return GL_TEXTURE_CUBE_MAP;
    case TextureTarget::PROXY_TEXTURE_CUBE_MAP:
        return GL_PROXY_TEXTURE_CUBE_MAP;
    // case TextureTarget::TEXTURE_CUBE_MAP_POSITIVE_X: return
    // GL_TEXTURE_CUBE_MAP_POSITIVE_X; case
    // TextureTarget::TEXTURE_CUBE_MAP_NEGATIVE_X: return
    // GL_TEXTURE_CUBE_MAP_NEGATIVE_X; case
    // TextureTarget::TEXTURE_CUBE_MAP_POSITIVE_Y: return
    // GL_TEXTURE_CUBE_MAP_POSITIVE_Y; case
    // TextureTarget::TEXTURE_CUBE_MAP_NEGATIVE_Y: return
    // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y; case
    // TextureTarget::TEXTURE_CUBE_MAP_POSITIVE_Z: return
    // GL_TEXTURE_CUBE_MAP_POSITIVE_Z; case
    // TextureTarget::TEXTURE_CUBE_MAP_NEGATIVE_Z: return
    // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
    case TextureTarget::TEXTURE_CUBE_MAP_ARRAY:
        return GL_TEXTURE_CUBE_MAP_ARRAY;
    case TextureTarget::PROXY_TEXTURE_CUBE_MAP_ARRAY:
        return GL_PROXY_TEXTURE_CUBE_MAP_ARRAY;
    case TextureTarget::TEXTURE_BUFFER:
        return GL_TEXTURE_BUFFER;
    default:
        return GL_TEXTURE_2D;
    }
}

TextureTarget ConvertGLEnumToTextureTarget(GLenum target) {
    switch (target) {
    case GL_TEXTURE_1D:
        return TextureTarget::TEXTURE_1D;
    case GL_PROXY_TEXTURE_1D:
        return TextureTarget::PROXY_TEXTURE_1D;
    case GL_TEXTURE_1D_ARRAY:
        return TextureTarget::TEXTURE_1D_ARRAY;
    case GL_PROXY_TEXTURE_1D_ARRAY:
        return TextureTarget::PROXY_TEXTURE_1D_ARRAY;
    case GL_TEXTURE_2D:
        return TextureTarget::TEXTURE_2D;
    case GL_PROXY_TEXTURE_2D:
        return TextureTarget::PROXY_TEXTURE_2D;
    case GL_TEXTURE_2D_ARRAY:
        return TextureTarget::TEXTURE_2D_ARRAY;
    case GL_PROXY_TEXTURE_2D_ARRAY:
        return TextureTarget::PROXY_TEXTURE_2D_ARRAY;
    case GL_TEXTURE_2D_MULTISAMPLE:
        return TextureTarget::TEXTURE_2D_MULTISAMPLE;
    case GL_PROXY_TEXTURE_2D_MULTISAMPLE:
        return TextureTarget::PROXY_TEXTURE_2D_MULTISAMPLE;
    case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
        return TextureTarget::TEXTURE_2D_MULTISAMPLE_ARRAY;
    case GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY:
        return TextureTarget::PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY;
    case GL_TEXTURE_3D:
        return TextureTarget::TEXTURE_3D;
    case GL_PROXY_TEXTURE_3D:
        return TextureTarget::PROXY_TEXTURE_3D;
    case GL_TEXTURE_RECTANGLE:
        return TextureTarget::TEXTURE_RECTANGLE;
    case GL_PROXY_TEXTURE_RECTANGLE:
        return TextureTarget::PROXY_TEXTURE_RECTANGLE;
    case GL_PROXY_TEXTURE_CUBE_MAP:
        return TextureTarget::PROXY_TEXTURE_CUBE_MAP;
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
    case GL_TEXTURE_CUBE_MAP:
        return TextureTarget::TEXTURE_CUBE_MAP;
    case GL_TEXTURE_CUBE_MAP_ARRAY:
        return TextureTarget::TEXTURE_CUBE_MAP_ARRAY;
    case GL_PROXY_TEXTURE_CUBE_MAP_ARRAY:
        return TextureTarget::PROXY_TEXTURE_CUBE_MAP_ARRAY;
    case GL_TEXTURE_BUFFER:
        return TextureTarget::TEXTURE_BUFFER;
    default:
        return TextureTarget::UNKNWON;
    }
}

// glActiveTexture is bounded by GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, not by
// GL_MAX_TEXTURE_IMAGE_UNITS, and that combined figure is routinely far larger:
// Mali-G77 reports 96. At 32 this table was smaller than the limit the layer
// itself advertised, so glActiveTexture(GL_TEXTURE32..95) -- legal by that
// advertisement, and accepted by the driver -- returned early without telling
// the driver anything, and the glBindTexture that followed silently landed on
// whichever unit was active before. mg_max_texture_units() now also caps what
// the layer is willing to promise, so the two can no longer disagree.
const int MAX_TEXTURE_IMAGE_UNITS = 128;

class TextureBindingSlot {
public:
    using TargetEnum = TextureTarget;

    TextureBindingSlot() : m_target((TargetEnum)0), m_boundObject(nullptr) {}

    explicit TextureBindingSlot(TargetEnum target) : m_target(target), m_boundObject(nullptr) {}

    void Bind(TextureObject* object) { m_boundObject = object; }

    TextureObject* GetBoundObject() const { return m_boundObject; }

    TargetEnum GetTarget() const { return m_target; }

private:
    TargetEnum m_target;
    TextureObject* m_boundObject;
};

class TextureUnit {
public:
    TextureBindingSlot& GetBindingSlot(TextureBindingSlot::TargetEnum target) { return m_slots[(int)target]; }

private:
    std::array<TextureBindingSlot, (int)TextureTarget::TEXTURES_COUNT> m_slots;
};

// Texture objects are shared across a share group; the texture unit bindings and
// the active unit are container state and belong to one context. Both used to be
// process-wide. See the note in gl/buffer.cpp for why this is a thread_local
// pointer swap rather than an accessor at every use.
namespace {

// Sampler-object half of the sampling-state shadow. Keyed by driver name:
// glGenSamplers passes straight through, so both id spaces are the same
// names. An entry that was never written is the GL default
// (NEAREST_MIPMAP_LINEAR / LINEAR / NONE), which is exactly what a
// default-constructed element answers, so an absent vector element is the
// correct answer and no glGenSamplers hook is needed.
struct sampler_shadow_t {
    GLint min_filter = GL_NEAREST_MIPMAP_LINEAR;
    GLint mag_filter = GL_LINEAR;
    GLenum compare_mode = GL_NONE;
};

struct texture_group_state_t {
    std::vector<TextureObject*> objects;
    // Sampler objects, like texture objects, live in the share group.
    std::vector<sampler_shadow_t> sampler_objects;
};
struct texture_ctx_state_t {
    std::array<TextureUnit, MAX_TEXTURE_IMAGE_UNITS> units;
    int current_unit = 0;
    // What this layer last handed to GLES.glBindTexture / GLES.glActiveTexture,
    // i.e. the driver's side of the same state `units` and `current_unit` describe
    // from the application's side. The two are not one table because they answer
    // different questions and can legitimately differ: the texture-buffer
    // emulation parks the object on unit 15's GL_TEXTURE_2D while the application
    // asked for GL_TEXTURE_BUFFER on whichever unit was active, and gl/drawing.cpp
    // needs to know what is on unit 15 without disturbing the active unit to ask.
    //
    // Zero-initialised, which is where GL starts a fresh context: no texture bound
    // anywhere and GL_TEXTURE0 active.
    std::array<std::array<GLuint, (int)TextureTarget::TEXTURES_COUNT>, MAX_TEXTURE_IMAGE_UNITS> driver_bindings{};
    int driver_active_unit = 0;
    // Which sampler object is bound to each unit. Context state, like the
    // texture bindings above; zero is GL's default of no sampler, which is
    // what a value-initialised array answers.
    std::array<GLuint, MAX_TEXTURE_IMAGE_UNITS> sampler_bindings{};
    // Which share group this context draws its texture objects from. Deleting an
    // object has to clear it out of every context in that group, not only the one
    // that happened to issue the glDeleteTextures.
    unsigned long long group = 0;
};

std::mutex g_tex_mutex;
// The tables hold their state by pointer. A thread_local pointer into an entry is
// the whole point of the design -- the ~90 access sites read through g_bg/g_bc
// rather than looking anything up -- and the map moves its elements when it
// grows, so the entry itself must not be what moves. The unique_ptr stays put
// while the map rehashes around it.
ska::flat_hash_map<unsigned long long, std::unique_ptr<texture_group_state_t>> g_tex_groups;
ska::flat_hash_map<unsigned long long, std::unique_ptr<texture_ctx_state_t>> g_tex_ctxs;
texture_group_state_t g_tex_group_default;
texture_ctx_state_t g_tex_ctx_default;
thread_local texture_group_state_t* g_tg = &g_tex_group_default;
thread_local texture_ctx_state_t* g_tc = &g_tex_ctx_default;

} // namespace

void mg_texture_bind_context(unsigned long long ctx_id, unsigned long long group_id) {
    if (ctx_id == 0) {
        g_tg = &g_tex_group_default;
        g_tc = &g_tex_ctx_default;
        return;
    }
    std::lock_guard<std::mutex> lock(g_tex_mutex);
    std::unique_ptr<texture_group_state_t>& group = g_tex_groups[group_id];
    if (!group) group = std::make_unique<texture_group_state_t>();
    std::unique_ptr<texture_ctx_state_t>& ctx = g_tex_ctxs[ctx_id];
    if (!ctx) ctx = std::make_unique<texture_ctx_state_t>();
    g_tg = group.get();
    g_tc = ctx.get();
    g_tc->group = group_id;
}

void mg_texture_forget_context(unsigned long long ctx_id) {
    if (ctx_id == 0) return;
    std::lock_guard<std::mutex> lock(g_tex_mutex);
    const auto it = g_tex_ctxs.find(ctx_id);
    if (it == g_tex_ctxs.end()) return;
    if (g_tc == it->second.get()) g_tc = &g_tex_ctx_default;
    g_tex_ctxs.erase(it);
    // The object table is not dropped: it belongs to the share group, whose other
    // contexts may still be alive, and the objects in it are owned by GL names the
    // application is still entitled to delete.
}

#define BufferObjectsVec (g_tg->objects)
#define TextureUnits (g_tc->units)
#define CurrentTextureUnitIndex (g_tc->current_unit)
#define DriverTextureBindings (g_tc->driver_bindings)
#define DriverActiveTextureUnit (g_tc->driver_active_unit)
#define SamplerShadowTable (g_tg->sampler_objects)
#define SamplerBindings (g_tc->sampler_bindings)

// The unit the emulated texture buffer is parked on. glBindTexture and
// gl/buffer.cpp's glTexBuffer both borrow it and hand the active unit back.
static const int MG_TEXTURE_BUFFER_EMULATION_UNIT = 15;

static inline bool driver_binding_key_valid(int unit, TextureTarget target) {
    return unit >= 0 && unit < MAX_TEXTURE_IMAGE_UNITS && (int)target >= 0 &&
           (int)target < (int)TextureTarget::TEXTURES_COUNT;
}

static inline void set_driver_texture_binding(int unit, TextureTarget target, GLuint texture) {
    if (driver_binding_key_valid(unit, target)) DriverTextureBindings[unit][(int)target] = texture;
}

static inline GLuint get_driver_texture_binding(int unit, TextureTarget target) {
    return driver_binding_key_valid(unit, target) ? DriverTextureBindings[unit][(int)target] : 0;
}

void InitTextureMap(size_t expectedSize) {
    BufferObjectsVec.reserve(expectedSize);
}

TextureObject* GetOrCreateTextureObject(GLuint index) {
    if (index >= BufferObjectsVec.size()) {
        BufferObjectsVec.resize(index + 100, nullptr);
    }

    auto& obj = BufferObjectsVec[index];
    if (!obj) {
        obj = new TextureObject();
        obj->texture = index;
    }
    return obj;
}

void ActivateTextureUnit(int unit) {
    if (unit < 0 || unit >= MAX_TEXTURE_IMAGE_UNITS) {
        LOG_E("Invalid texture unit: %d", unit);
        return;
    }
    CurrentTextureUnitIndex = unit;
}

int GetCurrentTextureUnitIndex() {
    return CurrentTextureUnitIndex;
}

TextureUnit& GetTextureUnit(int unit) {
    if (unit < 0 || unit >= MAX_TEXTURE_IMAGE_UNITS) {
        LOG_E("Invalid texture unit: %d", unit);
        return TextureUnits[0];
    }
    return TextureUnits[unit];
}

void MarkTextureObjectForDeletion(unsigned texture) {
    // Name 0 is not deletable. glBindTexture creates a record for it like any
    // other name and rewrites that record's target on every bind, so deleting it
    // would free an object the binding slots of every other target still point at.
    if (texture == 0) return;

    if (texture >= BufferObjectsVec.size() || !BufferObjectsVec[texture]) {
        LOG_D("Texture %u not found in BufferObjectsVec!", texture);
        return;
    }

    auto textureObject = BufferObjectsVec[texture];

    // The object table is per share group but the binding slots are per context,
    // so clearing only this context's slots left every sibling context in the group
    // holding a pointer to the record about to be freed. Sweep the whole group.
    //
    // Every target is scanned rather than just textureObject->target: that field
    // only remembers the target of the most recent bind, so slots for the other
    // targets this name was ever bound to would have been left behind.
    auto sweep = [&](texture_ctx_state_t& ctx) {
        for (auto& unit : ctx.units) {
            for (int t = 0; t < (int)TextureTarget::TEXTURES_COUNT; ++t) {
                auto& slot = unit.GetBindingSlot((TextureBindingSlot::TargetEnum)t);
                if (slot.GetBoundObject() == textureObject) slot.Bind(nullptr);
            }
        }
    };

    sweep(*g_tc);
    {
        std::lock_guard<std::mutex> lock(g_tex_mutex);
        const unsigned long long group = g_tc->group;
        for (const auto& entry : g_tex_ctxs) {
            if (entry.second.get() != g_tc && entry.second->group == group) sweep(*entry.second);
        }
    }
    // The fallback record is not in the map and is what every untracked context
    // shares, so it can hold a stale binding too.
    if (g_tc != &g_tex_ctx_default) sweep(g_tex_ctx_default);

    BufferObjectsVec[texture] = nullptr;
    delete textureObject;
}

int mg_max_texture_units(void) { return MAX_TEXTURE_IMAGE_UNITS; }

// Whether the shadow describes the context whose driver state is actually current.
//
// g_tex_ctx_default is not a per-context record. It is one object shared by every
// context this layer never saw created -- mg_texture_bind_context(0, 0) selects it
// for all of them -- and it is not even thread_local, because
// MarkTextureObjectForDeletion has to sweep it from whichever thread issued the
// delete. Two untracked contexts on two threads therefore have one set of shadow
// values between them and separate driver state each, so a unit or a binding that
// one of them set would suppress the call the other still needs. Nothing may be
// skipped on the strength of that record; gl/enable.cpp draws the same line with
// driver_synced, for the same reason.
static inline bool driver_shadow_tracks_this_context() { return g_tc != &g_tex_ctx_default; }

// The active unit on its own. FSR1 does not disturb it -- its GLStateGuard saves
// GL_ACTIVE_TEXTURE from the driver and puts it back -- so this half asks only
// whether the record belongs to the context that is current.
static inline bool driver_active_unit_shadow_trustworthy() { return driver_shadow_tracks_this_context(); }

// Whether the shadowed bindings can still be believed.
//
// Every internal path that moves a driver texture binding through GLES.* puts it
// back -- gl/buffer.cpp's glTexBuffer reads unit 15 and rebinds what it read,
// gl/drawing.cpp only borrows the active unit -- with one exception. FSR1's
// GLStateGuard saves GL_TEXTURE_BINDING_2D for the unit that was active when it
// was built, but ApplyFSR then switches to GL_TEXTURE0 and binds the render
// texture there; the destructor restores the saved unit and rebinds only that
// one, so with any unit but GL_TEXTURE0 active at swap time unit 0 keeps the FSR1
// texture and no shadow anywhere records it. That leak is per frame and silent,
// so while FSR1 is switched on the shadow is declared untrustworthy wholesale
// rather than for the one pair, which is cheap: the setting is off by default.
static inline bool driver_texture_shadow_trustworthy() {
    return driver_shadow_tracks_this_context() && global_settings.fsr1_setting == FSR1_Quality_Preset::Disabled;
}

int mg_driver_active_texture_unit(void) {
    if (driver_active_unit_shadow_trustworthy()) return DriverActiveTextureUnit;
    // Every caller uses this to put the active unit back after borrowing one, so
    // there is no answering "unknown": a number out of a record that describes some
    // other context would leave the driver on a unit nobody asked for. Ask the
    // driver instead. Only reachable for a context this layer never saw created.
    GLint queried = GL_TEXTURE0;
    GLES.glGetIntegerv(GL_ACTIVE_TEXTURE, &queried);
    const int unit = (int)(queried - GL_TEXTURE0);
    return (unit >= 0 && unit < MAX_TEXTURE_IMAGE_UNITS) ? unit : 0;
}

bool mg_driver_texture_binding_at_unit(int unit, GLenum target, GLuint* out) {
    const TextureTarget targetR = ConvertGLEnumToTextureTarget(target);
    if (!out || !driver_texture_shadow_trustworthy() || !driver_binding_key_valid(unit, targetR)) return false;
    *out = get_driver_texture_binding(unit, targetR);
    return true;
}

bool mg_driver_texture_binding(GLenum target, GLuint* out) {
    const bool answered = mg_driver_texture_binding_at_unit(DriverActiveTextureUnit, target, out);
#if GLOBAL_DEBUG
    // Same debug cross-check as mg_driver_bound_buffer, for the same reason: a
    // divergence between this shadow and the driver is undetectable in release
    // and only ever shows up downstream. Checked for the current unit only --
    // verifying another unit would mean switching the active unit, which is
    // exactly the disturbance a verification pass must not cause.
    if (answered && GLES.glGetIntegerv) {
        GLenum pname = 0;
        switch (target) {
        case GL_TEXTURE_2D:        pname = GL_TEXTURE_BINDING_2D; break;
        case GL_TEXTURE_3D:        pname = GL_TEXTURE_BINDING_3D; break;
        case GL_TEXTURE_2D_ARRAY:  pname = GL_TEXTURE_BINDING_2D_ARRAY; break;
        case GL_TEXTURE_CUBE_MAP:  pname = GL_TEXTURE_BINDING_CUBE_MAP; break;
        default: break;
        }
        if (pname != 0) {
            GLint driver = 0;
            GLES.glGetIntegerv(pname, &driver);
            if (static_cast<GLuint>(driver) != *out) {
                LOG_E("mg_driver_texture_binding(0x%X): shadow says %u but the driver holds %u on unit %d -- "
                      "something bound a texture without going through the frontend",
                      target, *out, static_cast<GLuint>(driver), DriverActiveTextureUnit)
            }
        }
    }
#endif
    return answered;
}

TextureObject* mgGetTexObjectByTarget(GLenum target) {
    return GetTextureUnit(GetCurrentTextureUnitIndex())
        .GetBindingSlot(ConvertGLEnumToTextureTarget(target))
        .GetBoundObject();
}

TextureObject* mgGetTexObjectByID(unsigned texture) {
    if (texture >= BufferObjectsVec.size() || !BufferObjectsVec[texture]) {
        LOG_E("Texture %u not found in BufferObjectsVec!", texture);
        return nullptr;
    }
    return BufferObjectsVec[texture];
}

// Inline mapping for various internal formats to format and type.
//
// has_data says whether this call carries bytes for `type` to describe -- the
// same thing mg_upload_fix_t::has_data() reports, asked early because this runs
// first. It matters only where an unsized internalformat has to be resolved: an
// allocation's `type` describes nothing, so resolving storage from it means
// resolving it from decoration.
void internal_convert(GLenum* internal_format, GLenum* type, GLenum* format, bool has_data) {
    // GL_BGRA is deliberately not renamed here: a rename converts the enum and
    // not the data. Every pixel-transfer entry point routes through
    // mg_upload_fix_t (gl/transfer.h) before calling this, which converts both.

    switch (*internal_format) {
    case GL_DEPTH_COMPONENT16:
        if (type) *type = GL_UNSIGNED_SHORT;
        break;
    case GL_DEPTH_COMPONENT24:
        if (type) *type = GL_UNSIGNED_INT;
        break;
    case GL_DEPTH_COMPONENT32:
        // No GLES driver accepts GL_DEPTH_COMPONENT32 as an internalformat --
        // GL_OES_depth32 is absent on both Mali and Adreno -- so it has to become
        // something else. GL_DEPTH_COMPONENT24 is that something: it keeps the
        // unorm distribution the name promises (GL_DEPTH_COMPONENT32F does not),
        // and it is the only depth-only form a depth blit will accept, because
        // glBlitFramebuffer compares the declared internalformat rather than the
        // bit count. Going through the unsized GL_DEPTH_COMPONENT instead made
        // the driver answer GL_DEPTH_COMPONENT32 (Mali) or GL_DEPTH_COMPONENT
        // (Adreno) to GL_TEXTURE_INTERNAL_FORMAT, neither of which ever matches a
        // framebuffer, so glCopyTexSubImage2D copied nothing at all.
        *internal_format = GL_DEPTH_COMPONENT24;
        if (type) *type = GL_UNSIGNED_INT;
        break;
    case GL_DEPTH_COMPONENT32F:
        if (type) *type = GL_FLOAT;
        break;
    case GL_DEPTH_COMPONENT:
        LOG_D("Find GL_DEPTH_COMPONENT: internalFormat: %s, format: %s, type: %s", glEnumToString(*internal_format),
              format ? glEnumToString(*format) : "(none)", type ? glEnumToString(*type) : "(none)");
        if (type && has_data) {
            // A glTexImage* that carries bytes. Deliberately left unsized:
            // deriving a sized format from format/type here is what used to make
            // drivers reject otherwise valid uploads, and ES2 compatibility --
            // which every ES3 driver carries -- accepts the unsized form with
            // data.
            //
            // GL_FLOAT is the one type that compatibility does not reach. ES2
            // depth textures predate float depth entirely, so the unsized form
            // with GL_FLOAT is rejected outright, and a rejected glTexImage2D
            // leaves the level at its defaults -- the application is then holding
            // a GL_RGBA texture with no depth bits, and nothing says so.
            // Measured on Mali-G77: GL_TEXTURE_INTERNAL_FORMAT came back 0x1908
            // with GL_TEXTURE_DEPTH_SIZE 0, and sampling it returned 0.
            // GL_DEPTH_COMPONENT32F is the only destination ES 3.0 offers for
            // float depth, and here the type is evidence: there really are float
            // bits, and GL 4.6 sec. 8.5 lets the effective internal format of a
            // base internal format depend on format and type.
            if (*type == GL_FLOAT) {
                *internal_format = GL_DEPTH_COMPONENT32F;
            } else {
                *internal_format = GL_DEPTH_COMPONENT;
                *type = GL_UNSIGNED_INT;
            }
        } else if (type) {
            // An allocation-only glTexImage*. The type describes no bytes, so it
            // must not choose the storage class: the ordinary shadow-map
            // allocation is glTexImage2D(GL_DEPTH_COMPONENT, ..., GL_FLOAT, NULL)
            // with GL_FLOAT written out of habit, and honouring it there silently
            // makes the whole texture floating-point. That matters because
            // glTexSubImage2D never revisits this function and never inspects the
            // level, so every later fixed-point upload into a level turned 32F is
            // rejected by ES -- which binds GL_DEPTH_COMPONENT32F to GL_FLOAT
            // alone, where GL 4.6 converts -- and the rejection is invisible.
            // Measured on Mali-G77: the allocation and a following
            // glTexSubImage2D(GL_UNSIGNED_INT) both succeed as written here, and
            // resolving to 32F turned that second call into GL_INVALID_OPERATION
            // with the texture left holding its old contents.
            *internal_format = GL_DEPTH_COMPONENT;
            *type = GL_UNSIGNED_INT;
        } else {
            // glTexStorage*. There is no data and no type to be wrong about, and
            // the unsized form is rejected outright by both vendors -- it is not
            // in the sized-internalformat table that glTexStorage* requires.
            // Leaving it alone allocated no storage at all while the layer went
            // on recording the texture as complete.
            *internal_format = GL_DEPTH_COMPONENT24;
        }
        break;
    case GL_DEPTH_STENCIL:
        // GL_DEPTH_STENCIL is unsized, so the type is what says which sized
        // format the application's bytes actually are. Answering
        // GL_DEPTH32F_STENCIL8 for all of them described 4-byte
        // GL_UNSIGNED_INT_24_8 data as the 8-byte float-plus-pad layout, which
        // the driver rejects -- GLES 3.0 has GL_DEPTH24_STENCIL8 and it matches
        // GL_UNSIGNED_INT_24_8 exactly.
        if (type && *type == GL_FLOAT_32_UNSIGNED_INT_24_8_REV) {
            *internal_format = GL_DEPTH32F_STENCIL8;
        } else {
            *internal_format = GL_DEPTH24_STENCIL8;
            if (type) *type = GL_UNSIGNED_INT_24_8;
        }
        break;
    case GL_RGB10_A2:
        if (type) *type = GL_UNSIGNED_INT_2_10_10_10_REV;
        break;
    case GL_RGB5_A1:
        if (type) *type = GL_UNSIGNED_SHORT_5_5_5_1;
        break;
    case GL_COMPRESSED_RED_RGTC1:
    case GL_COMPRESSED_RG_RGTC2:
        LOG_E("GL_COMPRESSED_RED_RGTC1 or GL_COMPRESSED_RG_RGTC2 is not supported!");
        break;
    case GL_SRGB8:
        if (type) *type = GL_UNSIGNED_BYTE;
        break;
    case GL_RGBA32F:
    case GL_RGB32F:
        if (type) *type = GL_FLOAT;
        break;
    case GL_RGB9_E5:
        if (type) *type = GL_UNSIGNED_INT_5_9_9_9_REV;
        break;
    case GL_R11F_G11F_B10F:
        if (type) *type = GL_UNSIGNED_INT_10F_11F_11F_REV;
        if (format) *format = GL_RGB;
        break;
    case GL_RGBA32UI:
    case GL_RGB32UI:
        if (type) *type = GL_UNSIGNED_INT;
        break;
    case GL_RGBA32I:
    case GL_RGB32I:
        if (type) *type = GL_INT;
        break;
    case GL_RGBA16: {
        if (g_gles_caps.GL_EXT_texture_norm16) {
            if (type) *type = GL_UNSIGNED_SHORT;
        } else {
            *internal_format = GL_RGBA16F;
            if (type) *type = GL_FLOAT;
        }
        break;
    }
    case GL_RGBA8:
    case GL_RGBA:
        if (type) *type = GL_UNSIGNED_BYTE;
        if (format) *format = GL_RGBA;
        break;
    case GL_RGBA16F:
        if (type) *type = GL_HALF_FLOAT;
        break;
    // The three cases below asked no question and always answered with a float
    // format, so on a device that does have GL_EXT_texture_norm16 a plain 16-bit
    // unorm texture was still turned into a float one -- unlike GL_RGBA16 above,
    // which has always checked. Where the extension is missing the float
    // substitution stays: the enums no longer describe the application's shorts,
    // so the driver rejects the transfer and the upload is dropped rather than
    // stored wrong.
    case GL_R16: {
        if (g_gles_caps.GL_EXT_texture_norm16) {
            if (type) *type = GL_UNSIGNED_SHORT;
        } else {
            *internal_format = GL_R16F;
            if (type) *type = GL_FLOAT;
        }
        if (format) *format = GL_RED;
        break;
    }
    case GL_RGB16: {
        if (g_gles_caps.GL_EXT_texture_norm16) {
            if (type) *type = GL_UNSIGNED_SHORT;
        } else {
            *internal_format = GL_RGB16F;
            if (type) *type = GL_HALF_FLOAT;
        }
        if (format) *format = GL_RGB;
        break;
    }
    case GL_RGB16F:
        if (type) *type = GL_HALF_FLOAT;
        if (format) *format = GL_RGB;
        break;
    case GL_RG16: {
        if (g_gles_caps.GL_EXT_texture_norm16) {
            if (type) *type = GL_UNSIGNED_SHORT;
        } else {
            *internal_format = GL_RG16F;
            if (type) *type = GL_HALF_FLOAT;
        }
        if (format) *format = GL_RG;
        break;
    }
        // Inline R and RG channel mappings
    case GL_R8:
        if (format) *format = GL_RED;
        if (type) *type = GL_UNSIGNED_BYTE;
        break;
    case GL_R8_SNORM:
        if (format) *format = GL_RED;
        if (type) *type = GL_BYTE;
        break;
    case GL_R16F:
        if (format) *format = GL_RED;
        if (type) *type = GL_HALF_FLOAT;
        break;
    case GL_RED:
        if (type) {
            switch (*type) {
            case GL_UNSIGNED_BYTE:
                *internal_format = GL_R8;
                if (format) *format = GL_RED;
                break;
            case GL_BYTE:
                *internal_format = GL_R8_SNORM;
                if (format) *format = GL_RED;
                break;
            case GL_HALF_FLOAT:
                *internal_format = GL_R16F;
                if (format) *format = GL_RED;
                break;
            case GL_FLOAT:
                *internal_format = GL_R32F;
                if (format) *format = GL_RED;
                break;
            default:
                LOG_E("Unsupported type for GL_RED: %s", glEnumToString(*type));
                if (type) *type = GL_UNSIGNED_BYTE; // Fallback to unsigned byte
                *internal_format = GL_R8;           // Fallback to R8
                if (format) *format = GL_RED;
                break;
            }
        }
        break;
    case GL_R8UI:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_UNSIGNED_BYTE;
        break;
    case GL_R8I:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_BYTE;
        break;
    case GL_R16UI:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_UNSIGNED_SHORT;
        break;
    case GL_R16I:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_SHORT;
        break;
    case GL_R32UI:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_UNSIGNED_INT;
        break;
    case GL_R32I:
        if (format) *format = GL_RED_INTEGER;
        if (type) *type = GL_INT;
        break;
    case GL_RG8:
        if (format) *format = GL_RG;
        if (type) *type = GL_UNSIGNED_BYTE;
        break;
    case GL_RG8_SNORM:
        if (format) *format = GL_RG;
        if (type) *type = GL_BYTE;
        break;
    case GL_RG16F:
        if (format) *format = GL_RG;
        if (type) *type = GL_HALF_FLOAT;
        break;
    case GL_RG32F:
        if (format) *format = GL_RG;
        if (type) *type = GL_FLOAT;
        break;
    case GL_RG8UI:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_UNSIGNED_BYTE;
        break;
    case GL_RG8I:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_BYTE;
        break;
    case GL_RG16UI:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_UNSIGNED_SHORT;
        break;
    case GL_RG16I:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_SHORT;
        break;
    case GL_RG32UI:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_UNSIGNED_INT;
        break;
    case GL_RG32I:
        if (format) *format = GL_RG_INTEGER;
        if (type) *type = GL_INT;
        break;
    case GL_R32F:
        if (format) *format = GL_RED;
        if (type) *type = GL_FLOAT;
        break;
    case GL_RGBA8_SNORM:
        if (format) *format = GL_RGBA;
        if (type) *type = GL_BYTE;
        // This case has never had a break; it used to fall into default:, whose
        // two tests both miss GL_RGBA8_SNORM, so the values above survived. The
        // GL_RGB case below was inserted between the two and silently became the
        // fallthrough target, rewriting them to GL_RGB + GL_UNSIGNED_BYTE.
        break;
    // The unsized spelling of the same thing, and the one classic desktop code
    // uses: glTexImage2D(GL_RGB, ..., GL_RGBA, GL_UNSIGNED_BYTE, rgba) is legal
    // GL, which drops the alpha during conversion. Only GL_RGB8 was recognised,
    // so unsized GL_RGB kept format GL_RGBA, ES rejected the pair as an illegal
    // triple, and the level was never defined -- the texture then sampled black.
    case GL_RGB:
        if (format) *format = GL_RGB;
        if (type && *type != GL_UNSIGNED_BYTE && *type != GL_UNSIGNED_SHORT_5_6_5) *type = GL_UNSIGNED_BYTE;
        break;
    default:
        // fallback handling for GL_RGB8, GL_RGBA16_SNORM etc.
        if (*internal_format == GL_RGB8) {
            if (type && *type != GL_UNSIGNED_BYTE) *type = GL_UNSIGNED_BYTE;
            if (format) *format = GL_RGB;
        } else if (*internal_format == GL_RGBA16_SNORM) {
            if (type && *type != GL_SHORT) *type = GL_SHORT;
        }
        break;
    }
}

// Record one of the three sampling pnames this layer shadows on the
// texture object. Answers false for a value the driver would reject
// (GL_INVALID_ENUM: state unchanged there, so unchanged here too) and for
// every other pname, which is the caller's cue to just forward.
static bool record_texture_sampling_param(TextureObject* tex, GLenum pname, GLint param) {
    if (!tex || !mg_filter_value_legal(pname, param)) return false;
    switch (pname) {
    case GL_TEXTURE_MIN_FILTER:
        tex->min_filter = param;
        return true;
    case GL_TEXTURE_MAG_FILTER:
        tex->mag_filter = param;
        return true;
    case GL_TEXTURE_COMPARE_MODE:
        tex->compare_mode = param;
        return true;
    default:
        return false;
    }
}

// Push the recorded sampling state to the driver for the texture bound to
// `target`. The application's request lives in the record; the driver gets
// the depth-legal form of the filters while the object is a sized depth or
// depth-stencil image with comparison off, because that pairing is
// filter-incomplete on strict ES hosts and samples as (0, 0, 0, 1)
// (gl/depth_filter.h). Comparison mode itself is forwarded verbatim:
// compare-enabled pairings are exempt from the rule.
static void push_depth_sampling_state(GLenum target, const TextureObject& tex) {
    const bool clamp = mg_is_sized_depth_format(tex.internal_format) && tex.compare_mode == GL_NONE;
    GLES.glTexParameteri(target, GL_TEXTURE_MIN_FILTER,
                         clamp ? mg_depth_filter_make_legal(GL_TEXTURE_MIN_FILTER, tex.min_filter) : tex.min_filter);
    GLES.glTexParameteri(target, GL_TEXTURE_MAG_FILTER,
                         clamp ? mg_depth_filter_make_legal(GL_TEXTURE_MAG_FILTER, tex.mag_filter) : tex.mag_filter);
    GLES.glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, tex.compare_mode);
}

// Record sampler-object sampling state. Never rewritten on the way out:
// one sampler object can serve colour and depth units at once (Minecraft's
// SamplerCache does), so only the per-unit scan at draw time decides what
// a pairing may keep, and the driver's value stays the application's
// value. A value the driver rejects stays rejected here too.
static void record_sampler_sampling_param(GLuint sampler, GLenum pname, GLint param) {
    if (sampler == 0 || !mg_filter_value_legal(pname, param)) return;
    if (sampler >= SamplerShadowTable.size()) SamplerShadowTable.resize(sampler + 1);
    sampler_shadow_t& s = SamplerShadowTable[sampler];
    switch (pname) {
    case GL_TEXTURE_MIN_FILTER:
        s.min_filter = param;
        return;
    case GL_TEXTURE_MAG_FILTER:
        s.mag_filter = param;
        return;
    case GL_TEXTURE_COMPARE_MODE:
        s.compare_mode = param;
        return;
    default:
        return;
    }
}

void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
    LOG()
    pname = pname_convert(pname);
    LOG_D("glTexParameterf, target: %d, pname: %d, param: %f", target, pname, param)

    if (pname == GL_TEXTURE_LOD_BIAS_QCOM && !g_gles_caps.GL_QCOM_texture_lod_bias) {
        LOG_D("Does not support GL_QCOM_texture_lod_bias, skipped!")
        return;
    }

    TextureObject* tex = mgGetTexObjectByTarget(target);
    const bool recorded = record_texture_sampling_param(tex, pname, (GLint)param);
    if (recorded && mg_is_sized_depth_format(tex->internal_format)) {
        push_depth_sampling_state(target, *tex);
    } else {
        GLES.glTexParameterf(target, pname, param);
    }
    CHECK_GL_ERROR
}

#define GET_TEXTURE_OBJECT(target)                                                                                     \
    unsigned __currentUnitIndex = GetCurrentTextureUnitIndex();                                                        \
    auto& __currentUnit = GetTextureUnit(__currentUnitIndex);                                                          \
    auto targetR = ConvertGLEnumToTextureTarget(target);                                                               \
    if (targetR == TextureTarget::UNKNWON) {                                                                           \
        LOG_E("%s: Unknown texture target: %s", __func__, glEnumToString(target))                                      \
        return;                                                                                                        \
    }                                                                                                                  \
    auto& __bindingSlot = __currentUnit.GetBindingSlot(targetR);                                                       \
    auto tex = __bindingSlot.GetBoundObject()

void glTexImage1D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLenum format,
                  GLenum type, const GLvoid* pixels) {
    LOG()
    LOG_D("glTexImage1D not implemented!")
    // Not implemented: no GLES storage call is ever issued, only the shadow
    // TextureObject fields below get written. That used to be a LOG_D and
    // nothing else -- invisible in a release build, and with glGetError
    // answering GL_NO_ERROR the application had every reason to believe its
    // level existed. Sampling it reads an incomplete texture instead.
    mg_set_gl_error(GL_INVALID_OPERATION);
    LOG_D("glTexImage1D, target: %d, level: %d, internalFormat: %d, width: %d, "
          "border: %d, format: %d, type: %d",
          target, level, internalFormat, width, border, format, type)
    internal_convert(reinterpret_cast<GLenum*>(&internalFormat), &type, &format, mg_upload_has_data(pixels));

    GLenum rtarget = map_tex_target(target);
    if (rtarget == GL_PROXY_TEXTURE_1D) {
        int max1 = 4096;
        GLES.glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max1);
        set_gl_state_proxy_width(((width << level) > max1) ? 0 : width);
        set_gl_state_proxy_intformat(internalFormat);
        return;
    }

    GET_TEXTURE_OBJECT(target);
    tex->target = ConvertGLEnumToTextureTarget(target);
    tex->depth = 1;
    tex->format = format;
    tex->internal_format = internalFormat;
    tex->width = width;
    tex->height = 1;
    tex->swizzle_param[0] = GL_RED;
    tex->swizzle_param[1] = GL_GREEN;
    tex->swizzle_param[2] = GL_BLUE;
    tex->swizzle_param[3] = GL_ALPHA;

    CHECK_GL_ERROR
}

void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type, const GLvoid* pixels) {
    LOG()
    LOG_D("mg_glTexImage2D,target: %s,level: %d,internalFormat: %s->%s,width: "
          "%d,height: %d,border: %d,format: %s,type: %s, pixels: 0x%x",
          glEnumToString(target), level, glEnumToString(internalFormat), glEnumToString(internalFormat), width, height,
          border, glEnumToString(format), glEnumToString(type), pixels)
    // internal_convert is asked first, on copies, and only then is the data
    // converted to match. It rewrites the client format and type from the
    // internalformat alone, without touching the bytes -- so running it last let
    // the enum outrun the data: a three-channel stream relabelled GL_RGBA had the
    // driver read four bytes per pixel out of a three-byte-per-pixel buffer.
    //
    // With data present, only the format is adopted from it, and only because the
    // conversion below is told to emit that many channels. The type always comes
    // from the conversion, which is the one thing that knows what the bytes are.
    // An allocation has no bytes to describe, so there both are adopted.
    GLenum want_if = static_cast<GLenum>(internalFormat), want_fmt = format, want_type = type;
    internal_convert(&want_if, &want_type, &want_fmt, mg_upload_has_data(pixels));
    internalFormat = static_cast<GLint>(want_if);

    mg_upload_fix_t fix(width, height, 1, format, type, pixels, want_fmt, /*three_d=*/false);
    // A conversion this layer refused -- a source it could not map, or dimensions
    // whose product does not fit in memory -- means the call has already raised its
    // error and must not go on to define the level. Without this, `pixels` having
    // been nulled turned the drop into an allocation with undefined contents, which
    // is not what GL does after an error: it does nothing.
    if (fix.dropped()) {
        CHECK_GL_ERROR
        return;
    }
    if (fix.has_data()) {
        format = fix.format;
        type = fix.type;
    } else {
        format = want_fmt;
        type = want_type;
    }

    LOG_D("GLES.glTexImage2D,target: %s,level: %d,internalFormat: %s->%s,width: "
          "%d,height: %d,border: %d,format: %s,type: %s, pixels: 0x%x",
          glEnumToString(target), level, glEnumToString(internalFormat), glEnumToString(internalFormat), width, height,
          border, glEnumToString(format), glEnumToString(type), pixels)
    GLenum rtarget = map_tex_target(target);
    if (rtarget == GL_PROXY_TEXTURE_2D) {
        int max1 = 4096;
        GLES.glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max1);
        set_gl_state_proxy_width(((width << level) > max1) ? 0 : width);
        set_gl_state_proxy_height(((height << level) > max1) ? 0 : height);
        set_gl_state_proxy_intformat(internalFormat);
        return;
    }

    GET_TEXTURE_OBJECT(target);
    tex->target = ConvertGLEnumToTextureTarget(target);
    tex->internal_format = internalFormat;
    tex->width = width;
    tex->height = height;
    tex->depth = 1;
    tex->swizzle_param[0] = GL_RED;
    tex->swizzle_param[1] = GL_GREEN;
    tex->swizzle_param[2] = GL_BLUE;
    tex->swizzle_param[3] = GL_ALPHA;

    tex->format = format;

    GLES.glTexImage2D(target, level, internalFormat, width, height, border, format, type, fix.pixels);
    // Storage now exists: a sized depth level gets the depth-legal view of
    // the recorded filters, because GL's own defaults are not legal for it
    // (min NEAREST_MIPMAP_LINEAR, mag LINEAR).
    if (mg_is_sized_depth_format(tex->internal_format)) push_depth_sampling_state(target, *tex);

    CHECK_GL_ERROR
}

void glTexImage3D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth,
                  GLint border, GLenum format, GLenum type, const GLvoid* pixels) {
    LOG()
    LOG_D("glTexImage3D, target: 0x%x, level: %d, internalFormat: 0x%x, width: "
          "0x%x, height: %d, depth: %d, border: %d, format: 0x%x, type: %d",
          target, level, internalFormat, width, height, depth, border, format, type)

    // Same ordering as glTexImage2D; see the note there.
    GLenum want_if = static_cast<GLenum>(internalFormat), want_fmt = format, want_type = type;
    internal_convert(&want_if, &want_type, &want_fmt, mg_upload_has_data(pixels));
    internalFormat = static_cast<GLint>(want_if);

    mg_upload_fix_t fix(width, height, depth, format, type, pixels, want_fmt);
    // A conversion this layer refused -- a source it could not map, or dimensions
    // whose product does not fit in memory -- means the call has already raised its
    // error and must not go on to define the level. Without this, `pixels` having
    // been nulled turned the drop into an allocation with undefined contents, which
    // is not what GL does after an error: it does nothing.
    if (fix.dropped()) {
        CHECK_GL_ERROR
        return;
    }
    if (fix.has_data()) {
        format = fix.format;
        type = fix.type;
    } else {
        format = want_fmt;
        type = want_type;
    }
    GLenum rtarget = map_tex_target(target);
    if (rtarget == GL_PROXY_TEXTURE_3D) {
        int max1 = 4096;
        GLES.glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max1);
        set_gl_state_proxy_width(((width << level) > max1) ? 0 : width);
        set_gl_state_proxy_height(((height << level) > max1) ? 0 : height);
        // set_gl_state_proxy_depth(((depth << level) > max1) ? 0 : depth);
        set_gl_state_proxy_intformat(internalFormat);
        return;
    }

    GLES.glTexImage3D(target, level, internalFormat, width, height, depth, border, format, type, fix.pixels);

    GET_TEXTURE_OBJECT(target);
    tex->target = ConvertGLEnumToTextureTarget(target);
    tex->internal_format = internalFormat;
    tex->width = width;
    tex->height = height;
    tex->depth = depth;
    tex->swizzle_param[0] = GL_RED;
    tex->swizzle_param[1] = GL_GREEN;
    tex->swizzle_param[2] = GL_BLUE;
    tex->swizzle_param[3] = GL_ALPHA;

    if (mg_is_sized_depth_format(tex->internal_format)) push_depth_sampling_state(target, *tex);

    CHECK_GL_ERROR
}

void glTexStorage1D(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width) {
    LOG()
    LOG_D("glTexStorage1D not implemented!")
    // Not implemented: no GLES storage call is ever issued, only the shadow
    // TextureObject fields below get written. That used to be a LOG_D and
    // nothing else -- invisible in a release build, and with glGetError
    // answering GL_NO_ERROR the application had every reason to believe its
    // level existed. Sampling it reads an incomplete texture instead.
    mg_set_gl_error(GL_INVALID_OPERATION);
    LOG_D("glTexStorage1D, target: %d, levels: %d, internalFormat: %d, width: %d", target, levels, internalFormat,
          width)
    internal_convert(&internalFormat, nullptr, nullptr, /*has_data=*/false);

    GET_TEXTURE_OBJECT(target);
    tex->target = ConvertGLEnumToTextureTarget(target);
    tex->internal_format = internalFormat;
    tex->width = width;
    tex->height = 1;
    tex->depth = 1;
    tex->swizzle_param[0] = GL_RED;
    tex->swizzle_param[1] = GL_GREEN;
    tex->swizzle_param[2] = GL_BLUE;
    tex->swizzle_param[3] = GL_ALPHA;

    CHECK_GL_ERROR
}

void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height) {
    LOG()
    LOG_D("glTexStorage2D, target: %d, levels: %d, internalFormat: %d, width: "
          "%d, height: %d",
          target, levels, internalFormat, width, height)

    internal_convert(&internalFormat, nullptr, nullptr, /*has_data=*/false);
    GLES.glTexStorage2D(target, levels, internalFormat, width, height);

    GET_TEXTURE_OBJECT(target);
    tex->target = ConvertGLEnumToTextureTarget(target);
    tex->internal_format = internalFormat;
    tex->width = width;
    tex->height = height;
    tex->depth = 1;
    tex->swizzle_param[0] = GL_RED;
    tex->swizzle_param[1] = GL_GREEN;
    tex->swizzle_param[2] = GL_BLUE;
    tex->swizzle_param[3] = GL_ALPHA;

    if (mg_is_sized_depth_format(tex->internal_format)) push_depth_sampling_state(target, *tex);

    GLenum ERR = GLES.glGetError();
    if (ERR != GL_NO_ERROR) LOG_E("glTexStorage2D ERROR: %d", ERR)
}

void glTexStorage3D(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height,
                    GLsizei depth) {
    LOG()
    LOG_D("glTexStorage3D, target: %d, levels: %d, internalFormat: %d, width: "
          "%d, height: %d, depth: %d",
          target, levels, internalFormat, width, height, depth)

    internal_convert(&internalFormat, nullptr, nullptr, /*has_data=*/false);

    GLES.glTexStorage3D(target, levels, internalFormat, width, height, depth);

    GET_TEXTURE_OBJECT(target);
    tex->target = ConvertGLEnumToTextureTarget(target);
    tex->internal_format = internalFormat;
    tex->width = width;
    tex->height = height;
    tex->depth = depth;
    tex->swizzle_param[0] = GL_RED;
    tex->swizzle_param[1] = GL_GREEN;
    tex->swizzle_param[2] = GL_BLUE;
    tex->swizzle_param[3] = GL_ALPHA;

    if (mg_is_sized_depth_format(tex->internal_format)) push_depth_sampling_state(target, *tex);

    CHECK_GL_ERROR
}

void glCopyTexImage1D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width,
                      GLint border) {
    LOG()
    LOG_D("glCopyTexImage1D not implemented!")
    // Not implemented: no GLES storage call is ever issued, only the shadow
    // TextureObject fields below get written. That used to be a LOG_D and
    // nothing else -- invisible in a release build, and with glGetError
    // answering GL_NO_ERROR the application had every reason to believe its
    // level existed. Sampling it reads an incomplete texture instead.
    mg_set_gl_error(GL_INVALID_OPERATION);
    LOG_D("glCopyTexImage1D, target: %d, level: %d, internalFormat: %d, x: %d, "
          "y: %d, width: %d, border: %d",
          target, level, internalFormat, x, y, width, border)

    GET_TEXTURE_OBJECT(target);
    tex->target = ConvertGLEnumToTextureTarget(target);
    tex->internal_format = internalFormat;
    tex->width = width;
    tex->height = 1;
    tex->depth = 1;
    tex->swizzle_param[0] = GL_RED;
    tex->swizzle_param[1] = GL_GREEN;
    tex->swizzle_param[2] = GL_BLUE;
    tex->swizzle_param[3] = GL_ALPHA;

    CHECK_GL_ERROR
}

// Depth without stencil. GL_DEPTH_COMPONENT32 belongs here even though no GLES
// driver accepts it as an internalformat: this predicate is fed by
// GL_TEXTURE_INTERNAL_FORMAT, and a level created from the unsized
// GL_DEPTH_COMPONENT is reported back as exactly GL_DEPTH_COMPONENT32 by Mali.
// Leaving it out sent those levels down the colour path, where
// glCopyTexSubImage2D silently copied nothing.
static int is_depth_format(GLenum format) {
    switch (format) {
    case GL_DEPTH_COMPONENT:
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32:
    case GL_DEPTH_COMPONENT32F:
        return 1;
    default:
        return 0;
    }
}

// Combined depth+stencil. Kept apart from is_depth_format() because these need a
// different attachment point and a different blit mask -- treating them as plain
// depth attaches only half the texture and drops the stencil half on the floor.
static int is_depth_stencil_format(GLenum format) {
    switch (format) {
    case GL_DEPTH_STENCIL:
    case GL_DEPTH24_STENCIL8:
    case GL_DEPTH32F_STENCIL8:
        return 1;
    default:
        return 0;
    }
}

static GLenum get_binding_for_target(GLenum target) {
    switch (target) {
    case GL_TEXTURE_2D:
        return GL_TEXTURE_BINDING_2D;
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        return GL_TEXTURE_BINDING_CUBE_MAP;
    default:
        return 0;
    }
}

void glCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width,
                      GLsizei height, GLint border) {
    LOG()

    // The source is the read framebuffer; under FSR1 that has to be the render
    // target, not the surface. Matters most on the depth path below, whose depth
    // buffer lives on the FSR1 target and was never on the surface at all.
    mg_fsr_read_scope_t fsr_read;

    INIT_CHECK_GL_ERROR

    // This call *defines* the level, so the internalformat it is given is the
    // caller's to choose. Overwriting it with the destination's own meant the
    // ordinary first call -- where the level does not exist yet and the query
    // answers 0 -- threw that choice away and always took the colour path below,
    // however the application had asked for the level to be created.
    //
    // The query is still worth asking for the one case it was protecting: an
    // application re-copying into a level that is already a depth texture needs
    // the blit path, and glCopyTexImage2D cannot reach it from a colour enum.
    GLint existingInternalFormat = 0;
    GLES.glGetTexLevelParameteriv(target, level, GL_TEXTURE_INTERNAL_FORMAT, &existingInternalFormat);
    if (!is_depth_format(internalFormat) && is_depth_format((GLenum)existingInternalFormat)) {
        internalFormat = (GLenum)existingInternalFormat;
    }

    LOG_D("glCopyTexImage2D, target: %d, level: %d, internalFormat: %d, x: %d, "
          "y: %d, width: %d, height: %d, border: %d",
          target, level, internalFormat, x, y, width, height, border)

    if (is_depth_format(internalFormat)) {
        GLenum format = GL_DEPTH_COMPONENT;
        GLenum type = GL_UNSIGNED_INT;
        internal_convert(&internalFormat, &type, &format, /*has_data=*/false);
        GLES.glTexImage2D(target, level, (GLint)internalFormat, width, height, border, format, type, nullptr);
        CHECK_GL_ERROR_NO_INIT
        GLint prevDrawFBO;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDrawFBO);
        CHECK_GL_ERROR_NO_INIT
        GLuint tempDrawFBO;
        glGenFramebuffers(1, &tempDrawFBO);
        CHECK_GL_ERROR_NO_INIT
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tempDrawFBO);
        CHECK_GL_ERROR_NO_INIT
        GLint currentTex;
        glGetIntegerv(get_binding_for_target(target), &currentTex);
        CHECK_GL_ERROR_NO_INIT
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, currentTex, level);
        CHECK_GL_ERROR_NO_INIT

        if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            CHECK_GL_ERROR_NO_INIT
            glDeleteFramebuffers(1, &tempDrawFBO);
            CHECK_GL_ERROR_NO_INIT
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
            CHECK_GL_ERROR_NO_INIT
            return;
        }
        CHECK_GL_ERROR_NO_INIT

        // Flush before reading depth through a blit -- see the note in
        // glCopyTexSubImage2D for the Adreno measurement behind this.
        if (GLES.glFlush != nullptr) GLES.glFlush();
        GLES.glBlitFramebuffer(x, y, x + width, y + height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        CHECK_GL_ERROR_NO_INIT

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
        CHECK_GL_ERROR_NO_INIT
        glDeleteFramebuffers(1, &tempDrawFBO);
        CHECK_GL_ERROR_NO_INIT
    } else {
        GLES.glCopyTexImage2D(target, level, internalFormat, x, y, width, height, border);
        CHECK_GL_ERROR_NO_INIT
    }

    GET_TEXTURE_OBJECT(target);
    tex->target = ConvertGLEnumToTextureTarget(target);
    tex->internal_format = internalFormat;
    tex->width = width;
    tex->height = height;
    tex->depth = 1;
    tex->swizzle_param[0] = GL_RED;
    tex->swizzle_param[1] = GL_GREEN;
    tex->swizzle_param[2] = GL_BLUE;
    tex->swizzle_param[3] = GL_ALPHA;

    if (mg_is_sized_depth_format(tex->internal_format)) push_depth_sampling_state(target, *tex);

    CHECK_GL_ERROR_NO_INIT
}

void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width,
                         GLsizei height) {
    LOG()
    // Same as glCopyTexImage2D: read from where the frame really is.
    mg_fsr_read_scope_t fsr_read;

    // The only thing this format decides is depth versus colour, and every entry
    // point that allocates a level records the format on the object. Asking the
    // driver instead is a synchronous round trip on a call Minecraft makes per
    // frame, so ask it only when there is nothing recorded to read.
    //
    // The record is not per level, and this call is. Levels of one texture can in
    // principle be given different formats through glTexImage2D, but not a
    // different depth-ness: a depth level and a colour level in the same texture
    // is not a texture any driver will sample.
    // mgGetTexObjectByTarget indexes the binding slots by the converted enum and
    // does not range-check it, and TextureTarget::UNKNWON is past the end.
    GLint internalFormat = 0;
    TextureObject* copy_dst =
        ConvertGLEnumToTextureTarget(target) != TextureTarget::UNKNWON ? mgGetTexObjectByTarget(target) : nullptr;
    if (copy_dst && copy_dst->internal_format != 0) {
        internalFormat = (GLint)copy_dst->internal_format;
    } else {
        GLES.glGetTexLevelParameteriv(target, level, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
        if (copy_dst) copy_dst->internal_format = (GLenum)internalFormat;
    }

    LOG_D("glCopyTexSubImage2D, target: %s, level: %d, xoffset: %d, yoffset: %d, "
          "x: %d, y: %d, width: %d, height: %d",
          glEnumToString(target), level, xoffset, yoffset, x, y, width, height)

    const int depth_stencil = is_depth_stencil_format((GLenum)internalFormat);
    if (depth_stencil || is_depth_format((GLenum)internalFormat)) {
        const GLenum attachment = depth_stencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
        const GLbitfield mask = depth_stencil ? (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT) : GL_DEPTH_BUFFER_BIT;

        // The read framebuffer is the source and is left exactly as the caller
        // had it; only the draw binding is borrowed and put back.
        GLint prevDrawFBO;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDrawFBO);

        GLuint tempDrawFBO;
        glGenFramebuffers(1, &tempDrawFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tempDrawFBO);

        GLint currentTex;
        glGetIntegerv(get_binding_for_target(target), &currentTex);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, target, currentTex, level);

        if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            TX_WARN_ONCE("glCopyTexSubImage2D: depth destination (internalformat 0x%04X) will not make a complete "
                         "framebuffer; the copy was skipped",
                         (unsigned)internalFormat);
            glDeleteFramebuffers(1, &tempDrawFBO);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
            return;
        }

        // glBlitFramebuffer is far stricter than glCopyTexSubImage2D: for the
        // depth and stencil bits GLES demands the source and destination formats
        // be identical, where GL 4.6 converts. A depth-only texture cannot be
        // filled from a depth+stencil framebuffer, nor the reverse, and the
        // failure is invisible from here -- this layer's glGetError never
        // reports. Say so once rather than copying nothing in silence.
        if (GLES.glGetError != nullptr) {
            while (GLES.glGetError() != GL_NO_ERROR) {
            }
        }
        // The flush is load-bearing. Adreno keeps the source's pending depth
        // writes in an unsubmitted tile pass, and this blit reads the buffer as
        // it is in memory -- for the first depth blit from a fresh default
        // framebuffer that is garbage, silently, with no error raised. Measured
        // on Adreno 750: without this, the first copy in a context reads junk
        // (two back-to-back blits BOTH read junk, so retrying is no fix) while a
        // single glFlush beforehand makes the same copy read the true values,
        // reproducibly. glFinish also works but synchronizes the CPU; the flush
        // is enough. Mali needs neither and is unaffected. Copies are rare
        // operations, so one flush here is cheap.
        if (GLES.glFlush != nullptr) GLES.glFlush();
        GLES.glBlitFramebuffer(x, y, x + width, y + height, xoffset, yoffset, xoffset + width, yoffset + height, mask,
                               GL_NEAREST);
        if (GLES.glGetError != nullptr && GLES.glGetError() != GL_NO_ERROR) {
            TX_WARN_ONCE("glCopyTexSubImage2D: the driver refused a depth blit into internalformat 0x%04X -- GLES "
                         "requires the source framebuffer to have exactly the same depth/stencil format, so nothing "
                         "was copied",
                         (unsigned)internalFormat);
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
        glDeleteFramebuffers(1, &tempDrawFBO);

    } else {
        GLES.glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
    }

    CHECK_GL_ERROR
}

void glRenderbufferStorage(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height) {
    LOG()

    INIT_CHECK_GL_ERROR_FORCE

    LOG_D("glRenderbufferStorage, target: %s, internalFormat: %s, width: %d, "
          "height: %d",
          glEnumToString(target), glEnumToString(internalFormat), width, height)

    GLES.glRenderbufferStorage(target, internalFormat, width, height);

    CHECK_GL_ERROR_NO_INIT
}

void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width,
                                      GLsizei height) {
    LOG()

    INIT_CHECK_GL_ERROR_FORCE

    LOG_D("glRenderbufferStorageMultisample, target: %d, samples: %d, "
          "internalFormat: %d, width: %d, height: %d",
          target, samples, internalFormat, width, height)

    GLES.glRenderbufferStorageMultisample(target, samples, internalFormat, width, height);

    CHECK_GL_ERROR_NO_INIT
}

void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat* params) {
    LOG()
    LOG_D("glGetTexLevelParameterfv,target: %d, level: %d, pname: %d", target, level, pname)
    if (gl_state) {
        GLenum rtarget = map_tex_target(target);
        if (rtarget == GL_PROXY_TEXTURE_2D) {
            switch (pname) {
            case GL_TEXTURE_WIDTH:
                (*params) = (float)nlevel(gl_state->proxy_width, level);
                return;
            case GL_TEXTURE_HEIGHT:
                (*params) = (float)nlevel(gl_state->proxy_height, level);
                return;
            case GL_TEXTURE_INTERNAL_FORMAT:
                (*params) = (float)gl_state->proxy_intformat;
                return;
            default:
                return;
            }
        }
    }
    GLES.glGetTexLevelParameterfv(target, level, pname, params);
    CHECK_GL_ERROR
}

void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params) {
    LOG()
    LOG_D("glGetTexLevelParameteriv,target: %s, level: %d, pname: %s", glEnumToString(target), level,
          glEnumToString(pname))
    if (gl_state) {
        GLenum rtarget = map_tex_target(target);
        if (rtarget == GL_PROXY_TEXTURE_2D) {
            switch (pname) {
            case GL_TEXTURE_WIDTH:
                (*params) = nlevel(gl_state->proxy_width, level);
                return;
            case GL_TEXTURE_HEIGHT:
                (*params) = nlevel(gl_state->proxy_height, level);
                return;
            case GL_TEXTURE_INTERNAL_FORMAT:
                (*params) = (GLint)gl_state->proxy_intformat;
                return;
            default:
                return;
            }
        }
    }
    LOG_D("es.glGetTexLevelParameteriv,target: %s, level: %d, pname: %s", glEnumToString(target), level,
          glEnumToString(pname))
    GLES.glGetTexLevelParameteriv(target, level, pname, params);
    CHECK_GL_ERROR
}

void glTexParameteriv(GLenum target, GLenum pname, const GLint* params) {
    LOG()
    LOG_D("glTexParameteriv, target: %s, pname: %s", glEnumToString(target), glEnumToString(pname))

    if (pname == GL_TEXTURE_SWIZZLE_RGBA) {
        LOG_D("find GL_TEXTURE_SWIZZLE_RGBA, now use glTexParameteri")
        if (params) {
            // deferred those call to draw call?
            GLES.glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, params[0]);
            GLES.glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, params[1]);
            GLES.glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, params[2]);
            GLES.glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, params[3]);

            // save states for now
            GET_TEXTURE_OBJECT(target);
            tex->swizzle_param[0] = params[0];
            tex->swizzle_param[1] = params[1];
            tex->swizzle_param[2] = params[2];
            tex->swizzle_param[3] = params[3];
        } else {
            LOG_E("glTexParameteriv: params is nullptr for GL_TEXTURE_SWIZZLE_RGBA")
        }
    } else if (params) {
        TextureObject* tex = mgGetTexObjectByTarget(target);
        const bool recorded = record_texture_sampling_param(tex, pname, params[0]);
        if (recorded && mg_is_sized_depth_format(tex->internal_format)) {
            push_depth_sampling_state(target, *tex);
        } else {
            GLES.glTexParameteriv(target, pname, params);
        }
    } else {
        GLES.glTexParameteriv(target, pname, params);
    }

    CHECK_GL_ERROR
}

// Was a straight NATIVE forward until the depth-completeness fix needed
// the record these calls maintain. Filter pnames behave exactly like
// glTexParameteriv above; everything else (swizzle included) still reaches
// the driver as float parameters, which ES accepts.
void glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params) {
    LOG()
    LOG_D("glTexParameterfv, target: %s, pname: %s", glEnumToString(target), glEnumToString(pname))
    if (params) {
        TextureObject* tex = mgGetTexObjectByTarget(target);
        const bool recorded = record_texture_sampling_param(tex, pname, (GLint)*params);
        if (recorded && mg_is_sized_depth_format(tex->internal_format)) {
            push_depth_sampling_state(target, *tex);
            CHECK_GL_ERROR
            return;
        }
    }
    GLES.glTexParameterfv(target, pname, params);
    CHECK_GL_ERROR
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                     GLenum format, GLenum type, const void* pixels) {
    LOG()

    LOG_D("glTexSubImage2D, target = %s, level = %d, xoffset = %d, yoffset = %d, "
          "width = %d, height = %d, format = %s, type = %s, pixels = 0x%x",
          glEnumToString(target), level, xoffset, yoffset, width, height, glEnumToString(format), glEnumToString(type),
          pixels)

    // A sub-upload must convert its data like any other transfer. The previous
    // code set a texture swizzle here instead -- sampling state, permanently
    // changed as a side effect of an upload, wrong the moment the application
    // uploads RGBA to the same texture, renders into it, or swizzles it itself.
    mg_upload_fix_t fix(width, height, 1, format, type, pixels, /*want_format=*/0, /*three_d=*/false);
    // A dropped conversion leaves pixels null, and glTexSubImage2D has no
    // allocate-only form: with the unpack buffer unbound the driver would read
    // client memory from address zero.
    if (fix.dropped()) {
        CHECK_GL_ERROR
        return;
    }

    GLES.glTexSubImage2D(target, level, xoffset, yoffset, width, height, fix.format, fix.type, fix.pixels);

    CHECK_GL_ERROR
}

void glBindTexture(GLenum target, GLuint texture) {
    LOG()
    LOG_D("glBindTexture(%s, %d)", glEnumToString(target), texture)
    INIT_CHECK_GL_ERROR

    int currentUnitIndex = GetCurrentTextureUnitIndex();
    auto& currentUnit = GetTextureUnit(currentUnitIndex);
    auto targetR = ConvertGLEnumToTextureTarget(target);

    const bool emulated_buffer_texture =
        hardware && gl_state && hardware->emulate_texture_buffer && target == GL_TEXTURE_BUFFER;
    // Where the driver actually keeps this binding, which is not where the
    // application put it: the emulation parks the buffer texture on unit 15's
    // GL_TEXTURE_2D. The plain branch lands on whichever unit the driver has
    // active, which is the driver's own count and not GetCurrentTextureUnitIndex.
    const int driver_unit = emulated_buffer_texture ? MG_TEXTURE_BUFFER_EMULATION_UNIT : DriverActiveTextureUnit;
    const TextureTarget driver_target = emulated_buffer_texture ? TextureTarget::TEXTURE_2D : targetR;

    // Rebinding what is already bound is the single most common redundant call
    // Minecraft makes, and only the driver call is skipped -- everything below
    // still runs, because the slot record and TextureObject::target are this
    // layer's own state and a caller may be re-binding to correct them.
    //
    // Both halves are required. The frontend half alone would miss the emulation
    // above writing a driver slot the application never named, and would trust a
    // slot record that survived a driver-side change. The driver half alone would
    // skip the bind that repairs a slot record, and would confuse the buffer
    // texture on unit 15 with an ordinary 2D texture bound there.
    //
    // Who else moves driver texture bindings through GLES.*, and whether it nets
    // to zero: gl/buffer.cpp's glTexBuffer borrows unit 15, reads its
    // GL_TEXTURE_BINDING_2D and rebinds the value it read, restoring the active
    // unit on all three of its exits -- zero. gl/drawing.cpp's
    // setupBufferTextureUniforms only reads unit 15 and restores the active unit --
    // zero. This function's own emulation branch is recorded below rather than
    // left to net out. FSR1's GLStateGuard does not net to zero, which is what
    // driver_texture_shadow_trustworthy() is for -- as is the shared fallback
    // record, whose values belong to no context in particular. gl/gl.cpp's
    // depth-clear triangle and the multidraw backends touch no texture state at
    // all; bench/ is a separate program.
    bool redundant = false;
    if (targetR != TextureTarget::UNKNWON && driver_texture_shadow_trustworthy()) {
        const TextureObject* bound = currentUnit.GetBindingSlot(targetR).GetBoundObject();
        redundant = bound != nullptr && bound->texture == texture &&
                    get_driver_texture_binding(driver_unit, driver_target) == texture;
    }

    if (!redundant) {
        if (emulated_buffer_texture) {
            GLES.glActiveTexture(GL_TEXTURE0 + MG_TEXTURE_BUFFER_EMULATION_UNIT);
            GLES.glBindTexture(GL_TEXTURE_2D, texture);
            GLES.glActiveTexture(GL_TEXTURE0 + gl_state->current_tex_unit);
            DriverActiveTextureUnit = (int)gl_state->current_tex_unit;
        } else {
            GLES.glBindTexture(target, texture);
        }
        set_driver_texture_binding(driver_unit, driver_target, texture);
    }
    CHECK_GL_ERROR_NO_INIT

    if (targetR == TextureTarget::UNKNWON) {
        LOG_E("glBindTexture: Unknown texture target: %s", glEnumToString(target));
        return;
    }
    auto& bindingSlot = currentUnit.GetBindingSlot(targetR);
    auto textureObject = GetOrCreateTextureObject(texture);
    if (!textureObject) {
        LOG_W("glBindTexture: Failed to get or create texture object for ID %d, it may be not tracked", texture);
        return;
    }
    bindingSlot.Bind(textureObject);
    textureObject->target = targetR;
}

void glDeleteTextures(GLsizei n, const GLuint* textures) {
    LOG()
    INIT_CHECK_GL_ERROR
    GLES.glDeleteTextures(n, textures);
    CHECK_GL_ERROR_NO_INIT

    for (GLsizei i = 0; i < n; ++i) {
        MarkTextureObjectForDeletion(textures[i]);
        // Deleting a bound texture resets that binding to 0, in the current context
        // only, so the driver shadow has to follow or it would keep reporting a
        // name that no longer exists. The sibling contexts of the share group are
        // deliberately left alone: GL leaves their bindings as they are.
        if (textures[i] == 0) continue;
        for (auto& unit : DriverTextureBindings) {
            for (GLuint& bound : unit) {
                if (bound == textures[i]) bound = 0;
            }
        }
    }
}

void glActiveTexture(GLenum texture) {
    LOG()
    LOG_D("glActiveTexture, texture = %s", glEnumToString(texture))
    if (texture < GL_TEXTURE0 || texture >= GL_TEXTURE0 + MAX_TEXTURE_IMAGE_UNITS) {
        // Returning here leaves the active unit where it was, so the caller's
        // next glBindTexture goes somewhere it did not ask for. Nothing can
        // report that -- this layer's glGetError never does -- so say it once.
        TX_WARN_ONCE("glActiveTexture: unit %d is past the %d this layer tracks; the call was ignored and the active "
                     "unit left unchanged",
                     (int)(texture - GL_TEXTURE0), MAX_TEXTURE_IMAGE_UNITS);
        LOG_E("Invalid texture enum: %s", glEnumToString(texture))
        return;
    }

    const int unit = (int)(texture - GL_TEXTURE0);
    set_gl_state_current_tex_unit(texture - GL_TEXTURE0);

    // Only the driver call is skipped; both pieces of bookkeeping around it run
    // either way, so a caller that re-selects the unit it is already on still
    // leaves this layer's idea of the active unit written.
    //
    // Every internal borrower of the active unit hands it back: gl/buffer.cpp's
    // glTexBuffer and gl/drawing.cpp's setupBufferTextureUniforms both return to
    // gl_state->current_tex_unit (or to the value they saved, which is the same
    // number) on every exit including their early ones, FSR1's GLStateGuard
    // restores GL_ACTIVE_TEXTURE in its destructor, and the emulation branch in
    // glBindTexture above writes DriverActiveTextureUnit itself rather than
    // relying on that. So the shadow is only ever consulted where the driver
    // agrees with it -- as long as it describes this context at all, which for the
    // shared fallback record it does not, hence the gate.
    if (!driver_active_unit_shadow_trustworthy() || DriverActiveTextureUnit != unit) {
        GLES.glActiveTexture(texture);
        DriverActiveTextureUnit = unit;
    }
    ActivateTextureUnit(unit);
    CHECK_GL_ERROR
}

void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, void* pixels) {
    LOG()
    LOG_D("glGetTexImage, target: 0x%x, level: %d, format: 0x%x, type: 0x%x, pixels: 0x%x", target, level, format, type,
          pixels)
    GLint prevDrawFBO;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDrawFBO);
    GLint prevReadFBO;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFBO);

    GLuint tempFBO = 0;
    glGenFramebuffers(1, &tempFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, tempFBO);

    GLint textureId = 0;
    GLenum textureBindingTarget;
    if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) {
        textureBindingTarget = GL_TEXTURE_BINDING_CUBE_MAP;
    } else if (target == GL_TEXTURE_2D) {
        textureBindingTarget = GL_TEXTURE_BINDING_2D;
    } else {
        LOG_E("glGetTexImage: Unsupported or complex target: 0x%x", target)
        // Said nothing to the application before. 3D, array and rectangle
        // targets are legal desktop reads this emulation cannot express as a
        // colour-attachment read, and returning quietly handed back an
        // untouched buffer that looked like a successful image.
        mg_set_gl_error(GL_INVALID_OPERATION);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
        glDeleteFramebuffers(1, &tempFBO);
        return;
    }
    glGetIntegerv(textureBindingTarget, &textureId);

    if (textureId == 0) {
        LOG_E("glGetTexImage: No texture bound to the specified target.")
        mg_set_gl_error(GL_INVALID_OPERATION);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
        glDeleteFramebuffers(1, &tempFBO);
        return;
    }

    GLint width = 0, height = 0;
    glGetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(target, level, GL_TEXTURE_HEIGHT, &height);

    if (width == 0 || height == 0) {
        LOG_E("glGetTexImage: Texture level %d has zero width or height.", level)
        mg_set_gl_error(GL_INVALID_VALUE);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
        glDeleteFramebuffers(1, &tempFBO);
        return;
    }

    if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, textureId, level);
    } else if (target == GL_TEXTURE_2D) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, level);
    }

    GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
        LOG_E("glGetTexImage: Failed to create complete framebuffer. Status: 0x%x", fboStatus)
        // Overwhelmingly this is a depth or depth-stencil level: it is not
        // colour-renderable, so hanging it off GL_COLOR_ATTACHMENT0 can never
        // complete. A shadow-map readback used to land here and return in
        // silence.
        mg_set_gl_error(GL_INVALID_OPERATION);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
        glDeleteFramebuffers(1, &tempFBO);
        return;
    }

    glReadBuffer(GL_COLOR_ATTACHMENT0);

    // Asked here, with the scratch framebuffer bound, because the pair GLES
    // accepts besides GL_RGBA/GL_UNSIGNED_BYTE is chosen per read framebuffer.
    // Forwarding a pair it refuses used to return with the destination untouched
    // and nothing said: the caller wrote its own uninitialised buffer out as a
    // texture dump.
    if (!mg_readback_pair_supported(format, type)) {
        LOG_E("glGetTexImage: %s + %s cannot be read back on this backend", glEnumToString(format),
              glEnumToString(type))
        mg_set_gl_error(GL_INVALID_OPERATION);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
        glDeleteFramebuffers(1, &tempFBO);
        return;
    }

    glReadPixels(0, 0, width, height, format, type, pixels);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
    glDeleteFramebuffers(1, &tempFBO);
}

#if GLOBAL_DEBUG || DEBUG
#include "../config/config.h"
#include <fstream>
#endif

void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width,
                     GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) {
    LOG()
    LOG_D("glTexSubImage3D, target = %s, level = %d, offset = (%d,%d,%d), size = (%d,%d,%d), format = %s, type = %s",
          glEnumToString(target), level, xoffset, yoffset, zoffset, width, height, depth, glEnumToString(format),
          glEnumToString(type))
    mg_upload_fix_t fix(width, height, depth, format, type, pixels);
    if (fix.dropped()) {
        CHECK_GL_ERROR
        return;
    }
    GLES.glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, fix.format, fix.type,
                         fix.pixels);
    CHECK_GL_ERROR
}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels) {
    LOG()
    LOG_D("glReadPixels, x=%d, y=%d, width=%d, height=%d, format=0x%x, "
          "type=0x%x, pixels=0x%x",
          x, y, width, height, format, type, pixels)

    // Source the frame the application actually drew, not the window surface it
    // has not been rendering into since FSR1 was switched on. No-op otherwise.
    mg_fsr_read_scope_t fsr_read;

    // Encodes BGRA and the packed 8888 layouts from an RGBA readback; the old
    // code renamed one type combination and wrote RGBA bytes into a buffer the
    // application would read as BGRA.
    if (mg_transfer_readback(x, y, width, height, format, type, pixels)) {
        CHECK_GL_ERROR
        return;
    }
    GLES.glReadPixels(x, y, width, height, format, type, pixels);

    CHECK_GL_ERROR
}

void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    LOG()
    pname = pname_convert(pname);
    LOG_D("glTexParameteri, pname: 0x%x", pname)

    if (pname == GL_TEXTURE_LOD_BIAS_QCOM && !g_gles_caps.GL_QCOM_texture_lod_bias) {
        LOG_D("Does not support GL_QCOM_texture_lod_bias, skipped!")
        return;
    }

    // A legal filter set on a depth object reaches the driver in the
    // depth-legal form (push_depth_sampling_state); everything else,
    // colour textures included, is one call exactly as before.
    TextureObject* tex = mgGetTexObjectByTarget(target);
    const bool recorded = record_texture_sampling_param(tex, pname, param);
    if (recorded && mg_is_sized_depth_format(tex->internal_format)) {
        push_depth_sampling_state(target, *tex);
    } else {
        GLES.glTexParameteri(target, pname, param);
    }
    CHECK_GL_ERROR
}

// The shadowed pnames answer from the record: on a depth texture with
// comparison off the driver holds the depth-legal filter while the
// application asked for the other one. Unknown targets, unbound targets
// and every other pname go to the driver as before (this was a NATIVE
// forward until the depth-completeness fix made the driver's answer a
// lie for exactly those three).
void glGetTexParameteriv(GLenum target, GLenum pname, GLint* params) {
    LOG()
    if (params && (pname == GL_TEXTURE_MIN_FILTER || pname == GL_TEXTURE_MAG_FILTER ||
                   pname == GL_TEXTURE_COMPARE_MODE)) {
        if (TextureObject* bound = mgGetTexObjectByTarget(target)) {
            switch (pname) {
            case GL_TEXTURE_MIN_FILTER:
                *params = bound->min_filter;
                return;
            case GL_TEXTURE_MAG_FILTER:
                *params = bound->mag_filter;
                return;
            default:
                *params = (GLint)bound->compare_mode;
                return;
            }
        }
    }
    GLES.glGetTexParameteriv(target, pname, params);
    CHECK_GL_ERROR
}

void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params) {
    LOG()
    if (params && (pname == GL_TEXTURE_MIN_FILTER || pname == GL_TEXTURE_MAG_FILTER ||
                   pname == GL_TEXTURE_COMPARE_MODE)) {
        if (TextureObject* bound = mgGetTexObjectByTarget(target)) {
            switch (pname) {
            case GL_TEXTURE_MIN_FILTER:
                *params = (GLfloat)bound->min_filter;
                return;
            case GL_TEXTURE_MAG_FILTER:
                *params = (GLfloat)bound->mag_filter;
                return;
            default:
                *params = (GLfloat)bound->compare_mode;
                return;
            }
        }
    }
    GLES.glGetTexParameterfv(target, pname, params);
    CHECK_GL_ERROR
}

// The sampler-object group: state recorded, calls forwarded unchanged.
// glGenSamplers and glIsSampler stay NATIVE (the shadow keys off the
// names they mint), glGetSamplerParameter* stays NATIVE because nothing
// here ever rewrites what the driver holds.

void glDeleteSamplers(GLsizei count, const GLuint* samplers) {
    LOG()
    LOG_D("glDeleteSamplers, count: %d", count)
    if (samplers) {
        for (GLsizei i = 0; i < count; ++i) {
            const GLuint id = samplers[i];
            if (id < SamplerShadowTable.size()) SamplerShadowTable[id] = sampler_shadow_t{};
            // A deleted sampler unbinds from every unit, shadow side the
            // same way the driver does it.
            for (int unit = 0; unit < mg_max_texture_units(); ++unit) {
                if (SamplerBindings[unit] == id) SamplerBindings[unit] = 0;
            }
        }
    }
    GLES.glDeleteSamplers(count, samplers);
    CHECK_GL_ERROR
}

void glBindSampler(GLuint unit, GLuint sampler) {
    LOG()
    LOG_D("glBindSampler, unit: %d, sampler: %u", unit, sampler)
    if (unit < (GLuint)mg_max_texture_units()) SamplerBindings[unit] = sampler;
    GLES.glBindSampler(unit, sampler);
    CHECK_GL_ERROR
}

void glSamplerParameteri(GLuint sampler, GLenum pname, GLint param) {
    LOG()
    LOG_D("glSamplerParameteri, sampler: %u, pname: %s, param: %d", sampler, glEnumToString(pname), param)
    record_sampler_sampling_param(sampler, pname, param);
    GLES.glSamplerParameteri(sampler, pname, param);
    CHECK_GL_ERROR
}

void glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint* param) {
    LOG()
    LOG_D("glSamplerParameteriv, sampler: %u, pname: %s", sampler, glEnumToString(pname))
    if (param) record_sampler_sampling_param(sampler, pname, param[0]);
    GLES.glSamplerParameteriv(sampler, pname, param);
    CHECK_GL_ERROR
}

void glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param) {
    LOG()
    LOG_D("glSamplerParameterf, sampler: %u, pname: %s, param: %f", sampler, glEnumToString(pname), param)
    record_sampler_sampling_param(sampler, pname, (GLint)param);
    GLES.glSamplerParameterf(sampler, pname, param);
    CHECK_GL_ERROR
}

void glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat* param) {
    LOG()
    LOG_D("glSamplerParameterfv, sampler: %u, pname: %s", sampler, glEnumToString(pname))
    if (param) record_sampler_sampling_param(sampler, pname, (GLint)param[0]);
    GLES.glSamplerParameterfv(sampler, pname, param);
    CHECK_GL_ERROR
}

// The draw-time half of the depth-completeness fix. The texture side was
// made legal where the state was set (push_depth_sampling_state); what
// only exists at draw time is the pairing, because a sampler object
// overrides the texture object's filters and one sampler can sit on
// colour and depth units at once. Unbinding the offending sampler on the
// offending unit(s) hands the draw the texture object's already-legal
// state; colour units sharing that sampler are not touched.
mg_depth_draw_guard::mg_depth_draw_guard() {
    const int max_units = mg_max_texture_units();
    int original_active = -1;
    for (int unit = 0; unit < max_units; ++unit) {
        TextureObject* tex = TextureUnits[unit].GetBindingSlot(TextureTarget::TEXTURE_2D).GetBoundObject();
        if (!tex || !mg_is_sized_depth_format(tex->internal_format)) continue;
        const GLuint sampler = SamplerBindings[unit];
        GLint min_f = tex->min_filter;
        GLint mag_f = tex->mag_filter;
        GLenum compare = tex->compare_mode;
        if (sampler != 0) {
            const sampler_shadow_t shadow =
                sampler < SamplerShadowTable.size() ? SamplerShadowTable[sampler] : sampler_shadow_t{};
            min_f = shadow.min_filter;
            mag_f = shadow.mag_filter;
            compare = shadow.compare_mode;
        }
        if (!mg_depth_pairing_incomplete(tex->internal_format, compare, min_f, mag_f)) continue;
        if (sampler == 0) {
            // Texture-object state is pushed legal at every transition
            // this layer sees (classic and DSA both route through those
            // paths), so an incomplete pairing with no sampler bound means
            // a path that does not. Say so; the driver's own judgement is
            // not something to second-guess blind.
            LOG_W("depth texture %u on unit %d would draw incomplete with no sampler bound", tex->texture, unit)
            continue;
        }
        Moved& rec = moved[moved_count++];
        rec = {unit, sampler, -1};
        GLES.glBindSampler(unit, 0);
        if (tex->compare_mode != GL_NONE) {
            // The texture object says comparison is on while the sampler
            // (which was overriding it) said NONE. Unbinding alone would
            // turn this draw's raw depth reads into reference compares, so
            // push NONE for the draw; the record restores it after.
            rec.saved_compare = (GLint)tex->compare_mode;
            if (original_active < 0) original_active = mg_driver_active_texture_unit();
            if (original_active != unit) GLES.glActiveTexture(GL_TEXTURE0 + unit);
            GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                 mg_depth_filter_make_legal(GL_TEXTURE_MIN_FILTER, tex->min_filter));
            GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                 mg_depth_filter_make_legal(GL_TEXTURE_MAG_FILTER, tex->mag_filter));
            GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        }
    }
    // Back to the application's active unit before the draw: the guard
    // borrowed it only for the compare rewrite above.
    if (original_active >= 0) GLES.glActiveTexture(GL_TEXTURE0 + original_active);
}

mg_depth_draw_guard::~mg_depth_draw_guard() {
    if (moved_count == 0) return;
    int original_active = -1;
    for (int i = 0; i < moved_count; ++i) {
        const Moved& rec = moved[i];
        GLES.glBindSampler(rec.unit, rec.sampler);
        if (rec.saved_compare < 0) continue;
        TextureObject* tex = TextureUnits[rec.unit].GetBindingSlot(TextureTarget::TEXTURE_2D).GetBoundObject();
        if (!tex) continue; // Binding gone; there is no compare mode to put back.
        if (original_active < 0) original_active = mg_driver_active_texture_unit();
        if (original_active != rec.unit) GLES.glActiveTexture(GL_TEXTURE0 + rec.unit);
        // Re-derived from the record: comparison back on, filters back to
        // what a compare-enabled pairing may carry, which is the exact
        // state push_depth_sampling_state put on the driver before the
        // draw unbound anything.
        push_depth_sampling_state(GL_TEXTURE_2D, *tex);
    }
    if (original_active >= 0) GLES.glActiveTexture(GL_TEXTURE0 + original_active);
}

namespace {

// The clear values are context state, not framebuffer state, so the ones
// glClearTexImage sets to clear its temporary attachment are the ones the
// application's next glClear used. Restoring them from a destructor is what
// makes that true of every exit from the function, not only the last one.
struct clear_state_guard_t {
    GLfloat color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    GLfloat depth = 1.0f;
    GLint stencil = 0;

    clear_state_guard_t() {
        GLES.glGetFloatv(GL_COLOR_CLEAR_VALUE, color);
        GLES.glGetFloatv(GL_DEPTH_CLEAR_VALUE, &depth);
        GLES.glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &stencil);
    }

    ~clear_state_guard_t() {
        GLES.glClearColor(color[0], color[1], color[2], color[3]);
        GLES.glClearDepthf(depth);
        GLES.glClearStencil(stencil);
    }

    clear_state_guard_t(const clear_state_guard_t&) = delete;
    clear_state_guard_t& operator=(const clear_state_guard_t&) = delete;
};

// Reads the one texel glClearTexImage was handed into the three clear
// registers. False means the combination cannot be read, which the caller
// drops: clearing to a value the application never named is a wrong image that
// looks like a right one.
bool decode_clear_value(GLenum format, GLenum type, const void* data, GLfloat* rgba, GLfloat* depth, GLint* stencil) {
    switch (format) {
    case GL_RGBA:
    case GL_RGB:
    case GL_BGRA:
    case GL_BGR: {
        const bool has_alpha = (format == GL_RGBA || format == GL_BGRA);
        const bool reversed = (format == GL_BGRA || format == GL_BGR);
        if (type == GL_UNSIGNED_BYTE) {
            const auto* b = static_cast<const GLubyte*>(data);
            rgba[0] = (GLfloat)b[reversed ? 2 : 0] / 255.0f;
            rgba[1] = (GLfloat)b[1] / 255.0f;
            rgba[2] = (GLfloat)b[reversed ? 0 : 2] / 255.0f;
            rgba[3] = has_alpha ? (GLfloat)b[3] / 255.0f : 1.0f;
            return true;
        }
        if (type == GL_FLOAT) {
            const auto* f = static_cast<const GLfloat*>(data);
            rgba[0] = f[reversed ? 2 : 0];
            rgba[1] = f[1];
            rgba[2] = f[reversed ? 0 : 2];
            rgba[3] = has_alpha ? f[3] : 1.0f;
            return true;
        }
        return false;
    }
    case GL_DEPTH_COMPONENT:
        if (type == GL_FLOAT) {
            *depth = static_cast<const GLfloat*>(data)[0];
            return true;
        }
        if (type == GL_UNSIGNED_SHORT) {
            *depth = (GLfloat)static_cast<const GLushort*>(data)[0] / 65535.0f;
            return true;
        }
        if (type == GL_UNSIGNED_INT) {
            *depth = (GLfloat)((double)static_cast<const GLuint*>(data)[0] / 4294967295.0);
            return true;
        }
        return false;
    case GL_STENCIL_INDEX:
        if (type == GL_UNSIGNED_BYTE) {
            *stencil = static_cast<const GLubyte*>(data)[0];
            return true;
        }
        if (type == GL_UNSIGNED_INT) {
            *stencil = (GLint)static_cast<const GLuint*>(data)[0];
            return true;
        }
        return false;
    case GL_DEPTH_STENCIL:
        if (type == GL_UNSIGNED_INT_24_8) {
            GLuint v;
            memcpy(&v, data, sizeof(v));
            *depth = (GLfloat)((double)(v >> 8) / 16777215.0);
            *stencil = (GLint)(v & 0xff);
            return true;
        }
        if (type == GL_FLOAT_32_UNSIGNED_INT_24_8_REV) {
            GLfloat d;
            GLuint s;
            memcpy(&d, data, sizeof(d));
            memcpy(&s, static_cast<const GLubyte*>(data) + sizeof(d), sizeof(s));
            *depth = d;
            *stencil = (GLint)(s & 0xff);
            return true;
        }
        return false;
    default:
        return false;
    }
}

} // namespace

void glClearTexImage(GLuint texture, GLint level, GLenum format, GLenum type, const void* data) {
    LOG()
    LOG_D("glClearTexImage, texture: %d, level: %d, format: %s, type: %s", texture, level, glEnumToString(format),
          glEnumToString(type))
    INIT_CHECK_GL_ERROR_FORCE

    // GL 4.6 sec. 8.15: a null pointer clears the level to zero, a non-null one
    // clears it to that value. A value this could not read used to fall through
    // as the transparent black set up front, which the application cannot tell
    // from a clear it asked for -- so it is dropped with a warning instead.
    GLfloat rgba[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    GLfloat depth = 0.0f;
    GLint stencil = 0;
    if (data != nullptr && !decode_clear_value(format, type, data, rgba, &depth, &stencil)) {
        TX_WARN_ONCE("glClearTexImage: cannot read a clear value of %s + %s, clear dropped", glEnumToString(format),
                     glEnumToString(type));
        return;
    }

    // Where the level has to be attached, and what clearing it means. A depth or
    // stencil texture hung on GL_COLOR_ATTACHMENT0 never completes, so those
    // formats used to leave the function having cleared nothing.
    GLenum attachment = GL_COLOR_ATTACHMENT0;
    GLbitfield mask = GL_COLOR_BUFFER_BIT;
    if (format == GL_DEPTH_COMPONENT) {
        attachment = GL_DEPTH_ATTACHMENT;
        mask = GL_DEPTH_BUFFER_BIT;
    } else if (format == GL_STENCIL_INDEX) {
        attachment = GL_STENCIL_ATTACHMENT;
        mask = GL_STENCIL_BUFFER_BIT;
    } else if (format == GL_DEPTH_STENCIL) {
        attachment = GL_DEPTH_STENCIL_ATTACHMENT;
        mask = GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    }

    GLuint fbo, prevDrawFBO, prevReadFBO;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (int*)&prevDrawFBO);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, (int*)&prevReadFBO);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    CHECK_GL_ERROR_NO_INIT

    clear_state_guard_t clear_guard;
    GLES.glClearColor(rgba[0], rgba[1], rgba[2], rgba[3]);
    GLES.glClearDepthf(depth);
    GLES.glClearStencil(stencil);
    CHECK_GL_ERROR_NO_INIT

    // Only a 2D-shaped image can be attached with glFramebufferTexture2D and
    // GL_TEXTURE_2D. A cube map is six of those and a 3D or array texture is one
    // attachment per layer; handing either to the 2D form attached nothing, so
    // the framebuffer was never complete and nothing was cleared.
    TextureObject* tex = mgGetTexObjectByID(texture);
    const TextureTarget textureTarget = tex ? tex->target : TextureTarget::TEXTURE_2D;

    auto attach_and_clear = [&](GLenum face, GLint layer) -> bool {
        if (layer < 0) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, face, texture, level);
        } else {
            GLES.glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, texture, level, layer);
        }
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) return false;
        GLES.glClear(mask);
        return true;
    };

    bool cleared = true;
    switch (textureTarget) {
    case TextureTarget::TEXTURE_CUBE_MAP:
        for (GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X; cleared && face <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; ++face) {
            cleared = attach_and_clear(face, -1);
        }
        break;
    case TextureTarget::TEXTURE_3D:
    case TextureTarget::TEXTURE_2D_ARRAY:
    case TextureTarget::TEXTURE_1D_ARRAY:
    case TextureTarget::TEXTURE_CUBE_MAP_ARRAY: {
        // The level is only whole once every layer of it has been cleared, so the
        // layer count has to be this level's, not some other level's.
        //
        // It is queried rather than derived from TextureObject::depth: glTexImage3D
        // writes that field on every call, including every mip level, so a texture
        // uploaded level by level leaves it holding the smallest mip's depth. Using
        // it would clear a handful of layers of level 0 and report success -- the
        // silent partial clear this whole change exists to remove. The driver knows
        // the real extent of the level being cleared.
        GLint queried = 0;
        GLES.glGetTexLevelParameteriv(ConvertTextureTargetToGLEnum(textureTarget), level, GL_TEXTURE_DEPTH, &queried);
        GLsizei layers = (GLsizei)queried;
        if (layers <= 0) {
            // No answer from the driver; fall back to the tracked depth, which is
            // right whenever the storage came from glTexStorage3D (recorded once)
            // and for arrays (no per-level shrink).
            layers = tex ? (GLsizei)nlevel(tex->depth, textureTarget == TextureTarget::TEXTURE_3D ? level : 0) : 0;
        }
        if (layers <= 0) {
            cleared = false;
            break;
        }
        for (GLint layer = 0; cleared && layer < layers; ++layer) {
            cleared = attach_and_clear(GL_NONE, layer);
        }
        break;
    }
    default:
        cleared = attach_and_clear(GL_TEXTURE_2D, -1);
        break;
    }

    if (!cleared) {
        TX_WARN_ONCE("glClearTexImage: texture %u (%s) level %d as %s cannot be attached to a framebuffer, "
                     "nothing was cleared",
                     texture, glEnumToString(ConvertTextureTargetToGLEnum(textureTarget)), level,
                     glEnumToString(format));
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
    glDeleteFramebuffers(1, &fbo);
    CHECK_GL_ERROR_NO_INIT
}

// Pixel-store parameters desktop GL has and GLES does not.
//
// Forwarding them earned a GL_INVALID_ENUM from the driver and nothing else --
// which, while glGetError was lying, the application could not even see. Swallow
// them instead: the error was never the application's fault, and raising one for
// a parameter it is entitled to set under the API this layer advertises would be
// worse than the silence. What the layer cannot do is honour them; nothing here
// byte-swaps, so an application feeding big-endian component data through
// GL_UNPACK_SWAP_BYTES still gets it unswapped, and says so once in the log.
void glPixelStorei(GLenum pname, GLint param) {
    LOG_D("glPixelStorei, pname = %s, param = %d", glEnumToString(pname), param)
    // Kept here rather than forwarded. GLES has neither the two SWAP_BYTES, the
    // two LSB_FIRST, nor PACK_IMAGE_HEIGHT / PACK_SKIP_IMAGES, so the driver
    // answered GL_INVALID_ENUM and dropped the value -- and glGetIntegerv, going
    // the same way, left the application's variable untouched, so the parameter
    // could be neither set nor read. The shadow in gl_state_s closes both halves;
    // what is actually honoured is documented there.
    if (mg_pixel_store_set(pname, param)) return;
    GLES.glPixelStorei(pname, param);
    CHECK_GL_ERROR
}

void glPixelStoref(GLenum pname, GLfloat param) {
    LOG_D("glPixelStoref, pname = %s, param = %f", glEnumToString(pname), param)
    // The two entry points set the same state; this one was a stub, so an
    // application that set its alignment or row length through the float form
    // transferred with whatever the previous state happened to be. Every
    // pixel-store parameter GLES has is integer-valued, and GL 4.6 sec. 8.4.1
    // rounds this form to the nearest integer for those.
    glPixelStorei(pname, (GLint)lroundf(param));
    CHECK_GL_ERROR
}
