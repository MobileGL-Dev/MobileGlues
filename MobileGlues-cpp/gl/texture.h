// MobileGlues - gl/texture.h
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

#ifndef MOBILEGLUES_TEXTURE_H
#define MOBILEGLUES_TEXTURE_H

#include <memory>

#ifdef __cplusplus
extern "C"
{
#endif

#include <GL/gl.h>

    GLAPI GLAPIENTRY void glTexParameterf(GLenum target, GLenum pname, GLfloat param);
    GLAPI GLAPIENTRY void glTexImage1D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border,
                                       GLenum format, GLenum type, const GLvoid* pixels);
    GLAPI GLAPIENTRY void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                                       GLint border, GLenum format, GLenum type, const GLvoid* pixels);
    GLAPI GLAPIENTRY void glTexImage3D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                                       GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
    GLAPI GLAPIENTRY void glTexStorage1D(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width);
    GLAPI GLAPIENTRY void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width,
                                         GLsizei height);
    GLAPI GLAPIENTRY void glTexStorage3D(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width,
                                         GLsizei height, GLsizei depth);
    GLAPI GLAPIENTRY void glCopyTexImage1D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y,
                                           GLsizei width, GLint border);
    GLAPI GLAPIENTRY void glCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y,
                                           GLsizei width, GLsizei height, GLint border);
    GLAPI GLAPIENTRY void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x,
                                              GLint y, GLsizei width, GLsizei height);
    GLAPI GLAPIENTRY void glRenderbufferStorage(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height);
    GLAPI GLAPIENTRY void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalFormat,
                                                           GLsizei width, GLsizei height);
    GLAPI GLAPIENTRY void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat* params);
    GLAPI GLAPIENTRY void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params);
    GLAPI GLAPIENTRY void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
                                          GLsizei height, GLenum format, GLenum type, const void* pixels);
    GLAPI GLAPIENTRY void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                          GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                                          GLenum format, GLenum type, const void* pixels);
    GLAPI GLAPIENTRY void glTexParameteriv(GLenum target, GLenum pname, const GLint* params);
    GLAPI GLAPIENTRY void glGenerateTextureMipmap(GLuint texture);
    GLAPI GLAPIENTRY void glBindTexture(GLenum target, GLuint texture);
    GLAPI GLAPIENTRY void glDeleteTextures(GLsizei n, const GLuint* textures);
    GLAPI GLAPIENTRY void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, void* pixels);
    GLAPI GLAPIENTRY void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type,
                                       void* pixels);
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
    TEXTURES_COUNT,
    UNKNWON
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
    // Sampling state as the application set it, GL defaults (ES 3.2 table
    // 8.19): min NEAREST_MIPMAP_LINEAR, mag LINEAR, compare NONE. The
    // driver holds the depth-legal view of these three on a sized depth
    // texture with comparison off (gl/texture.cpp push_depth_sampling_state),
    // so glGetTexParameter* answers from here rather than the driver.
    GLint min_filter = GL_NEAREST_MIPMAP_LINEAR;
    GLint mag_filter = GL_LINEAR;
    GLenum compare_mode = GL_NONE;
};

// How many texture units this layer can actually track. Anything the driver
// offers beyond this the layer cannot honour, so it must not be advertised
// either -- see the GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS case in gl/getter.cpp.
int mg_max_texture_units(void);

TextureObject* mgGetTexObjectByTarget(GLenum target);
TextureObject* mgGetTexObjectByID(unsigned texture);
void InitTextureMap(size_t expectedSize);

// The driver's own texture state, tracked as this layer issues it, so a caller
// that only wants to save and restore a binding does not have to make the driver
// round trip -- and, for a unit other than the active one, does not have to move
// the active unit twice just to be allowed to ask.
//
// Texture names are not renamed across this boundary, unlike buffer names: what
// the application binds is what the driver is given. So *out is exactly what
// GLES.glGetIntegerv(GL_TEXTURE_BINDING_<target>) would report with `unit` active,
// and it is exactly the value to hand back to glBindTexture to restore.
//
// It answers for the driver's slot, which is not always the application's: with
// hardware->emulate_texture_buffer set, glBindTexture(GL_TEXTURE_BUFFER, t) leaves
// the driver holding t on unit 15's GL_TEXTURE_2D and nothing on GL_TEXTURE_BUFFER
// anywhere. Ask for what the driver has, not for what the application asked for.
//
// False means the tracked value cannot be trusted right now and nothing was
// written to *out; the caller has to fall back to asking the driver. That happens
// while FSR1 is enabled, because its per-frame upscale leaves a texture bound on
// unit 0 that no shadow records -- see the note on the definition in
// gl/texture.cpp. Callers must keep their GLES.glGetIntegerv path for this case.
bool mg_driver_texture_binding(GLenum target, GLuint* out);
bool mg_driver_texture_binding_at_unit(int unit, GLenum target, GLuint* out);

// Which unit GLES.glActiveTexture was last given. Equal to
// gl_state->current_tex_unit outside the windows where this layer borrows a unit
// for its own work; this is the one that is right inside them too.
int mg_driver_active_texture_unit(void);

#endif