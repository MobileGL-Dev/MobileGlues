//
// Created by hanji on 2025/2/9.
//

#include "gpu_utils.h"
#include "../gles/loader.h"

#include <EGL/egl.h>

#if !defined(__APPLE__)
#include <vulkan/vulkan.h>
#endif

#include <string.h>

static const char *gles3_lib[] = {
        "libGLESv3_CM",
        "libGLESv3",
        NULL
};

const GLubyte * (*gles_glGetString)( GLenum name );

const char* getGPUInfo() {
    EGLDisplay eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (eglDisplay == EGL_NO_DISPLAY || eglInitialize(eglDisplay, NULL, NULL) != EGL_TRUE)
        return NULL;

    EGLint egl_attributes[] = {
            EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
            EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 24, EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE
    };

    EGLint num_configs = 0;
    if (eglChooseConfig(eglDisplay, egl_attributes, NULL, 0, &num_configs) != EGL_TRUE || num_configs == 0) {
        eglTerminate(eglDisplay);
        return NULL;
    }

    EGLConfig eglConfig;
    eglChooseConfig(eglDisplay, egl_attributes, &eglConfig, 1, &num_configs);

    const EGLint egl_context_attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    EGLContext context = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, egl_context_attributes);
    if (context == EGL_NO_CONTEXT) {
        eglTerminate(eglDisplay);
        return NULL;
    }

    if (eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, context) != EGL_TRUE) {
        eglDestroyContext(eglDisplay, context);
        eglTerminate(eglDisplay);
        return NULL;
    }

    const char* renderer = NULL;
    void* lib = open_lib(gles3_lib, NULL);
    if (lib) {
        gles_glGetString = (const GLubyte * (*)( GLenum ))dlsym(lib, "glGetString");
        if (gles_glGetString) {
            renderer = (const char*)gles_glGetString(GL_RENDERER);
        }
    }

    eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(eglDisplay, context);
    eglTerminate(eglDisplay);

    return renderer;
}

int isAdreno(const char* gpu) {
//    const char* gpu = getGPUInfo();
    if (!gpu)
        return 0;
    return strstr(gpu, "Adreno") != NULL;
}

int isAdreno740(const char* gpu) {
//    const char* gpu = getGPUInfo();
    if (!gpu)
        return 0;
    return isAdreno(gpu) && (strstr(gpu, "740") != NULL);
}

int isAdreno830(const char* gpu) {
//    const char* gpu = getGPUInfo();
    if (!gpu)
        return 0;
    return isAdreno(gpu) && (strstr(gpu, "830") != NULL);
}

int hasVulkan13() {
#if !defined(__APPLE__)
    VkResult result = VK_SUCCESS;
    uint32_t instanceExtensionCount = 0;

    result = vkEnumerateInstanceExtensionProperties(NULL, &instanceExtensionCount, NULL);
    if (result != VK_SUCCESS) {
        return 0;
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = NULL;
    appInfo.pApplicationName = "Vulkan Check";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "MobileGlues";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = NULL;
    createInfo.enabledExtensionCount = 0;
    createInfo.ppEnabledExtensionNames = 0;

    VkInstance instance = {};
    result = vkCreateInstance(&createInfo, NULL, &instance);
    if (result != VK_SUCCESS) {
        return 0;
    }

    uint32_t gpuCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &gpuCount, NULL);
    if (result != VK_SUCCESS || gpuCount == 0) {
        vkDestroyInstance(instance, NULL);
        return 0;
    }

    VkPhysicalDevice* physicalDevices = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * gpuCount);
    //VkPhysicalDevice physicalDevices[gpuCount];
    vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices);

    for (uint32_t i = 0; i < gpuCount; i++) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);

        if (deviceProperties.apiVersion >= VK_API_VERSION_1_3) {
            vkDestroyInstance(instance, NULL);
            return 1;
        }
    }

    free(physicalDevices);

    vkDestroyInstance(instance, NULL);
#endif
    return 0;
}