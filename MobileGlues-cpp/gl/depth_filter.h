// MobileGlues - gl/depth_filter.h
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header

#ifndef MOBILEGLUES_DEPTH_FILTER_H
#define MOBILEGLUES_DEPTH_FILTER_H

#include <GL/gl.h>

// The GLES depth-texture completeness rule, kept as pure predicates so the
// logic runs host-side in tests/depth_filter_test.cpp without a GPU.
//
// ES 3.0.6 section 3.8.13, carried into ES 3.2 section 8.17: a texture is
// incomplete while all of the following hold:
//   - the effective internal format is a SIZED depth or depth-stencil
//     format (table 8.11),
//   - TEXTURE_COMPARE_MODE is NONE,
//   - either the magnification filter is not NEAREST, or the minification
//     filter is neither NEAREST nor NEAREST_MIPMAP_NEAREST.
// An incomplete texture samples as (0, 0, 0, 1). Desktop GL has no such
// rule, so filter state an application legally keeps there is incomplete on
// strict ES hosts (ANGLE enforces it in its GLES layer on every backend).
// Minecraft 26.x hits this through GlSampler, which maps minFilter=NEAREST
// to 9986 (GL_NEAREST_MIPMAP_LINEAR): every D32F layer texture of the
// transparency composite goes incomplete and the per-pixel layer sort reads
// zeros. See issue #57.
//
// Unsize'd GL_DEPTH_COMPONENT is deliberately NOT in the sized set: the
// spec bullet names only sized formats, ANGLE keeps the legacy unsized hole
// open (see its Texture.cpp completeness check), and MG's internal_convert
// leaves data-carrying GL_DEPTH_COMPONENT uploads unsized on purpose.

// True for the sized depth and depth-stencil internalformats the rule names.
bool mg_is_sized_depth_format(GLenum internal_format);

// True when a sampling of `internal_format` under this pairing is
// filter-incomplete on a strict ES host. The filters are the EFFECTIVE
// state: sampler-object values when a sampler object is bound to the unit,
// texture-object values otherwise. Non-depth formats and compare-enabled
// pairings answer false; shadow comparison under LINEAR is exempt (ES 3.0.1
// clarification, and the reason PCF works on phones at all).
bool mg_depth_pairing_incomplete(GLenum internal_format, GLenum compare_mode, GLint min_filter, GLint mag_filter);

// Map one filter value to the nearest legal value for a depth pairing,
// preserving intent: non-mip LINEAR becomes NEAREST, mip-capable non-point
// min filters become NEAREST_MIPMAP_NEAREST (LOD selection kept, linear
// taps dropped), legal values pass through unchanged.
GLint mg_depth_filter_make_legal(GLenum pname, GLint filter);

// True when `filter` is a legal value for `pname` on a GLES host. Shadow
// state records only values the driver accepts, so the shadow and the
// driver cannot drift on rejected set (GL_INVALID_ENUM, state unchanged).
bool mg_filter_value_legal(GLenum pname, GLint filter);

#endif
