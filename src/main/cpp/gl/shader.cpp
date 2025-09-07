//
// Created by BZLZHH on 2025/1/26.
//

#include <cctype>
#include "shader.h"

#include <GL/gl.h>
#include "log.h"
#include "program.h"
#include "../gles/loader.h"
#include "../includes.h"
#include "glsl/glsl_for_es.h"
#include "../config/settings.h"
#include "FSR1/FSR1.h"
#include <unordered_map>
#include <vector>
#include <string_view>

#define DEBUG 0

struct shader_t shaderInfo;
std::unordered_map<GLuint, bool> shader_map_is_sampler_buffer_emulated;
std::unordered_map<GLuint, bool> shader_map_is_atomic_counter_emulated;

bool can_run_essl3(unsigned int esversion, const char *glsl) {
    if (strncmp(glsl, "#version 100", 12) == 0) return true;

    unsigned int glsl_version = 0;
    if      (strncmp(glsl, "#version 300 es", 15) == 0) glsl_version = 300;
    else if (strncmp(glsl, "#version 310 es", 15) == 0) glsl_version = 310;
    else if (strncmp(glsl, "#version 320 es", 15) == 0) glsl_version = 320;
    else return false;
    return esversion >= glsl_version;
}

inline bool is_direct_shader(const char *glsl) {
    return can_run_essl3(hardware->es_version, glsl);
}

inline bool check_if_sampler_buffer_used(const std::string_view &str) {
    return str.find("samplerBuffer") != std::string_view::npos;
}

// 优化字符串拼接与内存分配
void glShaderSource(GLuint shader, GLsizei count, const GLchar *const* string, const GLint *length) {
#if DEBUG
    LOG()
#endif
    shaderInfo.id = 0;
    shaderInfo.converted.clear();
    shaderInfo.frag_data_changed = 0;

    // 计算总长度，一次性分配
    size_t total_len = 0;
    if (length) {
        for (int i = 0; i < count; ++i)
            total_len += (length[i] >= 0) ? length[i] : strlen(string[i]);
    } else {
        for (int i = 0; i < count; ++i)
            total_len += strlen(string[i]);
    }

    std::string glsl_src;
    glsl_src.reserve(total_len + 1);

    if (length) {
        for (int i = 0; i < count; ++i) {
            if (length[i] >= 0)
                glsl_src.append(string[i], length[i]);
            else
                glsl_src.append(string[i]);
        }
    } else {
        for (int i = 0; i < count; ++i)
            glsl_src.append(string[i]);
    }

    bool is_sampler_buffer_emulated = hardware->emulate_texture_buffer && check_if_sampler_buffer_used(glsl_src);

    std::string essl_src;
    if (is_direct_shader(glsl_src.c_str())) {
#if DEBUG
        LOG_D("[INFO] [Shader] Direct shader source: ")
        LOG_D("%s", glsl_src.c_str())
#endif
        essl_src = std::move(glsl_src);
    } else {
        int glsl_version = getGLSLVersion(glsl_src.c_str());
#if DEBUG
        LOG_D("[INFO] [Shader] Shader source: ")
        LOG_D("%s", glsl_src.c_str())
#endif
        GLint shaderType;
        GLES.glGetShaderiv(shader, GL_SHADER_TYPE, &shaderType);
        int return_code = 0;
        essl_src = GLSLtoGLSLES(glsl_src.c_str(), shaderType, hardware->es_version, glsl_version, return_code);
        if (return_code == 1) { //atomicCounterEmulated
            shader_map_is_atomic_counter_emulated[shader] = true;
#if DEBUG
            LOG_D("[INFO] [Shader] Atomic counter emulated in shader %d", shader)
#endif
        }

        if (essl_src.empty()) {
            LOG_E("Failed to convert shader %d.", shader)
            return;
        }
#if DEBUG
        LOG_D("\n[INFO] [Shader] Converted Shader source: \n%s", essl_src.c_str())
#endif
    }

    if (!essl_src.empty()) {
        shaderInfo.id = shader;
        shaderInfo.converted = essl_src;
        const char* s[] = { essl_src.c_str() };
        GLES.glShaderSource(shader, 1, s, nullptr); // always 1 entry
        if (hardware->emulate_texture_buffer)
            shader_map_is_sampler_buffer_emulated[shader] = is_sampler_buffer_emulated;
    } else {
        LOG_E("Failed to convert glsl.")
    }
    CHECK_GL_ERROR
}

void glGetShaderiv(GLuint shader, GLenum pname, GLint *params) {
#if DEBUG
    LOG()
#endif
    GLES.glGetShaderiv(shader, pname, params);
    if (global_settings.ignore_error >= IgnoreErrorLevel::Partial && pname == GL_COMPILE_STATUS && !*params) {
        GLchar infoLog[512];
        GLES.glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        LOG_W_FORCE("Shader %d compilation failed: \n%s", shader, infoLog)
        LOG_W_FORCE("Now try to cheat.")
        *params = GL_TRUE; // 强制通过
    }
    CHECK_GL_ERROR
}

GLuint glCreateShader(GLenum shaderType) {
    if (global_settings.fsr1_setting != FSR1_Quality_Preset::Disabled && !fsrInitialized)
        InitFSRResources();

#if DEBUG
    LOG()
    LOG_D("glCreateShader(%s)", glEnumToString(shaderType))
#endif
    GLuint shader = GLES.glCreateShader(shaderType);
    if (shader && hardware->emulate_texture_buffer)
        shader_map_is_sampler_buffer_emulated[shader] = false;
    CHECK_GL_ERROR
    return shader;
}