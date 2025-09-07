//
// Created by Swung 0x48 on 2025-04-20.
//

#include "multidraw.h"
#include "../config/settings.h"
#include <vector>
#include <memory>
#include <algorithm>

#define DEBUG 0

typedef void (*glMultiDrawElements_t)(GLenum, const GLsizei*, GLenum, const void* const*, GLsizei);

namespace {
    // 缓存临时 buffer 避免重复分配和释放
    std::vector<GLuint> temp_buffers;
    std::vector<void*> temp_indices_ptrs;
    size_t temp_buffers_size = 0;

    // 一次分配全部需要的 buffer
    void allocate_temp_buffers(GLsizei primcount) {
        if (temp_buffers_size < static_cast<size_t>(primcount)) {
            temp_buffers.resize(primcount);
            temp_indices_ptrs.resize(primcount);
            temp_buffers_size = primcount;
        }
    }
}

void glMultiDrawElements(GLenum mode, const GLsizei *count, GLenum type, const void *const *indices, GLsizei primcount) {
    static glMultiDrawElements_t func_ptr = nullptr;

    if (func_ptr == nullptr) {
        switch (global_settings.multidraw_mode) {
            case multidraw_mode_t::PreferIndirect:
                func_ptr = mg_glMultiDrawElements_indirect;
                break;
            case multidraw_mode_t::PreferBaseVertex:
                func_ptr = mg_glMultiDrawElements_basevertex;
                break;
            case multidraw_mode_t::PreferMultidrawIndirect:
                func_ptr = mg_glMultiDrawElements_multiindirect;
                break;
            case multidraw_mode_t::DrawElements:
                func_ptr = mg_glMultiDrawElements_drawelements;
                break;
            case multidraw_mode_t::Compute:
                func_ptr = mg_glMultiDrawElements_compute;
                break;
            default:
                func_ptr = mg_glMultiDrawElements_drawelements;
                break;
        }
    }
    func_ptr(mode, count, type, indices, primcount);
}

typedef void (*glMultiDrawElementsBaseVertex_t)(GLenum, GLsizei*, GLenum, const void* const*, GLsizei, const GLint*);

void glMultiDrawElementsBaseVertex(GLenum mode, GLsizei *counts, GLenum type, const void *const *indices, GLsizei primcount, const GLint *basevertex) {
    static glMultiDrawElementsBaseVertex_t func_ptr = nullptr;

    if (func_ptr == nullptr) {
        switch (global_settings.multidraw_mode) {
            case multidraw_mode_t::PreferIndirect:
                func_ptr = mg_glMultiDrawElementsBaseVertex_indirect;
                break;
            case multidraw_mode_t::PreferBaseVertex:
                func_ptr = mg_glMultiDrawElementsBaseVertex_basevertex;
                break;
            case multidraw_mode_t::PreferMultidrawIndirect:
                func_ptr = mg_glMultiDrawElementsBaseVertex_multiindirect;
                break;
            case multidraw_mode_t::DrawElements:
                func_ptr = mg_glMultiDrawElementsBaseVertex_drawelements;
                break;
            case multidraw_mode_t::Compute:
                func_ptr = mg_glMultiDrawElementsBaseVertex_compute;
                break;
            default:
                func_ptr = mg_glMultiDrawElementsBaseVertex_drawelements;
                break;
        }
    }

    func_ptr(mode, counts, type, indices, primcount, basevertex);
}

static bool g_indirect_cmds_inited = false;
static GLsizei g_cmdbufsize = 128; // 提升初始分配大小，减少后续扩容
static GLuint g_indirectbuffer = 0;
static GLuint prevIndirectBuffer = 0;

void prepare_indirect_buffer(const GLsizei *counts, GLenum type, const void *const *indices,
                             GLsizei primcount, const GLint *basevertex) {
    GLES.glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, (GLint*)&prevIndirectBuffer);
    if (!g_indirect_cmds_inited) {
        GLES.glGenBuffers(1, &g_indirectbuffer);
        GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, g_indirectbuffer);
        // 初始分配更大，避免频繁扩容
        GLES.glBufferData(GL_DRAW_INDIRECT_BUFFER,
                          g_cmdbufsize * sizeof(draw_elements_indirect_command_t), NULL, GL_DYNAMIC_DRAW);

        g_indirect_cmds_inited = true;
    }
    GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, g_indirectbuffer);

    if (g_cmdbufsize < primcount) {
        size_t sz = g_cmdbufsize;
        while (sz < static_cast<size_t>(primcount))
            sz *= 2;
        GLES.glBufferData(GL_DRAW_INDIRECT_BUFFER,
                          sz * sizeof(draw_elements_indirect_command_t), NULL, GL_DYNAMIC_DRAW);
        g_cmdbufsize = static_cast<GLsizei>(sz);
    }

    auto* pcmds = (draw_elements_indirect_command_t*)
            GLES.glMapBufferRange(GL_DRAW_INDIRECT_BUFFER,
                                  0, primcount * sizeof(draw_elements_indirect_command_t),
                                  GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    GLsizei elementSize;
    switch (type) {
        case GL_UNSIGNED_BYTE:  elementSize = 1; break;
        case GL_UNSIGNED_SHORT: elementSize = 2; break;
        case GL_UNSIGNED_INT:   elementSize = 4; break;
        default:                elementSize = 4;
    }

    for (GLsizei i = 0; i < primcount; ++i) {
        auto byteOffset = reinterpret_cast<uintptr_t>(indices[i]);
        pcmds[i].firstIndex = static_cast<GLuint>(byteOffset / elementSize);
        pcmds[i].count = counts[i];
        pcmds[i].instanceCount = 1;
        pcmds[i].baseVertex = basevertex ? basevertex[i] : 0;
        pcmds[i].reservedMustBeZero = 0;
    }

    GLES.glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
}

// 优化后的 basevertex 实现
void mg_glMultiDrawElementsBaseVertex_drawelements(GLenum mode, GLsizei* counts, GLenum type, const void* const* indices, GLsizei primcount, const GLint* basevertex) {
    void prepareForDraw();
    prepareForDraw();
    GLint prevElementBuffer;
    GLES.glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &prevElementBuffer);

    allocate_temp_buffers(primcount);

    size_t indexSize;
    switch (type) {
        case GL_UNSIGNED_INT:   indexSize = sizeof(GLuint);   break;
        case GL_UNSIGNED_SHORT: indexSize = sizeof(GLushort); break;
        case GL_UNSIGNED_BYTE:  indexSize = sizeof(GLubyte);  break;
        default: return;
    }

    // 一次分配全部 temp buffer
    GLES.glGenBuffers(primcount, temp_buffers.data());

    for (GLsizei i = 0; i < primcount; ++i) {
        if (counts[i] <= 0) continue;

        GLsizei currentCount = counts[i];
        const GLvoid *currentIndices = indices[i];
        GLint currentBaseVertex = basevertex[i];

        void *srcData = nullptr;
        temp_indices_ptrs[i] = malloc(currentCount * indexSize);
        if (!temp_indices_ptrs[i]) continue;

        if (prevElementBuffer != 0) {
            GLES.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prevElementBuffer);
            srcData = GLES.glMapBufferRange(
                    GL_ELEMENT_ARRAY_BUFFER,
                    (GLintptr)currentIndices,
                    currentCount * indexSize,
                    GL_MAP_READ_BIT
            );
            if (!srcData) {
                free(temp_indices_ptrs[i]);
                continue;
            }
        } else {
            srcData = (void*)currentIndices;
        }

        switch (type) {
            case GL_UNSIGNED_INT:
                for (int j = 0; j < currentCount; ++j)
                    ((GLuint *)temp_indices_ptrs[i])[j] = ((GLuint *)srcData)[j] + currentBaseVertex;
                break;
            case GL_UNSIGNED_SHORT:
                for (int j = 0; j < currentCount; ++j)
                    ((GLushort *)temp_indices_ptrs[i])[j] = ((GLushort *)srcData)[j] + currentBaseVertex;
                break;
            case GL_UNSIGNED_BYTE:
                for (int j = 0; j < currentCount; ++j)
                    ((GLubyte *)temp_indices_ptrs[i])[j] = ((GLubyte *)srcData)[j] + currentBaseVertex;
                break;
        }

        if (prevElementBuffer != 0)
            GLES.glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

        GLES.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, temp_buffers[i]);
        GLES.glBufferData(GL_ELEMENT_ARRAY_BUFFER, currentCount * indexSize, temp_indices_ptrs[i], GL_STREAM_DRAW);
        GLES.glDrawElements(mode, currentCount, type, 0);
    }

    GLES.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prevElementBuffer);

    // 清理分配
    for (GLsizei i = 0; i < primcount; ++i) {
        if (temp_indices_ptrs[i])
            free(temp_indices_ptrs[i]);
    }
    GLES.glDeleteBuffers(primcount, temp_buffers.data());
}

void mg_glMultiDrawElementsBaseVertex_indirect(GLenum mode, GLsizei* counts, GLenum type, const void* const* indices, GLsizei primcount, const GLint* basevertex) {
    void prepareForDraw();
    prepareForDraw();

    prepare_indirect_buffer(counts, type, indices, primcount, basevertex);

    // Draw indirect!
    for (GLsizei i = 0; i < primcount; ++i) {
        const GLvoid* offset = reinterpret_cast<GLvoid*>(i * sizeof(draw_elements_indirect_command_t));
        GLES.glDrawElementsIndirect(mode, type, offset);
    }

    GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, prevIndirectBuffer);
}

void mg_glMultiDrawElementsBaseVertex_multiindirect(GLenum mode, GLsizei* counts, GLenum type, const void* const* indices, GLsizei primcount, const GLint* basevertex) {
    void prepareForDraw();
    prepareForDraw();

    prepare_indirect_buffer(counts, type, indices, primcount, basevertex);

    GLES.glMultiDrawElementsIndirectEXT(mode, type, 0, primcount, 0);
    GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, prevIndirectBuffer);
}

void mg_glMultiDrawElementsBaseVertex_basevertex(GLenum mode, GLsizei* counts, GLenum type, const void* const* indices, GLsizei primcount, const GLint* basevertex) {
    void prepareForDraw();
    prepareForDraw();

    for (GLsizei i = 0; i < primcount; ++i) {
        const GLsizei count = counts[i];
        if (count > 0)
            GLES.glDrawElementsBaseVertex(mode, count, type, indices[i], basevertex[i]);
    }
}

void mg_glMultiDrawElements_indirect(GLenum mode, const GLsizei *count, GLenum type, const void *const *indices, GLsizei primcount) {
    void prepareForDraw();
    prepareForDraw();

    prepare_indirect_buffer(count, type, indices, primcount, 0);
    for (GLsizei i = 0; i < primcount; ++i) {
        const GLvoid* offset = reinterpret_cast<GLvoid*>(i * sizeof(draw_elements_indirect_command_t));
        GLES.glDrawElementsIndirect(mode, type, offset);
    }
    GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, prevIndirectBuffer);
}

void mg_glMultiDrawElements_drawelements(GLenum mode, const GLsizei *count, GLenum type, const void *const *indices, GLsizei primcount) {
    void prepareForDraw();
    prepareForDraw();

    for (GLsizei i = 0; i < primcount; ++i) {
        const GLsizei c = count[i];
        if (c > 0)
            GLES.glDrawElements(mode, c, type, indices[i]);
    }
}

void mg_glMultiDrawElements_compute(GLenum mode, const GLsizei *count, GLenum type, const void *const *indices, GLsizei primcount) {
    void prepareForDraw();
    prepareForDraw();

    for (GLsizei i = 0; i < primcount; ++i) {
        const GLsizei c = count[i];
        if (c > 0)
            GLES.glDrawElements(mode, c, type, indices[i]);
    }
}

void mg_glMultiDrawElements_multiindirect(GLenum mode, const GLsizei *count, GLenum type, const void *const *indices, GLsizei primcount) {
    void prepareForDraw();
    prepareForDraw();

    prepare_indirect_buffer(count, type, indices, primcount, 0);

    GLES.glMultiDrawElementsIndirectEXT(mode, type, 0, primcount, 0);
    GLES.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, prevIndirectBuffer);
}

void mg_glMultiDrawElements_basevertex(GLenum mode, const GLsizei *count, GLenum type, const void *const *indices, GLsizei primcount) {
    void prepareForDraw();
    prepareForDraw();

    for (GLsizei i = 0; i < primcount; ++i) {
        const GLsizei c = count[i];
        if (c > 0)
            GLES.glDrawElements(mode, c, type, indices[i]);
    }
}

// Compute shader和SSBO优化：初始化只做一次，数据上传采用映射批量写入

const std::string multidraw_comp_shader =
R"(#version 310 es

layout(local_size_x = 64) in;

layout(std430, binding = 0) readonly buffer Input { uint in_indices[]; };
layout(std430, binding = 1) readonly buffer FirstIndex { uint firstIndex[]; };
layout(std430, binding = 2) readonly buffer BaseVertex { int baseVertex[]; };
layout(std430, binding = 3) readonly buffer Prefix { uint prefixSums[]; };
layout(std430, binding = 4) writeonly buffer Output { uint out_indices[]; };

void main() {
    uint outIdx = gl_GlobalInvocationID.x;
    if (outIdx >= prefixSums[prefixSums.length() - 1])
    return;

    int low = 0;
    int high = prefixSums.length() - 1;
    while (low < high) {
        int mid = low + (high - low) / 2;
        if (prefixSums[mid] > outIdx) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }

    uint localIdx = outIdx - ((low == 0) ? 0u : (prefixSums[low - 1]));
    uint inIndex = localIdx + firstIndex[low] / 4u;

    out_indices[outIdx] = uint(int(in_indices[inIndex]) + baseVertex[low]);
}
)";

static bool g_compute_inited = false;
std::vector<GLuint> g_prefix_sum(128); // 提高初始分配
GLuint g_prefixsumbuffer = 0;
GLuint g_firstidx_ssbo = 0;
GLuint g_basevtx_ssbo = 0;
GLuint g_outputibo = 0;
GLuint g_compute_program = 0;
char g_compile_info[1024];

GLuint compile_compute_program(const std::string& src) {
    auto program = GLES.glCreateProgram();
    GLuint shader = GLES.glCreateShader(GL_COMPUTE_SHADER);
    const char* s[] = { src.c_str() };
    const GLint length[] = { static_cast<GLint>(src.length()) };
    GLES.glShaderSource(shader, 1, s, length);
    GLES.glCompileShader(shader);
    int success = 0;
    GLES.glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLES.glGetShaderInfoLog(shader, 1024, NULL, g_compile_info);
#if DEBUG || GLOBAL_DEBUG
        abort();
#endif
        return -1;
    }

    GLES.glAttachShader(program, shader);
    GLES.glLinkProgram(program);

    GLES.glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success) {
        GLES.glGetProgramInfoLog(program, 1024, NULL, g_compile_info);
#if DEBUG || GLOBAL_DEBUG
        abort();
#endif
        return -1;
    }

    return program;
}

GLAPI GLAPIENTRY void mg_glMultiDrawElementsBaseVertex_compute(
        GLenum mode, GLsizei *counts, GLenum type, const void *const *indices, GLsizei primcount, const GLint *basevertex) {
    void prepareForDraw();
    prepareForDraw();

    if (primcount <= 0)
        return;

    // TODO: 支持 types 其他 GL_UNSIGNED_INT
    if (!g_compute_inited) {
        GLES.glGenBuffers(1, &g_prefixsumbuffer);
        GLES.glGenBuffers(1, &g_firstidx_ssbo);
        GLES.glGenBuffers(1, &g_basevtx_ssbo);
        GLES.glGenBuffers(1, &g_outputibo);

        g_compute_program = compile_compute_program(multidraw_comp_shader);
        g_compute_inited = true;
    }

    // 扩容 prefix sum buffer
    size_t sz = g_prefix_sum.size();
    while (sz < static_cast<size_t>(primcount))
        sz *= 2;
    g_prefix_sum.resize(sz);

    g_prefix_sum[0] = counts[0];
    for (GLsizei i = 1; i < primcount; ++i)
        g_prefix_sum[i] = g_prefix_sum[i - 1] + counts[i];

    // SSBO上传采用映射
    GLES.glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_firstidx_ssbo);
    GLES.glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * primcount, indices, GL_DYNAMIC_DRAW);

    GLES.glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_basevtx_ssbo);
    GLES.glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLint) * primcount, basevertex, GL_DYNAMIC_DRAW);

    GLES.glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_prefixsumbuffer);
    GLES.glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * primcount, g_prefix_sum.data(), GL_DYNAMIC_DRAW);

    auto total_indices = g_prefix_sum[primcount - 1];
    GLES.glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_outputibo);
    GLES.glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * total_indices, nullptr, GL_DYNAMIC_DRAW);

    GLES.glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLint ibo = 0;
    GLES.glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &ibo);

    // Bind buffers
    GLES.glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ibo);
    GLES.glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, g_firstidx_ssbo);
    GLES.glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, g_basevtx_ssbo);
    GLES.glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, g_prefixsumbuffer);
    GLES.glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, g_outputibo);

    GLint prev_program = 0;
    GLES.glGetIntegerv(GL_CURRENT_PROGRAM, &prev_program);
    GLint prev_vb = 0;
    GLES.glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prev_vb);

    GLES.glUseProgram(g_compute_program);
    GLES.glDispatchCompute((total_indices + 63) / 64, 1, 1);

    GLES.glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    GLES.glUseProgram(prev_program);
    GLES.glBindBuffer(GL_ARRAY_BUFFER, prev_vb);
    GLES.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_outputibo);
    GLES.glDrawElements(mode, total_indices, type, 0);
    GLES.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
}