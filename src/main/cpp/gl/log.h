//
// Created by BZLZHH on 2025/1/26.
//

#ifndef MOBILEGLUES_LOG_H

#define LOG() __android_log_print(ANDROID_LOG_DEBUG, RENDERERNAME, "Use function: %s", __FUNCTION__);

#define LOG_D(...) __android_log_print(ANDROID_LOG_DEBUG, RENDERERNAME, __VA_ARGS__);
#define LOG_I(...) __android_log_print(ANDROID_LOG_INFO, RENDERERNAME, __VA_ARGS__);
#define LOG_W(...) __android_log_print(ANDROID_LOG_WARN, RENDERERNAME, __VA_ARGS__);
#define LOG_E(...) __android_log_print(ANDROID_LOG_ERROR, RENDERERNAME, __VA_ARGS__);

#define MOBILEGLUES_LOG_H

#endif //MOBILEGLUES_LOG_H
