#ifndef _MOBILEGLUES_LOADER_H_
#define _MOBILEGLUES_LOADER_H_

#include <stdbool.h>
#include "../gl/log.h"
#include "../gl/gl.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void *(*gles_getProcAddress)(const char *name);
void *proc_address(void *lib, const char *name);
extern void *gles, *egl;

#define WARN_NULL(name) if (name == NULL) { LOG_W("%s line %d function %s: " #name " is NULL\n", __FILE__, __LINE__, __func__); }

#define PUSH_IF_COMPILING_EXT(nam, ...)                                     \
    if (glstate->list.active) {                                             \
        if (!glstate->list.pending) {                                       \
            NewStage(glstate->list.active, STAGE_GLCALL);                   \
            push_##nam(__VA_ARGS__);                                        \
            noerrorShim();						                            \
            return (nam##_RETURN)0;                                         \
        }                                                                   \
        else gl4es_flush();                                                 \
    }

#define LOAD_RAW_GLES(name, type, ...)                                      \
    {                                                                       \
        static bool first = true;                                           \
        if (first) {                                                        \
            first = false;                                                  \
            if (gles != NULL) {                                             \
                gles_##name = (name##_PTR)proc_address(gles, #name);        \
            }                                                               \
            WARN_NULL(gles_##name);                                         \
        }                                                                   \
    }

#define LOAD_LIB(type, name, ...)                                           \
    typedef type (*name##_PTR)(__VA_ARGS__);                                \
    name##_PTR gles_##name = NULL;                                          \
    LOAD_RAW_GLES(name, type, __VA_ARGS__)

#define LOAD_GLES(name,type, ...)            LOAD_LIB(type, name, __VA_ARGS__)
#define LOAD_GLES2(name,type, ...)           LOAD_LIB(type, name, __VA_ARGS__)
#define LOAD_GLES3(name,type, ...)           LOAD_LIB(type, name, __VA_ARGS__)

#define NATIVE_FUNCTION_HEAD(type,name,...)                                 \
GLAPI GLAPIENTRY type name(__VA_ARGS__) {                                   \
    typedef type (*name##_PTR)(__VA_ARGS__);                                \
    name##_PTR gles_##name = NULL;

#define NATIVE_FUNCTION_END(type,name,...)                                  \
    LOG_D("Use native function: %s @ %s(...)", RENDERERNAME, __FUNCTION__); \
    LOAD_RAW_GLES(name, type, __VA_ARGS__);                                 \
    return gles_##name(__VA_ARGS__);                                        \
}

#define NATIVE_FUNCTION_END_NO_RETURN(type,name,...)                        \
    LOG_D("Use native function: %s @ %s(...)", RENDERERNAME, __FUNCTION__); \
    LOAD_RAW_GLES(name, type, __VA_ARGS__);                                 \
    gles_##name(__VA_ARGS__);                                               \
    LOAD_GLES(glGetError, GLenum)                                           \
    GLenum ERR = gles_glGetError();                                         \
    if (ERR != GL_NO_ERROR)                                                 \
        LOG_E("ERROR: %d", ERR)                                             \
}

#define LOAD_EGL(name) LOAD_LIB(egl, name)

#endif // _MOBILEGLUES_LOADER_H_
