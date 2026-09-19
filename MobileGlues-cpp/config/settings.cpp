// MobileGlues - config/settings.cpp
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

#include "settings.h"
#include "config.h"
#include "../gl/log.h"
#include "../gl/envvars.h"
#include "gpu_utils.h"
#include "../gl/getter.h"

#define DEBUG 0

global_settings_t global_settings;

// Defined in the multi-draw section below; called at the end of init_settings().
static void parse_multidraw_orders();

void init_settings() {
#if defined(__APPLE__)
    global_settings.angle = AngleMode::Disabled;
    global_settings.angle_config = AngleConfig::DisableIfPossible;
    global_settings.angle_supported = false;
    global_settings.ignore_error = IgnoreErrorLevel::Partial;
    global_settings.ext_compute_shader = false;
    global_settings.max_glsl_cache_size = 30 * 1024 * 1024;
    global_settings.angle_depth_clear_fix_mode = AngleDepthClearFixMode::Disabled;
    global_settings.ext_direct_state_access = true;
    global_settings.custom_gl_version = {0, 0, 0}; // will go default
    global_settings.fsr1_setting = FSR1_Quality_Preset::Disabled;
    global_settings.hide_mg_env_level = HideMGEnvLevel::Disabled;

#else

    int success = initialized;
    if (!success) {
        success = config_refresh();
        if (!success) {
            LOG_V("Failed to load config. Use default config.")
        }
    }

    AngleConfig angleConfig =
        success ? static_cast<AngleConfig>(config_get_int("enableANGLE")) : AngleConfig::DisableIfPossible;
    NoErrorConfig noErrorConfig =
        success ? static_cast<NoErrorConfig>(config_get_int("enableNoError")) : NoErrorConfig::Auto;
    bool enableExtComputeShader = success ? (config_get_int("enableExtComputeShader") > 0) : false;
    bool enableExtTimerQuery = success ? (config_get_int("enableExtTimerQuery") > 0) : false;
    bool enableExtDirectStateAccess = success ? (config_get_int("enableExtDirectStateAccess") > 0) : false;
    AngleDepthClearFixMode angleDepthClearFixMode =
        success ? static_cast<AngleDepthClearFixMode>(config_get_int("angleDepthClearFixMode"))
                : AngleDepthClearFixMode::Disabled;
    int customGLVersionInt = success ? config_get_int("customGLVersion") : DEFAULT_GL_VERSION;
    FSR1_Quality_Preset fsr1Setting =
        success ? static_cast<FSR1_Quality_Preset>(config_get_int("fsr1Setting")) : FSR1_Quality_Preset::Disabled;
    HideMGEnvLevel hideMGEnvLevel =
        success ? static_cast<HideMGEnvLevel>(config_get_int("hideMGEnvLevel")) : HideMGEnvLevel::Disabled;

    if (customGLVersionInt < 0) {
        customGLVersionInt = 0;
    }

    size_t maxGlslCacheSize = 0;
    if (config_get_int("maxGlslCacheSize") > 0) {
        maxGlslCacheSize = success ? config_get_int("maxGlslCacheSize") * 1024 * 1024 : 0;
    }

    // config_get_int returns -1 for a key that is absent, so a config.json written
    // without "enableANGLE" -- what a launcher that sets only some keys produces --
    // arrives here, as does any out-of-range value. Both mean "the config did not
    // say", and that resolves to the same thing as having no config at all.
    //
    // It used to resolve to EnableIfPossible instead, which made an absent key mean
    // the opposite of an absent file. That never showed, because off Adreno 730/740
    // checkIfANGLESupported reduces to hasVulkan12(), and hasVulkan12 could not
    // succeed while config/gpu_utils.cpp handed dlopen library names with no ".so".
    // Fixing those names would have turned ANGLE on by itself, for the first time,
    // on any device whose *physical device* reports Vulkan 1.2 or newer -- so the
    // fallback is written down rather than left to a sign convention.
    //
    // The two range checks below already fall back to their neutral value; this one
    // was the exception.
    if (static_cast<int>(angleConfig) < 0 || static_cast<int>(angleConfig) > 3) {
        angleConfig = AngleConfig::DisableIfPossible;
    }
    if (static_cast<int>(noErrorConfig) < 0 || static_cast<int>(noErrorConfig) > 3) {
        noErrorConfig = NoErrorConfig::Auto;
    }
    if (static_cast<int>(angleDepthClearFixMode) < 0 ||
        static_cast<int>(angleDepthClearFixMode) >= static_cast<int>(AngleDepthClearFixMode::MaxValue)) {
        angleDepthClearFixMode = AngleDepthClearFixMode::Disabled;
    }
    if (customGLVersionInt > 46) {
        customGLVersionInt = 46;
    } else if (customGLVersionInt < 32 && customGLVersionInt != 0) {
        customGLVersionInt = 32;
    } else if (customGLVersionInt > 33 && customGLVersionInt < 40) {
        customGLVersionInt = 33;
    } else if (customGLVersionInt == 0) {
        customGLVersionInt = DEFAULT_GL_VERSION;
    }
    if (static_cast<int>(fsr1Setting) < 0 ||
        static_cast<int>(fsr1Setting) >= static_cast<int>(FSR1_Quality_Preset::MaxValue)) {
        fsr1Setting = FSR1_Quality_Preset::Disabled;
    }
    if (static_cast<int>(hideMGEnvLevel) < 0 ||
        static_cast<int>(hideMGEnvLevel) >= static_cast<int>(HideMGEnvLevel::MaxValue)) {
        hideMGEnvLevel = HideMGEnvLevel::Disabled;
    }

    Version customGLVersion(customGLVersionInt);

    int isInPluginApp = 0;
    GetEnvVarInt("MG_PLUGIN_STATUS", &isInPluginApp, 0);
    int fclVersion = 0;
    GetEnvVarInt("FCL_VERSION_CODE", &fclVersion, 0);
    int zlVersion = 0;
    GetEnvVarInt("ZALITH_VERSION_CODE", &zlVersion, 0);
    int pgwVersion = 0;
    GetEnvVarInt("PGW_VERSION_CODE", &pgwVersion, 0);
    int qclVersion = 0;
    GetEnvVarInt("QCL_VERSION_CODE", &qclVersion, 0);

    LOG_V("MG_DIR_PATH = %s", mg_directory_path ? mg_directory_path : "(default)")

    if (isInPluginApp == 0 && fclVersion == 0 && zlVersion == 0 && pgwVersion == 0 && qclVersion == 0 && !is_custom_mg_dir) {
        LOG_V("Unsupported launcher detected, force using default config.")
        angleConfig = AngleConfig::DisableIfPossible;
        noErrorConfig = NoErrorConfig::Auto;
        enableExtComputeShader = false;
        enableExtTimerQuery = true;
        enableExtDirectStateAccess = true;
        maxGlslCacheSize = 0;
        angleDepthClearFixMode = AngleDepthClearFixMode::Disabled;
        fsr1Setting = FSR1_Quality_Preset::Disabled;
        hideMGEnvLevel = HideMGEnvLevel::Disabled;
    }

    AngleMode finalAngleMode = AngleMode::Disabled;
    std::string gpuString = getGPUInfo();
    const char* gpu_cstr = gpuString.c_str();
    LOG_D("GPU: %s", gpu_cstr ? gpu_cstr : "(unknown)")

    int hasVk12 = hasVulkan12();
    int isQcom = isAdreno(gpu_cstr);
    int is730 = isAdreno730(gpu_cstr);
    int is740 = isAdreno740(gpu_cstr);
    int is830 = isAdreno830(gpu_cstr);
    bool isANGLESupported = checkIfANGLESupported(gpu_cstr);

    LOG_D("Has Vulkan 1.2? = %s", hasVk12 ? "true" : "false")
    LOG_D("Is Adreno? = %s", isQcom ? "true" : "false")
    LOG_D("Is Adreno 730? = %s", is730 ? "true" : "false")
    LOG_D("Is Adreno 740? = %s", is740 ? "true" : "false")
    LOG_D("Is Adreno 830? = %s", is830 ? "true" : "false")
    LOG_D("Is ANGLE supported? = %s", isANGLESupported ? "true" : "false")

    switch (angleConfig) {
    case AngleConfig::ForceDisable:
        finalAngleMode = AngleMode::Disabled;
        LOG_D("ANGLE: Force disabled");
        break;

    case AngleConfig::ForceEnable:
        finalAngleMode = AngleMode::Enabled;
        LOG_D("ANGLE: Force enabled");
        break;

    case AngleConfig::EnableIfPossible: {
        finalAngleMode = isANGLESupported ? AngleMode::Enabled : AngleMode::Disabled;
        LOG_D("ANGLE: Conditionally %s", (finalAngleMode == AngleMode::Enabled) ? "enabled" : "disabled");
        break;
    }

    case AngleConfig::DisableIfPossible:
    default:
        finalAngleMode = AngleMode::Disabled;
        break;
    }

    global_settings.angle = finalAngleMode;
    global_settings.angle_config = angleConfig;
    global_settings.angle_supported = isANGLESupported;
    LOG_D("Final ANGLE setting: %d", static_cast<int>(global_settings.angle))
    global_settings.buffer_coherent_as_flush = (global_settings.angle == AngleMode::Disabled);

    if (global_settings.angle == AngleMode::Enabled) {
        // setenv("LIBGL_GLES", "libGLESv2_angle.so", 1);
        // setenv("LIBGL_EGL", "libEGL_angle.so", 1);
    }

    switch (noErrorConfig) {
    case NoErrorConfig::Level1:
        global_settings.ignore_error = IgnoreErrorLevel::Partial;
        LOG_D("Error ignoring: Level 1 (Partial)");
        break;

    case NoErrorConfig::Level2:
        global_settings.ignore_error = IgnoreErrorLevel::Full;
        LOG_D("Error ignoring: Level 2 (Full)");
        break;

    case NoErrorConfig::Auto:
    case NoErrorConfig::Disable:
    default:
        global_settings.ignore_error = IgnoreErrorLevel::None;
        LOG_D("Error ignoring: Disabled");
        break;
    }

    global_settings.ext_compute_shader = enableExtComputeShader;
    global_settings.ext_timer_query = enableExtTimerQuery;
    global_settings.ext_direct_state_access = enableExtDirectStateAccess;
    global_settings.max_glsl_cache_size = maxGlslCacheSize;
    global_settings.angle_depth_clear_fix_mode = angleDepthClearFixMode;
    global_settings.custom_gl_version = customGLVersion;
    global_settings.fsr1_setting = fsr1Setting;
    global_settings.hide_mg_env_level = hideMGEnvLevel;
#endif

    LOG_V("[MobileGlues] Setting: enableAngle                 = %s",
          global_settings.angle == AngleMode::Enabled ? "true" : "false")
    LOG_V("[MobileGlues] Setting: ignoreError                 = %i", static_cast<int>(global_settings.ignore_error))
    LOG_V("[MobileGlues] Setting: enableExtComputeShader      = %s",
          global_settings.ext_compute_shader ? "true" : "false")
    LOG_V("[MobileGlues] Setting: enableExtTimerQuery         = %s", global_settings.ext_timer_query ? "true" : "false")
    LOG_V("[MobileGlues] Setting: enableExtDirectStateAccess  = %s",
          global_settings.ext_direct_state_access ? "true" : "false")
    LOG_V("[MobileGlues] Setting: maxGlslCacheSize            = %i",
          static_cast<int>(global_settings.max_glsl_cache_size / 1024 / 1024))
    LOG_V("[MobileGlues] Setting: angleDepthClearFixMode      = %i",
          static_cast<int>(global_settings.angle_depth_clear_fix_mode))
    LOG_V("[MobileGlues] Setting: bufferCoherentAsFlush       = %i",
          static_cast<int>(global_settings.buffer_coherent_as_flush))
    if (global_settings.custom_gl_version.isEmpty()) {
        LOG_V("[MobileGlues] Setting: customGLVersion             = (default)");
    } else {
        LOG_V("[MobileGlues] Setting: customGLVersion             = %s",
              global_settings.custom_gl_version.toString().c_str());
    }
    LOG_V("[MobileGlues] Setting: fsr1Setting                 = %i", static_cast<int>(global_settings.fsr1_setting))
    LOG_V("[MobileGlues] Setting: hideMGEnvLevel              = %i",
          static_cast<int>(global_settings.hide_mg_env_level))

    GLVersion =
        global_settings.custom_gl_version.isEmpty() ? Version(DEFAULT_GL_VERSION) : global_settings.custom_gl_version;

    // Multi-draw order parsing only needs config.json, not GL: capability
    // filtering happens later in init_settings_post(). Falls back to the default
    // order when the config is absent, so this is safe on every path above.
    parse_multidraw_orders();
}

// ---------------------------------------------------------------------------
// Multi-draw backend selection
//
// One global preference ORDER over every backend ("multidrawOrder", comma
// separated names, best first), optionally overridden per entry point by
// "multidrawOrder<EntryPoint>". The global order may contain the pseudo item
// "native", which is not a backend: at each entry point it stands for the GLES
// core/extension function of the same shape (glMultiDrawArrays ->
// glMultiDrawArraysEXT and so on), and expands to the backend wrapping that
// function. Per-entry exception orders list concrete backends only.
//
// Selection walks the order and takes the first backend that is a distinct
// implementation at that entry point AND that the device supports. The runtime
// fallback chains in gl/multidraw.cpp walk the same order, so a probe failure
// degrades to the user's next choice rather than to a hard-coded one.
//
// The old keys -- "multidrawMode", "multidrawMode<EntryPoint>",
// "multidrawDisableBackends" -- are no longer read; set_multidraw_setting()
// warns when they are still present.
// ---------------------------------------------------------------------------

namespace {

using B = md_backend_t;
using E = md_entry_t;

constexpr unsigned md_bit(B b) {
    return 1u << static_cast<int>(b);
}

struct md_backend_name_t {
    const char* name;
    B backend;
};

const md_backend_name_t k_md_backend_names[] = {
    {"auto", B::Auto},           {"unroll", B::Unroll},   {"basevertex", B::BaseVertex},
    {"indirect", B::Indirect},   {"multiindirect", B::MultiIndirect}, {"multibasevertex", B::MultiBaseVertex},
    {"multiarrays", B::MultiArrays}, {"compute", B::Compute},
};

// The default global order, "native" first. Kept close to what the old Auto
// ladders preferred: one-call batched backends before per-sub-draw loops, the
// native/EXT form of the same function before everything else, compute last
// because the old Auto never picked it on its own either.
const char* const k_md_default_global_order[] = {
    "native", "multiindirect", "multibasevertex", "multiarrays", "indirect", "basevertex", "unroll", "compute",
};
constexpr int MD_GLOBAL_ITEMS = 8;

struct md_entry_desc_t {
    const char* order_key;        // multidrawOrder<EntryPoint>, the exception key
    const char* legacy_mode_key;  // multidrawMode<EntryPoint>, warned about only
    const char* label;
    unsigned allowed;             // backends that are a DISTINCT implementation here
    B native_backend;             // what the pseudo item "native" means here
    const char* why;              // explains a rejection, so the log says why not just "invalid"
};

const md_entry_desc_t k_md_entries[MD_ENTRY_COUNT] = {
    {"multidrawOrderArrays", "multidrawModeArrays", "glMultiDrawArrays",
     md_bit(B::Unroll) | md_bit(B::MultiArrays) | md_bit(B::MultiIndirect),
     B::MultiArrays, // glMultiDrawArraysEXT
     "glMultiDrawArrays draws no indices, so index-side backends do not apply"},

    // BaseVertex/Compute are excluded: with no base vertex to apply or rebase,
    // they would be the same unrolled loop as Unroll.
    {"multidrawOrderElements", "multidrawModeElements", "glMultiDrawElements",
     md_bit(B::Unroll) | md_bit(B::Indirect) | md_bit(B::MultiIndirect) | md_bit(B::MultiBaseVertex) |
         md_bit(B::MultiArrays),
     B::MultiArrays, // glMultiDrawElementsEXT
     "glMultiDrawElements has no base vertex, so basevertex/compute are the same loop as unroll"},

    {"multidrawOrderElementsBaseVertex", "multidrawModeElementsBaseVertex", "glMultiDrawElementsBaseVertex",
     md_bit(B::Unroll) | md_bit(B::BaseVertex) | md_bit(B::Indirect) | md_bit(B::MultiIndirect) |
         md_bit(B::MultiBaseVertex) | md_bit(B::Compute),
     B::MultiBaseVertex, // glMultiDrawElementsBaseVertexEXT
     "multiarrays (EXT_multi_draw_arrays) carries no base vertex"},

    // These two receive a command buffer from the application; the only choice is
    // whether to hand the whole batch to the driver or walk it one command at a
    // time. "unroll" would mean the same thing as "indirect" here.
    {"multidrawOrderArraysIndirect", "multidrawModeArraysIndirect", "glMultiDrawArraysIndirect",
     md_bit(B::Indirect) | md_bit(B::MultiIndirect),
     B::MultiIndirect, // glMultiDrawArraysIndirectEXT
     "the application supplies the commands, so only indirect/multiindirect exist here"},

    {"multidrawOrderElementsIndirect", "multidrawModeElementsIndirect", "glMultiDrawElementsIndirect",
     md_bit(B::Indirect) | md_bit(B::MultiIndirect),
     B::MultiIndirect, // glMultiDrawElementsIndirectEXT
     "the application supplies the commands, so only indirect/multiindirect exist here"},
};

struct md_caps_t {
    bool basevertex;
    bool indirect_arrays;
    bool indirect_elements;
    bool multiindirect_arrays;
    bool multiindirect_elements;
    bool multibasevertex;
    bool multiarrays;
    bool compute;
};

bool md_is_arrays_side(E e) {
    return e == E::Arrays || e == E::ArraysIndirect;
}

bool md_backend_available(E e, B b, const md_caps_t& c) {
    switch (b) {
    case B::Unroll:
        return true;
    case B::BaseVertex:
        return c.basevertex;
    case B::Indirect:
        return md_is_arrays_side(e) ? c.indirect_arrays : c.indirect_elements;
    case B::MultiIndirect:
        return md_is_arrays_side(e) ? c.multiindirect_arrays : c.multiindirect_elements;
    case B::MultiBaseVertex:
        return c.multibasevertex;
    case B::MultiArrays:
        return c.multiarrays;
    case B::Compute:
        return c.compute;
    default:
        return false;
    }
}

// Case-insensitive, whitespace-tolerant name lookup.
bool md_parse_backend(const std::string& raw, B* out) {
    std::string s;
    for (char ch : raw) {
        if (ch == ' ' || ch == '\t' || ch == '_' || ch == '-') continue;
        s += static_cast<char>(ch >= 'A' && ch <= 'Z' ? ch - 'A' + 'a' : ch);
    }
    if (s.empty()) return false;
    for (const auto& n : k_md_backend_names) {
        if (s == n.name) {
            *out = n.backend;
            return true;
        }
    }
    return false;
}

std::string md_config_string(const char* key) {
    const char* v = config_get_string(const_cast<char*>(key));
    // config_get_string hands back a pointer into the live cJSON tree; copy now.
    return v ? std::string(v) : std::string();
}

} // namespace

const char* md_backend_name(md_backend_t b) {
    for (const auto& n : k_md_backend_names) {
        if (n.backend == b) return n.name;
    }
    return "(unknown)";
}

const char* md_backend_suffix(md_backend_t b) {
    switch (b) {
    case B::Unroll:
        return "_drawelements"; // historical symbol name, kept so it stays resolvable
    case B::BaseVertex:
        return "_basevertex";
    case B::Indirect:
        return "_indirect";
    case B::MultiIndirect:
        return "_multiindirect";
    case B::MultiBaseVertex:
        return "_multibasevertex";
    case B::MultiArrays:
        return "_multiarrays";
    case B::Compute:
        return "_compute";
    default:
        return nullptr; // Auto never survives resolution
    }
}

// One item of a parsed order list: a concrete backend, or the pseudo item
// "native" (global order only).
struct md_order_item_t {
    bool is_native;
    B backend;
};

// Splits a comma/semicolon separated order list. Unknown names are dropped with
// a warning; "native" is accepted only when allow_native is set. Returns the
// number of items written.
static int md_parse_order_list(const char* key, const std::string& raw, bool allow_native, md_order_item_t* out,
                               int out_max) {
    int n = 0;
    std::string token;
    for (size_t i = 0; i <= raw.size(); ++i) {
        const char ch = i < raw.size() ? raw[i] : ',';
        if (ch != ',' && ch != ';') {
            token += ch;
            continue;
        }
        // Normalise the token the same way md_parse_backend does.
        std::string s;
        for (char c : token) {
            if (c == ' ' || c == '\t' || c == '_' || c == '-') continue;
            s += static_cast<char>(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
        }
        token.clear();
        if (s.empty()) continue;
        if (n >= out_max) break;
        if (s == "native") {
            if (allow_native) {
                out[n++] = {true, B::Auto};
            } else {
                LOG_W_FORCE("%s: 'native' is only meaningful in the global multidrawOrder, ignored", key)
            }
            continue;
        }
        B b;
        if (!md_parse_backend(s, &b) || b == B::Auto) {
            LOG_W_FORCE("%s: '%s' is not a backend name, ignored", key, s.c_str())
            continue;
        }
        out[n++] = {false, b};
    }
    return n;
}

// The requested order per entry point, before capability filtering: parsed from
// config.json by parse_multidraw_orders() (called from init_settings(), when no
// GL is loaded yet), consumed by init_settings_post() once capabilities exist.
static B s_md_requested[MD_ENTRY_COUNT][MD_BACKEND_COUNT];
static int s_md_requested_len[MD_ENTRY_COUNT];

// Expands an order list into a total per-entry order: map "native" to this
// entry's native backend, keep the first occurrence of each backend that is a
// distinct implementation here, then append whatever the list missed by running
// the default global order through the same expansion. The result mentions
// every allowed backend exactly once, so ordering is total and the runtime
// chain always has a next rung.
static void md_expand_order(E e, const md_order_item_t* items, int item_count) {
    const md_entry_desc_t& d = k_md_entries[static_cast<int>(e)];
    B* out = s_md_requested[static_cast<int>(e)];
    int n = 0;
    unsigned seen = 0;

    auto push = [&](B b) {
        if ((d.allowed & md_bit(b)) == 0) return;
        if (seen & md_bit(b)) return;
        seen |= md_bit(b);
        out[n++] = b;
    };

    for (int i = 0; i < item_count; ++i) {
        push(items[i].is_native ? d.native_backend : items[i].backend);
    }
    // Pad with the default order so a hand-edited partial list still ranks every
    // backend. "native" sits first in the default, so the entry's native form
    // leads the padding as well.
    for (const char* name : k_md_default_global_order) {
        if (std::string(name) == "native") {
            push(d.native_backend);
        } else {
            B b;
            if (md_parse_backend(name, &b)) push(b);
        }
    }
    s_md_requested_len[static_cast<int>(e)] = n;
}

// Reads multidrawOrder / multidrawOrder<EntryPoint> into s_md_requested. Runs in
// init_settings(): config.json is loaded but GL is not, so no capability checks
// happen here.
static void parse_multidraw_orders() {
    md_order_item_t global_items[MD_BACKEND_COUNT + 1];
    int global_count = 0;

    const std::string raw_global = md_config_string("multidrawOrder");
    if (!raw_global.empty()) {
        global_count =
            md_parse_order_list("multidrawOrder", raw_global, true, global_items, MD_BACKEND_COUNT + 1);
    }
    if (global_count == 0) {
        for (const char* name : k_md_default_global_order) {
            md_order_item_t item{};
            if (std::string(name) == "native") {
                item.is_native = true;
            } else if (!md_parse_backend(name, &item.backend)) {
                continue;
            }
            global_items[global_count++] = item;
        }
    }

    for (int i = 0; i < MD_ENTRY_COUNT; ++i) {
        const E e = static_cast<E>(i);
        const md_entry_desc_t& d = k_md_entries[i];
        const std::string raw = md_config_string(d.order_key);
        if (!raw.empty()) {
            // Exception order: concrete backends only. Names that are not a
            // distinct implementation here are rejected in md_expand_order, with
            // d.why explaining the reason once below.
            md_order_item_t items[MD_BACKEND_COUNT];
            const int count = md_parse_order_list(d.order_key, raw, false, items, MD_BACKEND_COUNT);
            for (int k = 0; k < count; ++k) {
                if (!items[k].is_native && (d.allowed & md_bit(items[k].backend)) == 0) {
                    LOG_W_FORCE("%s: '%s' is not a distinct strategy for %s (%s), ignored", d.order_key,
                                md_backend_name(items[k].backend), d.label, d.why)
                }
            }
            md_expand_order(e, items, count);
        } else {
            md_expand_order(e, global_items, global_count);
        }
    }
}

void set_multidraw_setting() { // should be called after init_gles_target()
    // The pre-2.0 selection keys are no longer read: ordering replaced them.
    if (config_get_int(const_cast<char*>("multidrawMode")) != -1 ||
        config_get_string(const_cast<char*>("multidrawDisableBackends")) != nullptr) {
        LOG_W_FORCE("multidrawMode/multidrawDisableBackends are no longer used. The selection is an order "
                    "now: multidrawOrder (global, backend names best first, may contain \"native\") and "
                    "multidrawOrder<EntryPoint> for per-function exceptions.")
    }
    for (const auto& d : k_md_entries) {
        if (config_get_string(const_cast<char*>(d.legacy_mode_key)) != nullptr) {
            LOG_W_FORCE("%s is no longer used; see multidrawOrder / %s", d.legacy_mode_key, d.order_key)
            break;
        }
    }
}

md_backend_t md_next_backend(md_entry_t e, md_backend_t cur) {
    const int idx = static_cast<int>(e);
    const int len = global_settings.multidraw_order_len[idx];
    const B* order = global_settings.multidraw_order[idx];
    if (len <= 0) return B::Unroll; // cannot happen after init_settings_post; be safe
    for (int i = 0; i < len; ++i) {
        if (order[i] == cur) {
            return order[i + 1 < len ? i + 1 : len - 1];
        }
    }
    // `cur` is not ranked here: a directly dlsym'ed symbol. Hand it the terminal
    // rung so the walk ends.
    return order[len - 1];
}

void init_settings_post() {
    const bool has_es31 = (g_gles_caps.major > 3) || (g_gles_caps.major == 3 && g_gles_caps.minor >= 1);
    const bool has_es32 = (g_gles_caps.major > 3) || (g_gles_caps.major == 3 && g_gles_caps.minor >= 2);
    const bool has_bv_ext =
        g_gles_caps.GL_EXT_draw_elements_base_vertex || g_gles_caps.GL_OES_draw_elements_base_vertex;

    // A capability counts only when the extension string *and* the resolved entry
    // point agree. The GLES loader uses a plain dlsym, so a driver can advertise
    // GL_EXT_multi_draw_indirect while the symbol is missing from the library that
    // was actually opened; trusting the string alone meant a null jump on the
    // first frame that issued a multi-draw.
    const bool multidraw = g_gles_caps.GL_EXT_multi_draw_indirect && GLES.glMultiDrawElementsIndirectEXT != nullptr;
    const bool basevertex = (has_bv_ext || has_es32) && GLES.glDrawElementsBaseVertex != nullptr;
    const bool indirect = has_es31 && GLES.glDrawElementsIndirect != nullptr;
    // EXT/OES_draw_elements_base_vertex also define the multi-draw form, whose
    // signature matches GL 3.2 core exactly -- but only when EXT_multi_draw_arrays
    // is supported as well. gl/multidraw.cpp checks that string, because neither
    // the resolved symbol nor a runtime probe can: a driver without the extension
    // accepts the call, draws nothing, and reports no error.
    const bool multibasevertex = mg_multi_draw_elements_basevertex_ext_available();

    // Compute mode used to be accepted without checking anything at all.
    bool compute = false;
    if (has_es31 && GLES.glDispatchCompute) {
        GLint ssbo_blocks = 0;
        GLES.glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &ssbo_blocks);
        // The multidraw compute shader declares exactly four shader storage
        // blocks, which is the GLES 3.1 guaranteed minimum.
        compute = ssbo_blocks >= 4;
        if (!compute) LOG_W_FORCE("Compute multidraw needs 4 SSBO blocks, driver reports %d", ssbo_blocks)
    }

    // ---- per-entry-point backend selection ----
    md_caps_t md_caps = {};
    md_caps.basevertex = basevertex;
    md_caps.indirect_elements = indirect;
    md_caps.indirect_arrays = has_es31 && GLES.glDrawArraysIndirect != nullptr;
    md_caps.multiindirect_elements = multidraw;
    md_caps.multiindirect_arrays =
        g_gles_caps.GL_EXT_multi_draw_indirect && GLES.glMultiDrawArraysIndirectEXT != nullptr;
    md_caps.multibasevertex = multibasevertex;
    md_caps.multiarrays = mg_multi_draw_arrays_ext_available();
    md_caps.compute = compute;

    // Filter each entry's requested order down to what this device can run. The
    // result is the runtime fallback chain; its first item is the resolved
    // backend. Unroll survives for the three list-taking entry points because it
    // is always available; for the two *Indirect entry points an empty result
    // means the context has no indirect draw at all, and their single exported
    // function already warns and draws nothing in that case -- keep the best
    // rung anyway so the order is never empty.
    for (int i = 0; i < MD_ENTRY_COUNT; ++i) {
        const md_entry_t e = static_cast<md_entry_t>(i);
        int n = 0;
        for (int k = 0; k < s_md_requested_len[i]; ++k) {
            const B cand = s_md_requested[i][k];
            if (!md_backend_available(e, cand, md_caps)) continue;
            global_settings.multidraw_order[i][n++] = cand;
        }
        if (n == 0) {
            LOG_W_FORCE("%s: no backend in the order is available on this device; keeping %s", k_md_entries[i].label,
                        md_backend_name(s_md_requested[i][0]))
            global_settings.multidraw_order[i][n++] = s_md_requested[i][0];
        }
        global_settings.multidraw_order_len[i] = n;
        global_settings.multidraw_backend[i] = global_settings.multidraw_order[i][0];

        std::string order_str;
        for (int k = 0; k < n; ++k) {
            if (!order_str.empty()) order_str += " > ";
            order_str += md_backend_name(global_settings.multidraw_order[i][k]);
        }
        LOG_V("[MobileGlues] %-34s = %s", k_md_entries[i].order_key, order_str.c_str())
    }
}

std::string dump_settings_string(std::string prefix) {
    std::stringstream ss;

    ss << prefix << "Angle: " << (global_settings.angle == AngleMode::Enabled ? "Enabled" : "Disabled") << "\n";
    ss << prefix << "IgnoreError: ";
    switch (global_settings.ignore_error) {
    case IgnoreErrorLevel::None:
        ss << "None";
        break;
    case IgnoreErrorLevel::Partial:
        ss << "Partial";
        break;
    case IgnoreErrorLevel::Full:
        ss << "Full";
        break;
    }
    ss << "\n";

    ss << prefix << "ExtComputeShader: " << (global_settings.ext_compute_shader ? "True" : "False") << "\n";
    ss << prefix << "ExtTimerQuery: " << (global_settings.ext_timer_query ? "True" : "False") << "\n";
    ss << prefix << "ExtDirectStateAccess: " << (global_settings.ext_direct_state_access ? "True" : "False") << "\n";
    ss << prefix << "MaxGlslCacheSize: " << (global_settings.max_glsl_cache_size / 1024 / 1024) << "MB\n";

    for (int i = 0; i < MD_ENTRY_COUNT; ++i) {
        ss << prefix << k_md_entries[i].order_key << ": ";
        for (int k = 0; k < global_settings.multidraw_order_len[i]; ++k) {
            if (k > 0) ss << " > ";
            ss << md_backend_name(global_settings.multidraw_order[i][k]);
        }
        ss << "\n";
    }

    ss << prefix << "AngleDepthClearFixMode: "
       << (global_settings.angle_depth_clear_fix_mode == AngleDepthClearFixMode::Disabled ? "Disabled" : "Enabled")
       << "\n";

    ss << prefix << "BufferCoherentAsFlush: " << (global_settings.buffer_coherent_as_flush ? "True" : "False") << "\n";

    ss << prefix << "CustomGLVersion: "
       << ((GLVersion.toInt(2) == DEFAULT_GL_VERSION) ? "(Default)" : std::to_string(GLVersion.toInt(2))) << "\n";

    ss << prefix << "Fsr1Setting: ";

    switch (global_settings.fsr1_setting) {
    case FSR1_Quality_Preset::Disabled:
        ss << "Disabled";
        break;
    case FSR1_Quality_Preset::UltraQuality:
        ss << "UltraQuality";
        break;
    case FSR1_Quality_Preset::Quality:
        ss << "Quality";
        break;
    case FSR1_Quality_Preset::Balanced:
        ss << "Balanced";
        break;
    case FSR1_Quality_Preset::Performance:
        ss << "Performance";
        break;
    default:
        ss << "Unknown";
        break;
    }
    ss << "\n";

    ss << prefix << "HideMGEnvLevel: "
       << ((global_settings.hide_mg_env_level == HideMGEnvLevel::Disabled)
               ? "Disabled"
               : std::to_string(static_cast<int>(global_settings.hide_mg_env_level)));

    ss << "\n";

    return ss.str();
}
