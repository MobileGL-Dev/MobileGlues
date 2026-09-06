// MobileGlues - gl/program.cpp
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

#include <regex.h>
#include "GL/glext.h"
#include "GLES3/gl32.h"
#include "log.h"
#include "shader.h"
#include "program.h"
#include <regex>
#include <cstring>
#include <iostream>
#include "../config/settings.h"
#include "drawing.h"
#include "glsl/uniform_defaults.h"

#define DEBUG 0

extern UnorderedMap<GLuint, bool> shader_map_is_sampler_buffer_emulated;
extern UnorderedMap<GLuint, std::vector<uniform_default_t>> shader_uniform_defaults;
UnorderedMap<GLuint, bool> program_map_is_sampler_buffer_emulated;

enum class ShouldGenerateFSState : int {
    Never = 0,
    Maybe = 1,
    Unknown = 2
};

UnorderedMap<GLuint, ShouldGenerateFSState> program_map_should_generate_fs;

std::string updateLayoutLocation(const std::string& esslSource, GLuint color, const char* name) {
    const std::string& shaderCode = esslSource;

    std::string pattern = std::string(R"((layout\s*$[^)]*location\s*=\s*\d+[^)]*$\s*)?)") +
                          R"(out\s+((?:highp|mediump|lowp|\w+\s+)*\w+)\s+)" + name + R"(\s*;)";

    std::string replacement = "layout (location = " + std::to_string(color) + ") out $2 " + name + ";";

    std::regex reg(pattern);
    std::string modifiedCode = std::regex_replace(shaderCode, reg, replacement);

    return modifiedCode;
}

void glBindFragDataLocation(GLuint program, GLuint color, const GLchar* name) {
    LOG()
    LOG_D("glBindFragDataLocation(%d, %d, %s)", program, color, name)

    if (strlen(name) > 8 && strncmp(name, "outColor", 8) == 0) {
        const char* numberStr = name + 8;
        bool isNumber = true;
        for (int i = 0; numberStr[i] != '\0'; ++i) {
            if (!isdigit(numberStr[i])) {
                isNumber = false;
                break;
            }
        }

        if (isNumber) {
            unsigned int extractedColor = static_cast<unsigned int>(std::stoul(numberStr));
            if (extractedColor == color) {
                // outColor was bound in glsl process. exit now
                LOG_D("Find outColor* with color *, skipping")
                return;
            }
        }
    }

    // Copied before the call, not aliased into it: the result is assigned back
    // over the same member that supplies the input.
    const std::string origin_glsl =
        shaderInfo.frag_data_changed ? shaderInfo.frag_data_changed_converted : shaderInfo.converted;

    shaderInfo.frag_data_changed_converted = updateLayoutLocation(origin_glsl, color, name);
    shaderInfo.frag_data_changed = 1;
}

static std::string DefaultFSSource;
static unsigned CurrentDefaultFSSourceVersion = 0; // the version (hardware->es_version) may change during runtime

void GenerateDefaultFSSource() {
    if (CurrentDefaultFSSourceVersion != hardware->es_version) {
        CurrentDefaultFSSourceVersion = hardware->es_version;
        std::ostringstream ss;
        ss << "#version " << CurrentDefaultFSSourceVersion << " es\n";
        ss << "precision mediump float;\n\n";
        ss << "out vec4 fragColor;\n\n";
        ss << "void main() {\n";
        ss << "    fragColor = vec4(1.0, 1.0, 1.0, 1.0);\n";
        ss << "}\n";

        DefaultFSSource = ss.str();
    }
}

// One glUniform*v call for one stripped initialiser. `values` is laid out the way
// the GL call wants it: elements in order, each column-major.
static void set_uniform_default(GLint location, const uniform_default_t& d) {
    const size_t n = d.values.size();
    if (n != static_cast<size_t>(d.rows) * static_cast<size_t>(d.columns) * static_cast<size_t>(d.count)) return;
    const GLsizei count = d.count;

    if (d.base == uniform_base_t::Float) {
        std::vector<GLfloat> v(n);
        for (size_t k = 0; k < n; ++k)
            v[k] = static_cast<GLfloat>(d.values[k]);
        if (d.columns == 1) {
            switch (d.rows) {
            case 1:
                GLES.glUniform1fv(location, count, v.data());
                return;
            case 2:
                GLES.glUniform2fv(location, count, v.data());
                return;
            case 3:
                GLES.glUniform3fv(location, count, v.data());
                return;
            case 4:
                GLES.glUniform4fv(location, count, v.data());
                return;
            default:
                return;
            }
        }
        switch (d.columns * 10 + d.rows) {
        case 22:
            GLES.glUniformMatrix2fv(location, count, GL_FALSE, v.data());
            return;
        case 33:
            GLES.glUniformMatrix3fv(location, count, GL_FALSE, v.data());
            return;
        case 44:
            GLES.glUniformMatrix4fv(location, count, GL_FALSE, v.data());
            return;
        case 23:
            GLES.glUniformMatrix2x3fv(location, count, GL_FALSE, v.data());
            return;
        case 32:
            GLES.glUniformMatrix3x2fv(location, count, GL_FALSE, v.data());
            return;
        case 24:
            GLES.glUniformMatrix2x4fv(location, count, GL_FALSE, v.data());
            return;
        case 42:
            GLES.glUniformMatrix4x2fv(location, count, GL_FALSE, v.data());
            return;
        case 34:
            GLES.glUniformMatrix3x4fv(location, count, GL_FALSE, v.data());
            return;
        case 43:
            GLES.glUniformMatrix4x3fv(location, count, GL_FALSE, v.data());
            return;
        default:
            return;
        }
    }

    if (d.base == uniform_base_t::Uint) {
        std::vector<GLuint> v(n);
        for (size_t k = 0; k < n; ++k)
            v[k] = static_cast<GLuint>(d.values[k]);
        switch (d.rows) {
        case 1:
            GLES.glUniform1uiv(location, count, v.data());
            return;
        case 2:
            GLES.glUniform2uiv(location, count, v.data());
            return;
        case 3:
            GLES.glUniform3uiv(location, count, v.data());
            return;
        case 4:
            GLES.glUniform4uiv(location, count, v.data());
            return;
        default:
            return;
        }
    }

    // Int, and Bool, which GL sets through the integer entry points.
    std::vector<GLint> v(n);
    for (size_t k = 0; k < n; ++k)
        v[k] = static_cast<GLint>(d.values[k]);
    switch (d.rows) {
    case 1:
        GLES.glUniform1iv(location, count, v.data());
        return;
    case 2:
        GLES.glUniform2iv(location, count, v.data());
        return;
    case 3:
        GLES.glUniform3iv(location, count, v.data());
        return;
    case 4:
        GLES.glUniform4iv(location, count, v.data());
        return;
    default:
        return;
    }
}

// Desktop GL gives a uniform its initialiser when the program links; ESSL has no
// initialisers, so the translated shaders reported theirs and they are set here,
// right after the link and before the application can touch the program. A
// uniform the driver optimised away has no location and is skipped. The GL
// uniform entry points act on the current program, so it is switched and put
// back through the driver directly: this layer's own tracking is not touched.
static void apply_uniform_defaults(GLuint program) {
    if (shader_uniform_defaults.empty()) return;
    GLint linked = 0;
    GLES.glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) return;

    GLuint shaders[8] = {};
    GLsizei attached = 0;
    GLES.glGetAttachedShaders(program, 8, &attached, shaders);

    GLint previous = -1;
    for (GLsizei s = 0; s < attached; ++s) {
        const auto it = shader_uniform_defaults.find(shaders[s]);
        if (it == shader_uniform_defaults.end() || it->second.empty()) continue;
        if (previous < 0) {
            GLES.glGetIntegerv(GL_CURRENT_PROGRAM, &previous);
            GLES.glUseProgram(program);
        }
        for (const auto& d : it->second) {
            GLint location = GLES.glGetUniformLocation(program, d.name.c_str());
            // An array may be listed under its first element's name.
            if (location < 0) location = GLES.glGetUniformLocation(program, (d.name + "[0]").c_str());
            if (location < 0) {
                LOG_D("uniform default: %s is not active in program %u, skipped", d.name.c_str(), program)
                continue;
            }
            LOG_D("uniform default: %s -> location %d in program %u", d.name.c_str(), location, program)
            set_uniform_default(location, d);
        }
    }
    if (previous >= 0) GLES.glUseProgram(static_cast<GLuint>(previous));
}

static UnorderedMap<unsigned, GLuint> DefaultFSMap; // essl version <-> shader id
void glLinkProgram(GLuint program) {
    LOG()

    LOG_D("glLinkProgram(%d)", program)
    if (!shaderInfo.converted.empty() && shaderInfo.frag_data_changed) {
        const GLchar* patched = shaderInfo.frag_data_changed_converted.c_str();
        GLES.glShaderSource(shaderInfo.id, 1, &patched, nullptr);
        GLES.glCompileShader(shaderInfo.id);
        GLint status = 0;
        GLES.glGetShaderiv(shaderInfo.id, GL_COMPILE_STATUS, &status);
        if (status != GL_TRUE) {
            char tmp[500];
            GLES.glGetShaderInfoLog(shaderInfo.id, 500, nullptr, tmp);
            LOG_E("Failed to compile patched shader, log:\n%s", tmp)
        }
        GLES.glDetachShader(program, shaderInfo.id);
        GLES.glAttachShader(program, shaderInfo.id);
        CHECK_GL_ERROR
    }
    shaderInfo.id = 0;
    shaderInfo.converted = "";
    shaderInfo.frag_data_changed_converted.clear();
    shaderInfo.frag_data_changed = 0;

    // Generate defaut fragment shader if needed
    if (program_map_should_generate_fs[program] == ShouldGenerateFSState::Maybe) {
        GenerateDefaultFSSource();
        GLuint& default_fs = DefaultFSMap[CurrentDefaultFSSourceVersion];
        if (!default_fs) {
            default_fs = GLES.glCreateShader(GL_FRAGMENT_SHADER);
            const char* src = DefaultFSSource.c_str();
            GLES.glShaderSource(default_fs, 1, &src, nullptr);

            GLES.glCompileShader(default_fs);

            GLint success = 0;
            GLES.glGetShaderiv(default_fs, GL_COMPILE_STATUS, &success);
            if (!success) {
                GLint logLength = 0;
                GLES.glGetShaderiv(default_fs, GL_INFO_LOG_LENGTH, &logLength);
                std::vector<char> log(logLength);
                GLES.glGetShaderInfoLog(default_fs, logLength, nullptr, log.data());
                LOG_E("Default fragment shader compile error for program %u :\n%s\n", program, log.data());
                GLES.glDeleteShader(default_fs);
                default_fs = 0;
            }
        }

        if (default_fs) {
            LOG_D("Try to attach missing default FS for program %u...", program);
            GLES.glAttachShader(program, default_fs);
        }
    }

    GLES.glLinkProgram(program);
    apply_uniform_defaults(program);

    CHECK_GL_ERROR
}

void glGetProgramiv(GLuint program, GLenum pname, GLint* params) {
    LOG()
    GLES.glGetProgramiv(program, pname, params);
    if (global_settings.ignore_error >= IgnoreErrorLevel::Partial &&
        (pname == GL_LINK_STATUS || pname == GL_VALIDATE_STATUS) && !*params) {
        GLchar infoLog[512];
        GLES.glGetProgramInfoLog(program, 512, nullptr, infoLog);

        LOG_W_FORCE("Program %d linking failed: \n%s", program, infoLog);
        LOG_W_FORCE("Now try to cheat.");
        *params = GL_TRUE;
    }
    CHECK_GL_ERROR
}

void glUseProgram(GLuint program) {
    LOG()
    LOG_D("glUseProgram(%d)", program)
    if (program != gl_state->current_program) {
        gl_state->current_program = program;
        GLES.glUseProgram(program);
        CHECK_GL_ERROR
    }
}

void glAttachShader(GLuint program, GLuint shader) {
    LOG()
    LOG_D("glAttachShader(%u, %u)", program, shader)
    if (hardware->emulate_texture_buffer && shader_map_is_sampler_buffer_emulated[shader])
        program_map_is_sampler_buffer_emulated[program] = true;

    GLint type = 0;
    GLES.glGetShaderiv(shader, GL_SHADER_TYPE, &type);
    auto& should_gen_fs_map = program_map_should_generate_fs;
    if (type == GL_FRAGMENT_SHADER) {
        should_gen_fs_map[program] = ShouldGenerateFSState::Never;
    } else if (type == GL_VERTEX_SHADER) {
        auto it = should_gen_fs_map.find(program);
        if (it == should_gen_fs_map.end() || should_gen_fs_map[program] != ShouldGenerateFSState::Never) {
            should_gen_fs_map[program] = ShouldGenerateFSState::Maybe;
        }
    }

    GLES.glAttachShader(program, shader);
    CHECK_GL_ERROR
}

extern UnorderedMap<GLuint, SamplerInfo> g_samplerCacheForSamplerBuffer;

GLuint glCreateProgram() {
    LOG()
    LOG_D("glCreateProgram")
    GLuint program = GLES.glCreateProgram();
    if (hardware->emulate_texture_buffer) {
        program_map_is_sampler_buffer_emulated[program] = false;
        if (g_samplerCacheForSamplerBuffer.find(program) != g_samplerCacheForSamplerBuffer.end()) {
            g_samplerCacheForSamplerBuffer.erase(program);
        }
    }
    program_map_should_generate_fs[program] = ShouldGenerateFSState::Unknown;

    CHECK_GL_ERROR
    return program;
}

// GL 3.1's name-only half of the active-uniform query, on top of the ES call that
// already returns the same string.
//
// It was a stub -- a no-op that wrote neither the name nor the length and, being a
// stub rather than an error, left glGetError clean. Callers got whatever was
// already in the buffer they passed.
//
// That is not a cosmetic gap. The standard way to build a name -> location map is
// to walk the active uniforms by index and ask for each name, and a caller doing
// that ended up with a map keyed on garbage: every later lookup missed, so the
// uniforms never got set and kept whatever the driver had zero-initialised them
// to. NeoForge's early loading window does exactly this, and a screenSize of
// (0, 0) turned its every vertex into a division by zero -- gl_Position came out
// non-finite, every primitive was discarded, and the window rendered black with
// nothing anywhere reporting a problem.
void glGetActiveUniformName(GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei* length,
                            GLchar* uniformName) {
    LOG()
    LOG_D("glGetActiveUniformName(program: %u, index: %u, bufSize: %d)", program, uniformIndex, bufSize)

    if (length) *length = 0;
    if (bufSize <= 0 || uniformName == nullptr) {
        // Nothing to write. Still forwarded when bufSize is negative so the driver
        // raises the GL_INVALID_VALUE the caller is owed.
        if (bufSize < 0) GLES.glGetActiveUniform(program, uniformIndex, bufSize, nullptr, nullptr, nullptr, nullptr);
        CHECK_GL_ERROR
        return;
    }

    // Same buffer contract in both calls: at most bufSize-1 characters plus the
    // terminator, and a length that excludes it. The size and type this also
    // returns are what glGetActiveUniformsiv is for; they are discarded here.
    GLint size = 0;
    GLenum type = 0;
    GLsizei written = 0;
    uniformName[0] = '\0';
    GLES.glGetActiveUniform(program, uniformIndex, bufSize, &written, &size, &type, uniformName);
    if (length) *length = written;

    LOG_D("  -> \"%s\"", uniformName)
    CHECK_GL_ERROR
}
