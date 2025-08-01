//
// Created by BZLZHH on 2025/1/27.
//

#ifndef MOBILEGLUES_TEXTURE_H
#define MOBILEGLUES_TEXTURE_H

#include <unordered_map>

#ifdef __cplusplus
extern "C" {
#endif

#include <GL/gl.h>

GLAPI GLAPIENTRY void glTexParameterf(GLenum target, GLenum pname, GLfloat param);
GLAPI GLAPIENTRY void glTexImage1D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
GLAPI GLAPIENTRY void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
GLAPI GLAPIENTRY void glTexImage3D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
GLAPI GLAPIENTRY void glTexStorage1D(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width);
GLAPI GLAPIENTRY void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height);
GLAPI GLAPIENTRY void glTexStorage3D(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth);
GLAPI GLAPIENTRY void glCopyTexImage1D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
GLAPI GLAPIENTRY void glCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
GLAPI GLAPIENTRY void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
GLAPI GLAPIENTRY void glRenderbufferStorage(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height);
GLAPI GLAPIENTRY void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height);
GLAPI GLAPIENTRY void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params);
GLAPI GLAPIENTRY void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params);
GLAPI GLAPIENTRY void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
GLAPI GLAPIENTRY void glTexParameteriv(GLenum target, GLenum pname, const GLint* params);
GLAPI GLAPIENTRY void glGenerateTextureMipmap(GLuint texture);
GLAPI GLAPIENTRY void glBindTexture(GLenum target, GLuint texture);
GLAPI GLAPIENTRY void glDeleteTextures(GLsizei n, const GLuint *textures);
GLAPI GLAPIENTRY void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, void* pixels);
GLAPI GLAPIENTRY void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels);
GLAPI GLAPIENTRY void glTexParameteri(GLenum target, GLenum pname, GLint param);
GLAPI GLAPIENTRY void glClearTexImage(GLuint texture, GLint level, GLenum format, GLenum type, const void* data);
GLAPI GLAPIENTRY void glPixelStorei(GLenum pname, GLint param);

#ifdef __cplusplus
}
#endif

enum class TextureTarget : unsigned int {
    TEXTURE_1D = 0,
    PROXY_TEXTURE_1D,
    TEXTURE_1D_ARRAY,
    PROXY_TEXTURE_1D_ARRAY,
    TEXTURE_2D,
    PROXY_TEXTURE_2D,
    TEXTURE_2D_ARRAY,
    PROXY_TEXTURE_2D_ARRAY,
    TEXTURE_2D_MULTISAMPLE,
    PROXY_TEXTURE_2D_MULTISAMPLE,
    TEXTURE_2D_MULTISAMPLE_ARRAY,
    PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY,
    TEXTURE_3D,
    PROXY_TEXTURE_3D,
    TEXTURE_RECTANGLE,
    PROXY_TEXTURE_RECTANGLE,
    TEXTURE_CUBE_MAP,
    PROXY_TEXTURE_CUBE_MAP,
    // TEXTURE_CUBE_MAP_POSITIVE_X,
    // TEXTURE_CUBE_MAP_NEGATIVE_X,
    // TEXTURE_CUBE_MAP_POSITIVE_Y,
    // TEXTURE_CUBE_MAP_NEGATIVE_Y,
    // TEXTURE_CUBE_MAP_POSITIVE_Z,
    // TEXTURE_CUBE_MAP_NEGATIVE_Z,
    TEXTURE_CUBE_MAP_ARRAY,
    PROXY_TEXTURE_CUBE_MAP_ARRAY,
    TEXTURE_BUFFER,
    TEXTURES_COUNT
};

GLenum ConvertTextureTargetToGLEnum(TextureTarget target);
TextureTarget ConvertGLEnumToTextureTarget(GLenum target);

class TextureObject { // TODO: Make this a more standard class
public:
    TextureTarget target;
    GLuint texture;
    GLenum internal_format;
    GLenum format;
    GLint swizzle_param[4];
    GLsizei width;
    GLsizei height;
    GLsizei depth;
};

std::shared_ptr<TextureObject> mgGetTexObjectByTarget(GLenum target);
std::shared_ptr<TextureObject> mgGetTexObjectByID(unsigned texture);

#endif