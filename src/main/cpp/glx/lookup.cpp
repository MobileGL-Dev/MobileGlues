//
// Created by BZLZHH on 2025/1/27.
//

#include "lookup.h"

#include <stdio.h>
#include <dlfcn.h>
#include <EGL/egl.h>
#include <string.h>
#include "../includes.h"
#include "../gl/log.h"
#include "../gl/envvars.h"

#define DEBUG 0

void *glXGetProcAddress(const char *name) {
    LOG()
#if defined __APPLE__
    return dlsym((void*)(~(uintptr_t)0), name);
#else
    void* proc = dlsym(RTLD_DEFAULT, (const char*)name);

    if (!proc) {
        fprintf(stderr, "Failed to get OpenGL function %s: %s\n", name, dlerror());
        LOG_W("Failed to get OpenGL function: %s", (const char*)name);
        return nullptr;
    }

    return proc;
#endif
}

void *glXGetProcAddressARB(const char *name) {
    return glXGetProcAddress(name);
}