// MobileGlues - egl/trace.h
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

#ifndef MOBILEGLUES_EGL_TRACE_H
#define MOBILEGLUES_EGL_TRACE_H

#include "../gl/log.h"
#include <EGL/egl.h>
#if defined(__APPLE__)
#include <pthread.h>
#include <stdint.h>
#else
#include <sys/syscall.h>
#include <unistd.h>
#endif

// Tracing for the EGL layer.
//
// The EGL wrappers are the seam between the host process, this layer's virtual
// context records, and whichever GLES driver is behind them -- and nearly
// everything that goes wrong at that seam is a sequencing problem. Which thread
// made what current, which display is still held by whom, whether the handle the
// driver just returned is one it handed out before. An argument dump per entry
// point answers none of that, so every line here carries the calling thread and
// says what came back, not just what went in.
//
//   adb logcat -s MobileGlues:* | grep '\[EGL'
//   adb logcat -s MobileGlues:* | grep '\[EGL t12345\]'   # one thread's story
//
// This is deliberately LOG_I rather than LOG_D: it is meant to be readable from a
// release build on a user's device, without GLOBAL_DEBUG turning the whole
// renderer into a log firehose that drops the interesting lines. Set MG_EGL_TRACE
// to 0 to compile all of it out.
#define MG_EGL_TRACE 0

// A thread id, via the syscall rather than gettid(), which bionic only exposes as
// a real symbol from API 30 and this library targets 21. __NR_gettid is a Linux
// syscall number and does not exist on Darwin, where the iOS build broke on it;
// there the kernel thread id comes from pthread_threadid_np.
static inline int mg_egl_tid(void) {
#if defined(__APPLE__)
    uint64_t tid = 0;
    pthread_threadid_np(pthread_self(), &tid);
    return (int)tid;
#else
    return (int)syscall(__NR_gettid);
#endif
}

#if MG_EGL_TRACE
#define EGL_TRACE(...) LOG_I(__VA_ARGS__)
#define EGL_TRACE_TAG "[EGL t%d] "
#define EGL_T mg_egl_tid()
#else
#define EGL_TRACE(...)                                                                                                 \
    {}
#define EGL_TRACE_TAG ""
#define EGL_T 0
#endif

// Shorthand for the common shape: prefix, thread, then the message.
#define ETRACE(fmt, ...) EGL_TRACE(EGL_TRACE_TAG fmt, EGL_T, ##__VA_ARGS__)

static inline const char* mg_egl_error_name(EGLint error) {
    switch (error) {
    case EGL_SUCCESS:
        return "SUCCESS";
    case EGL_NOT_INITIALIZED:
        return "NOT_INITIALIZED";
    case EGL_BAD_ACCESS:
        return "BAD_ACCESS";
    case EGL_BAD_ALLOC:
        return "BAD_ALLOC";
    case EGL_BAD_ATTRIBUTE:
        return "BAD_ATTRIBUTE";
    case EGL_BAD_CONFIG:
        return "BAD_CONFIG";
    case EGL_BAD_CONTEXT:
        return "BAD_CONTEXT";
    case EGL_BAD_CURRENT_SURFACE:
        return "BAD_CURRENT_SURFACE";
    case EGL_BAD_DISPLAY:
        return "BAD_DISPLAY";
    case EGL_BAD_MATCH:
        return "BAD_MATCH";
    case EGL_BAD_NATIVE_PIXMAP:
        return "BAD_NATIVE_PIXMAP";
    case EGL_BAD_NATIVE_WINDOW:
        return "BAD_NATIVE_WINDOW";
    case EGL_BAD_PARAMETER:
        return "BAD_PARAMETER";
    case EGL_BAD_SURFACE:
        return "BAD_SURFACE";
    case EGL_CONTEXT_LOST:
        return "CONTEXT_LOST";
    default:
        return "?";
    }
}

static inline const char* mg_egl_api_name(EGLenum api) {
    switch (api) {
    case EGL_OPENGL_API:
        return "OpenGL";
    case EGL_OPENGL_ES_API:
        return "OpenGL ES";
    case EGL_OPENVG_API:
        return "OpenVG";
    default:
        return "?";
    }
}

#endif // MOBILEGLUES_EGL_TRACE_H
