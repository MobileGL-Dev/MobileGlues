//
// Created by BZLZHH on 2025/1/28.
//

#include "getter.h"
#include "buffer.h"
#include <string>
#include <format>
#include <vector>
#include "FSR1/FSR1.h"

#define DEBUG 0

Version GLVersion;

void glGetIntegerv(GLenum pname, GLint *params) {
    LOG()
    LOG_D("glGetIntegerv, pname: %s", glEnumToString(pname))
    switch (pname) {
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
            // Why is the real GL_MAX_TEXTURE_IMAGE_UNITS bigger than what GLES.glGetIntegerv returns?
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

GLenum glGetError() {
    LOG()
    GLenum err = GLES.glGetError();
    // just clear gles error, no reporting
    if (err != GL_NO_ERROR) {
        // no logging without DEBUG
        LOG_W("glGetError\n -> %d", err)
        LOG_W("Now try to cheat.")
    }
    return GL_NO_ERROR;
}

static std::string es_ext;
std::string GetExtensionsList() {
    return es_ext.c_str();
}

void InitGLESBaseExtensions() {
    es_ext = "GL_ARB_fragment_program "
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
             "GL_KHR_no_error ";
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

std::string getGpuName() {
    std::string gpuName = std::string((char *)GLES.glGetString(GL_RENDERER));

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
    std::string ESVersionStr = getBeforeThirdSpace(std::string((const char*)GLES.glGetString(GL_VERSION)));
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
    return getBeforeThirdSpace(std::string((char *)GLES.glGetString(GL_VERSION)));
}

static std::string rendererString;
static std::string vendorString;
static std::string versionString;
const GLubyte * glGetString( GLenum name ) {
    LOG()
    switch (name) {
        case GL_VENDOR: {
            if(vendorString.empty()) {
                std::string vendor = "Swung0x48, BZLZHH, Tungsten";
                vendorString = vendor;
            }
            return (const GLubyte *)vendorString.c_str();
        }
        case GL_VERSION: {
            if (versionString.empty()) {
                versionString = GLVersion.toString();
                if (GLVersion.toInt(2) == DEFAULT_GL_VERSION) {
					versionString += " MobileGlues ";
                }
                else {
					Version defaultVersion = Version(DEFAULT_GL_VERSION);
                    versionString += " §4§l(" + defaultVersion.toString() + ") MobileGlues§r ";
                }

                versionString += std::to_string(MAJOR) + "."
                                +  std::to_string(MINOR) + "."
                                +  std::to_string(REVISION);
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
#endif
#endif
                versionString += VERSION_SUFFIX;
            }
            return (const GLubyte *)versionString.c_str();
        }

        case GL_RENDERER: 
        {
            if (rendererString == std::string("")) {
                std::string gpuName = getGpuName();
                std::string glesName = getGLESName();
                rendererString = std::string(gpuName) + " | " + std::string(glesName);
            }
            return (const GLubyte *)rendererString.c_str();
        }
        case GL_SHADING_LANGUAGE_VERSION:
            if (hardware->es_version < 310)
                return (const GLubyte *) "4.00 MobileGlues with glslang and SPIRV-Cross";
            else
                return (const GLubyte *) "4.60 MobileGlues with glslang and SPIRV-Cross";
        case GL_EXTENSIONS:
            return (const GLubyte *) GetExtensionsList().c_str();
        default:
            return GLES.glGetString(name);
    }
}

const GLubyte * glGetStringi(GLenum name, GLuint index) {
    LOG()
    typedef struct {
        GLenum name;
        const char** parts;
        GLuint count;
    } StringCache;
    static StringCache caches[] = {
            {GL_EXTENSIONS, nullptr, 0},
            {GL_VENDOR, nullptr, 0},
            {GL_VERSION, nullptr, 0},
            {GL_SHADING_LANGUAGE_VERSION, nullptr, 0}
    };
    static int initialized = 0;
    if (!initialized) {
        for (auto & cache : caches) {
            GLenum target = cache.name;
            const GLubyte* str = nullptr;
            const char* delimiter = " ";

            switch (target) {
                case GL_VENDOR:
                    str = (const GLubyte*)"Swung0x48, BZLZHH, Tungsten";
                    delimiter = ", ";
                    break;
                case GL_VERSION:
                    str = (const GLubyte*)
                        (GLVersion.toString() + " MobileGlues").c_str();
                    delimiter = " .";
                    break;
                case GL_SHADING_LANGUAGE_VERSION:
                    str = (const GLubyte*)"4.60 MobileGlues with glslang and SPIRV-Cross";
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

    for (auto & cache : caches) {
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
