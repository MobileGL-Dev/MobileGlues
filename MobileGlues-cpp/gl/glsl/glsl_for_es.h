// MobileGlues - gl/glsl/glsl_for_es.h
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header
#ifndef GLSL_FOR_ES
#define GLSL_FOR_ES
#include "../../gles/loader.h"
#include "../../includes.h"
#include "../glcorearb.h"
#include "../log.h"
#include <GL/gl.h>
#include <stdio.h>
#include <string>
#include <vector>
#include "uniform_defaults.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include "../mg.h"
#ifdef __cplusplus
}
#endif

// Translates desktop GLSL to ESSL. When `uniform_defaults` is given it receives
// the initialisers the shader gave its uniforms, which ESSL cannot carry; see
// uniform_defaults.h.
std::string GLSLtoGLSLES(const char* glsl_code, GLenum glsl_type, uint essl_version, uint glsl_version,
                         int& return_code, std::vector<uniform_default_t>* uniform_defaults = nullptr);
std::string GLSLtoGLSLES_1(const char* glsl_code, GLenum glsl_type, uint esversion, int& return_code);
std::string GLSLtoGLSLES_2(const char* glsl_code, GLenum glsl_type, uint essl_version, int& return_code);
int getGLSLVersion(const char* glsl_code);

#endif