//
// Created by Administrator on 2025/1/27.
//

#ifndef MOBILEGLUES_LOOKUP_H
#define MOBILEGLUES_LOOKUP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <GL/gl.h>
    
GLAPI GLAPIENTRY void *glXGetProcAddress(const char *name);
GLAPI GLAPIENTRY void *glXGetProcAddressARB(const char *name);

#ifdef __cplusplus
}
#endif

#endif //MOBILEGLUES_LOOKUP_H
