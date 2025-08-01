//
// Created by hanji on 2025/2/9.
//

#ifndef MOBILEGLUES_PLUGIN_SETTINGS_H
#define MOBILEGLUES_PLUGIN_SETTINGS_H

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>

#if !defined(__APPLE__)
#include <__stddef_size_t.h>
#else
    typedef unsigned long size_t;
#endif

#define DEFAULT_GL_VERSION 40

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

struct Version {
    int Major{ 0 };
    int Minor{ 0 };
    int Patch{ 0 };

    Version() = default;

    Version(int major, int minor, int patch)
        : Major(major), Minor(minor), Patch(patch) {
    }

    Version(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        size_t end = str.find_last_not_of(" \t\n\r");
        if (start == std::string::npos) {
            return;
        }
        std::string s = str.substr(start, end - start + 1);

        std::vector<int> parts;
        std::istringstream iss(s);
        std::string token;
        while (std::getline(iss, token, '.') && parts.size() < 3) {
            try {
                parts.push_back(std::stoi(token));
            }
            catch (...) {
                parts.push_back(0);
            }
        }
        while (parts.size() < 3) {
            parts.push_back(0);
        }
        Major = parts[0];
        Minor = parts[1];
        Patch = parts[2];
    }

    explicit Version(int code) {
        if (code < 0) code = -code;
        std::string s = std::to_string(code);
        for (size_t i = 0; i < 3; ++i) {
            int digit = 0;
            if (i < s.size() && std::isdigit(s[i])) {
                digit = s[i] - '0';
            }
            switch (i) {
            case 0: Major = digit; break;
            case 1: Minor = digit; break;
            case 2: Patch = digit; break;
            }
        }
    }

    std::string toString() const {
        std::ostringstream oss;
        oss << Major << '.' << Minor << '.' << Patch;
        return oss.str();
    }

    int toInt(int digit_count = 3) const {
        switch (digit_count) {
        case 1:
            return Major;
        case 2:
            return Major * 10 + Minor;
        default:
            return Major * 100 + Minor * 10 + Patch;
        }
    }

    bool isEmpty() const {
        return Major == 0 && Minor == 0 && Patch == 0;
    }
};

typedef enum class FSR1_Quality_Preset : int { // may be useless
    Disabled = 0,
    UltraQuality, // 1
    Quality,      // 2 
	Balanced,     // 3
	Performance,  // 4
	MaxValue      // 5
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
	Version custom_gl_version;
	FSR1_Quality_Preset fsr1_setting;
};

extern global_settings_t global_settings;

void init_settings();
void init_settings_post();
    
#endif //MOBILEGLUES_PLUGIN_SETTINGS_H
