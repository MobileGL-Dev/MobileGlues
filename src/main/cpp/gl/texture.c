//
// Created by BZLZHH on 2025/1/27.
//

#include "texture.h"

#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <android/log.h>

#include "gl.h"
#include "../gles/gles.h"
#include "log.h"
#include "../gles/loader.h"
#include "../includes.h"
#include "glsl/glsl_for_es.h"
#include "mg.h"

int nlevel(int size, int level) {
    if(size) {
        size>>=level;
        if(!size) size=1;
    }
    return size;
}

GLenum internal_convert(GLenum internal_format, GLenum type) {
    switch (internal_format) {
        case GL_DEPTH_COMPONENT:
            if(type == GL_UNSIGNED_SHORT)
                return GL_DEPTH_COMPONENT16;
            if(type == GL_UNSIGNED_INT)
                return GL_DEPTH_COMPONENT24;
            if(type == GL_FLOAT)
                return GL_DEPTH_COMPONENT32F;
    }
    return internal_format;
}

void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
    LOG();
    pname = pname_convert(pname);
    if(pname == GL_TEXTURE_LOD_BIAS)
        return;
    LOG_D("glTexParameterf, target: %d, pname: %d, param: %f",target, pname, param);
    LOAD_GLES(glTexParameterf, void, GLenum target, GLenum pname, GLfloat param);
    gles_glTexParameterf(target,pname, param);
    LOAD_GLES(glGetError, GLenum)
    GLenum ERR = gles_glGetError();
    if (ERR != GL_NO_ERROR)
        LOG_E("ERROR: %d", ERR)
}

void glTexImage1D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* pixels) {
    LOG();
    LOG_D("glTexImage1D, target: %d, level: %d, internalFormat: %d, width: %d, border: %d, format: %d, type: %d",
          target, level, internalFormat, width, border, format, type);
    GLenum convertedInternal = internal_convert(internalFormat, type);

    GLenum rtarget = map_tex_target(target);
    if (rtarget == GL_PROXY_TEXTURE_1D) {
        int max1 = 4096;
        LOAD_GLES(glGetIntegerv, void, GLenum pname, GLint * data)
        gles_glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max1);
        set_gl_state_proxy_width(((width << level) > max1) ? 0 : width);
        set_gl_state_proxy_intformat(convertedInternal);
        return;
    }

    LOAD_GLES(glTexImage1D, void, GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
    gles_glTexImage1D(target, level, convertedInternal, width, border, format, type, pixels);

    LOAD_GLES(glGetError, GLenum);
    GLenum ERR = gles_glGetError();
    if (ERR != GL_NO_ERROR)
        LOG_E("glTexImage1D ERROR: %d", ERR);
}

void glTexImage2D(GLenum target, GLint level,GLint internalFormat,GLsizei width, GLsizei height,GLint border, GLenum format, GLenum type,const GLvoid* pixels) {
    LOG();
    GLenum convertedInternal = internal_convert(internalFormat, type);
    LOG_D("glTexImage2D,target: %d,level: %d,internalFormat: %d,width: %d,height: %d,border: %d,format: %d,type: %d",target,level,convertedInternal,width,height,border,format,type);
    GLenum rtarget = map_tex_target(target);
    if(rtarget == GL_PROXY_TEXTURE_2D) {
        int max1 = 4096;
        LOAD_GLES(glGetIntegerv, void, GLenum pname, GLint * data)
        gles_glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max1);
        set_gl_state_proxy_width(((width<<level)>max1)?0:width);
        set_gl_state_proxy_height(((height<<level)>max1)?0:height);
        set_gl_state_proxy_intformat(convertedInternal);
        return;
    }
    LOAD_GLES(glTexImage2D, void, GLenum target, GLint level,GLint internalFormat,GLsizei width, GLsizei height,GLint border, GLenum format, GLenum type,const GLvoid* pixels);
    gles_glTexImage2D(target,level,convertedInternal,width,height,border,format,type,pixels);
    LOAD_GLES(glGetError, GLenum)
    GLenum ERR = gles_glGetError();
    if (ERR != GL_NO_ERROR)
        LOG_E("ERROR: %d", ERR)
}

void glTexImage3D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels) {
    LOG();
    LOG_D("glTexImage3D, target: %d, level: %d, internalFormat: %d, width: %d, height: %d, depth: %d, border: %d, format: %d, type: %d",
          target, level, internalFormat, width, height, depth, border, format, type);

    GLenum convertedInternal = internal_convert(internalFormat, type);

    GLenum rtarget = map_tex_target(target);
    if (rtarget == GL_PROXY_TEXTURE_3D) {
        int max1 = 4096;
        LOAD_GLES(glGetIntegerv, void, GLenum pname, GLint * data)
        gles_glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max1);
        set_gl_state_proxy_width(((width << level) > max1) ? 0 : width);
        set_gl_state_proxy_height(((height << level) > max1) ? 0 : height);
        //set_gl_state_proxy_depth(((depth << level) > max1) ? 0 : depth);
        set_gl_state_proxy_intformat(convertedInternal);
        return;
    }

    LOAD_GLES(glTexImage3D, void, GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
    gles_glTexImage3D(target, level, convertedInternal, width, height, depth, border, format, type, pixels);

    LOAD_GLES(glGetError, GLenum);
    GLenum ERR = gles_glGetError();
    if (ERR != GL_NO_ERROR)
        LOG_E("glTexImage3D ERROR: %d", ERR);
}

void glTexStorage1D(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width) {
    LOG();
    LOG_D("glTexStorage1D, target: %d, levels: %d, internalFormat: %d, width: %d",
          target, levels, internalFormat, width);

    GLenum convertedInternal = internal_convert(internalFormat, GL_UNSIGNED_BYTE);

    LOAD_GLES(glTexStorage1D, void, GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width);
    gles_glTexStorage1D(target, levels, convertedInternal, width);

    LOAD_GLES(glGetError, GLenum);
    GLenum ERR = gles_glGetError();
    if (ERR != GL_NO_ERROR)
        LOG_E("glTexStorage1D ERROR: %d", ERR);
}

void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height) {
    LOG();
    LOG_D("glTexStorage2D, target: %d, levels: %d, internalFormat: %d, width: %d, height: %d",
          target, levels, internalFormat, width, height);

    GLenum convertedInternal = internal_convert(internalFormat, GL_UNSIGNED_BYTE);

    LOAD_GLES(glTexStorage2D, void, GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height);
    gles_glTexStorage2D(target, levels, convertedInternal, width, height);

    LOAD_GLES(glGetError, GLenum);
    GLenum ERR = gles_glGetError();
    if (ERR != GL_NO_ERROR)
        LOG_E("glTexStorage2D ERROR: %d", ERR);
}

void glTexStorage3D(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth) {
    LOG();
    LOG_D("glTexStorage3D, target: %d, levels: %d, internalFormat: %d, width: %d, height: %d, depth: %d",
          target, levels, internalFormat, width, height, depth);

    GLenum convertedInternal = internal_convert(internalFormat, GL_UNSIGNED_BYTE);

    LOAD_GLES(glTexStorage3D, void, GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth);
    gles_glTexStorage3D(target, levels, convertedInternal, width, height, depth);

    LOAD_GLES(glGetError, GLenum);
    GLenum ERR = gles_glGetError();
    if (ERR != GL_NO_ERROR)
        LOG_E("glTexStorage3D ERROR: %d", ERR);
}

void glCopyTexImage1D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border) {
    LOG();
    LOG_D("glCopyTexImage1D, target: %d, level: %d, internalFormat: %d, x: %d, y: %d, width: %d, border: %d",
          target, level, internalFormat, x, y, width, border);

    GLenum convertedInternal = internal_convert(internalFormat, GL_UNSIGNED_BYTE);

    LOAD_GLES(glCopyTexImage1D, void, GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
    gles_glCopyTexImage1D(target, level, convertedInternal, x, y, width, border);

    LOAD_GLES(glGetError, GLenum);
    GLenum ERR = gles_glGetError();
    if (ERR != GL_NO_ERROR)
        LOG_E("glCopyTexImage1D ERROR: %d", ERR);
}

void glCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
    LOG();
    LOG_D("glCopyTexImage2D, target: %d, level: %d, internalFormat: %d, x: %d, y: %d, width: %d, height: %d, border: %d",
          target, level, internalFormat, x, y, width, height, border);

    GLenum convertedInternal = internal_convert(internalFormat, GL_UNSIGNED_BYTE);

    LOAD_GLES(glCopyTexImage2D, void, GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
    gles_glCopyTexImage2D(target, level, convertedInternal, x, y, width, height, border);

    LOAD_GLES(glGetError, GLenum);
    GLenum ERR = gles_glGetError();
    if (ERR != GL_NO_ERROR)
        LOG_E("glCopyTexImage2D ERROR: %d", ERR);
}

void glRenderbufferStorage(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height) {
    LOG();
    LOG_D("glRenderbufferStorage, target: %d, internalFormat: %d, width: %d, height: %d",
          target, internalFormat, width, height);

    GLenum convertedInternal = internal_convert(internalFormat, GL_UNSIGNED_BYTE);

    LOAD_GLES(glRenderbufferStorage, void, GLenum target, GLenum internalFormat, GLsizei width, GLsizei height);
    gles_glRenderbufferStorage(target, convertedInternal, width, height);

    LOAD_GLES(glGetError, GLenum);
    GLenum ERR = gles_glGetError();
    if (ERR != GL_NO_ERROR)
        LOG_E("glRenderbufferStorage ERROR: %d", ERR);
}

void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height) {
    LOG();
    LOG_D("glRenderbufferStorageMultisample, target: %d, samples: %d, internalFormat: %d, width: %d, height: %d",
          target, samples, internalFormat, width, height);

    GLenum convertedInternal = internal_convert(internalFormat, GL_UNSIGNED_BYTE);

    LOAD_GLES(glRenderbufferStorageMultisample, void, GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height);
    gles_glRenderbufferStorageMultisample(target, samples, convertedInternal, width, height);

    LOAD_GLES(glGetError, GLenum);
    GLenum ERR = gles_glGetError();
    if (ERR != GL_NO_ERROR)
        LOG_E("glRenderbufferStorageMultisample ERROR: %d", ERR);
}

void glGetTexLevelParameterfv(GLenum target, GLint level,GLenum pname, GLfloat *params) {
    LOG();
    LOG_D("glGetTexLevelParameterfv,target: %d, level: %d, pname: %d",target,level,pname);
    GLenum rtarget = map_tex_target(target);
    if (rtarget==GL_PROXY_TEXTURE_2D) {
        switch (pname) {
            case GL_TEXTURE_WIDTH:
                (*params) = nlevel(gl_state->proxy_width, level);
                break;
            case GL_TEXTURE_HEIGHT:
                (*params) = nlevel(gl_state->proxy_height, level);
                break;
            case GL_TEXTURE_INTERNAL_FORMAT:
                (*params) = gl_state->proxy_intformat;
                break;
        }
        return;
    }
    LOAD_GLES(glGetTexLevelParameterfv, void, GLenum target, GLint level,GLenum pname, GLfloat *params);
    gles_glGetTexLevelParameterfv(target,level,pname,params);
    LOAD_GLES(glGetError, GLenum)
    GLenum ERR = gles_glGetError();
    if (ERR != GL_NO_ERROR)
        LOG_E("ERROR: %d", ERR)
}

void glGetTexLevelParameteriv(GLenum target, GLint level,GLenum pname, GLint *params) {
    LOG();
    LOG_D("glGetTexLevelParameteriv,target: %d, level: %d, pname: %d",target,level,pname);
    GLenum rtarget = map_tex_target(target);
    if (rtarget==GL_PROXY_TEXTURE_2D) {
        switch (pname) {
            case GL_TEXTURE_WIDTH:
                (*params) = nlevel(gl_state->proxy_width, level);
                break;
            case GL_TEXTURE_HEIGHT:
                (*params) = nlevel(gl_state->proxy_height, level);
                break;
            case GL_TEXTURE_INTERNAL_FORMAT:
                (*params) = gl_state->proxy_intformat;
                break;
        }
        return;
    }
    LOAD_GLES(glGetTexLevelParameteriv, void, GLenum target, GLint level,GLenum pname, GLint *params);
    GLint *fparams = NULL;
    gles_glGetTexLevelParameteriv(target,level,pname,fparams);
    if(pname==GL_TEXTURE_BORDER_COLOR) {
        for(int i=0; i<4; ++i) {
            params[i] = fparams[i];
        }
    } else {
        (*params) = fparams[0];
    }
    LOAD_GLES(glGetError, GLenum)
    GLenum ERR = gles_glGetError();
    if (ERR != GL_NO_ERROR)
        LOG_E("ERROR: %d", ERR)
}