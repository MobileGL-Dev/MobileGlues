//
// Created by Administrator on 2025/1/27.
//

#include "mg.h"

hard_ext_t hard_ext;
gl_state_t gl_state;

GLenum pname_convert(GLenum pname){
    switch (pname) {
        case GL_TEXTURE_LOD_BIAS:
            LOG_D("pnameConvert: GL_TEXTURE_LOD_BIAS -> GL_TEXTURE_LOD_BIAS_EXT");
            return GL_TEXTURE_LOD_BIAS_QCOM;
    }
    return pname;
}

GLenum map_tex_target(GLenum target) {
    switch (target) {
        case GL_TEXTURE_1D:
        case GL_TEXTURE_3D:
        case GL_TEXTURE_RECTANGLE_ARB:
            target = GL_TEXTURE_2D;
            break;
        case GL_PROXY_TEXTURE_1D:
        case GL_PROXY_TEXTURE_3D:
        case GL_PROXY_TEXTURE_RECTANGLE_ARB:
            target = GL_PROXY_TEXTURE_2D;
            break;
    }
    return target;
}