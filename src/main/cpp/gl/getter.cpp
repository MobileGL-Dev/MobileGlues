//
// Created by BZLZHH on 2025/1/28.
//

#include "getter.h"
#include "buffer.h"
#include <string>
#include <format>
#include <vector>
#include <memory>
#include <cstring>
#include "FSR1/FSR1.h"

#define DEBUG 0

Version GLVersion;

inline void glGetIntegerv(GLenum pname, GLint *params) {
    LOG()
    LOG_D("glGetIntegerv, pname: %s", glEnumToString(pname))
    switch (pname) {
        case GL_CONTEXT_PROFILE_MASK:
            (*params) = GL_CONTEXT_CORE_PROFILE_BIT;
            break;
        case GL_NUM_EXTENSIONS: {
            static GLint num_extensions = -1;
            if (num_extensions == -1) {
                const GLubyte* ext_str = glGetString(GL_EXTENSIONS);
                if (ext_str) {
                    int count = 1;
                    for (const char* p = (const char*)ext_str; *p; ++p) {
                        if (*p == ' ') ++count;
                    }
                    num_extensions = count;
                } else {
                    num_extensions = 0;
                }
            }
            (*params) = num_extensions;
            break;
        }
        case GL_MAJOR_VERSION:
            (*params) = GLVersion.Major;
            break;
        case GL_MINOR_VERSION:
            (*params) = GLVersion.Minor;
            break;
        case GL_MAX_TEXTURE_IMAGE_UNITS: {
            int es_params = 16;
            GLES.glGetIntegerv(pname, &es_params);
            CHECK_GL_ERROR
            (*params) = es_params * 2;
            break;
        }
        case GL_CONTEXT_FLAGS: {
            (*params) = GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT | GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT | GL_CONTEXT_FLAG_NO_ERROR_BIT;
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
            (*params) = (int) find_bound_buffer(pname);
            LOG_D("  -> %d",*params)
            break;
        case GL_VERTEX_ARRAY_BINDING:
            (*params) = (int) find_bound_array();
            break;
        default:
            GLES.glGetIntegerv(pname, params);
            LOG_D("  -> %d",*params)
            CHECK_GL_ERROR
    }
}

inline GLenum glGetError() {
    LOG()
    GLenum err = GLES.glGetError();
    if (err != GL_NO_ERROR) {
        LOG_W("glGetError\n -> %d", err)
        LOG_W("Now try to cheat.")
    }
    return GL_NO_ERROR;
}

static std::string es_ext;
inline std::string GetExtensionsList() {
    return es_ext;
}

inline void InitGLESBaseExtensions() {
    if (!es_ext.empty()) return;
    es_ext =
        "GL_ARB_fragment_program "
        "GL_ARB_vertex_buffer_object "
        "GL_ARB_vertex_array_object "
        "GL_ARB_vertex_buffer "
        "GL_EXT_vertex_array "
        "GL_ARB_ES2_compatibility "
        "GL_ARB_ES3_compatibility "
        "GL_EXT_packed_depth_stencil "
        "GL_EXT_depth_texture "
        "GL_ARB_depth_texture "
        "GL_ARB_shading_language_100 "
        "GL_ARB_imaging "
        "GL_ARB_draw_buffers_blend "
        "OpenGL15 "
        "GL_ARB_shader_storage_buffer_object "
        "GL_ARB_shader_image_load_store "
        "GL_ARB_clear_texture "
        "GL_ARB_get_program_binary "
        "GL_ARB_separate_shader_objects "
        "GL_ARB_multi_bind "
        "GL_ARB_buffer_storage "
        "GL_KHR_no_error ";
}

inline void AppendExtension(const char* ext) {
    es_ext += ext;
    es_ext += ' ';
}

inline std::string getBeforeThirdSpace(const std::string& str) {
    int spaceCount = 0;
    size_t endPos = str.size();
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == ' ' && ++spaceCount == 3) {
            endPos = i;
            break;
        }
    }
    return str.substr(0, endPos);
}

inline std::string getGpuName() {
    std::string gpuName((char *)GLES.glGetString(GL_RENDERER));
    if (gpuName.empty()) return "<unknown>";

    if (gpuName.find("MetalANGLE, ANGLE") != std::string::npos) {
        if (gpuName.length() < 25) return gpuName;
        return gpuName.substr(23, gpuName.length() - 24) + " | MetalANGLE | Metal";
    }

    if (gpuName.rfind("ANGLE", 0) == 0 && gpuName.find("Vulkan") != std::string::npos) {
        size_t firstParen = gpuName.find('(');
        size_t secondParen = gpuName.find('(', firstParen + 1);
        size_t lastParen = gpuName.rfind('(');
        std::string gpu = secondParen != std::string::npos && lastParen != std::string::npos ?
                          gpuName.substr(secondParen + 1, lastParen - secondParen - 2) : gpuName;

        size_t vulkanStart = gpuName.find("Vulkan ");
        size_t vulkanEnd = gpuName.find(' ', vulkanStart + 7);
        std::string vulkanVersion = (vulkanStart != std::string::npos && vulkanEnd != std::string::npos) ?
                                    gpuName.substr(vulkanStart + 7, vulkanEnd - (vulkanStart + 7)) : "";

        return gpu + " | ANGLE | Vulkan " + vulkanVersion;
    }

    return gpuName;
}

inline void set_es_version() {
    std::string ESVersionStr = getBeforeThirdSpace(std::string((const char*)GLES.glGetString(GL_VERSION)));
    int major = 0, minor = 0;
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

inline std::string getGLESName() {
    return getBeforeThirdSpace(std::string((char *)GLES.glGetString(GL_VERSION)));
}

static std::string rendererString;
static std::string vendorString;
static std::string versionString;

inline const GLubyte * glGetString( GLenum name ) {
    LOG()
    switch (name) {
        case GL_VENDOR: {
            if (vendorString.empty()) {
                vendorString = "Swung0x48, BZLZHH, Tungsten, Uniaball";
            }
            return (const GLubyte *)vendorString.c_str();
        }
        case GL_VERSION: {
            if (versionString.empty()) {
                versionString = GLVersion.toString();
                if (GLVersion.toInt(2) == DEFAULT_GL_VERSION) {
                    versionString += " DesktopGlues ";
                } else {
                    Version defaultVersion(DEFAULT_GL_VERSION);
                    versionString += " §4§l(" + defaultVersion.toString() + ") DesktopGlues§r ";
                }
                versionString += std::to_string(MAJOR) + "."
                               + std::to_string(MINOR) + "."
                               + std::to_string(REVISION);
#if PATCH != 0
                versionString += "." + std::to_string(PATCH);
#endif
#if defined(VERSION_TYPE)
#if VERSION_TYPE == VERSION_ALPHA
                versionString += "·Alpha";
#elif VERSION_TYPE == VERSION_BETA
                versionString += "·Beta";
#elif VERSION_TYPE == VERSION_DEVELOPMENT
                versionString += "·Dev";
#elif VERSION_TYPE == VERSION_RC
                versionString += "·RC" + std::to_string(VERSION_RC_NUMBER);
#endif
#endif
                versionString += VERSION_SUFFIX;
            }
            return (const GLubyte *)versionString.c_str();
        }
        case GL_RENDERER: {
            if (rendererString.empty()) {
                rendererString = getGpuName() + " | " + getGLESName();
            }
            return (const GLubyte *)rendererString.c_str();
        }
        case GL_SHADING_LANGUAGE_VERSION:
            return (const GLubyte *)(hardware->es_version < 310 ?
                "4.00 DesktopGlues with glslang and SPIRV-Cross" :
                "4.60 DesktopGlues with glslang and SPIRV-Cross");
        case GL_EXTENSIONS:
            return (const GLubyte *) GetExtensionsList().c_str();
        default:
            return GLES.glGetString(name);
    }
}

inline const GLubyte * glGetStringi(GLenum name, GLuint index) {
    LOG()
    struct StringCache {
        GLenum name;
        std::vector<std::string> parts;
        bool initialized = false;
    };
    static StringCache caches[] = {
        {GL_EXTENSIONS, {}, false},
        {GL_VENDOR, {}, false},
        {GL_VERSION, {}, false},
        {GL_SHADING_LANGUAGE_VERSION, {}, false}
    };

    for (auto& cache : caches) {
        if (cache.name == name && !cache.initialized) {
            cache.parts.clear();
            std::string src;
            switch (name) {
                case GL_VENDOR: src = "Swung0x48, BZLZHH, Tungsten, Uniaball"; break;
                case GL_VERSION: src = GLVersion.toString() + " DesktopGlues"; break;
                case GL_SHADING_LANGUAGE_VERSION: src = "4.60 DesktopGlues with glslang and SPIRV-Cross"; break;
                case GL_EXTENSIONS: src = GetExtensionsList(); break;
                default: src = (const char*)GLES.glGetString(name); break;
            }
            size_t start = 0, end = 0;
            char delim = (name == GL_VENDOR) ? ',' : ' ';
            while ((end = src.find(delim, start)) != std::string::npos) {
                if (end > start) cache.parts.emplace_back(src.substr(start, end - start));
                start = end + 1;
                if (name == GL_VENDOR && src[start] == ' ') ++start;
            }
            if (start < src.size()) cache.parts.emplace_back(src.substr(start));
            cache.initialized = true;
        }
        if (cache.name == name) {
            if (index < cache.parts.size())
                return (const GLubyte*)cache.parts[index].c_str();
            return nullptr;
        }
    }
    return GLES.glGetStringi(name, index);
}

inline void glGetQueryObjectiv(GLuint id, GLenum pname, GLint* params) {
    LOG()
    if (GLES.glGetQueryObjectivEXT) {
        GLES.glGetQueryObjectivEXT(id, pname, params);
        CHECK_GL_ERROR
    }
}

inline void glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64* params) {
    LOG()
    if (GLES.glGetQueryObjecti64vEXT) {
        GLES.glGetQueryObjecti64vEXT(id, pname, params);
        CHECK_GL_ERROR
    }
}