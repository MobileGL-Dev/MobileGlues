//
// Created by BZLZHH on 2025/1/28.
//

#include "getter.h"

GLAPI GLAPIENTRY void glGetIntegerv(GLenum pname, GLint *params) {
    LOG();
    LOG_D("glGetIntegerv, pname: %d",pname);
    if (pname == GL_CONTEXT_PROFILE_MASK) {
        (*params) = GL_CONTEXT_COMPATIBILITY_PROFILE_BIT;
        return;
    }
    LOAD_GLES(glGetIntegerv, void, GLenum pname, GLint *params);
    gles_glGetIntegerv(pname, params);
    LOAD_GLES(glGetError, GLenum)
    GLenum ERR = gles_glGetError();
    if (ERR != GL_NO_ERROR)
        LOG_E("ERROR: %d", ERR)
}