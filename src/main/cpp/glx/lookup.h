//
// Created by Administrator on 2025/1/27.
//

#ifndef MOBILEGLUES_LOOKUP_H
#define MOBILEGLUES_LOOKUP_H

void *glXGetProcAddress(const char *name) __attribute__((visibility("default")));
void *glXGetProcAddressARB(const char *name) __attribute__((visibility("default")));

#endif //MOBILEGLUES_LOOKUP_H
