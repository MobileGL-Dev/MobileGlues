// MobileGlues - gl/depth_filter.cpp
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

#include "depth_filter.h"

bool mg_is_sized_depth_format(GLenum internal_format) {
    switch (internal_format) {
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32:
    case GL_DEPTH_COMPONENT32F:
    case GL_DEPTH24_STENCIL8:
    case GL_DEPTH32F_STENCIL8:
        return true;
    default:
        // Includes unsized GL_DEPTH_COMPONENT: the rule names only sized
        // formats, and internal_convert hands that enum to data-carrying
        // depth uploads deliberately.
        return false;
    }
}

bool mg_depth_pairing_incomplete(GLenum internal_format, GLenum compare_mode, GLint min_filter, GLint mag_filter) {
    if (!mg_is_sized_depth_format(internal_format)) return false;
    if (compare_mode != GL_NONE) return false;
    if (mag_filter != GL_NEAREST) return true;
    return min_filter != GL_NEAREST && min_filter != GL_NEAREST_MIPMAP_NEAREST;
}

GLint mg_depth_filter_make_legal(GLenum pname, GLint filter) {
    if (pname == GL_TEXTURE_MAG_FILTER) {
        // MAG has exactly two legal values.
        return filter == GL_LINEAR ? GL_NEAREST : filter;
    }
    switch (filter) {
    case GL_LINEAR:
        // Not mip-requiring before, not mip-requiring after.
        return GL_NEAREST;
    case GL_LINEAR_MIPMAP_NEAREST:
    case GL_NEAREST_MIPMAP_LINEAR:
    case GL_LINEAR_MIPMAP_LINEAR:
        // Mip-requiring before, mip-requiring after: an app that supplied a
        // complete chain keeps sampling levels by LOD, only the linear taps
        // go away. NEAREST and NEAREST_MIPMAP_NEAREST fall to default.
        return GL_NEAREST_MIPMAP_NEAREST;
    default:
        return filter;
    }
}

bool mg_filter_value_legal(GLenum pname, GLint filter) {
    if (pname == GL_TEXTURE_MAG_FILTER) return filter == GL_NEAREST || filter == GL_LINEAR;
    if (pname == GL_TEXTURE_MIN_FILTER) {
        switch (filter) {
        case GL_NEAREST:
        case GL_LINEAR:
        case GL_NEAREST_MIPMAP_NEAREST:
        case GL_LINEAR_MIPMAP_NEAREST:
        case GL_NEAREST_MIPMAP_LINEAR:
        case GL_LINEAR_MIPMAP_LINEAR:
            return true;
        default:
            return false;
        }
    }
    if (pname == GL_TEXTURE_COMPARE_MODE) return filter == GL_NONE || filter == GL_COMPARE_REF_TO_TEXTURE;
    // Not this layer's value set to judge (wrap modes, LOD, swizzle, ...).
    return true;
}
