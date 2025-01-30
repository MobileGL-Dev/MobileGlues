#ifndef _MOBILEGLUES_LOADER_H_
#define _MOBILEGLUES_LOADER_H_

#include <stdbool.h>
#include "../gl/log.h"
#include "../gl/gl.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void *(*gles_getProcAddress)(const char *name);
void *proc_address(void *lib, const char *name);
extern void *gles, *egl;


#ifdef __cplusplus
}
#endif

void init_target_gles();

#define WARN_NULL(name) if (name == NULL) { LOG_W("%s line %d function %s: " #name " is NULL\n", __FILE__, __LINE__, __func__); }

void *open_lib(const char **names, const char *override);
#define LOAD_RAW_GLES(name, type, ...)                                      \
    {                                                                       \
        if (gles != NULL) {                                                 \
            gles_##name = (name##_PTR)proc_address(gles, #name);            \
        }                                                                   \
        WARN_NULL(gles_##name);                                             \
    }

#define LOAD_LIB(type, name, ...)                                           \
    typedef type (*name##_PTR)(__VA_ARGS__);                                \
    name##_PTR gles_##name = NULL;                                          \
    LOAD_RAW_GLES(name, type, __VA_ARGS__)

#define LOAD_GLES(name,type, ...)            LOAD_LIB(type, name, __VA_ARGS__)
#define LOAD_GLES2(name,type, ...)           LOAD_LIB(type, name, __VA_ARGS__)
#define LOAD_GLES3(name,type, ...)           LOAD_LIB(type, name, __VA_ARGS__)
#define LOAD_EGL(name) \
static name##_PTR egl_##name = NULL; \
{ \
        static bool first = true; \
        if (first) { \
            first = false; \
            if (egl != NULL) { \
                egl_##name = (name##_PTR)proc_address(egl, #name); \
            } \
            WARN_NULL(egl_##name); \
        } \
}

#define CHECK_GL_ERROR                                                      \
    LOAD_GLES(glGetError, GLenum)                                           \
    GLenum ERR = gles_glGetError();                                         \
    if (ERR != GL_NO_ERROR)                                                 \
        LOG_E("ERROR: %d", ERR)                                             \

#define NATIVE_FUNCTION_HEAD(type,name,...)                                 \
GLAPI GLAPIENTRY type name##ARB(__VA_ARGS__) __attribute__((alias(#name))); \
GLAPI GLAPIENTRY type name(__VA_ARGS__)  {                                  \
typedef type (*name##_PTR)(__VA_ARGS__);                                    \
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

#define STUB_FUNCTION_HEAD(type,name,...)                                   \
GLAPI GLAPIENTRY type name(__VA_ARGS__) {

#define STUB_FUNCTION_END(type,name,...)                                    \
    LOG_W("No function: %s @ %s(...)", RENDERERNAME, __FUNCTION__);         \
    return (type)0;                                                         \
}

#define STUB_FUNCTION_END_NO_RETURN(type,name,...)                          \
    LOG_W("No function: %s @ %s(...)", RENDERERNAME, __FUNCTION__);         \
}

#endif // _MOBILEGLUES_LOADER_H_
