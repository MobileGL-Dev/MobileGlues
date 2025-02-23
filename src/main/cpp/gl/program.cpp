//
// Created by hanji on 2025/2/3.
//

#include <regex.h>
#include "log.h"
#include "shader.h"
#include "program.h"
#include "../config/settings.h"

#define DEBUG 0

void glBindFragDataLocation(GLuint program, GLuint color, const GLchar *name) {
    LOG()
    LOG_D("glBindFragDataLocation(%d, %d, %s)", program, color, name)
//    if (g_gles_caps.GL_EXT_blend_func_extended) {
//        LOAD_GLES_FUNC(glBindFragDataLocationEXT)
//        gles_glBindFragDataLocationEXT(program, color, name);
//    } else {
//        LOG_W("Warning: No GL_EXT_blend_func_extended, skipping glBindFragDataLocation...");
//    }
    char* origin_glsl = NULL;

    if (shaderInfo.frag_data_changed) {
        size_t glslLen  = strlen(shaderInfo.frag_data_changed_converted) + 1;
        origin_glsl = (char *)malloc(glslLen);
        if (origin_glsl == NULL) {
            LOG_E("Memory reallocation failed for frag_data_changed_converted\n");
            return;
        }
        strcpy(origin_glsl, shaderInfo.frag_data_changed_converted);
    } else {
        size_t glslLen  = strlen(shaderInfo.converted) + 1;
        origin_glsl = (char *)malloc(glslLen);
        if (origin_glsl == NULL) {
            LOG_E("Memory reallocation failed for converted\n");
            return;
        }
        strcpy(origin_glsl, shaderInfo.converted);
    }

    int len = strlen(name);
    int tlen = len + 32;
    char *targetPattern = (char*)malloc(sizeof(char) * tlen);
    if (!targetPattern) {
        LOG_E("Memory allocation failed for targetPattern")
        return;
    }
    sprintf(targetPattern, "out[ ]+[A-Za-z0-9 ]+[ ]+%s", name);
    regex_t regex;
    regmatch_t pmatch[1];
    char *origin = NULL;
    char *result = NULL;
    if (regcomp(&regex, targetPattern, REG_EXTENDED) != 0) {
        LOG_E("Failed to compile regex\n");
        return;
    }
    char *searchStart = origin_glsl;
    while (regexec(&regex, searchStart, 1, pmatch, 0) == 0) {
        size_t matchLen = pmatch[0].rm_eo - pmatch[0].rm_so;
        origin = (char *) malloc(matchLen + 1);
        if (!origin) {
            LOG_E("Memory allocation failed\n");
            break;
        }
        strncpy(origin, searchStart + pmatch[0].rm_so, matchLen);
        origin[matchLen] = '\0';

        size_t resultLen =
                strlen(origin) + 30; // "layout (location = )" + colorNumber + null terminator
        result = (char *) malloc(resultLen);
        if (!result) {
            LOG_E("Memory allocation failed\n");
            free(origin);
            break;
        }
        snprintf(result, resultLen, "layout (location = %d) %s", color, origin);

        char *temp = strstr(searchStart, origin);
        if (temp) {
            size_t prefixLen = temp - origin_glsl;
            size_t suffixLen = strlen(temp + matchLen);
            size_t newLen = prefixLen + strlen(result) + suffixLen + 1;

            char *newConverted = (char *) malloc(newLen);
            if (!newConverted) {
                LOG_E("Memory allocation failed\n");
                free(origin);
                free(result);
                break;
            }

            strncpy(newConverted, origin_glsl, prefixLen);
            newConverted[prefixLen] = '\0';
            strcat(newConverted, result);
            strcat(newConverted, temp + matchLen);

            size_t newLen_2 = strlen(newConverted) + 1;
            shaderInfo.frag_data_changed_converted = (char *) realloc(
                    shaderInfo.frag_data_changed_converted, newLen_2);
            if (shaderInfo.frag_data_changed_converted == NULL) {
                LOG_E("Memory reallocation failed for frag_data_changed_converted\n");
                return;
            }
            strcpy(shaderInfo.frag_data_changed_converted, newConverted);

            free(newConverted);
        }

        free(origin);
        free(result);

        searchStart += pmatch[0].rm_eo;
    }
    free(targetPattern);
    regfree(&regex);//LOG_D("Patched shader:\n%s", origin_glsl)
    free(origin_glsl);
    shaderInfo.frag_data_changed = 1;
}

void glLinkProgram(GLuint program) {
    LOG()

    LOG_D("glLinkProgram(%d)", program)
    if (shaderInfo.converted && shaderInfo.frag_data_changed) {
        LOAD_GLES(glShaderSource, void, GLuint shader, GLsizei count, const GLchar *const* string, const GLint *length)
        LOAD_GLES(glCompileShader, void, GLuint shader)
        LOAD_GLES(glDetachShader, void, GLuint program, GLuint shader)
        LOAD_GLES(glAttachShader, void, GLuint program, GLuint shader)
        gles_glShaderSource(shaderInfo.id, 1, (const GLchar * const*) &shaderInfo.frag_data_changed_converted, NULL);
        gles_glCompileShader(shaderInfo.id);
        LOAD_GLES(glGetShaderiv, void, GLuint shader, GLenum pname, GLint *params)
        GLint status = 0;
        gles_glGetShaderiv(shaderInfo.id, GL_COMPILE_STATUS, &status);
        if(status!=GL_TRUE) {
            char tmp[500];
            LOAD_GLES(glGetShaderInfoLog, void, GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
            gles_glGetShaderInfoLog(shaderInfo.id, 500, NULL, tmp);
            LOG_E("Failed to compile patched shader, log:\n%s", tmp)
        }
        gles_glDetachShader(program, shaderInfo.id);
        gles_glAttachShader(program, shaderInfo.id);
        CHECK_GL_ERROR
    }
    shaderInfo.id = 0;
    shaderInfo.converted = NULL;
    shaderInfo.frag_data_changed_converted = NULL;
    shaderInfo.frag_data_changed = 0;
    LOAD_GLES(glLinkProgram, void, GLuint program)
    gles_glLinkProgram(program);

    CHECK_GL_ERROR
}

void glGetProgramiv(GLuint program, GLenum pname, GLint *params) {
    LOG()
    LOAD_GLES_FUNC(glGetProgramiv)
    gles_glGetProgramiv(program, pname, params);
    if(global_settings.ignore_error >= 1 && (pname == GL_LINK_STATUS || pname == GL_VALIDATE_STATUS) && !*params) {
        GLchar infoLog[512];
        LOAD_GLES_FUNC(glGetShaderInfoLog)
        gles_glGetShaderInfoLog(program, 512, NULL, infoLog);
        LOG_W_FORCE("Program %d linking failed: \n%s", program, infoLog);
        LOG_W_FORCE("Now try to cheat.");
        *params = GL_TRUE;
    }
    CHECK_GL_ERROR
}