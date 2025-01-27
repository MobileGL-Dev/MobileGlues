//
// Created by Swung 0x48 on 2024/10/10.
//

#include <linux/limits.h>
#include <string.h>
#include <stdio.h>
#include "loader.h"
#include "../includes.h"
#include "loader.h"
#include "../gl/gl.h"
#include "../gl/glext.h"
#include "../gl/envvars.h"
#include "../gl/log.h"
#include "../gl/mg.h"

void *gles = NULL, *egl = NULL;

static const char *path_prefix[] = {
        "",
        "/opt/vc/lib/",
        "/usr/local/lib/",
        "/usr/lib/",
        NULL,
};

static const char *lib_ext[] = {
#ifndef NO_GBM
        "so.19",
#endif
        "so",
        "so.1",
        "so.2",
        "dylib",
        "dll",
        NULL,
};

static const char *gles3_lib[] = {
        "libGLESv3_CM",
        "libGLESv3",
        NULL
};

static const char *egl_lib[] = {
#if defined(BCMHOST)
        "libbrcmEGL",
#endif
        "libEGL",
        NULL
};

void *open_lib(const char **names, const char *override) {
    void *lib = NULL;

    char path_name[PATH_MAX + 1];
    int flags = RTLD_LOCAL | RTLD_NOW;
    if (override) {
        if ((lib = dlopen(override, flags))) {
            strncpy(path_name, override, PATH_MAX);
            LOG_D("LIBGL:loaded: %s\n", path_name);
            return lib;
        } else {
            LOG_E("LIBGL_GLES override failed: %s\n", dlerror());
        }
    }
    for (int p = 0; path_prefix[p]; p++) {
        for (int i = 0; names[i]; i++) {
            for (int e = 0; lib_ext[e]; e++) {
                snprintf(path_name, PATH_MAX, "%s%s.%s", path_prefix[p], names[i], lib_ext[e]);
                if ((lib = dlopen(path_name, flags))) {
                    return lib;
                }
            }
        }
    }
    return lib;
}

void load_libs() {
    static int first = 1;
    if (! first) return;
    first = 0;
    const char *gles_override = GetEnvVar("LIBGL_GLES");
    gles = open_lib(gles3_lib, gles_override);
    const char *egl_override = GetEnvVar("LIBGL_EGL");
    egl = open_lib(egl_lib, egl_override);
}

void *(*gles_getProcAddress)(const char *name);

void *proc_address(void *lib, const char *name) {
    if (gles_getProcAddress)
        return gles_getProcAddress(name);
    return dlsym(lib, name);
}

void set_hard_ext() {
    hard_ext = (hard_ext_t)calloc(1, sizeof(struct hard_ext_s));
    
}

void init_gl_state() {
    gl_state = (gl_state_t)calloc(1, sizeof(struct gl_state_s));
    set_gl_state_proxy_height(0);
    set_gl_state_proxy_width(0);
    set_gl_state_proxy_intformat(0);
}

void init_target_gles() {
    LOG_I("Initializing %s @ %s", RENDERERNAME, __FUNCTION__);
    LOG_I("Initializing %s @ OpenGL ES", RENDERERNAME);
    load_libs();
    LOG_I("Initializing %s @ hard_ext", RENDERERNAME);
    set_hard_ext();
    LOG_I("Initializing %s @ gl_state", RENDERERNAME);
    init_gl_state();
}