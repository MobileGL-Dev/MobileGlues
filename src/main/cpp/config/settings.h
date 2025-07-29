//
// Created by hanji on 2025/2/9.
//

#ifndef MOBILEGLUES_PLUGIN_SETTINGS_H
#define MOBILEGLUES_PLUGIN_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__APPLE__)
#include <__stddef_size_t.h>
#else
    typedef unsigned long size_t;
#endif

    enum class multidraw_mode_t : int {
        Auto = 0,
        PreferIndirect,
        PreferBaseVertex,
        PreferMultidrawIndirect,
        DrawElements,
        Compute,
        MaxValue
    };

    enum class AngleConfig : int {
        DisableIfPossible = 0,
        EnableIfPossible = 1,
        ForceDisable = 2,
        ForceEnable = 3
    };

    enum class AngleMode : int {
        Disabled = 0,
        Enabled = 1
    };

    enum class IgnoreErrorLevel : int {
        None = 0,
        Partial = 1,
        Full = 2
    };

    enum class NoErrorConfig : int {
        Auto = 0,
        Disable = 1,
        Level1 = 2,
        Level2 = 3
    };

    enum class AngleDepthClearFixMode : int {
        Disabled = 0,
        Mode1 = 1,
        Mode2 = 2,
        MaxValue
	};

    struct global_settings_t {
        AngleMode angle;
        IgnoreErrorLevel ignore_error;
        bool ext_gl43;
        bool ext_compute_shader;
        bool ext_timer_query;
        bool ext_direct_state_access;
        bool buffer_coherent_as_flush;
        size_t max_glsl_cache_size;
        multidraw_mode_t multidraw_mode;
        AngleDepthClearFixMode angle_depth_clear_fix_mode;
    };

    extern struct global_settings_t global_settings;

    void init_settings();
    void init_settings_post();

#ifdef __cplusplus
}
#endif

#endif //MOBILEGLUES_PLUGIN_SETTINGS_H
