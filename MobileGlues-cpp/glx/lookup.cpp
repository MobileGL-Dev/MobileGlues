// MobileGlues - glx/lookup.cpp
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

#include "lookup.h"

#include "../config/settings.h"
#include "../gl/envvars.h"
#include "../gl/log.h"
#include "../gl/mg.h"
#include "../includes.h"
#include <EGL/egl.h>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>

#define DEBUG 0

std::string handle_multidraw_func_name(std::string name) {
    std::string namestr = name;
    if (namestr != "glMultiDrawElementsBaseVertex" && namestr != "glMultiDrawElements") {
        return name;
    } else {
        namestr = "mg_" + namestr;
    }

    switch (global_settings.multidraw_mode) {
    case multidraw_mode_t::PreferIndirect:
        namestr += "_indirect";
        break;
    case multidraw_mode_t::PreferBaseVertex:
        namestr += "_basevertex";
        break;
    case multidraw_mode_t::PreferMultidrawIndirect:
        namestr += "_multiindirect";
        break;
    case multidraw_mode_t::DrawElements:
        namestr += "_drawelements";
        break;
    case multidraw_mode_t::Compute:
        namestr += "_compute";
        break;
    default:
        LOG_W("get_multidraw_func() cannot determine multidraw emulation mode!")
        return {};
    }

    return namestr;
}

void* glXGetProcAddress(const char* name) {
    LOG()
    std::string real_func_name = handle_multidraw_func_name(std::string(name));
#ifdef __APPLE__
    // (void*)~0 is RTLD_NEXT on Darwin: "search images AFTER the caller",
    // which skips libmobileglues itself and hands the app raw ANGLE entry
    // points, bypassing every GL translation wrapper. RTLD_SELF starts the
    // search in this image so the exported wrappers win; the internal GLES
    // backend table still resolves through to ANGLE (gles/loader.cpp), so
    // wrapper -> ANGLE calls do not recurse.
    return dlsym(RTLD_SELF, real_func_name.c_str());
#else

    void* proc = nullptr;

    proc = dlsym(RTLD_DEFAULT, real_func_name.c_str());

    if (!proc) {
        LOG_W("Failed to get OpenGL function: %s", real_func_name.c_str())
        return nullptr;
    }

    return proc;
#endif
}

void* glXGetProcAddressARB(const char* name) {
    return glXGetProcAddress(name);
}