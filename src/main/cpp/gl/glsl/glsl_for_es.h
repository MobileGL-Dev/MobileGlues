#ifndef GLSL_FOR_ES
#define GLSL_FOR_ES
#include <stdio.h>
#include "../includes.h"
#include "../gl.h"
#include "../glcorearb.h"
#include "../log.h"
#include "../loader.h"
#include "../gles/loader.h"

#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif
__attribute__((visibility("default"))) extern char *GLSLtoGLSLES(char *glsl_code, GLenum glsl_type, uint essl_version);
__attribute__((visibility("default"))) extern int getGLSLVersion(const char* glsl_code);
#ifdef __cplusplus
}
#endif
#endif