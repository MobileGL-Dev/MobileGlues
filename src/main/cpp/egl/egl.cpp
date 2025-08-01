//
// Created by Swung0x48 on 2024/10/10.
//

#include "egl.h"
#include "loader.h"
#include "../gles/loader.h"
#include "../gl/FSR1/FSR1.h"
#include "../glx/lookup.h"
#include "../config/settings.h"

#define DEBUG 0

extern "C" {
#define EGL_API __attribute__((visibility("default")))
    EGL_API EGLint eglGetError(void) {
		LOG_D("eglGetError");
        LOAD_EGL(eglGetError)
    
        return egl_eglGetError();
    }

    EGL_API EGLDisplay eglGetDisplay(EGLNativeDisplayType display_id) {
		LOG_D("eglGetDisplay, display_id: %p", display_id);
        LOAD_EGL(eglGetDisplay)
        return egl_eglGetDisplay(display_id);
    }

    EGL_API EGLBoolean eglInitialize(EGLDisplay dpy, EGLint* major, EGLint* minor) {
		LOG_D("eglInitialize, dpy: %p, major: %p, minor: %p", dpy, major, minor);
        LOAD_EGL(eglInitialize)
        return egl_eglInitialize(dpy, major, minor);
    }

    EGL_API EGLBoolean eglTerminate(EGLDisplay dpy) {
		LOG_D("eglTerminate, dpy: %p", dpy);
        LOAD_EGL(eglTerminate)
        return egl_eglTerminate(dpy);
    }

    EGL_API const char* eglQueryString(EGLDisplay dpy, EGLint name) {
		LOG_D("eglQueryString, dpy: %p, name: %d", dpy, name);
        LOAD_EGL(eglQueryString)
        return egl_eglQueryString(dpy, name);
    }

    EGL_API EGLBoolean eglGetConfigs(EGLDisplay dpy, EGLConfig* configs, EGLint config_size, EGLint* num_config) {
		LOG_D("eglGetConfigs, dpy: %p, configs: %p, config_size: %d, num_config: %p", dpy, configs, config_size, num_config);
        LOAD_EGL(eglGetConfigs)
        return egl_eglGetConfigs(dpy, configs, config_size, num_config);
    }

    EGL_API EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint* attrib_list, EGLConfig* configs, EGLint config_size, EGLint* num_config) {
		LOG_D("eglChooseConfig, dpy: %p, attrib_list: %p, configs: %p, config_size: %d, num_config: %p", dpy, attrib_list, configs, config_size, num_config);
        LOAD_EGL(eglChooseConfig)
        return egl_eglChooseConfig(dpy, attrib_list, configs, config_size, num_config);
    }

    EGL_API EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint* value) {
		LOG_D("eglGetConfigAttrib, dpy: %p, config: %p, attribute: %d, value: %p", dpy, config, attribute, value);
        LOAD_EGL(eglGetConfigAttrib)
        return egl_eglGetConfigAttrib(dpy, config, attribute, value);
    }

    EGL_API EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint* attrib_list) {
		LOG_D("eglCreateWindowSurface, dpy: %p, config: %p, win: %p, attrib_list: %p", dpy, config, win, attrib_list);
        LOAD_EGL(eglCreateWindowSurface)
        return egl_eglCreateWindowSurface(dpy, config, win, attrib_list);
    }

    EGL_API EGLSurface eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint* attrib_list) {
		LOG_D("eglCreatePbufferSurface, dpy: %p, config: %p, attrib_list: %p", dpy, config, attrib_list);
        LOAD_EGL(eglCreatePbufferSurface)
        return egl_eglCreatePbufferSurface(dpy, config, attrib_list);
    }

    EGL_API EGLSurface eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint* attrib_list) {
		LOG_D("eglCreatePixmapSurface, dpy: %p, config: %p, pixmap: %p, attrib_list: %p", dpy, config, pixmap, attrib_list);
        LOAD_EGL(eglCreatePixmapSurface)
        return egl_eglCreatePixmapSurface(dpy, config, pixmap, attrib_list);
    }

    EGL_API EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface) {
		LOG_D("eglDestroySurface, dpy: %p, surface: %p", dpy, surface);
        LOAD_EGL(eglDestroySurface)
        return egl_eglDestroySurface(dpy, surface);
    }

    EGL_API EGLBoolean eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint* value) {
		LOG_D("eglQuerySurface, dpy: %p, surface: %p, attribute: %d, value: %p", dpy, surface, attribute, value);
        LOAD_EGL(eglQuerySurface)
        return egl_eglQuerySurface(dpy, surface, attribute, value);
    }

    EGL_API EGLBoolean eglBindAPI(EGLenum api) {
		LOG_D("eglBindAPI, api: %d", api);
        LOAD_EGL(eglBindAPI)
        return egl_eglBindAPI(api);
    }

    EGL_API EGLenum eglQueryAPI(void) {
		LOG_D("eglQueryAPI");
        LOAD_EGL(eglQueryAPI)
        return egl_eglQueryAPI();
    }

    EGL_API EGLBoolean eglWaitClient(void) {
		LOG_D("eglWaitClient");
        LOAD_EGL(eglWaitClient)
        return egl_eglWaitClient();
    }

    EGL_API EGLBoolean eglReleaseThread(void) {
		LOG_D("eglReleaseThread");
        LOAD_EGL(eglReleaseThread)
        return egl_eglReleaseThread();
    }

    EGL_API EGLSurface eglCreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint* attrib_list) {
		LOG_D("eglCreatePbufferFromClientBuffer, dpy: %p, buftype: %d, buffer: %p, config: %p, attrib_list: %p", dpy, buftype, buffer, config, attrib_list);
        LOAD_EGL(eglCreatePbufferFromClientBuffer)
        return egl_eglCreatePbufferFromClientBuffer(dpy, buftype, buffer, config, attrib_list);
    }

    EGL_API EGLBoolean eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value) {
		LOG_D("eglSurfaceAttrib, dpy: %p, surface: %p, attribute: %d, value: %d", dpy, surface, attribute, value);
        LOAD_EGL(eglSurfaceAttrib)
        return egl_eglSurfaceAttrib(dpy, surface, attribute, value);
    }

    EGL_API EGLBoolean eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer) {
		LOG_D("eglBindTexImage, dpy: %p, surface: %p, buffer: %d", dpy, surface, buffer);
        LOAD_EGL(eglBindTexImage)
        return egl_eglBindTexImage(dpy, surface, buffer);
    }

    EGL_API EGLBoolean eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer) {
		LOG_D("eglReleaseTexImage, dpy: %p, surface: %p, buffer: %d", dpy, surface, buffer);
        LOAD_EGL(eglReleaseTexImage)
        return egl_eglReleaseTexImage(dpy, surface, buffer);
    }

    EGL_API EGLBoolean eglSwapInterval(EGLDisplay dpy, EGLint interval) {
		LOG_D("eglSwapInterval, dpy: %p, interval: %d", dpy, interval);
        LOAD_EGL(eglSwapInterval)
        return egl_eglSwapInterval(dpy, interval);
    }

    EGL_API EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint* attrib_list) {
		LOG_D("eglCreateContext, dpy: %p, config: %p, share_context: %p, attrib_list: %p", dpy, config, share_context, attrib_list);
        LOAD_EGL(eglCreateContext)
        return egl_eglCreateContext(dpy, config, share_context, attrib_list);
    }

    EGL_API EGLBoolean eglDestroyContext(EGLDisplay dpy, EGLContext ctx) {
		LOG_D("eglDestroyContext, dpy: %p, ctx: %p", dpy, ctx);
        LOAD_EGL(eglDestroyContext)
        return egl_eglDestroyContext(dpy, ctx);
    }

    EGL_API EGLBoolean eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx) {
		LOG_D("eglMakeCurrent, dpy: %p, draw: %p, read: %p, ctx: %p", dpy, draw, read, ctx);
        LOAD_EGL(eglMakeCurrent)
        return egl_eglMakeCurrent(dpy, draw, read, ctx);
    }

    EGL_API EGLContext eglGetCurrentContext(void) {
		LOG_D("eglGetCurrentContext");
        LOAD_EGL(eglGetCurrentContext)
        return egl_eglGetCurrentContext();
    }

    EGL_API EGLSurface eglGetCurrentSurface(EGLint readdraw) {
		LOG_D("eglGetCurrentSurface, readdraw: %d", readdraw);
        LOAD_EGL(eglGetCurrentSurface)
        return egl_eglGetCurrentSurface(readdraw);
    }

    EGL_API EGLDisplay eglGetCurrentDisplay(void) {
		LOG_D("eglGetCurrentDisplay");
        LOAD_EGL(eglGetCurrentDisplay)
        return egl_eglGetCurrentDisplay();
    }

    EGL_API EGLBoolean eglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint* value) {
		LOG_D("eglQueryContext, dpy: %p, ctx: %p, attribute: %d, value: %p", dpy, ctx, attribute, value);
        LOAD_EGL(eglQueryContext)
        return egl_eglQueryContext(dpy, ctx, attribute, value);
    }

    EGL_API EGLBoolean eglWaitGL(void) {
		LOG_D("eglWaitGL");
        LOAD_EGL(eglWaitGL)
        return egl_eglWaitGL();
    }

    EGL_API EGLBoolean eglWaitNative(EGLint engine) {
		LOG_D("eglWaitNative, engine: %d", engine);
        LOAD_EGL(eglWaitNative)
        return egl_eglWaitNative(engine);
    }

    EGL_API EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
		LOG_D("eglSwapBuffers, dpy: %p, surface: %p", dpy, surface);
        LOAD_EGL(eglSwapBuffers)
        EGLBoolean result;
        if (global_settings.fsr1_setting != FSR1_Quality_Preset::Disabled) {
            ApplyFSR();
            result = egl_eglSwapBuffers(dpy, surface);
            CheckResolutionChange();
        } else {
            result = egl_eglSwapBuffers(dpy, surface);
        }
		return result;
    }

    EGL_API EGLBoolean eglCopyBuffers(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target) {
		LOG_D("eglCopyBuffers, dpy: %p, surface: %p, target: %p", dpy, surface, target);
        LOAD_EGL(eglCopyBuffers)
        return egl_eglCopyBuffers(dpy, surface, target);
    }

    EGL_API EGLDisplay eglGetPlatformDisplay(EGLenum platform, void* native_display, const EGLAttrib* attrib_list) {
		LOG_D("eglGetPlatformDisplay, platform: %d, native_display: %p, attrib_list: %p", platform, native_display, attrib_list);
        LOAD_EGL(eglGetPlatformDisplay)
        return egl_eglGetPlatformDisplay(platform, native_display, (const EGLint*)attrib_list);
    }

    EGL_API EGLAPI __eglMustCastToProperFunctionPointerType EGLAPIENTRY eglGetProcAddress(const char* procname) {
        return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(glXGetProcAddress(procname));
    }
}