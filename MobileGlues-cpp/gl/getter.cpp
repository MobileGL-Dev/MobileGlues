// MobileGlues - gl/getter.cpp
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

#include "getter.h"
#include "enable.h"
#include "../egl/context.h"
#include "buffer.h"
#include "texture.h"
#include <string>
#include <format>
#include <vector>
#include <random>
#include "FSR1/FSR1.h"
#include "log.h"
#include "mg.h"
#include "pixel.h"
#include "random_string_gen.h"
#include "../config/settings.h"

#define DEBUG 0

Version GLVersion;

namespace {
// See mg_set_gl_error in gl/mg.h for why this exists and why it is per thread.
thread_local GLenum g_frontend_error = GL_NO_ERROR;
} // namespace

void mg_set_gl_error(GLenum error) {
    if (error == GL_NO_ERROR) return;
    // First error wins. A later, vaguer failure must not paper over the one that
    // actually explains what the application did wrong.
    if (g_frontend_error != GL_NO_ERROR) return;
    g_frontend_error = error;
    LOG_D("MobileGlues raised %s", glEnumToString(error))
}

void glGetIntegerv(GLenum pname, GLint* params) {
    LOG()
    LOG_D("glGetIntegerv, pname: %s", glEnumToString(pname))
    switch (pname) {
    case GL_NUM_EXTENSIONS + GL_BACKEND_GETTER_MG:
        GLES.glGetIntegerv(pname - GL_BACKEND_GETTER_MG, params);
        return;
    case GL_CONTEXT_PROFILE_MASK:
        (*params) = GL_CONTEXT_CORE_PROFILE_BIT;
        break;
    case GL_NUM_EXTENSIONS:
        static GLint num_extensions = -1;
        if (num_extensions == -1) {
            const GLubyte* ext_str = glGetString(GL_EXTENSIONS);
            if (ext_str) {
                std::string copy_str((const char*)ext_str);
                std::string token;
                size_t pos = 0;
                num_extensions = 0;
                while ((pos = copy_str.find(' ')) != std::string::npos) {
                    num_extensions++;
                    copy_str.erase(0, pos + 1);
                }
                if (!copy_str.empty()) num_extensions++; // Count the last token
            } else {
                num_extensions = 0;
            }
        }
        (*params) = num_extensions;
        break;
    case GL_MAJOR_VERSION:
        // What THIS context was granted, which after the version gate was relaxed
        // is what the application asked for rather than the configured maximum.
        //
        // Only for a desktop context. An ES context has no granted desktop version
        // to report -- eglCreateContext records the values the bootstrap probe read
        // off the driver, which describe the driver, not this context -- so it goes
        // to the driver, which knows the real answer.
        if (g_current_ctx && g_current_ctx->client_type == EGL_OPENGL_API) {
            (*params) = g_current_ctx->granted_major;
        } else if (g_current_ctx) {
            GLES.glGetIntegerv(GL_MAJOR_VERSION, params);
        } else {
            (*params) = GLVersion.Major;
        }
        break;
    case GL_MINOR_VERSION:
        if (g_current_ctx && g_current_ctx->client_type == EGL_OPENGL_API) {
            (*params) = g_current_ctx->granted_minor;
        } else if (g_current_ctx) {
            GLES.glGetIntegerv(GL_MINOR_VERSION, params);
        } else {
            (*params) = GLVersion.Minor;
        }
        break;
    case GL_MAX_TEXTURE_IMAGE_UNITS: {
        int es_params = 16;
        GLES.glGetIntegerv(pname, &es_params);
        CHECK_GL_ERROR(*params) = es_params * 2;
        // Why is the real GL_MAX_TEXTURE_IMAGE_UNITS bigger than what GLES.glGetIntegerv returns?
        break;
    }
    case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS: {
        // This is the bound on glActiveTexture, and drivers report generously --
        // Mali-G77 says 96. Forwarding it unchanged promised more units than the
        // layer keeps bindings for, and the units past the end were accepted by
        // the driver but dropped here, so a glBindTexture after one of them went
        // to the wrong unit with nothing reporting it. Promise only what
        // gl/texture.cpp can actually track.
        int es_params = 32;
        GLES.glGetIntegerv(pname, &es_params);
        CHECK_GL_ERROR
        const int tracked = mg_max_texture_units();
        (*params) = es_params < tracked ? es_params : tracked;
        break;
    }
    case GL_CONTEXT_FLAGS: {
        // Reported from what the context was actually created with. Claiming
        // flags the application never asked for -- as this did before it was
        // reduced to 0 -- makes a loader believe it has a debug or robust context
        // that does not behave like one.
        (*params) = g_current_ctx ? g_current_ctx->context_flags : 0;
        break;
    }
    case GL_ARRAY_BUFFER_BINDING:
    case GL_ATOMIC_COUNTER_BUFFER_BINDING:
    case GL_COPY_READ_BUFFER_BINDING:
    case GL_COPY_WRITE_BUFFER_BINDING:
    case GL_DRAW_INDIRECT_BUFFER_BINDING:
    case GL_DISPATCH_INDIRECT_BUFFER_BINDING:
    case GL_ELEMENT_ARRAY_BUFFER_BINDING:
    case GL_PIXEL_PACK_BUFFER_BINDING:
    case GL_PIXEL_UNPACK_BUFFER_BINDING:
    case GL_SHADER_STORAGE_BUFFER_BINDING:
    case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
    case GL_UNIFORM_BUFFER_BINDING:
        (*params) = (int)find_bound_buffer(pname);
        LOG_D("  -> %d", *params)
        break;
    case GL_VERTEX_ARRAY_BINDING:
        (*params) = (int)find_bound_array();
        break;
    // GL_FRAMEBUFFER_BINDING is the same enum as GL_DRAW_FRAMEBUFFER_BINDING.
    case GL_DRAW_FRAMEBUFFER_BINDING: {
        GLES.glGetIntegerv(pname, params);
        // Hide the FSR1 redirect. While FSR1 is on, the application's framebuffer
        // 0 really is g_renderFBO (gl/framebuffer.cpp), and handing that name back
        // let the application save it and bind it again later -- by which point a
        // resolution change may have deleted and recreated the target, so the
        // restore named a dead framebuffer and stuck. Answering 0 means a restore
        // goes back through the redirect and lands wherever it currently points.
        if (FSR1_Context::g_renderFBO != 0 && *params == (GLint)FSR1_Context::g_renderFBO) *params = 0;
        LOG_D("  -> %d", *params)
        break;
    }
    default: {
        // The enable table owns every enable capability and the handful of limits
        // that describe it, so glGetIntegerv can never disagree with glIsEnabled.
        GLboolean enabled = GL_FALSE;
        GLint ival = 0;
        if (mg_enable_query(pname, &enabled)) {
            (*params) = enabled ? 1 : 0;
            break;
        }
        if (mg_enable_query_int(pname, &ival)) {
            (*params) = ival;
            break;
        }
        // The pixel-store parameters GLES has no answer for. Forwarding them
        // returned GL_INVALID_ENUM and left *params exactly as the caller left it.
        if (mg_pixel_store_query_int(pname, params)) {
            LOG_D("  -> %d", *params)
            break;
        }
        GLES.glGetIntegerv(pname, params);
        LOG_D("  -> %d", *params)
        CHECK_GL_ERROR
    }
    }
}

GLenum glGetError() {
    LOG()
    // Both are consumed whether or not they get reported: leaving either latched
    // would hand it to a later, unrelated glGetError.
    const GLenum backend = GLES.glGetError();
    const GLenum frontend = g_frontend_error;
    g_frontend_error = GL_NO_ERROR;

    // GL_NO_ERROR, always, in every configuration and whatever ignoreError says.
    //
    // Deliberate, and not the same thing as not knowing. This layer emulates
    // enough of desktop GL on top of GLES that a faithfully forwarded error is
    // more often an artefact of how a call had to be translated than something the
    // application got wrong -- and hosts treat errors as fatal or fall back to
    // slower paths on them. One example from inside this very library:
    // gl/buffer.cpp's glMapBuffer asks glGetError and returns nullptr if it is
    // not clear, a branch that only stays dead because of this.
    //
    // What the errors are still for is the log. Every path that raises one names
    // itself right next to the call, so a quiet failure is diagnosable from a
    // logcat even though the application will never be told.
    const GLenum swallowed = frontend != GL_NO_ERROR ? frontend : backend;
    if (swallowed != GL_NO_ERROR) {
        LOG_W("glGetError -> %s, reported to the application as GL_NO_ERROR", glEnumToString(swallowed))
    }
    return GL_NO_ERROR;
}

static std::string es_ext;
std::string GetExtensionsList() {
    return es_ext;
}

void InitGLESBaseExtensions() {
    std::vector<std::string> extensions;

    if (global_settings.hide_mg_env_level == HideMGEnvLevel::Disabled) {
        extensions.push_back("GL_MG_mobileglues");
        extensions.push_back("GL_MG_backend_string_getter_access");
        extensions.push_back("GL_MG_settings_string_dump");
    }

    const char* base_exts[] = {"GL_ARB_fragment_program",
                               "GL_ARB_vertex_buffer_object",
                               "GL_ARB_vertex_array_object",
                               "GL_ARB_vertex_buffer",
                               "GL_EXT_vertex_array",
                               "GL_ARB_ES2_compatibility",
                               "GL_ARB_ES3_compatibility",
                               "GL_EXT_packed_depth_stencil",
                               "GL_EXT_depth_texture",
                               "GL_ARB_depth_texture",
                               "GL_ARB_shading_language_100",
                               "GL_ARB_imaging",
                               "GL_ARB_draw_buffers_blend",
                               "OpenGL15",
                               "GL_ARB_shader_storage_buffer_object",
                               "GL_ARB_shader_image_load_store",
                               "GL_ARB_clear_texture",
                               "GL_ARB_get_program_binary",
                               "GL_ARB_separate_shader_objects",
                               "GL_ARB_multi_bind",
                               "GL_KHR_no_error"};

    extensions.insert(extensions.end(), std::begin(base_exts), std::end(base_exts));

    if (global_settings.hide_mg_env_level >= HideMGEnvLevel::Level1) {
        for (int i = extensions.size() - 1; i > 0; --i) {
            int j = rand() % (i + 1);
            std::swap(extensions[i], extensions[j]);
        }
    }

    es_ext.clear();
    for (const auto& ext : extensions) {
        es_ext += ext;
        es_ext += " ";
    }
}

void AppendExtension(const char* ext) {
    es_ext += ext;
    es_ext += ' ';
}

std::string getBeforeThirdSpace(const std::string& str) {
    int spaceCount = 0;
    size_t endPos = 0;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == ' ') {
            spaceCount++;
            if (spaceCount == 3) {
                endPos = i;
                break;
            }
        }
        if (spaceCount < 3) endPos = str.length();
    }

    return str.substr(0, endPos);
}

// What the backend said about itself while the bootstrap probe context was
// current. Kept because the answers are properties of the driver, not of any one
// context, and this layer's GL_RENDERER string is built from them.
static std::string g_probe_renderer;
static std::string g_probe_version;

// The backend's string for `name`, or the probe-time copy when the backend has
// none.
//
// glGetString returns null when no context is current on the calling thread,
// and constructing a std::string from that is strlen(nullptr): a SIGSEGV inside
// this library's glGetString, which is how a host whose context was made current
// through some other EGL than the one this layer drives first showed itself --
// GlDevice's very first query, GL_RENDERER, took the process down with nothing
// in the log. The strings are process-wide facts, so answer from the probe and
// say why.
static std::string backend_string(GLenum name, const std::string& probe_copy, const char* what) {
    const GLubyte* live = GLES.glGetString ? GLES.glGetString(name) : nullptr;
    if (live) return reinterpret_cast<const char*>(live);
    LOG_E("glGetString(%s): the backend returned null, so no context is current on this thread for the GLES "
          "library this layer loaded; answering from the bootstrap probe (\"%s\")",
          what, probe_copy.c_str())
    return probe_copy;
}

std::string getGpuName() {
    std::string gpuName = backend_string(GL_RENDERER, g_probe_renderer, "GL_RENDERER");

    if (gpuName.empty()) {
        return "<unknown>";
    }

    // MetalANGLE, ANGLE (Metal Renderer: Apple * GPU)
    if (gpuName.find("MetalANGLE, ANGLE") != std::string::npos) {
        if (gpuName.length() < 25) {
            return gpuName;
        }

        std::string gpu = gpuName.substr(23, gpuName.length() - 24);
        std::string formattedGpuName = gpu + " | MetalANGLE | Metal";
        return formattedGpuName;
    }

    // Vulkan ANGLE
    if (gpuName.rfind("ANGLE", 0) == 0 && gpuName.find("Vulkan") != std::string::npos) {
        size_t firstParen = gpuName.find('(');
        size_t secondParen = gpuName.find('(', firstParen + 1);
        size_t lastParen = gpuName.rfind('(');

        std::string gpu = gpuName.substr(secondParen + 1, lastParen - secondParen - 2);

        size_t vulkanStart = gpuName.find("Vulkan ");
        size_t vulkanEnd = gpuName.find(' ', vulkanStart + 7);
        std::string vulkanVersion = gpuName.substr(vulkanStart + 7, vulkanEnd - (vulkanStart + 7));

        std::string formattedGpuName = gpu + " | ANGLE | Vulkan " + vulkanVersion;

        return formattedGpuName;
    }

    return gpuName;
}

void set_es_version() {
    // Under the bootstrap probe context. A probe that failed to come up leaves
    // both null, which used to be a crash at library load rather than a message.
    const GLubyte* renderer = GLES.glGetString ? GLES.glGetString(GL_RENDERER) : nullptr;
    const GLubyte* version = GLES.glGetString ? GLES.glGetString(GL_VERSION) : nullptr;
    g_probe_renderer = renderer ? reinterpret_cast<const char*>(renderer) : "";
    g_probe_version = version ? reinterpret_cast<const char*>(version) : "";
    if (!version)
        LOG_E("set_es_version: the backend answered GL_VERSION with null; the bootstrap context is not current")
    std::string ESVersionStr = getBeforeThirdSpace(g_probe_version);
    int major, minor;

    if (sscanf(ESVersionStr.c_str(), "OpenGL ES %d.%d", &major, &minor) == 2) {
        hardware->es_version = major * 100 + minor * 10;
    } else {
        hardware->es_version = 300;
    }
    LOG_I("OpenGL ES Version: %s (%d)", ESVersionStr.c_str(), hardware->es_version)
    if (hardware->es_version < 300) {
        LOG_I("OpenGL ES version is lower than 3.0! This version is not supported!")
    }
}

std::string getGLESName() {
    return getBeforeThirdSpace(backend_string(GL_VERSION, g_probe_version, "GL_VERSION"));
}

static std::string rendererString;
static std::string vendorString;
static std::string versionString;
const GLubyte* glGetString(GLenum name) {
    LOG()
    LOG_D("glGetString, %s", glEnumToString(name))
    switch (name) {
    case GL_VENDOR: {
        if (vendorString.empty()) {
            if (global_settings.hide_mg_env_level == HideMGEnvLevel::Disabled) {
                std::string vendor = "Swung0x48, BZLZHH, Tungsten";
                vendorString = vendor;
            } else {
                const char choices[] = "AINM";
                vendorString = choices[rand() % 4];

                RandomStringOptions randStrOpts;
                randStrOpts.includeDigits = false;
                randStrOpts.minLength = 3;
                randStrOpts.maxLength = 8;
                randStrOpts.includeLowercase = false;
                randStrOpts.includeUppercase = false;
                randStrOpts.customChars = "IMenaNtMseAVlD";
                vendorString += GenerateRandomString(randStrOpts);
            }
        }
        return (const GLubyte*)vendorString.c_str();
    }
    case GL_VERSION: {
        if (versionString.empty()) {
            versionString = GLVersion.toString();
            if (global_settings.hide_mg_env_level == HideMGEnvLevel::Disabled) {
                if (GLVersion.toInt(2) == DEFAULT_GL_VERSION) {
                    versionString += " MobileGlues ";
                } else {
                    Version defaultVersion = Version(DEFAULT_GL_VERSION);
                    versionString += " §4§l(" + defaultVersion.toString() + ") MobileGlues§r ";
                }

                versionString += std::to_string(MAJOR) + "." + std::to_string(MINOR) + "." + std::to_string(REVISION);
#if PATCH != 0
                versionString += "." + std::to_string(PATCH);
#endif
#if defined(VERSION_TYPE)
#if VERSION_TYPE == VERSION_ALPHA
                versionString += "·Alpha";
#elif VERSION_TYPE == VERSION_BETA
                versionString += "·Beta";
#elif VERSION_TYPE == VERSION_DEVELOPMENT
                versionString += "·Dev" + std::to_string(VERSION_DEV_NUMBER);
#elif VERSION_TYPE == VERSION_RC
                versionString += "·RC" + std::to_string(VERSION_RC_NUMBER);
#endif
#endif
                versionString += VERSION_SUFFIX;
            } else {
                const char choices[] = "AIN";
                versionString += " ";
                versionString += choices[rand() % 3];

                RandomStringOptions randStrOpts;
                randStrOpts.includeDigits = false;
                randStrOpts.customChars = " ";
                versionString += GenerateRandomString(randStrOpts);

                RandomStringOptions randStrOpts2;
                randStrOpts2.includeDigits = false;
                randStrOpts2.includeUppercase = false;
                randStrOpts2.minLength = 1;
                randStrOpts2.maxLength = 4;

                versionString += std::to_string(MAJOR) + GenerateRandomString(randStrOpts2) + std::to_string(MINOR) +
                                 GenerateRandomString(randStrOpts2) + std::to_string(REVISION) +
                                 GenerateRandomString(randStrOpts2) + std::to_string(PATCH) +
                                 GenerateRandomString(randStrOpts2);
            }
        }
        return (const GLubyte*)versionString.c_str();
    }
    case GL_RENDERER: {
        if (rendererString == std::string("")) {
            if (global_settings.hide_mg_env_level == HideMGEnvLevel::Disabled) {
                std::string gpuName = getGpuName();
                std::string glesName = getGLESName();
                rendererString = std::string(gpuName) + " | " + std::string(glesName);
            } else {
                const char choices[] = "AINM";
                rendererString = choices[rand() % 4];

                RandomStringOptions randStrOpts;
                randStrOpts.includeDigits = true;
                randStrOpts.minLength = 6;
                randStrOpts.maxLength = 12;
                randStrOpts.includeLowercase = false;
                randStrOpts.includeUppercase = false;
                randStrOpts.customChars = "IRMenaNtfsoerAceVlDG";
                rendererString += GenerateRandomString(randStrOpts);

                int junkInfoTime = rand() % 3 + 1;
                for (int i = 0; i < junkInfoTime; ++i) {
                    rendererString += " ";
                    RandomStringOptions randStrOpts2;
                    randStrOpts2.minLength = 3;
                    randStrOpts2.maxLength = 6;
                    randStrOpts2.includeLowercase = false;
                    randStrOpts2.includeUppercase = false;
                    randStrOpts2.customChars = "IRenaNtfsoerAcieVDcsG";
                    rendererString += GenerateRandomString(randStrOpts2);
                }
            }
        }
        return (const GLubyte*)rendererString.c_str();
    }
    case GL_SHADING_LANGUAGE_VERSION: {
        static std::string shadingLangString;

        if (shadingLangString.empty()) {
            std::string baseVer;
            if (hardware->es_version < 310) {
                baseVer = "4.00";
            } else {
                baseVer = "4.60";
            }

            if (global_settings.hide_mg_env_level >= HideMGEnvLevel::Level1) {
                shadingLangString = baseVer;

                int junkCount = rand() % 2 + 1;
                for (int i = 0; i < junkCount; ++i) {
                    shadingLangString += " ";
                    RandomStringOptions junkOpts;
                    junkOpts.minLength = 2;
                    junkOpts.maxLength = 5;
                    junkOpts.includeLowercase = false;
                    junkOpts.includeUppercase = false;
                    junkOpts.customChars = "IAneNDtVsaMIl";
                    shadingLangString += GenerateRandomString(junkOpts);
                }
            } else {
                shadingLangString = baseVer + " MobileGlues with glslang and SPIRV-Cross";
            }
        }

        return reinterpret_cast<const GLubyte*>(shadingLangString.c_str());
    }
    case GL_EXTENSIONS: {
        // GetExtensionsList() returns by value, so assigning it on every call freed the buffer handed
        // out last time and dangled every pointer already returned. Build it once, like the strings above.
        static std::string extensionsString;

        if (extensionsString.empty()) {
            extensionsString = GetExtensionsList();
        }

        return (const GLubyte*)extensionsString.c_str();
    }
    case GL_SETTINGS_MG: {
        if (global_settings.hide_mg_env_level >= HideMGEnvLevel::Level1) return GLES.glGetString(name);

        static char* settings_string = nullptr;
        std::string tmp = dump_settings_string("  ");
        settings_string = strdup(tmp.c_str());
        return reinterpret_cast<const GLubyte*>(settings_string);
    }
    case GL_VERSION + GL_BACKEND_GETTER_MG:
    case GL_VENDOR + GL_BACKEND_GETTER_MG:
    case GL_RENDERER + GL_BACKEND_GETTER_MG:
    case GL_EXTENSIONS + GL_BACKEND_GETTER_MG:
    case GL_SHADING_LANGUAGE_VERSION + GL_BACKEND_GETTER_MG:
        if (global_settings.hide_mg_env_level == HideMGEnvLevel::Disabled)
            return GLES.glGetString(name - GL_BACKEND_GETTER_MG);
        else
            return GLES.glGetString(name);
    default:
        return GLES.glGetString(name);
    }
}

const GLubyte* glGetStringi(GLenum name, GLuint index) {
    LOG()
    if (name == GL_EXTENSIONS + GL_BACKEND_GETTER_MG && global_settings.hide_mg_env_level == HideMGEnvLevel::Disabled) {
        return GLES.glGetStringi(name - GL_BACKEND_GETTER_MG, index);
    }

    typedef struct {
        GLenum name;
        const char** parts;
        GLuint count;
    } StringCache;
    static StringCache caches[] = {{GL_EXTENSIONS, nullptr, 0},
                                   {GL_VENDOR, nullptr, 0},
                                   {GL_VERSION, nullptr, 0},
                                   {GL_SHADING_LANGUAGE_VERSION, nullptr, 0}};
    static int initialized = 0;
    if (!initialized) {
        for (auto& cache : caches) {
            GLenum target = cache.name;
            const GLubyte* str = nullptr;
            const char* delimiter = " ";

            switch (target) {
            case GL_VENDOR:
                str = glGetString(GL_VENDOR);
                delimiter = ", ";
                break;
            case GL_VERSION:
                str = glGetString(GL_VERSION);
                delimiter = " .";
                break;
            case GL_SHADING_LANGUAGE_VERSION:
                str = glGetString(GL_SHADING_LANGUAGE_VERSION);
                break;
            case GL_EXTENSIONS:
                str = glGetString(GL_EXTENSIONS);
                break;
            default:
                return GLES.glGetStringi(name, index);
            }

            if (!str) continue;

            std::string copy_str((const char*)str);
            std::string token_str;
            size_t start = 0;
            size_t end = copy_str.find_first_of(delimiter);

            while (end != std::string::npos) {
                token_str = copy_str.substr(start, end - start);
                cache.parts = (const char**)realloc(cache.parts, (cache.count + 1) * sizeof(char*));
                cache.parts[cache.count++] = strdup(token_str.c_str());
                start = end + 1;
                end = copy_str.find_first_of(delimiter, start);
            }
            token_str = copy_str.substr(start); // Get the last token
            cache.parts = (const char**)realloc(cache.parts, (cache.count + 1) * sizeof(char*));
            cache.parts[cache.count++] = strdup(token_str.c_str());
        }
        initialized = 1;
    }

    for (auto& cache : caches) {
        if (cache.name == name) {
            if (index >= cache.count) {
                return nullptr;
            }
            return (const GLubyte*)cache.parts[index];
        }
    }

    return nullptr;
}

void glGetQueryObjectiv(GLuint id, GLenum pname, GLint* params) {
    LOG()
    if (GLES.glGetQueryObjectivEXT) {
        GLES.glGetQueryObjectivEXT(id, pname, params);
        CHECK_GL_ERROR
    }
}

void glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64* params) {
    LOG()
    if (GLES.glGetQueryObjecti64vEXT) {
        GLES.glGetQueryObjecti64vEXT(id, pname, params);
        CHECK_GL_ERROR
    }
}
