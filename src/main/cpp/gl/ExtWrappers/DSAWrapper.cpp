//
// Created by BZLZHH on 2025/7/26.
//

#include "DSAWrapper.h"
#include <cassert>
#include "../texture.h"

#define DEBUG 0

GLenum GetBindingQuery(GLenum target, bool forceTexture = false) {
	switch (target) {
	case GL_TEXTURE_BUFFER:                return forceTexture ? GL_TEXTURE_BINDING_BUFFER : GL_TEXTURE_BUFFER_BINDING;

	case GL_ARRAY_BUFFER:                  return GL_ARRAY_BUFFER_BINDING;
	case GL_ATOMIC_COUNTER_BUFFER:         return GL_ATOMIC_COUNTER_BUFFER_BINDING;
	case GL_COPY_READ_BUFFER:              return GL_COPY_READ_BUFFER_BINDING;
	case GL_COPY_WRITE_BUFFER:             return GL_COPY_WRITE_BUFFER_BINDING;
	case GL_DISPATCH_INDIRECT_BUFFER:      return GL_DISPATCH_INDIRECT_BUFFER_BINDING;
	case GL_DRAW_INDIRECT_BUFFER:          return GL_DRAW_INDIRECT_BUFFER_BINDING;
	case GL_ELEMENT_ARRAY_BUFFER:          return GL_ELEMENT_ARRAY_BUFFER_BINDING;
	case GL_PIXEL_PACK_BUFFER:             return GL_PIXEL_PACK_BUFFER_BINDING;
	case GL_PIXEL_UNPACK_BUFFER:           return GL_PIXEL_UNPACK_BUFFER_BINDING;
	case GL_QUERY_BUFFER:                  return GL_QUERY_BUFFER_BINDING;
	case GL_SHADER_STORAGE_BUFFER:         return GL_SHADER_STORAGE_BUFFER_BINDING;
	case GL_TRANSFORM_FEEDBACK_BUFFER:     return GL_TRANSFORM_FEEDBACK_BUFFER_BINDING;
	case GL_UNIFORM_BUFFER:                return GL_UNIFORM_BUFFER_BINDING;

	case GL_FRAMEBUFFER:                   return GL_FRAMEBUFFER_BINDING;
	case GL_DRAW_FRAMEBUFFER:              return GL_DRAW_FRAMEBUFFER_BINDING;
	case GL_READ_FRAMEBUFFER:              return GL_READ_FRAMEBUFFER_BINDING;

	case GL_RENDERBUFFER:                  return GL_RENDERBUFFER_BINDING;

	case GL_VERTEX_ARRAY:                  return GL_VERTEX_ARRAY_BINDING;
	case GL_VERTEX_ARRAY_BINDING:          return GL_VERTEX_ARRAY_BINDING;

	case GL_PROGRAM_PIPELINE:              return GL_PROGRAM_PIPELINE_BINDING;

	case GL_PROGRAM:                       return GL_CURRENT_PROGRAM;

	case GL_SAMPLER:                       return GL_SAMPLER_BINDING;

	case GL_TEXTURE:                       return GL_TEXTURE_BINDING_2D;
	case GL_TEXTURE_1D:                    return GL_TEXTURE_BINDING_1D;
	case GL_TEXTURE_1D_ARRAY:              return GL_TEXTURE_BINDING_1D_ARRAY;
	case GL_TEXTURE_2D:                    return GL_TEXTURE_BINDING_2D;
	case GL_TEXTURE_2D_ARRAY:              return GL_TEXTURE_BINDING_2D_ARRAY;
	case GL_TEXTURE_2D_MULTISAMPLE:        return GL_TEXTURE_BINDING_2D_MULTISAMPLE;
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:  return GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY;
	case GL_TEXTURE_3D:                    return GL_TEXTURE_BINDING_3D;
	case GL_TEXTURE_CUBE_MAP:              return GL_TEXTURE_BINDING_CUBE_MAP;
	case GL_TEXTURE_CUBE_MAP_ARRAY:        return GL_TEXTURE_BINDING_CUBE_MAP_ARRAY;
	case GL_TEXTURE_RECTANGLE:             return GL_TEXTURE_BINDING_RECTANGLE;

	case GL_TRANSFORM_FEEDBACK:            return GL_TRANSFORM_FEEDBACK_BINDING;

	case GL_SAMPLES_PASSED:                return GL_SAMPLES_PASSED;
	case GL_PRIMITIVES_GENERATED:          return GL_PRIMITIVES_GENERATED;

	case GL_DEBUG_OUTPUT:                  return GL_DEBUG_OUTPUT;
	case GL_DEBUG_OUTPUT_SYNCHRONOUS:      return GL_DEBUG_OUTPUT_SYNCHRONOUS;

	default:
		LOG_E("[DSA] GetBindingQuery: unknown target %u", target);
		return 0;
	}
}

// buffer
static thread_local ankerl::unordered_dense::map<GLenum, std::vector<GLuint>> bufferBindingStack;
void temporarilyBindBuffer(GLuint bufferID, GLenum target = GL_ARRAY_BUFFER) {
	GLenum bindingQuery = GetBindingQuery(target);
	GLint prev = 0;
	glGetIntegerv(bindingQuery, &prev);
	if (prev == bufferID) {
		bufferBindingStack[target].push_back(-1);
		return;
	}
	bufferBindingStack[target].push_back(static_cast<GLuint>(prev));

	LOG_D("[DSA] [TempBind] target=0x%X, prev=%u -> bind=%u", target, prev, bufferID);
	CHECK_GL_ERROR;
	glBindBuffer(target, bufferID);
	CHECK_GL_ERROR_NO_INIT;
}
void restoreTemporaryBufferBinding(GLenum target = GL_ARRAY_BUFFER) {
	auto it = bufferBindingStack.find(target);
	if (it == bufferBindingStack.end() || it->second.empty()) {
	LOG_D("[DSA] [Restore] no saved binding for target 0x%X", target);
		return;
	}

	GLuint toRestore = it->second.back();
	it->second.pop_back();

	if (toRestore == static_cast<GLuint>(-1)) {
		LOG_D("[DSA] [Restore] target=0x%X, no binding to restore", target);
		return;
	}

	LOG_D("[DSA] [Restore] target=0x%X, bind back to %u", target, toRestore);
	CHECK_GL_ERROR;
	glBindBuffer(target, toRestore);
	CHECK_GL_ERROR_NO_INIT;

	if (it->second.empty())
		bufferBindingStack.erase(it);
}

void glCreateBuffers(GLsizei n, GLuint* buffers) {
	LOG()
	LOG_D("[DSA] glCreateBuffers, n: %d, buffers: %p", n, buffers);
	
	if (n <= 0 || !buffers) {
		LOG_E("[DSA] Invalid parameters for glCreateBuffers");
		return;
	}

	for (GLsizei i = 0; i < n; ++i) {
		GLuint bufID = 0;
		glGenBuffers(1, &bufID);
		if (bufID == 0) {
			LOG_E("[DSA] Failed to create buffer at index %d", i);
			continue;
		}

		temporarilyBindBuffer(bufID); // after binding, the buffer object should be created
		restoreTemporaryBufferBinding();
		buffers[i] = bufID;
	}
	CHECK_GL_ERROR;
	
	LOG_D("[DSA] Created %d buffers successfully", n);
}

void glNamedBufferStorage(GLuint buffer, GLsizeiptr size, const void* data, GLbitfield flags) {
	LOG()
	LOG_D("[DSA] glNamedBufferStorage, buffer: %u, size: %lld, data: %p, flags: %u", buffer, size, data, flags);
	
	if (buffer == 0 || size <= 0) {
		LOG_E("[DSA] Invalid parameters for glNamedBufferStorage");
		return;
	}

	temporarilyBindBuffer(buffer);
	glBufferStorage(GL_ARRAY_BUFFER, size, data, flags);
	CHECK_GL_ERROR;
	restoreTemporaryBufferBinding();
	
	LOG_D("[DSA] Buffer %u stored with size %lld", buffer, size);
}

void glNamedBufferData(GLuint buffer, GLsizeiptr size, const void* data, GLenum usage) {
	LOG()
	LOG_D("[DSA] glNamedBufferData, buffer: %u, size: %lld, data: %p, usage: %u", buffer, size, data, usage);
	
	if (buffer == 0 || size <= 0) {
		LOG_E("[DSA] Invalid parameters for glNamedBufferData");
		return;
	}
	
	temporarilyBindBuffer(buffer);
	glBufferData(GL_ARRAY_BUFFER, size, data, usage);
	CHECK_GL_ERROR;
	restoreTemporaryBufferBinding();
	
	LOG_D("[DSA] Buffer %u data set with size %lld", buffer, size);
}

void glNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, const void* data) {
	LOG()
	LOG_D("[DSA] glNamedBufferSubData, buffer: %u, offset: %lld, size: %lld, data: %p", buffer, offset, size, data);
	
	if (buffer == 0 || size <= 0 || offset < 0) {
		LOG_E("[DSA] Invalid parameters for glNamedBufferSubData");
		return;
	}
	temporarilyBindBuffer(buffer);
	glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
	CHECK_GL_ERROR;
	restoreTemporaryBufferBinding();
	
	LOG_D("[DSA] Buffer %u sub-data set with size %lld at offset %lld", buffer, size, offset);
}

void glCopyNamedBufferSubData(GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) {
	LOG()
	LOG_D("[DSA] glCopyNamedBufferSubData, readBuffer: %u, writeBuffer: %u, readOffset: %lld, writeOffset: %lld, size: %lld", readBuffer, writeBuffer, readOffset, writeOffset, size);
	
	if (readBuffer == 0 || writeBuffer == 0 || size <= 0 || readOffset < 0 || writeOffset < 0) {
		LOG_E("[DSA] Invalid parameters for glCopyNamedBufferSubData");
		return;
	}
	temporarilyBindBuffer(readBuffer, GL_COPY_READ_BUFFER);
	temporarilyBindBuffer(writeBuffer, GL_COPY_WRITE_BUFFER);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, readOffset, writeOffset, size);
	CHECK_GL_ERROR;
	restoreTemporaryBufferBinding(GL_COPY_READ_BUFFER);
	restoreTemporaryBufferBinding(GL_COPY_WRITE_BUFFER);
	LOG_D("[DSA] Copied %lld bytes from buffer %u to buffer %u", size, readBuffer, writeBuffer);
}

void glClearNamedBufferData(GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void* data) {
	LOG()
	LOG_D("[DSA] glClearNamedBufferData, buffer: %u, internalformat: 0x%X, format: 0x%X, type: 0x%X, data: %p", buffer, internalformat, format, type, data);
	
	if (buffer == 0) {
		LOG_E("[DSA] Invalid buffer ID for glClearNamedBufferData");
		return;
	}
	temporarilyBindBuffer(buffer);
	glClearBufferData(GL_ARRAY_BUFFER, internalformat, format, type, data);
	CHECK_GL_ERROR;
	restoreTemporaryBufferBinding();
	
	LOG_D("[DSA] Cleared buffer %u with specified data", buffer);
}

void glClearNamedBufferSubData(GLuint buffer, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void* data) {
	LOG()
	LOG_D("[DSA] glClearNamedBufferSubData, buffer: %u, internalformat: 0x%X, offset: %lld, size: %lld, format: 0x%X, type: 0x%X, data: %p", buffer, internalformat, offset, size, format, type, data);
	
	if (buffer == 0 || size <= 0 || offset < 0) {
		LOG_E("[DSA] Invalid parameters for glClearNamedBufferSubData");
		return;
	}
	temporarilyBindBuffer(buffer);
	glClearBufferSubData(GL_ARRAY_BUFFER, internalformat, offset, size, format, type, data);
	CHECK_GL_ERROR;
	restoreTemporaryBufferBinding();
	
	LOG_D("[DSA] Cleared sub-data of buffer %u with size %lld at offset %lld", buffer, size, offset);
}

void* glMapNamedBuffer(GLuint buffer, GLenum access) {
	LOG()
	LOG_D("[DSA] glMapNamedBuffer, buffer: %u, access: 0x%X", buffer, access);
	
	if (buffer == 0) {
		LOG_E("[DSA] Invalid buffer ID for glMapNamedBuffer");
		return nullptr;
	}
	temporarilyBindBuffer(buffer);
	void* mappedData = glMapBuffer(GL_ARRAY_BUFFER, access);
	CHECK_GL_ERROR;
	restoreTemporaryBufferBinding();
	
	if (!mappedData) {
		LOG_E("[DSA] Failed to map buffer %u", buffer);
	} else {
	LOG_D("[DSA] Mapped buffer %u successfully", buffer);
	}
	return mappedData;
}

GLvoid* glMapNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access) {
	LOG()
	LOG_D("[DSA] glMapNamedBufferRange, buffer: %u, offset: %lld, length: %lld, access: 0x%X", buffer, offset, length, access);
	
	if (buffer == 0 || length <= 0 || offset < 0) {
		LOG_E("[DSA] Invalid parameters for glMapNamedBufferRange");
		return nullptr;
	}
	temporarilyBindBuffer(buffer);
	void* mappedData = glMapBufferRange(GL_ARRAY_BUFFER, offset, length, access);
	CHECK_GL_ERROR;
	restoreTemporaryBufferBinding();
	
	if (!mappedData) {
		LOG_E("[DSA] Failed to map buffer range for buffer %u", buffer);
	} else {
	LOG_D("[DSA] Mapped buffer range for buffer %u successfully", buffer);
	}
	return mappedData;
}

GLboolean glUnmapNamedBuffer(GLuint buffer) {
	LOG()
	LOG_D("[DSA] glUnmapNamedBuffer, buffer: %u", buffer);
	
	if (buffer == 0) {
		LOG_E("[DSA] Invalid buffer ID for glUnmapNamedBuffer");
		return GL_FALSE;
	}
	temporarilyBindBuffer(buffer);
	GLboolean result = glUnmapBuffer(GL_ARRAY_BUFFER);
	CHECK_GL_ERROR;
	restoreTemporaryBufferBinding();
	
	if (result == GL_FALSE) {
		LOG_E("[DSA] Failed to unmap buffer %u", buffer);
	} else {
	LOG_D("[DSA] Unmapped buffer %u successfully", buffer);
	}
	return result;
}

void glFlushMappedNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length) {
	LOG()
	LOG_D("[DSA] glFlushMappedNamedBufferRange, buffer: %u, offset: %lld, length: %lld", buffer, offset, length);
	
	if (buffer == 0 || length <= 0 || offset < 0) {
		LOG_E("[DSA] Invalid parameters for glFlushMappedNamedBufferRange");
		return;
	}
	temporarilyBindBuffer(buffer);
	glFlushMappedBufferRange(GL_ARRAY_BUFFER, offset, length);
	CHECK_GL_ERROR;
	restoreTemporaryBufferBinding();
	
	LOG_D("[DSA] Flushed mapped range of buffer %u from offset %lld with length %lld", buffer, offset, length);
}

void glGetNamedBufferParameteriv(GLuint buffer, GLenum pname, GLint* params) {
	LOG()
	LOG_D("[DSA] glGetNamedBufferParameteriv, buffer: %u, pname: 0x%X, params: %p", buffer, pname, params);
	
	if (buffer == 0 || !params) {
		LOG_E("[DSA] Invalid parameters for glGetNamedBufferParameteriv");
		return;
	}
	temporarilyBindBuffer(buffer);
	glGetBufferParameteriv(GL_ARRAY_BUFFER, pname, params);
	CHECK_GL_ERROR;
	restoreTemporaryBufferBinding();
	
	LOG_D("[DSA] Retrieved buffer parameter 0x%X for buffer %u", pname, buffer);
}

void glGetNamedBufferParameteri64v(GLuint buffer, GLenum pname, GLint64* params) {
	LOG()
	LOG_D("[DSA] glGetNamedBufferParameteri64v, buffer: %u, pname: 0x%X, params: %p", buffer, pname, params);
	
	if (buffer == 0 || !params) {
		LOG_E("[DSA] Invalid parameters for glGetNamedBufferParameteri64v");
		return;
	}
	temporarilyBindBuffer(buffer);
	glGetBufferParameteri64v(GL_ARRAY_BUFFER, pname, params);
	CHECK_GL_ERROR;
	restoreTemporaryBufferBinding();
	
	LOG_D("[DSA] Retrieved 64-bit buffer parameter 0x%X for buffer %u", pname, buffer);
}

void glGetNamedBufferPointerv(GLuint buffer, GLenum pname, void** params) {
	LOG()
	LOG_D("[DSA] glGetNamedBufferPointerv, buffer: %u, pname: 0x%X, params: %p", buffer, pname, params);
	
	if (buffer == 0 || !params) {
		LOG_E("[DSA] Invalid parameters for glGetNamedBufferPointerv");
		return;
	}
	temporarilyBindBuffer(buffer);
	glGetBufferPointerv(GL_ARRAY_BUFFER, pname, params);
	CHECK_GL_ERROR;
	restoreTemporaryBufferBinding();
	
	LOG_D("[DSA] Retrieved buffer pointer parameter 0x%X for buffer %u", pname, buffer);
}

void glGetNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, void* data) {
	LOG()
	LOG_D("[DSA] glGetNamedBufferSubData, buffer: %u, offset: %lld, size: %lld, data: %p", buffer, offset, size, data);
	
	if (buffer == 0 || size <= 0 || offset < 0 || !data) {
		LOG_E("[DSA] Invalid parameters for glGetNamedBufferSubData");
		return;
	}
	temporarilyBindBuffer(buffer);
	glGetBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
	CHECK_GL_ERROR;
	restoreTemporaryBufferBinding();
	
	LOG_D("[DSA] Retrieved sub-data from buffer %u with size %lld at offset %lld", buffer, size, offset);
}

// framebuffer
static thread_local ankerl::unordered_dense::map<GLenum, std::vector<GLuint>> framebufferBindingStack;
void temporarilyBindFramebuffer(GLuint framebufferID, GLenum target = GL_DRAW_FRAMEBUFFER) {
	GLenum bindingQuery = GetBindingQuery(target);
	GLint prev = 0;
	glGetIntegerv(bindingQuery, &prev);
	if (prev == framebufferID) {
		framebufferBindingStack[target].push_back(-1);
		return;
	}
	framebufferBindingStack[target].push_back(static_cast<GLuint>(prev));
	LOG_D("[DSA] [TempBind] target=0x%X, prev=%u -> bind=%u", target, prev, framebufferID);
	CHECK_GL_ERROR;
	glBindFramebuffer(target, framebufferID);
	CHECK_GL_ERROR_NO_INIT;
}
void restoreTemporaryFramebufferBinding(GLenum target = GL_DRAW_FRAMEBUFFER) {
	auto it = framebufferBindingStack.find(target);
	if (it == framebufferBindingStack.end() || it->second.empty()) {
	LOG_D("[DSA] [Restore] no saved binding for target 0x%X", target);
		return;
	}
	GLuint toRestore = it->second.back();
	it->second.pop_back();
	if (toRestore == static_cast<GLuint>(-1)) {
		LOG_D("[DSA] [Restore] target=0x%X, no binding to restore", target);
		return;
	}
	LOG_D("[DSA] [Restore] target=0x%X, bind back to %u", target, toRestore);
	CHECK_GL_ERROR;
	glBindFramebuffer(target, toRestore);
	CHECK_GL_ERROR_NO_INIT;
	if (it->second.empty())
		framebufferBindingStack.erase(it);
}

void glCreateFramebuffers(GLsizei n, GLuint* framebuffers) {
	LOG()
	LOG_D("[DSA] glCreateFramebuffers, n: %d, framebuffers: %p", n, framebuffers);
	
	if (n <= 0 || !framebuffers) {
		LOG_E("[DSA] Invalid parameters for glCreateFramebuffers");
		return;
	}
	for (GLsizei i = 0; i < n; ++i) {
		GLuint fboID = 0;
		glGenFramebuffers(1, &fboID);
		if (fboID == 0) {
			LOG_E("[DSA] Failed to create framebuffer at index %d", i);
			continue;
		}
		temporarilyBindFramebuffer(fboID); // after binding, the framebuffer object should be created
		restoreTemporaryFramebufferBinding();
		framebuffers[i] = fboID;
	}
	CHECK_GL_ERROR;
	
	LOG_D("[DSA] Created %d framebuffers successfully", n);
}

void glNamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
	LOG()
	LOG_D("[DSA] glNamedFramebufferRenderbuffer, framebuffer: %u, attachment: 0x%X, renderbuffertarget: 0x%X, renderbuffer: %u", framebuffer, attachment, renderbuffertarget, renderbuffer);
	
	temporarilyBindFramebuffer(framebuffer);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, attachment, renderbuffertarget, renderbuffer);
	CHECK_GL_ERROR;
	restoreTemporaryFramebufferBinding();
	
	LOG_D("[DSA] Attached renderbuffer %u to framebuffer %u with attachment 0x%X", renderbuffer, framebuffer, attachment);
}

void glNamedFramebufferParameteri(GLuint framebuffer, GLenum pname, GLint param) {
	LOG()
	LOG_D("[DSA] glNamedFramebufferParameteri, framebuffer: %u, pname: 0x%X, param: %d", framebuffer, pname, param);
	
	temporarilyBindFramebuffer(framebuffer);
	glFramebufferParameteri(GL_DRAW_FRAMEBUFFER, pname, param);
	CHECK_GL_ERROR;
	restoreTemporaryFramebufferBinding();
	
	LOG_D("[DSA] Set framebuffer parameter 0x%X to %d for framebuffer %u", pname, param, framebuffer);
}

void glNamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level) {
	LOG()
	LOG_D("[DSA] glNamedFramebufferTexture, framebuffer: %u, attachment: 0x%X, texture: %u, level: %d", framebuffer, attachment, texture, level);
	
	temporarilyBindFramebuffer(framebuffer);
	glFramebufferTexture(GL_DRAW_FRAMEBUFFER, attachment, texture, level);
	LOG_D("[DSA] glFramebufferTexture called: attachment=0x%X, texture=%u, level=%d", attachment, texture, level);
	CHECK_GL_ERROR;
	restoreTemporaryFramebufferBinding(GL_DRAW_FRAMEBUFFER);
	
	LOG_D("[DSA] Attached texture %u to framebuffer %u with attachment 0x%X at level %d", texture, framebuffer, attachment, level);
}

void glNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer) {
	LOG()
	LOG_D("[DSA] glNamedFramebufferTextureLayer, framebuffer: %u, attachment: 0x%X, texture: %u, level: %d, layer: %d", framebuffer, attachment, texture, level, layer);
	
	temporarilyBindFramebuffer(framebuffer);
	glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, attachment, texture, level, layer);
	CHECK_GL_ERROR;
	restoreTemporaryFramebufferBinding();
	
	LOG_D("[DSA] Attached texture %u to framebuffer %u with attachment 0x%X at level %d and layer %d", texture, framebuffer, attachment, level, layer);
}

void glNamedFramebufferDrawBuffer(GLuint framebuffer, GLenum mode) {
	LOG()
	LOG_D("[DSA] glNamedFramebufferDrawBuffer, framebuffer: %u, mode: 0x%X", framebuffer, mode);
	
	temporarilyBindFramebuffer(framebuffer);
	glDrawBuffer(mode);
	CHECK_GL_ERROR;
	restoreTemporaryFramebufferBinding();
	
	LOG_D("[DSA] Set draw buffer mode 0x%X for framebuffer %u", mode, framebuffer);
}

void glNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n, const GLenum* bufs) {
	LOG()
	LOG_D("[DSA] glNamedFramebufferDrawBuffers, framebuffer: %u, n: %d, bufs: %p", framebuffer, n, bufs);
	
	if (n <= 0 || !bufs) {
		LOG_E("[DSA] Invalid parameters for glNamedFramebufferDrawBuffers");
		return;
	}
	temporarilyBindFramebuffer(framebuffer);
	glDrawBuffers(n, bufs);
	CHECK_GL_ERROR;
	restoreTemporaryFramebufferBinding();
	
	LOG_D("[DSA] Set %d draw buffers for framebuffer %u", n, framebuffer);
}

void glNamedFramebufferReadBuffer(GLuint framebuffer, GLenum mode) {
	LOG()
	LOG_D("[DSA] glNamedFramebufferReadBuffer, framebuffer: %u, mode: 0x%X", framebuffer, mode);
	
	temporarilyBindFramebuffer(framebuffer, GL_READ_FRAMEBUFFER);
	glReadBuffer(mode);
	CHECK_GL_ERROR;
	restoreTemporaryFramebufferBinding(GL_READ_FRAMEBUFFER);
	
	LOG_D("[DSA] Set read buffer mode 0x%X for framebuffer %u", mode, framebuffer);
}

void glInvalidateNamedFramebufferData(GLuint framebuffer, GLsizei numAttachments, const GLenum* attachments) {
	LOG()
	LOG_D("[DSA] glInvalidateNamedFramebufferData, framebuffer: %u, numAttachments: %d, attachments: %p", framebuffer, numAttachments, attachments);
	
	if (numAttachments <= 0 || !attachments) {
		LOG_E("[DSA] Invalid parameters for glInvalidateNamedFramebufferData");
		return;
	}
	temporarilyBindFramebuffer(framebuffer, GL_READ_FRAMEBUFFER);
	temporarilyBindFramebuffer(framebuffer, GL_DRAW_FRAMEBUFFER);
	glInvalidateFramebuffer(GL_FRAMEBUFFER, numAttachments, attachments);
	CHECK_GL_ERROR;
	restoreTemporaryFramebufferBinding(GL_READ_FRAMEBUFFER);
	restoreTemporaryFramebufferBinding(GL_DRAW_FRAMEBUFFER);
	
	LOG_D("[DSA] Invalidated framebuffer %u with %d attachments", framebuffer, numAttachments);
}

void glInvalidateNamedFramebufferSubData(GLuint framebuffer, GLsizei numAttachments, const GLenum* attachments, GLint x, GLint y, GLsizei width, GLsizei height) {
	LOG()
	LOG_D("[DSA] glInvalidateNamedFramebufferSubData, framebuffer: %u, numAttachments: %d, attachments: %p, x: %d, y: %d, width: %d, height: %d", framebuffer, numAttachments, attachments, x, y, width, height);
	
	if (numAttachments <= 0 || !attachments || width <= 0 || height <= 0) {
		LOG_E("[DSA] Invalid parameters for glInvalidateNamedFramebufferSubData");
		return;
	}
	temporarilyBindFramebuffer(framebuffer, GL_READ_FRAMEBUFFER);
	temporarilyBindFramebuffer(framebuffer, GL_DRAW_FRAMEBUFFER);
	glInvalidateSubFramebuffer(GL_FRAMEBUFFER, numAttachments, attachments, x, y, width, height);
	CHECK_GL_ERROR;
	restoreTemporaryFramebufferBinding(GL_READ_FRAMEBUFFER);
	restoreTemporaryFramebufferBinding(GL_DRAW_FRAMEBUFFER);
	
	LOG_D("[DSA] Invalidated sub-data of framebuffer %u with %d attachments at (%d, %d) with size (%d, %d)", framebuffer, numAttachments, x, y, width, height);
}

void glClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint* value) {
	LOG()
	LOG_D("[DSA] glClearNamedFramebufferiv, framebuffer: %u, buffer: 0x%X, drawbuffer: %d, value: %p", framebuffer, buffer, drawbuffer, value);
	
	if (!value) {
		LOG_E("[DSA] Invalid parameters for glClearNamedFramebufferiv");
		return;
	}
	temporarilyBindFramebuffer(framebuffer);
	glClearBufferiv(buffer, drawbuffer, value);
	CHECK_GL_ERROR;
	restoreTemporaryFramebufferBinding();
	
	LOG_D("[DSA] Cleared framebuffer %u with buffer 0x%X at drawbuffer %d", framebuffer, buffer, drawbuffer);
}

void glClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint* value) {
	LOG()
	LOG_D("[DSA] glClearNamedFramebufferuiv, framebuffer: %u, buffer: 0x%X, drawbuffer: %d, value: %p", framebuffer, buffer, drawbuffer, value);
	
	if (!value) {
		LOG_E("[DSA] Invalid parameters for glClearNamedFramebufferuiv");
		return;
	}
	temporarilyBindFramebuffer(framebuffer);
	glClearBufferuiv(buffer, drawbuffer, value);
	CHECK_GL_ERROR;
	restoreTemporaryFramebufferBinding();
	
	LOG_D("[DSA] Cleared framebuffer %u with unsigned int buffer 0x%X at drawbuffer %d", framebuffer, buffer, drawbuffer);
}

void glClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat* value) {
	LOG()
	LOG_D("[DSA] glClearNamedFramebufferfv, framebuffer: %u, buffer: 0x%X, drawbuffer: %d, value: %p", framebuffer, buffer, drawbuffer, value);
	
	if (!value) {
		LOG_E("[DSA] Invalid parameters for glClearNamedFramebufferfv");
		return;
	}
	temporarilyBindFramebuffer(framebuffer);
	glClearBufferfv(buffer, drawbuffer, value);
	CHECK_GL_ERROR;
	restoreTemporaryFramebufferBinding();
	
	LOG_D("[DSA] Cleared framebuffer %u with float buffer 0x%X at drawbuffer %d", framebuffer, buffer, drawbuffer);
}

void glClearNamedFramebufferfi(GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {
	LOG()
	LOG_D("[DSA] glClearNamedFramebufferfi, framebuffer: %u, buffer: 0x%X, drawbuffer: %d, depth: %f, stencil: %d", framebuffer, buffer, drawbuffer, depth, stencil);
	
	temporarilyBindFramebuffer(framebuffer);
	glClearBufferfi(buffer, drawbuffer, depth, stencil);
	CHECK_GL_ERROR;
	restoreTemporaryFramebufferBinding();
	
	LOG_D("[DSA] Cleared framebuffer %u with float and int buffer 0x%X at drawbuffer %d", framebuffer, buffer, drawbuffer);
}

void glBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
	GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
	LOG()
	LOG_D("[DSA] glBlitNamedFramebuffer, readFramebuffer: %u, drawFramebuffer: %u, src: (%d, %d) to (%d, %d), dst: (%d, %d) to (%d, %d), mask: 0x%X, filter: 0x%X",
		readFramebuffer, drawFramebuffer, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
	
	temporarilyBindFramebuffer(readFramebuffer, GL_READ_FRAMEBUFFER);
	temporarilyBindFramebuffer(drawFramebuffer, GL_DRAW_FRAMEBUFFER);
	glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1,
		dstX0, dstY0, dstX1, dstY1,
		mask, filter);
	CHECK_GL_ERROR;
	restoreTemporaryFramebufferBinding(GL_READ_FRAMEBUFFER);
	restoreTemporaryFramebufferBinding(GL_DRAW_FRAMEBUFFER);
	
	LOG_D("[DSA] Blitted from framebuffer %u to framebuffer %u", readFramebuffer, drawFramebuffer);
}

GLenum glCheckNamedFramebufferStatus(GLuint framebuffer, GLenum target) {
	LOG()
	LOG_D("[DSA] glCheckNamedFramebufferStatus, framebuffer: %u, target: 0x%X", framebuffer, target);
	
	temporarilyBindFramebuffer(framebuffer, target);
	GLenum status = glCheckFramebufferStatus(target);
	CHECK_GL_ERROR;
	restoreTemporaryFramebufferBinding(target);
	
	LOG_D("[DSA] Checked framebuffer %u status: 0x%X", framebuffer, status);
	return status;
}

void glGetNamedFramebufferParameteriv(GLuint framebuffer, GLenum pname, GLint* param) {
	LOG()
	LOG_D("[DSA] glGetNamedFramebufferParameteriv, framebuffer: %u, pname: 0x%X, param: %p", framebuffer, pname, param);
	
	if (!param) {
		LOG_E("[DSA] Invalid parameters for glGetNamedFramebufferParameteriv");
		return;
	}
	temporarilyBindFramebuffer(framebuffer);
	glGetFramebufferParameteriv(GL_DRAW_FRAMEBUFFER, pname, param);
	CHECK_GL_ERROR;
	restoreTemporaryFramebufferBinding();
	
	LOG_D("[DSA] Retrieved framebuffer parameter 0x%X for framebuffer %u", pname, framebuffer);
}

void glGetNamedFramebufferAttachmentParameteriv(GLuint framebuffer, GLenum attachment, GLenum pname, GLint* params) {
	LOG()
	LOG_D("[DSA] glGetNamedFramebufferAttachmentParameteriv, framebuffer: %u, attachment: 0x%X, pname: 0x%X, params: %p", framebuffer, attachment, pname, params);
	
	if (!params) {
		LOG_E("[DSA] Invalid parameters for glGetNamedFramebufferAttachmentParameteriv");
		return;
	}
	temporarilyBindFramebuffer(framebuffer);
	glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, attachment, pname, params);
	CHECK_GL_ERROR;
	restoreTemporaryFramebufferBinding();
	
	LOG_D("[DSA] Retrieved framebuffer attachment parameter 0x%X for framebuffer %u and attachment 0x%X", pname, framebuffer, attachment);
}

// renderbuffer
static thread_local ankerl::unordered_dense::map<GLenum, std::vector<GLuint>> renderbufferBindingStack;
void temporarilyBindRenderbuffer(GLuint renderbufferID) {
	GLenum bindingQuery = GetBindingQuery(GL_RENDERBUFFER);
	GLint prev = 0;
	glGetIntegerv(bindingQuery, &prev);
	if (prev == renderbufferID) {
		renderbufferBindingStack[GL_RENDERBUFFER].push_back(-1);
		return;
	}
	renderbufferBindingStack[GL_RENDERBUFFER].push_back(static_cast<GLuint>(prev));
	LOG_D("[DSA] [TempBind] prev=%u -> bind=%u", prev, renderbufferID);
	CHECK_GL_ERROR;
	glBindRenderbuffer(GL_RENDERBUFFER, renderbufferID);
	CHECK_GL_ERROR_NO_INIT;
}
void restoreTemporaryRenderbufferBinding() {
	auto it = renderbufferBindingStack.find(GL_RENDERBUFFER);
	if (it == renderbufferBindingStack.end() || it->second.empty()) {
		LOG_D("[DSA] [Restore] no saved binding for GL_RENDERBUFFER");
		return;
	}
	GLuint toRestore = it->second.back();
	it->second.pop_back();
	if (toRestore == static_cast<GLuint>(-1)) {
		LOG_D("[DSA] [Restore] no binding to restore for GL_RENDERBUFFER");
		return;
	}
	LOG_D("[DSA] [Restore] bind back to %u", toRestore);
	CHECK_GL_ERROR;
	glBindRenderbuffer(GL_RENDERBUFFER, toRestore);
	CHECK_GL_ERROR_NO_INIT;
	if (it->second.empty())
		renderbufferBindingStack.erase(it);
}

void glCreateRenderbuffers(GLsizei n, GLuint* renderbuffers) {
	LOG()
	LOG_D("[DSA] glCreateRenderbuffers, n: %d, renderbuffers: %p", n, renderbuffers);
	
	if (n <= 0 || !renderbuffers) {
		LOG_E("[DSA] Invalid parameters for glCreateRenderbuffers");
		return;
	}
	for (GLsizei i = 0; i < n; ++i) {
		GLuint rboID = 0;
		glGenRenderbuffers(1, &rboID);
		if (rboID == 0) {
			LOG_E("[DSA] Failed to create renderbuffer at index %d", i);
			continue;
		}
		temporarilyBindRenderbuffer(rboID); // after binding, the renderbuffer object should be created
		restoreTemporaryRenderbufferBinding();
		renderbuffers[i] = rboID;
	}
	CHECK_GL_ERROR;
	
	LOG_D("[DSA] Created %d renderbuffers successfully", n);
}

void glNamedRenderbufferStorage(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height) {
	LOG()
	LOG_D("[DSA] glNamedRenderbufferStorage, renderbuffer: %u, internalformat: 0x%X, width: %d, height: %d", renderbuffer, internalformat, width, height);
	
	if (renderbuffer == 0 || width <= 0 || height <= 0) {
		LOG_E("[DSA] Invalid parameters for glNamedRenderbufferStorage");
		return;
	}
	temporarilyBindRenderbuffer(renderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, internalformat, width, height);
	CHECK_GL_ERROR;
	restoreTemporaryRenderbufferBinding();
	
	LOG_D("[DSA] Set storage for renderbuffer %u with internal format 0x%X and size (%d, %d)", renderbuffer, internalformat, width, height);
}

void glNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
	LOG()
	LOG_D("[DSA] glNamedRenderbufferStorageMultisample, renderbuffer: %u, samples: %d, internalformat: 0x%X, width: %d, height: %d", renderbuffer, samples, internalformat, width, height);
	
	if (renderbuffer == 0 || samples <= 0 || width <= 0 || height <= 0) {
		LOG_E("[DSA] Invalid parameters for glNamedRenderbufferStorageMultisample");
		return;
	}
	temporarilyBindRenderbuffer(renderbuffer);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, internalformat, width, height);
	CHECK_GL_ERROR;
	restoreTemporaryRenderbufferBinding();
	
	LOG_D("[DSA] Set multisample storage for renderbuffer %u with internal format 0x%X and size (%d, %d)", renderbuffer, internalformat, width, height);
}

void glGetNamedRenderbufferParameteriv(GLuint renderbuffer, GLenum pname, GLint* params) {
	LOG()
	LOG_D("[DSA] glGetNamedRenderbufferParameteriv, renderbuffer: %u, pname: 0x%X, params: %p", renderbuffer, pname, params);
	
	if (renderbuffer == 0 || !params) {
		LOG_E("[DSA] Invalid parameters for glGetNamedRenderbufferParameteriv");
		return;
	}
	temporarilyBindRenderbuffer(renderbuffer);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, pname, params);
	CHECK_GL_ERROR;
	restoreTemporaryRenderbufferBinding();
	
	LOG_D("[DSA] Retrieved renderbuffer parameter 0x%X for renderbuffer %u", pname, renderbuffer);
}

// texture
static thread_local ankerl::unordered_dense::map<GLenum, std::vector<GLuint>> textureBindingStack;

GLenum GetTexTarget(GLuint texture) {
	return ConvertTextureTargetToGLEnum(mgGetTexObjectByID(texture)->target);
}

void temporarilyBindTexture(GLuint textureID, GLenum possibleTarget = 0) {
	GLenum target = possibleTarget ? possibleTarget : GetTexTarget(textureID);
	GLenum bindingQuery = GetBindingQuery(target, true);
	GLint prev = 0;
	glGetIntegerv(bindingQuery, &prev);
	if (prev == static_cast<GLint>(textureID)) {
		textureBindingStack[target].push_back(-1);
		return;
	}
	textureBindingStack[target].push_back(static_cast<GLuint>(prev));
	LOG_D("[DSA] [TempBind] target=0x%X, prev=%u -> bind=%u", target, prev, textureID);
	CHECK_GL_ERROR;
	glBindTexture(target, textureID);
	CHECK_GL_ERROR_NO_INIT;
}

void restoreTemporaryTextureBinding(GLuint textureID, GLenum possibleTarget = 0) {
	GLenum target = possibleTarget ? possibleTarget : GetTexTarget(textureID);
	auto stackIt = textureBindingStack.find(target);
	if (stackIt == textureBindingStack.end() || stackIt->second.empty()) {
		LOG_D("[DSA] [Restore] no saved binding for target 0x%X", target);
		return;
	}

	GLuint toRestore = stackIt->second.back();
	stackIt->second.pop_back();
	if (toRestore == static_cast<GLuint>(-1)) {
		LOG_D("[DSA] [Restore] target=0x%X, no binding to restore", target);
		return;
	}
	LOG_D("[DSA] [Restore] target=0x%X, bind back to %u", target, toRestore);
	CHECK_GL_ERROR;
	glBindTexture(target, toRestore);
	CHECK_GL_ERROR_NO_INIT;

	if (stackIt->second.empty()) {
		textureBindingStack.erase(stackIt);
	}
}

void glCreateTextures(GLenum target, GLsizei n, GLuint* textures) {
	LOG()
		LOG_D("[DSA] glCreateTextures, target: 0x%X, n: %d, textures: %p", target, n, textures);

	if (n <= 0 || !textures) {
		LOG_E("[DSA] Invalid parameters for glCreateTextures");
		return;
	}

	for (GLsizei i = 0; i < n; ++i) {
		GLuint texID = 0;
		glGenTextures(1, &texID);
		if (texID == 0) {
			LOG_E("[DSA] Failed to create texture at index %d", i);
			continue;
		}
		temporarilyBindTexture(texID, target);
		restoreTemporaryTextureBinding(texID, target);
		textures[i] = texID;
	}
	CHECK_GL_ERROR;

	LOG_D("[DSA] Created %d textures successfully", n);
}

void glTextureBuffer(GLuint texture, GLenum internalformat, GLuint buffer) {
	LOG()
		LOG_D("[DSA] glTextureBuffer, texture: %u, internalformat: 0x%X, buffer: %u", texture, internalformat, buffer);

	if (buffer == 0) {
		LOG_E("[DSA] Invalid parameters for glTextureBuffer");
		return;
	}

	temporarilyBindTexture(texture);
	glTexBuffer(GL_TEXTURE_BUFFER, internalformat, buffer);
	CHECK_GL_ERROR;
	restoreTemporaryTextureBinding(texture);

	LOG_D("[DSA] Set buffer for texture %u with internal format 0x%X", texture, internalformat);
}

void glTextureBufferRange(GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) {
	LOG()
		LOG_D("[DSA] glTextureBufferRange, texture: %u, internalformat: 0x%X, buffer: %u, offset: %lld, size: %lld",
			texture, internalformat, buffer, offset, size);

	if (buffer == 0 || size <= 0 || offset < 0) {
		LOG_E("[DSA] Invalid parameters for glTextureBufferRange");
		return;
	}

	temporarilyBindTexture(texture);
	glTexBufferRange(GL_TEXTURE_BUFFER, internalformat, buffer, offset, size);
	CHECK_GL_ERROR;
	restoreTemporaryTextureBinding(texture);

	LOG_D("[DSA] Set buffer range for texture %u with internal format 0x%X and size %lld at offset %lld",
		texture, internalformat, size, offset);
}

#define TEXTURE_OP_FUNC_BEGIN(func_name) \
    LOG() \
    LOG_D(#func_name ", texture: %u", texture); \
    GLenum target = GetTexTarget(texture); \
    temporarilyBindTexture(texture);

#define TEXTURE_OP_FUNC_END \
    CHECK_GL_ERROR; \
    restoreTemporaryTextureBinding(texture);

void glTextureStorage1D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width) {
	TEXTURE_OP_FUNC_BEGIN(glTextureStorage1D)
		glTexStorage1D(target, levels, internalformat, width);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Set 1D storage for texture %u with internal format 0x%X and width %d", texture, internalformat, width);
}

void glTextureStorage2D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
	TEXTURE_OP_FUNC_BEGIN(glTextureStorage2D)
		glTexStorage2D(target, levels, internalformat, width, height);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Set 2D storage for texture %u with internal format 0x%X and size (%d, %d)",
			texture, internalformat, width, height);
}

void glTextureStorage3D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {
	TEXTURE_OP_FUNC_BEGIN(glTextureStorage3D)
		glTexStorage3D(target, levels, internalformat, width, height, depth);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Set 3D storage for texture %u with internal format 0x%X and size (%d, %d, %d)",
			texture, internalformat, width, height, depth);
}

void glTextureStorage2DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {
	TEXTURE_OP_FUNC_BEGIN(glTextureStorage2DMultisample)
		glTexStorage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Set 2D multisample storage for texture %u with internal format 0x%X and size (%d, %d)",
			texture, internalformat, width, height);
}

void glTextureStorage3DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
	TEXTURE_OP_FUNC_BEGIN(glTextureStorage3DMultisample)
		glTexStorage3DMultisample(target, samples, internalformat, width, height, depth, fixedsamplelocations);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Set 3D multisample storage for texture %u with internal format 0x%X and size (%d, %d, %d)",
			texture, internalformat, width, height, depth);
}

void glTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels) {
	TEXTURE_OP_FUNC_BEGIN(glTextureSubImage1D)
		glTexSubImage1D(target, level, xoffset, width, format, type, pixels);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Updated 1D sub-image of texture %u at level %d with size %d at offset %d",
			texture, level, width, xoffset);
}

void glTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) {
	TEXTURE_OP_FUNC_BEGIN(glTextureSubImage2D)
		glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Updated 2D sub-image of texture %u at level %d with size (%d, %d) at offset (%d, %d)",
			texture, level, width, height, xoffset, yoffset);
}

void glTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) {
	TEXTURE_OP_FUNC_BEGIN(glTextureSubImage3D)
		glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Updated 3D sub-image of texture %u at level %d with size (%d, %d, %d) at offset (%d, %d, %d)",
			texture, level, width, height, depth, xoffset, yoffset, zoffset);
}

void glCompressedTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void* data) {
	TEXTURE_OP_FUNC_BEGIN(glCompressedTextureSubImage1D)
		glCompressedTexSubImage1D(target, level, xoffset, width, format, imageSize, data);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Updated compressed 1D sub-image of texture %u at level %d with size %d at offset %d",
			texture, level, width, xoffset);
}

void glCompressedTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) {
	TEXTURE_OP_FUNC_BEGIN(glCompressedTextureSubImage2D)
		glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Updated compressed 2D sub-image of texture %u at level %d with size (%d, %d) at offset (%d, %d)",
			texture, level, width, height, xoffset, yoffset);
}

void glCompressedTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data) {
	TEXTURE_OP_FUNC_BEGIN(glCompressedTextureSubImage3D)
		glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Updated compressed 3D sub-image of texture %u at level %d with size (%d, %d, %d) at offset (%d, %d, %d)",
			texture, level, width, height, depth, xoffset, yoffset, zoffset);
}

void glCopyTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {
	TEXTURE_OP_FUNC_BEGIN(glCopyTextureSubImage1D)
		glCopyTexSubImage1D(target, level, xoffset, x, y, width);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Copied 1D sub-image to texture %u at level %d with size %d at offset %d",
			texture, level, width, xoffset);
}

void glCopyTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
	TEXTURE_OP_FUNC_BEGIN(glCopyTextureSubImage2D)
		glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Copied 2D sub-image to texture %u at level %d with size (%d, %d) at offset (%d, %d)",
			texture, level, width, height, xoffset, yoffset);
}

void glCopyTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
	TEXTURE_OP_FUNC_BEGIN(glCopyTextureSubImage3D)
		glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Copied 3D sub-image to texture %u at level %d with size (%d, %d) at offset (%d, %d)",
			texture, level, width, height, xoffset, yoffset);
}

void glTextureParameterf(GLuint texture, GLenum pname, GLfloat param) {
	TEXTURE_OP_FUNC_BEGIN(glTextureParameterf)
		glTexParameterf(target, pname, param);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Set float parameter 0x%X for texture %u to %f", pname, texture, param);
}

void glTextureParameterfv(GLuint texture, GLenum pname, const GLfloat* param) {
	TEXTURE_OP_FUNC_BEGIN(glTextureParameterfv)
		glTexParameterfv(target, pname, param);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Set float vector parameter 0x%X for texture %u", pname, texture);
}

void glTextureParameteri(GLuint texture, GLenum pname, GLint param) {
	TEXTURE_OP_FUNC_BEGIN(glTextureParameteri)
		glTexParameteri(target, pname, param);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Set integer parameter 0x%X for texture %u to %d", pname, texture, param);
}

void glTextureParameterIiv(GLuint texture, GLenum pname, const GLint* params) {
	TEXTURE_OP_FUNC_BEGIN(glTextureParameterIiv)
		glTexParameterIiv(target, pname, params);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Set integer vector parameter 0x%X for texture %u", pname, texture);
}

void glTextureParameterIuiv(GLuint texture, GLenum pname, const GLuint* params) {
	TEXTURE_OP_FUNC_BEGIN(glTextureParameterIuiv)
		glTexParameterIuiv(target, pname, params);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Set unsigned integer vector parameter 0x%X for texture %u", pname, texture);
}

void glTextureParameteriv(GLuint texture, GLenum pname, const GLint* param) {
	TEXTURE_OP_FUNC_BEGIN(glTextureParameteriv)
		glTexParameteriv(target, pname, param);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Set integer vector parameter 0x%X for texture %u", pname, texture);
}

void glGenerateTextureMipmap(GLuint texture) {
	TEXTURE_OP_FUNC_BEGIN(glGenerateTextureMipmap)
		glGenerateMipmap(target);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Generated mipmap for texture %u", texture);
}

void glBindTextureUnit(GLuint unit, GLuint texture) {
	LOG()
		LOG_D("[DSA] glBindTextureUnit, unit: %u, texture: %u", unit, texture);

	if (unit >= GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS) {
		LOG_E("[DSA] Invalid parameters for glBindTextureUnit");
		return;
	}
	GLint prevUnit = 0;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &prevUnit);
	GLenum target = GetTexTarget(texture);
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(target, texture);
	glActiveTexture(prevUnit);
	LOG_D("[DSA] Bound texture %u to texture unit %u", texture, unit);
}

void glGetTextureImage(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels) {
	TEXTURE_OP_FUNC_BEGIN(glGetTextureImage)
		glGetTexImage(target, level, format, type, pixels);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Retrieved texture image from texture %u at level %d", texture, level);
}

void glGetCompressedTextureImage(GLuint texture, GLint level, GLsizei bufSize, void* pixels) {
	TEXTURE_OP_FUNC_BEGIN(glGetCompressedTextureImage)
		glGetCompressedTexImage(target, level, pixels);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Retrieved compressed texture image from texture %u at level %d", texture, level);
}

void glGetTextureLevelParameterfv(GLuint texture, GLint level, GLenum pname, GLfloat* params) {
	TEXTURE_OP_FUNC_BEGIN(glGetTextureLevelParameterfv)
		glGetTexLevelParameterfv(target, level, pname, params);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Retrieved texture level parameter 0x%X for texture %u at level %d", pname, texture, level);
}

void glGetTextureLevelParameteriv(GLuint texture, GLint level, GLenum pname, GLint* params) {
	TEXTURE_OP_FUNC_BEGIN(glGetTextureLevelParameteriv)
		glGetTexLevelParameteriv(target, level, pname, params);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Retrieved texture level parameter 0x%X for texture %u at level %d", pname, texture, level);
}

void glGetTextureParameterfv(GLuint texture, GLenum pname, GLfloat* params) {
	TEXTURE_OP_FUNC_BEGIN(glGetTextureParameterfv)
		glGetTexParameterfv(target, pname, params);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Retrieved texture parameter 0x%X for texture %u", pname, texture);
}

void glGetTextureParameterIiv(GLuint texture, GLenum pname, GLint* params) {
	TEXTURE_OP_FUNC_BEGIN(glGetTextureParameterIiv)
		glGetTexParameterIiv(target, pname, params);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Retrieved integer texture parameter 0x%X for texture %u", pname, texture);
}

void glGetTextureParameterIuiv(GLuint texture, GLenum pname, GLuint* params) {
	TEXTURE_OP_FUNC_BEGIN(glGetTextureParameterIuiv)
		glGetTexParameterIuiv(target, pname, params);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Retrieved unsigned integer texture parameter 0x%X for texture %u", pname, texture);
}

void glGetTextureParameteriv(GLuint texture, GLenum pname, GLint* params) {
	TEXTURE_OP_FUNC_BEGIN(glGetTextureParameteriv)
		glGetTexParameteriv(target, pname, params);
	TEXTURE_OP_FUNC_END
		LOG_D("[DSA] Retrieved integer texture parameter 0x%X for texture %u", pname, texture);
}

// vertex array
static thread_local GLint prevVAO = -1;
void temporarilyBindVertexArray(GLint vaoID) {
	if (prevVAO == vaoID) {
		prevVAO = -1;
		return;
	}
	LOG_D("[DSA] [TempBind] VAO: %u -> bind=%u", prevVAO, vaoID);
	CHECK_GL_ERROR;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
	glBindVertexArray(vaoID);
	CHECK_GL_ERROR_NO_INIT;
}
void restoreTemporaryVertexArrayBinding() {
	if (prevVAO == -1) {
		return;
	}
	LOG_D("[DSA] [Restore] VAO: bind back to %u", prevVAO);
	CHECK_GL_ERROR;
	glBindVertexArray(prevVAO);
	CHECK_GL_ERROR_NO_INIT;
	prevVAO = -1;
}

void glCreateVertexArrays(GLsizei n, GLuint* arrays) {
	LOG()
	LOG_D("[DSA] glCreateVertexArrays, n: %d, arrays: %p", n, arrays);
	
	if (n <= 0 || !arrays) {
		LOG_E("[DSA] Invalid parameters for glCreateVertexArrays");
		return;
	}
	for (GLsizei i = 0; i < n; ++i) {
		GLuint vaoID = 0;
		glGenVertexArrays(1, &vaoID);
		if (vaoID == 0) {
			LOG_E("[DSA] Failed to create vertex array at index %d", i);
			continue;
		}
		temporarilyBindVertexArray(vaoID); // after binding, the vertex array object should be created
		restoreTemporaryVertexArrayBinding();
		arrays[i] = vaoID;
	}
	CHECK_GL_ERROR;
	
	LOG_D("[DSA] Created %d vertex arrays successfully", n);
}

void glDisableVertexArrayAttrib(GLuint vaobj, GLuint index) {
	LOG()
	LOG_D("[DSA] glDisableVertexArrayAttrib, vaobj: %u, index: %u", vaobj, index);
	
	if (vaobj == 0 || index >= GL_MAX_VERTEX_ATTRIBS) {
		LOG_E("[DSA] Invalid parameters for glDisableVertexArrayAttrib");
		return;
	}
	temporarilyBindVertexArray(vaobj);
	glDisableVertexAttribArray(index);
	CHECK_GL_ERROR;
	restoreTemporaryVertexArrayBinding();
	
	LOG_D("[DSA] Disabled vertex array attribute %u for vertex array object %u", index, vaobj);
}

void glEnableVertexArrayAttrib(GLuint vaobj, GLuint index) {
	LOG()
	LOG_D("[DSA] glEnableVertexArrayAttrib, vaobj: %u, index: %u", vaobj, index);
	
	if (vaobj == 0 || index >= GL_MAX_VERTEX_ATTRIBS) {
		LOG_E("[DSA] Invalid parameters for glEnableVertexArrayAttrib");
		return;
	}
	temporarilyBindVertexArray(vaobj);
	glEnableVertexAttribArray(index);
	CHECK_GL_ERROR;
	restoreTemporaryVertexArrayBinding();
	
	LOG_D("[DSA] Enabled vertex array attribute %u for vertex array object %u", index, vaobj);
}

void glVertexArrayElementBuffer(GLuint vaobj, GLuint buffer) {
	LOG()
	LOG_D("[DSA] glVertexArrayElementBuffer, vaobj: %u, buffer: %u", vaobj, buffer);
	
	if (vaobj == 0 || buffer == 0) {
		LOG_E("[DSA] Invalid parameters for glVertexArrayElementBuffer");
		return;
	}
	temporarilyBindVertexArray(vaobj);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
	CHECK_GL_ERROR;
	restoreTemporaryVertexArrayBinding();
	
	LOG_D("[DSA] Bound element buffer %u to vertex array object %u", buffer, vaobj);
}

void glVertexArrayVertexBuffer(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {
	LOG()
	LOG_D("[DSA] glVertexArrayVertexBuffer, vaobj: %u, bindingindex: %u, buffer: %u, offset: %lld, stride: %d", vaobj, bindingindex, buffer, offset, stride);
	
	if (vaobj == 0 || bindingindex >= GL_MAX_VERTEX_ATTRIB_BINDINGS || buffer == 0 || stride < 0 || offset < 0) {
		LOG_E("[DSA] Invalid parameters for glVertexArrayVertexBuffer");
		return;
	}
	temporarilyBindVertexArray(vaobj);
	glBindVertexBuffer(bindingindex, buffer, offset, stride);
	CHECK_GL_ERROR;
	restoreTemporaryVertexArrayBinding();
	
	LOG_D("[DSA] Bound vertex buffer %u to binding index %u for vertex array object %u with offset %lld and stride %d", buffer, bindingindex, vaobj, offset, stride);
}

void glVertexArrayVertexBuffers(GLuint vaobj, GLuint first, GLsizei count, const GLuint* buffers, const GLintptr* offsets, const GLsizei* strides) {
	LOG()
	LOG_D("[DSA] glVertexArrayVertexBuffers, vaobj: %u, first: %u, count: %d, buffers: %p, offsets: %p, strides: %p", vaobj, first, count, buffers, offsets, strides);
	
	if (vaobj == 0 || first >= GL_MAX_VERTEX_ATTRIB_BINDINGS || count <= 0 || !buffers || !offsets || !strides) {
		LOG_E("[DSA] Invalid parameters for glVertexArrayVertexBuffers");
		return;
	}
	temporarilyBindVertexArray(vaobj);
	glBindVertexBuffers(first, count, buffers, offsets, strides);
	CHECK_GL_ERROR;
	restoreTemporaryVertexArrayBinding();
	
	LOG_D("[DSA] Bound vertex buffers starting from index %u for vertex array object %u", first, vaobj);
}

void glVertexArrayAttribFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {
	LOG()
	LOG_D("[DSA] glVertexArrayAttribFormat, vaobj: %u, attribindex: %u, size: %d, type: 0x%X, normalized: %d, relativeoffset: %u", vaobj, attribindex, size, type, normalized, relativeoffset);
	
	if (vaobj == 0 || attribindex >= GL_MAX_VERTEX_ATTRIBS || size <= 0 || (type != GL_FLOAT && type != GL_INT && type != GL_UNSIGNED_INT)) {
		LOG_E("[DSA] Invalid parameters for glVertexArrayAttribFormat");
		return;
	}
	temporarilyBindVertexArray(vaobj);
	glVertexAttribFormat(attribindex, size, type, normalized, relativeoffset);
	CHECK_GL_ERROR;
	restoreTemporaryVertexArrayBinding();
	
	LOG_D("[DSA] Set vertex array attribute format for index %u in vertex array object %u", attribindex, vaobj);
}

void glVertexArrayAttribIFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
	LOG()
	LOG_D("[DSA] glVertexArrayAttribIFormat, vaobj: %u, attribindex: %u, size: %d, type: 0x%X, relativeoffset: %u", vaobj, attribindex, size, type, relativeoffset);
	
	if (vaobj == 0 || attribindex >= GL_MAX_VERTEX_ATTRIBS || size <= 0 || (type != GL_INT && type != GL_UNSIGNED_INT)) {
		LOG_E("[DSA] Invalid parameters for glVertexArrayAttribIFormat");
		return;
	}
	temporarilyBindVertexArray(vaobj);
	glVertexAttribIFormat(attribindex, size, type, relativeoffset);
	CHECK_GL_ERROR;
	restoreTemporaryVertexArrayBinding();
	
	LOG_D("[DSA] Set integer vertex array attribute format for index %u in vertex array object %u", attribindex, vaobj);
}

void glVertexArrayAttribLFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
	LOG()
	LOG_D("[DSA] glVertexArrayAttribLFormat, vaobj: %u, attribindex: %u, size: %d, type: 0x%X, relativeoffset: %u", vaobj, attribindex, size, type, relativeoffset);
	
	if (vaobj == 0 || attribindex >= GL_MAX_VERTEX_ATTRIBS || size <= 0 || type != GL_DOUBLE) {
		LOG_E("[DSA] Invalid parameters for glVertexArrayAttribLFormat");
		return;
	}
	temporarilyBindVertexArray(vaobj);
	glVertexAttribLFormat(attribindex, size, type, relativeoffset);
	CHECK_GL_ERROR;
	restoreTemporaryVertexArrayBinding();
	
	LOG_D("[DSA] Set double vertex array attribute format for index %u in vertex array object %u", attribindex, vaobj);
}

void glVertexArrayAttribBinding(GLuint vaobj, GLuint attribindex, GLuint bindingindex) {
	LOG()
	LOG_D("[DSA] glVertexArrayAttribBinding, vaobj: %u, attribindex: %u, bindingindex: %u", vaobj, attribindex, bindingindex);
	
	if (vaobj == 0 || attribindex >= GL_MAX_VERTEX_ATTRIBS || bindingindex >= GL_MAX_VERTEX_ATTRIB_BINDINGS) {
		LOG_E("[DSA] Invalid parameters for glVertexArrayAttribBinding");
		return;
	}
	temporarilyBindVertexArray(vaobj);
	glVertexAttribBinding(attribindex, bindingindex);
	CHECK_GL_ERROR;
	restoreTemporaryVertexArrayBinding();
	
	LOG_D("[DSA] Set vertex array attribute binding for index %u in vertex array object %u to binding index %u", attribindex, vaobj, bindingindex);
}

void glVertexArrayBindingDivisor(GLuint vaobj, GLuint bindingindex, GLuint divisor) {
	LOG()
	LOG_D("[DSA] glVertexArrayBindingDivisor, vaobj: %u, bindingindex: %u, divisor: %u", vaobj, bindingindex, divisor);
	
	if (vaobj == 0 || bindingindex >= GL_MAX_VERTEX_ATTRIB_BINDINGS) {
		LOG_E("[DSA] Invalid parameters for glVertexArrayBindingDivisor");
		return;
	}
	temporarilyBindVertexArray(vaobj);
	glVertexBindingDivisor(bindingindex, divisor);
	CHECK_GL_ERROR;
	restoreTemporaryVertexArrayBinding();
	
	LOG_D("[DSA] Set vertex array binding divisor for binding index %u in vertex array object %u to %u", bindingindex, vaobj, divisor);
}

void glGetVertexArrayiv(GLuint vaobj, GLenum pname, GLint* param) {
	LOG()
		LOG_D("[DSA] glGetVertexArrayiv, vaobj: %u, pname: 0x%X, param: %p", vaobj, pname, param);

	if (vaobj == 0 || !param) {
		LOG_E("[DSA] Invalid parameters for glGetVertexArrayiv");
		return;
	}

	if (pname != GL_ELEMENT_ARRAY_BUFFER_BINDING) {
		LOG_E("[DSA] Invalid pname 0x%X for glGetVertexArrayiv. Only GL_ELEMENT_ARRAY_BUFFER_BINDING is allowed.", pname);
		return;
	}

	temporarilyBindVertexArray(vaobj);
	glGetIntegerv(pname, param);
	CHECK_GL_ERROR;
	restoreTemporaryVertexArrayBinding();

	LOG_D("[DSA] Retrieved vertex array parameter 0x%X for vertex array object %u, value: %d", pname, vaobj, *param);
}

void glGetVertexArrayIndexediv(GLuint vaobj, GLuint index, GLenum pname, GLint* param) {
	LOG()
		LOG_D("[DSA] glGetVertexArrayIndexediv, vaobj: %u, index: %u, pname: 0x%X, param: %p", vaobj, index, pname, param);

	if (vaobj == 0 || index >= GL_MAX_VERTEX_ATTRIBS || !param) {
		LOG_E("[DSA] Invalid parameters for glGetVertexArrayIndexediv");
		return;
	}

	temporarilyBindVertexArray(vaobj);
	glGetVertexAttribiv(index, pname, param);
	CHECK_GL_ERROR;
	restoreTemporaryVertexArrayBinding();

	LOG_D("[DSA] Retrieved indexed vertex array parameter 0x%X for VAO %u at index %u", pname, vaobj, index);
}

void glGetVertexArrayIndexed64iv(GLuint vaobj, GLuint index, GLenum pname, GLint64* param) {
	LOG()
		LOG_D("[DSA] glGetVertexArrayIndexed64iv, vaobj: %u, index: %u, pname: 0x%X, param: %p", vaobj, index, pname, param);

	if (vaobj == 0 || index >= GL_MAX_VERTEX_ATTRIBS || !param) {
		LOG_E("[DSA] Invalid parameters for glGetVertexArrayIndexed64iv");
		return;
	}

	temporarilyBindVertexArray(vaobj);
	glGetVertexAttribIiv(index, pname, (GLint*)param);
	CHECK_GL_ERROR;
	restoreTemporaryVertexArrayBinding();

	LOG_D("[DSA] Retrieved indexed 64-bit vertex array parameter 0x%X for VAO %u at index %u", pname, vaobj, index);
}

// sampler
void glCreateSamplers(GLsizei n, GLuint* samplers) {
	LOG()
	LOG_D("[DSA] glCreateSamplers, n: %d, samplers: %p", n, samplers);
	
	if (n <= 0 || !samplers) {
		LOG_E("[DSA] Invalid parameters for glCreateSamplers");
		return;
	}
	for (GLsizei i = 0; i < n; ++i) {
		GLuint samplerID = 0;
		glGenSamplers(1, &samplerID);
		if (samplerID == 0) {
			LOG_E("[DSA] Failed to create sampler at index %d", i);
			continue;
		}

		GLuint prevSampler = 0;
		glGetIntegerv(GL_SAMPLER_BINDING, (GLint*)&prevSampler);
		glBindSampler(1, samplerID);
		glBindSampler(1, prevSampler);

		samplers[i] = samplerID;
	}
	CHECK_GL_ERROR;
	
	LOG_D("[DSA] Created %d samplers successfully", n);
}

// program pipeline
void glCreateProgramPipelines(GLsizei n, GLuint* pipelines) {
	LOG()
	LOG_D("[DSA] glCreateProgramPipelines, n: %d, pipelines: %p", n, pipelines);
	
	if (n <= 0 || !pipelines) {
		LOG_E("[DSA] Invalid parameters for glCreateProgramPipelines");
		return;
	}
	for (GLsizei i = 0; i < n; ++i) {
		GLuint pipelineID = 0;
		glGenProgramPipelines(1, &pipelineID);
		if (pipelineID == 0) {
			LOG_E("[DSA] Failed to create program pipeline at index %d", i);
			continue;
		}

		GLuint prevPipeline = 0;
		glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, (GLint*)&prevPipeline);
		glBindProgramPipeline(pipelineID);
		CHECK_GL_ERROR;
		glBindProgramPipeline(prevPipeline);
		CHECK_GL_ERROR_NO_INIT;
		pipelines[i] = pipelineID;
	}
	
	LOG_D("[DSA] Created %d program pipelines successfully", n);
}

// query
void glCreateQueries(GLenum target, GLsizei n, GLuint* ids) {
	LOG()
	LOG_D("[DSA] glCreateQueries, target: 0x%X, n: %d, ids: %p", target, n, ids);
	if (n <= 0 || !ids) return;
	glGenQueries(n, ids);
}

static GLint pushQueryBufferBinding(GLuint buffer) {
	LOG_D("[DSA] pushQueryBufferBinding, buffer: %u", buffer);
	GLint prev = 0;
	glGetIntegerv(GL_QUERY_BUFFER_BINDING, &prev);
	glBindBuffer(GL_QUERY_BUFFER, buffer);
	CHECK_GL_ERROR;
	return prev;
}
static void popQueryBufferBinding(GLint prev) {
	LOG_D("[DSA] popQueryBufferBinding, prev: %d", prev);
	glBindBuffer(GL_QUERY_BUFFER, (GLuint)prev);
	CHECK_GL_ERROR;
}

void glGetQueryBufferObjectiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {
	LOG()
	LOG_D("[DSA] glGetQueryBufferObjectiv, id: %u, buffer: %u, pname: 0x%X, offset: %lld", id, buffer, pname, offset);
	assert(pname == GL_QUERY_RESULT || pname == GL_QUERY_RESULT_AVAILABLE);
	GLint prev = pushQueryBufferBinding(buffer);
	GLint value = 0;
	glGetQueryObjectiv(id, pname, &value);
	glBufferSubData(GL_QUERY_BUFFER, offset, sizeof(value), &value);
	CHECK_GL_ERROR;
	popQueryBufferBinding(prev);
}

void glGetQueryBufferObjectuiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {
	LOG()
	LOG_D("[DSA] glGetQueryBufferObjectuiv, id: %u, buffer: %u, pname: 0x%X, offset: %lld", id, buffer, pname, offset);
	assert(pname == GL_QUERY_RESULT || pname == GL_QUERY_RESULT_AVAILABLE);
	GLint prev = pushQueryBufferBinding(buffer);
	GLuint value = 0;
	glGetQueryObjectuiv(id, pname, &value);
	glBufferSubData(GL_QUERY_BUFFER, offset, sizeof(value), &value);
	CHECK_GL_ERROR;
	popQueryBufferBinding(prev);
}

void glGetQueryBufferObjecti64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {
	LOG()
	LOG_D("[DSA] glGetQueryBufferObjecti64v, id: %u, buffer: %u, pname: 0x%X, offset: %lld", id, buffer, pname, offset);
	assert(pname == GL_QUERY_RESULT || pname == GL_QUERY_RESULT_AVAILABLE);
	GLint prev = pushQueryBufferBinding(buffer);
	GLint64 value = 0;
	glGetQueryObjecti64v(id, pname, &value);
	glBufferSubData(GL_QUERY_BUFFER, offset, sizeof(value), &value);
	CHECK_GL_ERROR;
	popQueryBufferBinding(prev);
}

void glGetQueryBufferObjectui64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {
	LOG()
	LOG_D("[DSA] glGetQueryBufferObjectui64v, id: %u, buffer: %u, pname: 0x%X, offset: %lld", id, buffer, pname, offset);
	assert(pname == GL_QUERY_RESULT || pname == GL_QUERY_RESULT_AVAILABLE);
	GLint prev = pushQueryBufferBinding(buffer);
	GLuint64 value = 0;
	glGetQueryObjectui64v(id, pname, &value);
	glBufferSubData(GL_QUERY_BUFFER, offset, sizeof(value), &value);
	CHECK_GL_ERROR;
	popQueryBufferBinding(prev);
}
static thread_local std::vector<GLint> g_xfbBindingStack;

static void pushXFB(GLuint xfb) {
	LOG_D("[DSA] pushXFB, xfb: %u", xfb);
	GLint prev = 0;
	glGetIntegerv(GL_TRANSFORM_FEEDBACK_BINDING, &prev);
	if (xfb == prev) {
		g_xfbBindingStack.push_back(-1);
		return;
	}
	g_xfbBindingStack.push_back(prev);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, xfb);
	CHECK_GL_ERROR;
}

static void popXFB() {
	LOG_D("[DSA] popXFB");
	assert(!g_xfbBindingStack.empty());
	GLint prev = g_xfbBindingStack.back();
	g_xfbBindingStack.pop_back();
	if (prev == -1) {
		LOG_D("[DSA] No previous XFB binding to restore");
		return;
	}
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, (GLuint)prev);
	CHECK_GL_ERROR;
}

GLAPI void glCreateTransformFeedbacks(GLsizei n, GLuint* ids) {
	LOG();
	LOG_D("[DSA] glCreateTransformFeedbacks, n=%d, ids=%p", n, ids);
	if (n <= 0 || !ids) {
		LOG_E("[DSA] Invalid parameters for glCreateTransformFeedbacks");
		return;
	}
	glGenTransformFeedbacks(n, ids);
	CHECK_GL_ERROR;
	LOG_D("[DSA] Created %d transform feedback objects", n);
}

GLAPI void glTransformFeedbackBufferBase(GLuint xfb, GLuint index, GLuint buffer) {
	LOG();
	LOG_D("[DSA] glTransformFeedbackBufferBase, xfb=%u, index=%u, buffer=%u", xfb, index, buffer);
	if (xfb == 0 || index >= (GLuint)GL_MAX_TRANSFORM_FEEDBACK_BUFFERS || buffer == 0) {
		LOG_E("[DSA] Invalid parameters for glTransformFeedbackBufferBase");
		return;
	}
	pushXFB(xfb);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, index, buffer);
	CHECK_GL_ERROR;
	popXFB();
	LOG_D("[DSA] Bound buffer %u to TFBO %u at index %u", buffer, xfb, index);
}

GLAPI void glTransformFeedbackBufferRange(GLuint xfb, GLuint index,
	GLuint buffer,
	GLintptr offset, GLsizeiptr size)
{
	LOG();
	LOG_D("[DSA] glTransformFeedbackBufferRange, xfb=%u, index=%u, buffer=%u, offset=%lld, size=%lld",
		xfb, index, buffer, offset, size);
	if (xfb == 0 ||
		index >= (GLuint)GL_MAX_TRANSFORM_FEEDBACK_BUFFERS ||
		buffer == 0 ||
		offset < 0 || size <= 0)
	{
		LOG_E("[DSA] Invalid parameters for glTransformFeedbackBufferRange");
		return;
	}
	pushXFB(xfb);
	glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, index, buffer, offset, size);
	CHECK_GL_ERROR;
	popXFB();
	LOG_D("[DSA] Bound buffer %u to TFBO %u at index %u (offset=%lld, size=%lld)",
		buffer, xfb, index, offset, size);
}

GLAPI void glGetTransformFeedbackiv(GLuint xfb, GLenum pname, GLint* param) {
	LOG();
	LOG_D("[DSA] glGetTransformFeedbackiv, xfb=%u, pname=0x%X, param=%p", xfb, pname, param);
	if (xfb == 0 || !param) {
		LOG_E("[DSA] Invalid parameters for glGetTransformFeedbackiv");
		return;
	}
	pushXFB(xfb);
	glGetTransformFeedbackiv(GL_TRANSFORM_FEEDBACK, pname, param);
	CHECK_GL_ERROR;
	popXFB();
	LOG_D("[DSA] Retrieved TFBO %u param 0x%X = %d", xfb, pname, *param);
}

GLAPI void glGetTransformFeedbacki_v(GLuint xfb, GLenum pname, GLuint index, GLint* param) {
	LOG();
	LOG_D("[DSA] glGetTransformFeedbacki_v, xfb=%u, pname=0x%X, index=%u, param=%p",
		xfb, pname, index, param);
	if (xfb == 0 || index >= (GLuint)GL_MAX_TRANSFORM_FEEDBACK_BUFFERS || !param) {
		LOG_E("[DSA] Invalid parameters for glGetTransformFeedbacki_v");
		return;
	}
	pushXFB(xfb);
	glGetTransformFeedbacki_v(GL_TRANSFORM_FEEDBACK, pname, index, param);
	CHECK_GL_ERROR;
	popXFB();
	LOG_D("[DSA] Retrieved TFBO %u param 0x%X at index %u = %d", xfb, pname, index, *param);
}

GLAPI void glGetTransformFeedbacki64_v(GLuint xfb, GLenum pname, GLuint index, GLint64* param) {
	LOG();
	LOG_D("[DSA] glGetTransformFeedbacki64_v, xfb=%u, pname=0x%X, index=%u, param=%p",
		xfb, pname, index, param);
	if (xfb == 0 || index >= (GLuint)GL_MAX_TRANSFORM_FEEDBACK_BUFFERS || !param) {
		LOG_E("[DSA] Invalid parameters for glGetTransformFeedbacki64_v");
		return;
	}
	pushXFB(xfb);
	glGetTransformFeedbacki64_v(GL_TRANSFORM_FEEDBACK, pname, index, param);
	CHECK_GL_ERROR;
	popXFB();
	LOG_D("[DSA] Retrieved TFBO %u param 0x%X at index %u = %lld", xfb, pname, index, *param);
}
