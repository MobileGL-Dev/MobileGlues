// MobileGlues - gl/glsl/uniform_defaults.h
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header
#ifndef MOBILEGLUES_UNIFORM_DEFAULTS_H
#define MOBILEGLUES_UNIFORM_DEFAULTS_H

#include <string>
#include <vector>

// A default value the shader gave one of its uniforms.
//
// Desktop GLSL lets a uniform carry an initialiser and takes that value at link
// time; ESSL does not allow the initialiser at all. process_uniform_declarations
// strips it from the translated source and reports it here, so glLinkProgram can
// hand it back to the driver with glUniform* once the program links. Without that
// second half the shader silently sees 0 wherever it wrote a default.
//
// One record is one thing the GL uniform API can address: a scalar, a vector, a
// matrix, or an array of those. A struct-typed uniform is split into its members
// ("light.color"), an array of structs into one record per element and member
// ("lights[1].color").
enum class uniform_base_t {
    Float,
    Int,
    Uint,
    Bool
};

struct uniform_default_t {
    std::string name;
    uniform_base_t base = uniform_base_t::Float;
    int rows = 1;    // components per column: 1 for a scalar, 2-4 for a vector or matrix
    int columns = 1; // 1 for a scalar or vector, 2-4 for a matrix
    int count = 1;   // array length, 1 when not an array
    // rows * columns * count numbers, one element after another, each column-major.
    // Int, Uint and Bool are stored exactly; Bool is 0 or 1.
    std::vector<double> values;
};

// Removes the initialiser from every uniform declaration in `essl` and leaves the
// rest of the text untouched: array sizes, further declarators, layout and precision
// qualifiers all stay where they were. Interface blocks, comments and preprocessor
// lines are skipped. When `defaults` is given it is cleared and filled with the
// values that were removed, evaluated from the constant expressions SPIRV-Cross
// emits (literals, constructors, array and struct constructors, references to
// global constants). An initialiser this cannot evaluate is still removed, it just
// produces no record.
std::string process_uniform_declarations(const std::string& essl, std::vector<uniform_default_t>* defaults = nullptr);

#endif // MOBILEGLUES_UNIFORM_DEFAULTS_H
