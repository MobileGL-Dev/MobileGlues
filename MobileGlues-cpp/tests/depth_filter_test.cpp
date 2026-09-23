// MobileGlues - tests/depth_filter_test.cpp
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header
//
// Host-side boundary proof for gl/depth_filter.cpp. Two numbers at every
// edge: the state that trips the ES depth-completeness rule, and the state
// next to it that passes.

#include "gl/depth_filter.h"

#include <cstdio>

static int g_failures = 0;

#define CHECK(expr, want)                                                                                       \
    do {                                                                                                       \
        const auto got = (expr);                                                                                \
        const bool ok = got == (want);                                                                          \
        if (!ok) ++g_failures;                                                                                  \
        std::printf("%-4s %s: got %lld, want %lld\n", ok ? "ok" : "FAIL", #expr, (long long)got,               \
                    (long long)(want));                                                                         \
    } while (0)

int main() {
    // Format membership: the six sized tokens the rule names, and the
    // neighbours that must stay outside it.
    CHECK(mg_is_sized_depth_format(GL_DEPTH_COMPONENT16), true);
    CHECK(mg_is_sized_depth_format(GL_DEPTH_COMPONENT24), true);
    CHECK(mg_is_sized_depth_format(GL_DEPTH_COMPONENT32F), true);
    CHECK(mg_is_sized_depth_format(GL_DEPTH24_STENCIL8), true);
    CHECK(mg_is_sized_depth_format(GL_DEPTH32F_STENCIL8), true);
    CHECK(mg_is_sized_depth_format(GL_DEPTH_COMPONENT), false); // unsized, ANGLE legacy hole
    CHECK(mg_is_sized_depth_format(GL_RGBA8), false);
    CHECK(mg_is_sized_depth_format(0), false);

    // The MC 26.x pairing trips: D32F, compare NONE, GlSampler's 9986
    // (GL_NEAREST_MIPMAP_LINEAR), mag NEAREST.
    CHECK(mg_depth_pairing_incomplete(GL_DEPTH_COMPONENT32F, GL_NONE, GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST), true);
    // The same pairing one filter value over passes: min NEAREST_MIPMAP_NEAREST.
    CHECK(mg_depth_pairing_incomplete(GL_DEPTH_COMPONENT32F, GL_NONE, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST), false);

    // Mag half of the rule trips alone: min NEAREST, mag LINEAR.
    CHECK(mg_depth_pairing_incomplete(GL_DEPTH_COMPONENT24, GL_NONE, GL_NEAREST, GL_LINEAR), true);
    // And min half alone: min LINEAR_MIPMAP_LINEAR trips, min NEAREST with
    // the same mag passes (checked above).
    CHECK(mg_depth_pairing_incomplete(GL_DEPTH_COMPONENT16, GL_NONE, GL_LINEAR_MIPMAP_LINEAR, GL_NEAREST), true);

    // Compare-enabled pairing passes under the filters that fail above:
    // PCF stays exempt.
    CHECK(mg_depth_pairing_incomplete(GL_DEPTH_COMPONENT32F, GL_COMPARE_REF_TO_TEXTURE, GL_LINEAR_MIPMAP_LINEAR,
                                      GL_LINEAR),
          false);

    // Unsized depth passes (spec bullet names sized formats only), and a
    // colour format never trips regardless of filters.
    CHECK(mg_depth_pairing_incomplete(GL_DEPTH_COMPONENT, GL_NONE, GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST), false);
    CHECK(mg_depth_pairing_incomplete(GL_RGBA8, GL_NONE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR), false);

    // Mapping: mip-capable non-point keeps mip selection, non-mip LINEAR
    // stays non-mip, legal values pass through.
    CHECK(mg_depth_filter_make_legal(GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR), GL_NEAREST_MIPMAP_NEAREST);
    CHECK(mg_depth_filter_make_legal(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR), GL_NEAREST_MIPMAP_NEAREST);
    CHECK(mg_depth_filter_make_legal(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST), GL_NEAREST_MIPMAP_NEAREST);
    CHECK(mg_depth_filter_make_legal(GL_TEXTURE_MIN_FILTER, GL_LINEAR), GL_NEAREST);
    CHECK(mg_depth_filter_make_legal(GL_TEXTURE_MIN_FILTER, GL_NEAREST), GL_NEAREST);
    CHECK(mg_depth_filter_make_legal(GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST), GL_NEAREST_MIPMAP_NEAREST);
    CHECK(mg_depth_filter_make_legal(GL_TEXTURE_MAG_FILTER, GL_LINEAR), GL_NEAREST);
    CHECK(mg_depth_filter_make_legal(GL_TEXTURE_MAG_FILTER, GL_NEAREST), GL_NEAREST);

    // Record-time validation: every legal min value and mag's two, one
    // rejected min, one rejected mag, compare's two and a rejected token.
    CHECK(mg_filter_value_legal(GL_TEXTURE_MAG_FILTER, GL_NEAREST), true);
    CHECK(mg_filter_value_legal(GL_TEXTURE_MAG_FILTER, GL_LINEAR), true);
    CHECK(mg_filter_value_legal(GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST), false);
    CHECK(mg_filter_value_legal(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR), true);
    CHECK(mg_filter_value_legal(GL_TEXTURE_MIN_FILTER, 1234), false);
    CHECK(mg_filter_value_legal(GL_TEXTURE_COMPARE_MODE, GL_NONE), true);
    CHECK(mg_filter_value_legal(GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE), true);
    CHECK(mg_filter_value_legal(GL_TEXTURE_COMPARE_MODE, GL_NEAREST_MIPMAP_LINEAR), false);

    if (g_failures) {
        std::printf("depth_filter_test: %d failures\n", g_failures);
        return 1;
    }
    std::printf("depth_filter_test: all checks passed\n");
    return 0;
}
