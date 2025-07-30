#pragma once
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <regex.h>
#include "../log.h"
#include "../shader.h"
#include "../program.h"
#include "../buffer.h"
#include <regex>
#include <cstring>
#include <iostream>
#include "../../config/settings.h"
#include <ankerl/unordered_dense.h>
#include "../drawing.h"

extern "C" {
	/* Transform Feedback object functions */
	GLAPI void glCreateTransformFeedbacks(GLsizei n, GLuint* ids);
	GLAPI void glTransformFeedbackBufferBase(GLuint xfb, GLuint index, GLuint buffer);
	GLAPI void glTransformFeedbackBufferRange(GLuint xfb, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
	GLAPI void glGetTransformFeedbackiv(GLuint xfb, GLenum pname, GLint* param);
	GLAPI void glGetTransformFeedbacki_v(GLuint xfb, GLenum pname, GLuint index, GLint* param);
	GLAPI void glGetTransformFeedbacki64_v(GLuint xfb, GLenum pname, GLuint index, GLint64* param);

	/* Buffer object functions */
	GLAPI void glCreateBuffers(GLsizei n, GLuint* buffers);
	GLAPI void glNamedBufferStorage(GLuint buffer, GLsizeiptr size, const void* data, GLbitfield flags);
	GLAPI void glNamedBufferData(GLuint buffer, GLsizeiptr size, const void* data, GLenum usage);
	GLAPI void glNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, const void* data);
	GLAPI void glCopyNamedBufferSubData(GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
	GLAPI void glClearNamedBufferData(GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void* data);
	GLAPI void glClearNamedBufferSubData(GLuint buffer, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void* data);
	GLAPI void* glMapNamedBuffer(GLuint buffer, GLenum access);
	GLAPI void* glMapNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access);
	GLAPI GLboolean glUnmapNamedBuffer(GLuint buffer);
	GLAPI void glFlushMappedNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length);
	GLAPI void glGetNamedBufferParameteriv(GLuint buffer, GLenum pname, GLint* params);
	GLAPI void glGetNamedBufferParameteri64v(GLuint buffer, GLenum pname, GLint64* params);
	GLAPI void glGetNamedBufferPointerv(GLuint buffer, GLenum pname, void** params);
	GLAPI void glGetNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, void* data);

	/* Framebuffer object functions */
	GLAPI void glCreateFramebuffers(GLsizei n, GLuint* framebuffers);
	GLAPI void glNamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
	GLAPI void glNamedFramebufferParameteri(GLuint framebuffer, GLenum pname, GLint param);
	GLAPI void glNamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level);
	GLAPI void glNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer);
	GLAPI void glNamedFramebufferDrawBuffer(GLuint framebuffer, GLenum mode);
	GLAPI void glNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n, const GLenum* bufs);
	GLAPI void glNamedFramebufferReadBuffer(GLuint framebuffer, GLenum mode);
	GLAPI void glInvalidateNamedFramebufferData(GLuint framebuffer, GLsizei numAttachments, const GLenum* attachments);
	GLAPI void glInvalidateNamedFramebufferSubData(GLuint framebuffer, GLsizei numAttachments, const GLenum* attachments, GLint x, GLint y, GLsizei width, GLsizei height);
	GLAPI void glClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint* value);
	GLAPI void glClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint* value);
	GLAPI void glClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat* value);
	GLAPI void glClearNamedFramebufferfi(GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);
	GLAPI void glBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
	GLAPI GLenum glCheckNamedFramebufferStatus(GLuint framebuffer, GLenum target);
	GLAPI void glGetNamedFramebufferParameteriv(GLuint framebuffer, GLenum pname, GLint* param);
	GLAPI void glGetNamedFramebufferAttachmentParameteriv(GLuint framebuffer, GLenum attachment, GLenum pname, GLint* params);

	/* Renderbuffer object functions */
	GLAPI void glCreateRenderbuffers(GLsizei n, GLuint* renderbuffers);
	GLAPI void glNamedRenderbufferStorage(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height);
	GLAPI void glNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
	GLAPI void glGetNamedRenderbufferParameteriv(GLuint renderbuffer, GLenum pname, GLint* params);

	/* Texture object functions */
	GLAPI void glCreateTextures(GLenum target, GLsizei n, GLuint* textures);
	GLAPI void glTextureBuffer(GLuint texture, GLenum internalformat, GLuint buffer);
	GLAPI void glTextureBufferRange(GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size);
	GLAPI void glTextureStorage1D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width);
	GLAPI void glTextureStorage2D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
	GLAPI void glTextureStorage3D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
	GLAPI void glTextureStorage2DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
	GLAPI void glTextureStorage3DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);
	GLAPI void glTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels);
	GLAPI void glTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels);
	GLAPI void glTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels);
	GLAPI void glCompressedTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void* data);
	GLAPI void glCompressedTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data);
	GLAPI void glCompressedTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data);
	GLAPI void glCopyTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
	GLAPI void glCopyTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	GLAPI void glCopyTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	GLAPI void glTextureParameterf(GLuint texture, GLenum pname, GLfloat param);
	GLAPI void glTextureParameterfv(GLuint texture, GLenum pname, const GLfloat* param);
	GLAPI void glTextureParameteri(GLuint texture, GLenum pname, GLint param);
	GLAPI void glTextureParameterIiv(GLuint texture, GLenum pname, const GLint* params);
	GLAPI void glTextureParameterIuiv(GLuint texture, GLenum pname, const GLuint* params);
	GLAPI void glTextureParameteriv(GLuint texture, GLenum pname, const GLint* param);
	GLAPI void glGenerateTextureMipmap(GLuint texture);
	GLAPI void glBindTextureUnit(GLuint unit, GLuint texture);
	GLAPI void glGetTextureImage(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels);
	GLAPI void glGetCompressedTextureImage(GLuint texture, GLint level, GLsizei bufSize, void* pixels);
	GLAPI void glGetTextureLevelParameterfv(GLuint texture, GLint level, GLenum pname, GLfloat* params);
	GLAPI void glGetTextureLevelParameteriv(GLuint texture, GLint level, GLenum pname, GLint* params);
	GLAPI void glGetTextureParameterfv(GLuint texture, GLenum pname, GLfloat* params);
	GLAPI void glGetTextureParameterIiv(GLuint texture, GLenum pname, GLint* params);
	GLAPI void glGetTextureParameterIuiv(GLuint texture, GLenum pname, GLuint* params);
	GLAPI void glGetTextureParameteriv(GLuint texture, GLenum pname, GLint* params);

	/* Vertex Array object functions */
	GLAPI void glCreateVertexArrays(GLsizei n, GLuint* arrays);
	GLAPI void glDisableVertexArrayAttrib(GLuint vaobj, GLuint index);
	GLAPI void glEnableVertexArrayAttrib(GLuint vaobj, GLuint index);
	GLAPI void glVertexArrayElementBuffer(GLuint vaobj, GLuint buffer);
	GLAPI void glVertexArrayVertexBuffer(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride);
	GLAPI void glVertexArrayVertexBuffers(GLuint vaobj, GLuint first, GLsizei count, const GLuint* buffers, const GLintptr* offsets, const GLsizei* strides);
	GLAPI void glVertexArrayAttribFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset);
	GLAPI void glVertexArrayAttribIFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
	GLAPI void glVertexArrayAttribLFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
	GLAPI void glVertexArrayAttribBinding(GLuint vaobj, GLuint attribindex, GLuint bindingindex);
	GLAPI void glVertexArrayBindingDivisor(GLuint vaobj, GLuint bindingindex, GLuint divisor);
	GLAPI void glGetVertexArrayiv(GLuint vaobj, GLenum pname, GLint* param);
	GLAPI void glGetVertexArrayIndexediv(GLuint vaobj, GLuint index, GLenum pname, GLint* param);
	GLAPI void glGetVertexArrayIndexed64iv(GLuint vaobj, GLuint index, GLenum pname, GLint64* param);

	/* Sampler object functions */
	GLAPI void glCreateSamplers(GLsizei n, GLuint* samplers);

	/* Program Pipeline object functions */
	GLAPI void glCreateProgramPipelines(GLsizei n, GLuint* pipelines);

	/* Query object functions */
	GLAPI void glCreateQueries(GLenum target, GLsizei n, GLuint* ids);
	GLAPI void glGetQueryBufferObjectiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset);
	GLAPI void glGetQueryBufferObjectuiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset);
	GLAPI void glGetQueryBufferObjecti64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset);
	GLAPI void glGetQueryBufferObjectui64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset);
}