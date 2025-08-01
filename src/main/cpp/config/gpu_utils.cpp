//
// Created by hanji on 2025/2/9.
//

#include "gpu_utils.h"
#include "../gles/loader.h"
#if !defined(__APPLE__)
#include "vulkan/vulkan.h"
#endif

#include <EGL/egl.h>
#include <cstring>
#include <optional>
typedef const char* cstr;
static const cstr gles3_lib[] = {
    "libGLESv3_CM",
    "libGLESv3",
    nullptr
};
static const cstr egl_libs[] = {
    "libEGL",
    nullptr
};
static const cstr vk_lib[] = {
    "libvulkan",
    nullptr
};

namespace egl_func {
    PFNEGLGETDISPLAYPROC        eglGetDisplay = nullptr;
    PFNEGLINITIALIZEPROC        eglInitialize = nullptr;
    PFNEGLCHOOSECONFIGPROC      eglChooseConfig = nullptr;
    PFNEGLCREATECONTEXTPROC     eglCreateContext = nullptr;
    PFNEGLMAKECURRENTPROC       eglMakeCurrent = nullptr;
    PFNEGLDESTROYCONTEXTPROC    eglDestroyContext = nullptr;
    PFNEGLTERMINATEPROC         eglTerminate = nullptr;
}

template <typename T>
static void* open_lib(const T names[], const char* override) {
    void* handle = nullptr;
    int flags = RTLD_LOCAL | RTLD_NOW;
    if (override) {
        handle = dlopen(override, flags);
        if (handle) return handle;
    }
    for (int i = 0; names[i]; ++i) {
        handle = dlopen(names[i], flags);
        if (handle) break;
    }
    return handle;
}

static bool loadEGLFunctions(void* lib) {
    if (!lib) return false;
    egl_func::eglGetDisplay = (PFNEGLGETDISPLAYPROC)dlsym(lib, "eglGetDisplay");
    egl_func::eglInitialize = (PFNEGLINITIALIZEPROC)dlsym(lib, "eglInitialize");
    egl_func::eglChooseConfig = (PFNEGLCHOOSECONFIGPROC)dlsym(lib, "eglChooseConfig");
    egl_func::eglCreateContext = (PFNEGLCREATECONTEXTPROC)dlsym(lib, "eglCreateContext");
    egl_func::eglMakeCurrent = (PFNEGLMAKECURRENTPROC)dlsym(lib, "eglMakeCurrent");
    egl_func::eglDestroyContext = (PFNEGLDESTROYCONTEXTPROC)dlsym(lib, "eglDestroyContext");
    egl_func::eglTerminate = (PFNEGLTERMINATEPROC)dlsym(lib, "eglTerminate");

    return egl_func::eglGetDisplay && egl_func::eglInitialize && egl_func::eglChooseConfig &&
        egl_func::eglCreateContext && egl_func::eglMakeCurrent &&
        egl_func::eglDestroyContext && egl_func::eglTerminate;
}

std::string getGPUInfo() {
    void* egllib = open_lib(egl_libs, nullptr);
    if (!loadEGLFunctions(egllib)) {
        if (egllib) dlclose(egllib);
        return std::string();
    }

    EGLDisplay display = egl_func::eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        egl_func::eglTerminate(display);
        dlclose(egllib);
        return std::string();
    }
    if (egl_func::eglInitialize(display, nullptr, nullptr) != EGL_TRUE) {
        egl_func::eglTerminate(display);
        dlclose(egllib);
        return std::string();
    }

    const EGLint attribs[] = {
        EGL_BLUE_SIZE,   8,
        EGL_GREEN_SIZE,  8,
        EGL_RED_SIZE,    8,
        EGL_ALPHA_SIZE,  8,
        EGL_DEPTH_SIZE, 24,
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };
    EGLint numConfigs = 0;
    if (egl_func::eglChooseConfig(display, attribs, nullptr, 0, &numConfigs) != EGL_TRUE || numConfigs == 0) {
        egl_func::eglTerminate(display);
        dlclose(egllib);
        return std::string();
    }
    EGLConfig config;
    egl_func::eglChooseConfig(display, attribs, &config, 1, &numConfigs);

    const EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    EGLContext ctx = egl_func::eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttribs);
    if (ctx == EGL_NO_CONTEXT) {
        egl_func::eglTerminate(display);
        dlclose(egllib);
        return std::string();
    }

    if (egl_func::eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx) != EGL_TRUE) {
        egl_func::eglDestroyContext(display, ctx);
        egl_func::eglTerminate(display);
        dlclose(egllib);
        return std::string();
    }

    void* glesLib = open_lib(gles3_lib, nullptr);
    std::string renderer;
    if (glesLib) {
        auto glGetString = (const GLubyte * (*)(GLenum))dlsym(glesLib, "glGetString");
        if (glGetString) {
            renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
        }
        dlclose(glesLib);
    }

    egl_func::eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    egl_func::eglDestroyContext(display, ctx);
    egl_func::eglTerminate(display);
    dlclose(egllib);

    return renderer;
}

int isAdreno(const char* gpu) {
//    const char* gpu = getGPUInfo();
    if (!gpu)
        return 0;
    return strstr(gpu, "Adreno") != nullptr;
}

int isAdreno740(const char* gpu) {
    //    const char* gpu = getGPUInfo();
    if (!gpu)
        return 0;
    return isAdreno(gpu) && (strstr(gpu, "740") != nullptr);
}

int isAdreno730(const char* gpu) {
    //    const char* gpu = getGPUInfo();
    if (!gpu)
        return 0;
    return isAdreno(gpu) && (strstr(gpu, "730") != nullptr);
}

bool checkIfANGLESupported(const char* gpu) {
    return !isAdreno730(gpu) && !isAdreno740(gpu) && hasVulkan12();
}

int isAdreno830(const char* gpu) {
//    const char* gpu = getGPUInfo();
    if (!gpu)
        return 0;
    return isAdreno(gpu) && (strstr(gpu, "830") != nullptr);
}

static std::optional<int> hasVk12;
int hasVulkan12() {
    if (hasVk12.has_value())
        return hasVk12.value();
    void* vulkan_lib = open_lib(vk_lib, nullptr);
    if (!vulkan_lib)
        return 0;

#ifndef __APPLE__
    
    typedef VkResult (*PFN_vkEnumerateInstanceExtensionProperties)(const char*, uint32_t*, VkExtensionProperties*);
    typedef VkResult (*PFN_vkCreateInstance)(const VkInstanceCreateInfo*, const VkAllocationCallbacks*, VkInstance*);
    typedef void (*PFN_vkDestroyInstance)(VkInstance, const VkAllocationCallbacks*);
    typedef VkResult (*PFN_vkEnumeratePhysicalDevices)(VkInstance, uint32_t*, VkPhysicalDevice*);
    typedef void (*PFN_vkGetPhysicalDeviceProperties)(VkPhysicalDevice, VkPhysicalDeviceProperties*);

    auto vkEnumerateInstanceExtensionProperties =
            (PFN_vkEnumerateInstanceExtensionProperties)dlsym(vulkan_lib, "vkEnumerateInstanceExtensionProperties");
    auto vkCreateInstance =
            (PFN_vkCreateInstance)dlsym(vulkan_lib, "vkCreateInstance");
    auto vkDestroyInstance =
            (PFN_vkDestroyInstance)dlsym(vulkan_lib, "vkDestroyInstance");
    auto vkEnumeratePhysicalDevices =
            (PFN_vkEnumeratePhysicalDevices)dlsym(vulkan_lib, "vkEnumeratePhysicalDevices");
    auto vkGetPhysicalDeviceProperties =
            (PFN_vkGetPhysicalDeviceProperties)dlsym(vulkan_lib, "vkGetPhysicalDeviceProperties");

    if (!vkEnumerateInstanceExtensionProperties || !vkCreateInstance ||
        !vkDestroyInstance || !vkEnumeratePhysicalDevices || !vkGetPhysicalDeviceProperties) {
        dlclose(vulkan_lib);
        return 0;
    }

    VkResult result = VK_SUCCESS;
    uint32_t instanceExtensionCount = 0;

    result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
    if (result != VK_SUCCESS) {
        return 0;
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = "Vulkan Check";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "MobileGlues";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;
    createInfo.enabledExtensionCount = 0;
    createInfo.ppEnabledExtensionNames = nullptr;

    VkInstance instance = {};
    result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS) {
        hasVk12 = false;
        return 0;
    }

    uint32_t gpuCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr);
    if (result != VK_SUCCESS || gpuCount == 0) {
        vkDestroyInstance(instance, nullptr);
        hasVk12 = false;
        return 0;
    }

    auto* physicalDevices = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * gpuCount);
    vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices);

    for (uint32_t i = 0; i < gpuCount; i++) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);

        if (deviceProperties.apiVersion >= VK_API_VERSION_1_2) {
            vkDestroyInstance(instance, nullptr);
            hasVk12 = true;
            return 1;
        }
    }

    free(physicalDevices);

    vkDestroyInstance(instance, nullptr);

    dlclose(vulkan_lib);
    hasVk12 = false;
    return 0;
    
#endif
}