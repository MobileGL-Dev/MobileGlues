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

#ifndef __APPLE__
namespace {

// A hidden address in this translation unit gives dladdr an unambiguous route
// back to this exact MobileGlues image, even when another GL implementation is
// earlier in the process-global symbol scope.
void mobileglues_lookup_anchor() {}

struct self_dso_info_t {
    const char* path;
    void* base;
};

const self_dso_info_t& get_self_dso_info() {
    static const self_dso_info_t info = [] {
        Dl_info dl_info{};
        if (dladdr(reinterpret_cast<const void*>(&mobileglues_lookup_anchor), &dl_info) == 0 ||
            dl_info.dli_fname == nullptr || dl_info.dli_fbase == nullptr) {
            return self_dso_info_t{nullptr, nullptr};
        }
        return self_dso_info_t{dl_info.dli_fname, dl_info.dli_fbase};
    }();
    return info;
}

void* lookup_self_symbol(const char* name) {
    const self_dso_info_t& self = get_self_dso_info();
    if (self.path == nullptr) return nullptr;

    int flags = RTLD_NOW | RTLD_LOCAL;
#ifdef RTLD_NOLOAD
    // MobileGlues is necessarily loaded while this code is running. NOLOAD
    // makes that invariant explicit and cannot run this DSO's constructors.
    flags |= RTLD_NOLOAD;
#endif
    void* handle = dlopen(self.path, flags);
    if (handle == nullptr) return nullptr;

    void* proc = dlsym(handle, name);
    Dl_info owner{};
    // dlsym on a DSO handle may also search its DT_NEEDED dependencies. Only
    // accept an address from this image; dependency/global hits belong to the
    // existing fallback below.
    const bool belongs_to_self = proc != nullptr && dladdr(proc, &owner) != 0 && owner.dli_fbase == self.base;
    dlclose(handle);
    return belongs_to_self ? proc : nullptr;
}

} // namespace
#endif

// The application can ask for a multi-draw entry point by name and call the
// result directly, bypassing the dispatcher in gl/multidraw.cpp. Hand back the
// symbol implementing whatever backend that entry point resolved to, so both
// routes agree. The suffix table lives in config/settings.cpp for exactly that
// reason.
std::string handle_multidraw_func_name(std::string name) {
    md_entry_t entry;
    if (name == "glMultiDrawElements") {
        entry = md_entry_t::Elements;
    } else if (name == "glMultiDrawElementsBaseVertex") {
        entry = md_entry_t::ElementsBaseVertex;
    } else {
        // Everything else -- glMultiDrawArrays, the two *Indirect entry points and
        // every EXT/ARB alias -- is a single exported definition that selects its
        // own backend internally, so the plain name is already correct.
        return name;
    }

    const char* suffix = md_backend_suffix(multidraw_backend_of(entry));
    if (!suffix) {
        // Auto should never survive init_settings_post. Fall back to the
        // dispatcher rather than to dlsym of a name that does not exist.
        LOG_W_FORCE("handle_multidraw_func_name: %s has no resolved backend, using the dispatcher", name.c_str())
        return name;
    }
    return "mg_" + name + suffix;
}

void* glXGetProcAddress(const char* name) {
    LOG()
    if (name == nullptr) return nullptr;
    std::string real_func_name = handle_multidraw_func_name(std::string(name));
#ifdef __APPLE__
    return dlsym((void*)(~(uintptr_t)0), real_func_name.c_str());
#else

    void* proc = lookup_self_symbol(real_func_name.c_str());
    if (!proc) proc = dlsym(RTLD_DEFAULT, real_func_name.c_str());

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
