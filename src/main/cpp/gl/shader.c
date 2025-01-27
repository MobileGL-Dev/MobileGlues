//
// Created by BZLZHH on 2025/1/26.
//

#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <android/log.h>
#include "shader.h"

#include "gl.h"
#include "log.h"
#include "../gles/loader.h"
#include "../includes.h"
#include "glsl/glsl_for_es.h"

bool can_run_essl3(int esversion, const char *glsl) {
    int glsl_version = 0;

    if (strncmp(glsl, "#version 100", 12) == 0) {
        return true;
    } else if (strncmp(glsl, "#version 300 es", 15) == 0) {
        return true;
    } else if (strncmp(glsl, "#version 310 es", 15) == 0) {
        glsl_version = 310;
    } else if (strncmp(glsl, "#version 320 es", 15) == 0) {
        glsl_version = 320;
    } else {
        return false;
    }

    if (esversion >= glsl_version) {
        return true;
    } else {
        return false;
    }
}

bool is_direct_shader(char *glsl)
{
    bool es3_ability = can_run_essl3(320, glsl); //Assuming it is 320
    return es3_ability;
}

void glShaderSource(GLuint shader, GLsizei count, const GLchar *const* string, const GLint *length) {    
    LOG();
    int l = 0;
    for (int i=0; i<count; i++) l+=(length && length[i] >= 0)?length[i]:strlen(string[i]);
    char* source = NULL;
    char* converted = NULL;
    source = malloc(l+1);
    memset(source, 0, l+1);
    if(length) {
        for (int i=0; i<count; i++) {
            if(length[i] >= 0)
                strncat(source, string[i], length[i]);
            else
                strcat(source, string[i]);
        }
    } else {
        for (int i=0; i<count; i++)
            strcat(source, string[i]);
    }
    LOAD_GLES2(glShaderSource, void, GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
    if (gles_glShaderSource) {
        if(is_direct_shader(source))
            converted = strdup(source);
        else{
            int glsl_version = getGLSLVersion(source);
            LOG_D("[INFO] [Shader] Shader source: ");
            LOG_D("%s", source);
            GLint shaderType;
            glGetShaderiv(shader, GL_SHADER_TYPE, &shaderType);
            if(glsl_version < 140) {
                
            }
            else {
                
                converted = GLSLtoGLSLES(source, shaderType, 320);
            }
            LOG_D("\n[INFO] [Shader] Converted Shader source: \n%s", converted);
        }

        gles_glShaderSource(shader, count, (const GLchar * const*)((converted)?(&converted):(&source)), length);
    }
}
    
