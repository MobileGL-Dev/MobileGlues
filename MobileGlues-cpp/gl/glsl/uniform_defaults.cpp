// MobileGlues - gl/glsl/uniform_defaults.cpp
// Copyright (c) 2025-2026 MobileGL-Dev
// Licensed under the GNU Lesser General Public License v2.1:
//   https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
// SPDX-License-Identifier: LGPL-2.1-only
// End of Source File Header
#include "uniform_defaults.h"

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <unordered_map>

// The previous version of this pass looked for the seven letters "uniform"
// anywhere in the text and rebuilt whatever followed as
// `uniform <precision> <type> <name>;`. That rewrote any identifier containing
// the word (nonuniformEXT, nonuniformScale, uniforms), dropped array sizes and
// second declarators, split a type whose name starts with a precision keyword,
// and swallowed the head of an interface block whose first member had a
// layout(offset = ...). It also threw the default value away.
//
// This version tokenises the source, walks only the global scope, and deletes
// exactly the `= <expression>` spans of uniform declarators. Nothing is rebuilt,
// so anything it does not understand is left as it was.

namespace {

    // ---------------------------------------------------------------- lexing ----

    enum class tok_kind_t {
        Ident,
        Number,
        Punct
    };

    struct token_t {
        tok_kind_t kind;
        size_t begin;
        size_t end;
    };

    bool is_ident_start(unsigned char c) {
        return std::isalpha(c) || c == '_';
    }
    bool is_ident_char(unsigned char c) {
        return std::isalnum(c) || c == '_';
    }

    // Comments and preprocessor lines produce no tokens, so their text can never be
    // taken for a declaration.
    std::vector<token_t> lex(const std::string& s) {
        std::vector<token_t> toks;
        const size_t n = s.size();
        size_t i = 0;
        bool line_start = true; // nothing but whitespace since the last newline
        while (i < n) {
            const unsigned char c = static_cast<unsigned char>(s[i]);
            if (c == '\n') {
                line_start = true;
                ++i;
                continue;
            }
            if (std::isspace(c)) {
                ++i;
                continue;
            }
            if (c == '/' && i + 1 < n && s[i + 1] == '/') {
                while (i < n && s[i] != '\n')
                    ++i;
                continue;
            }
            if (c == '/' && i + 1 < n && s[i + 1] == '*') {
                const size_t close = s.find("*/", i + 2);
                i = (close == std::string::npos) ? n : close + 2;
                continue;
            }
            if (c == '#' && line_start) {
                // A directive runs to the end of its line; a backslash continues it.
                while (i < n && s[i] != '\n') {
                    if (s[i] == '\\' && i + 1 < n && s[i + 1] == '\n')
                        i += 2;
                    else
                        ++i;
                }
                continue;
            }
            line_start = false;
            const size_t begin = i;
            if (is_ident_start(c)) {
                while (i < n && is_ident_char(static_cast<unsigned char>(s[i])))
                    ++i;
                toks.push_back({tok_kind_t::Ident, begin, i});
            } else if (std::isdigit(c) ||
                       (c == '.' && i + 1 < n && std::isdigit(static_cast<unsigned char>(s[i + 1])))) {
                const bool hex = (c == '0' && i + 1 < n && (s[i + 1] == 'x' || s[i + 1] == 'X'));
                while (i < n) {
                    const unsigned char d = static_cast<unsigned char>(s[i]);
                    if (std::isalnum(d) || d == '.') {
                        ++i;
                    } else if (!hex && (d == '+' || d == '-') && (s[i - 1] == 'e' || s[i - 1] == 'E')) {
                        ++i; // exponent sign
                    } else {
                        break;
                    }
                }
                toks.push_back({tok_kind_t::Number, begin, i});
            } else {
                ++i;
                toks.push_back({tok_kind_t::Punct, begin, i});
            }
        }
        return toks;
    }

    // ----------------------------------------------------------------- types ----

    struct shape_t {
        uniform_base_t base;
        int rows;
        int columns;
    };

    // The transparent types the GL uniform API can set. Doubles are not in ESSL and
    // opaque types cannot have initialisers, so neither appears here.
    bool basic_shape(const std::string& name, shape_t& out) {
        static const std::unordered_map<std::string, shape_t> table = {
            {"float", {uniform_base_t::Float, 1, 1}},
            {"vec2", {uniform_base_t::Float, 2, 1}},
            {"vec3", {uniform_base_t::Float, 3, 1}},
            {"vec4", {uniform_base_t::Float, 4, 1}},
            {"int", {uniform_base_t::Int, 1, 1}},
            {"ivec2", {uniform_base_t::Int, 2, 1}},
            {"ivec3", {uniform_base_t::Int, 3, 1}},
            {"ivec4", {uniform_base_t::Int, 4, 1}},
            {"uint", {uniform_base_t::Uint, 1, 1}},
            {"uvec2", {uniform_base_t::Uint, 2, 1}},
            {"uvec3", {uniform_base_t::Uint, 3, 1}},
            {"uvec4", {uniform_base_t::Uint, 4, 1}},
            {"bool", {uniform_base_t::Bool, 1, 1}},
            {"bvec2", {uniform_base_t::Bool, 2, 1}},
            {"bvec3", {uniform_base_t::Bool, 3, 1}},
            {"bvec4", {uniform_base_t::Bool, 4, 1}},
            // matCxR is C columns of R rows.
            {"mat2", {uniform_base_t::Float, 2, 2}},
            {"mat3", {uniform_base_t::Float, 3, 3}},
            {"mat4", {uniform_base_t::Float, 4, 4}},
            {"mat2x2", {uniform_base_t::Float, 2, 2}},
            {"mat2x3", {uniform_base_t::Float, 3, 2}},
            {"mat2x4", {uniform_base_t::Float, 4, 2}},
            {"mat3x2", {uniform_base_t::Float, 2, 3}},
            {"mat3x3", {uniform_base_t::Float, 3, 3}},
            {"mat3x4", {uniform_base_t::Float, 4, 3}},
            {"mat4x2", {uniform_base_t::Float, 2, 4}},
            {"mat4x3", {uniform_base_t::Float, 3, 4}},
            {"mat4x4", {uniform_base_t::Float, 4, 4}},
        };
        const auto it = table.find(name);
        if (it == table.end()) return false;
        out = it->second;
        return true;
    }

    bool is_qualifier(const std::string& w) {
        static const char* const words[] = {
            "highp",         "mediump",   "lowp",   "precise",   "invariant", "flat",     "smooth",
            "noperspective", "centroid",  "sample", "patch",     "coherent",  "volatile", "restrict",
            "readonly",      "writeonly", "layout", "const",     "in",        "out",      "inout",
            "uniform",       "buffer",    "shared", "attribute", "varying"};
        for (const char* q : words)
            if (w == q) return true;
        return false;
    }

    // GLSL scalar conversions, on values kept as doubles. Int and Uint convert into
    // each other by bit pattern, Float truncates, Bool is a zero test.
    double convert(double v, uniform_base_t from, uniform_base_t to) {
        if (from == to) return v;
        switch (to) {
        case uniform_base_t::Float:
            return from == uniform_base_t::Bool ? (v != 0.0 ? 1.0 : 0.0) : v;
        case uniform_base_t::Bool:
            return v != 0.0 ? 1.0 : 0.0;
        case uniform_base_t::Int:
        case uniform_base_t::Uint: {
            if (from == uniform_base_t::Bool) return v != 0.0 ? 1.0 : 0.0;
            if (!std::isfinite(v)) return 0.0;
            const auto wrapped = static_cast<uint32_t>(static_cast<uint64_t>(static_cast<int64_t>(std::trunc(v))));
            return to == uniform_base_t::Int ? static_cast<double>(static_cast<int32_t>(wrapped))
                                             : static_cast<double>(wrapped);
        }
        }
        return v;
    }

    // A declared type: its name plus array dimensions, outermost first. -1 is an
    // unsized dimension whose length comes from the initialiser.
    struct type_ref_t {
        std::string name;
        std::vector<int> dims;
    };

    struct member_t {
        type_ref_t type;
        std::string name;
    };

    struct struct_def_t {
        std::vector<member_t> members;
    };

    // A global `const` and the token range of its initialiser, for initialisers that
    // refer to one by name.
    struct const_def_t {
        size_t init_begin;
        size_t init_end;
    };

    // An evaluated constant expression.
    struct value_t {
        enum class kind_t {
            Basic,
            Array,
            Struct
        } kind = kind_t::Basic;
        shape_t shape{uniform_base_t::Float, 1, 1};
        std::vector<double> comps;  // Basic: rows * columns numbers, column-major
        std::vector<value_t> elems; // Array elements or Struct members, in order
    };

    // ---------------------------------------------------------------- parser ----

    class parser_t {
    public:
        parser_t(const std::string& src, std::vector<uniform_default_t>* out) : src_(src), toks_(lex(src)), out_(out) {}

        std::string run() {
            size_t i = 0;
            int braces = 0;
            int parens = 0;
            while (i < toks_.size()) {
                if (toks_[i].kind == tok_kind_t::Punct) {
                    switch (src_[toks_[i].begin]) {
                    case '{':
                        ++braces;
                        break;
                    case '}':
                        if (braces > 0) --braces;
                        break;
                    case '(':
                        ++parens;
                        break;
                    case ')':
                        if (parens > 0) --parens;
                        break;
                    default:
                        break;
                    }
                    ++i;
                    continue;
                }
                // Declarations live at global scope only; anything inside a function
                // body or a parameter list is left alone, whatever it is called.
                if (braces == 0 && parens == 0 && toks_[i].kind == tok_kind_t::Ident) {
                    const std::string w = text(i);
                    if (w == "uniform") {
                        i = handle_uniform(i);
                        continue;
                    }
                    if (w == "struct") {
                        i = handle_struct(i);
                        continue;
                    }
                    if (w == "const") {
                        i = handle_const(i);
                        continue;
                    }
                }
                ++i;
            }

            std::string result;
            result.reserve(src_.size());
            size_t pos = 0;
            for (const auto& r : removals_) {
                result.append(src_, pos, r.first - pos);
                pos = r.second;
            }
            result.append(src_, pos, std::string::npos);
            return result;
        }

    private:
        const std::string& src_;
        std::vector<token_t> toks_;
        std::vector<uniform_default_t>* out_;
        std::unordered_map<std::string, struct_def_t> structs_;
        std::unordered_map<std::string, const_def_t> consts_;
        std::vector<std::pair<size_t, size_t>> removals_; // byte ranges of `= <expr>`, in order

        // -- token helpers --

        bool at_end(size_t i) const { return i >= toks_.size(); }
        std::string text(size_t i) const { return src_.substr(toks_[i].begin, toks_[i].end - toks_[i].begin); }
        bool is_ident(size_t i) const { return !at_end(i) && toks_[i].kind == tok_kind_t::Ident; }
        bool is_number(size_t i) const { return !at_end(i) && toks_[i].kind == tok_kind_t::Number; }
        bool is_punct(size_t i, char c) const {
            return !at_end(i) && toks_[i].kind == tok_kind_t::Punct && src_[toks_[i].begin] == c;
        }
        static bool is_opener(char c) { return c == '(' || c == '[' || c == '{'; }
        static bool is_closer(char c) { return c == ')' || c == ']' || c == '}'; }

        // Index of the ';' that ends the statement starting at `i`, at the statement's
        // own nesting level, or the token count when the source runs out first. An
        // interface block's members sit one level down, so their ';' do not count.
        size_t statement_end(size_t i) const {
            int depth = 0;
            for (; i < toks_.size(); ++i) {
                if (toks_[i].kind != tok_kind_t::Punct) continue;
                const char c = src_[toks_[i].begin];
                if (is_opener(c)) {
                    ++depth;
                } else if (is_closer(c)) {
                    if (depth > 0) --depth;
                } else if (c == ';' && depth == 0) {
                    return i;
                }
            }
            return toks_.size();
        }

        // `i` is at an opener; returns the index after its matching closer.
        size_t skip_group(size_t i) const {
            int depth = 0;
            for (; i < toks_.size(); ++i) {
                if (toks_[i].kind != tok_kind_t::Punct) continue;
                const char c = src_[toks_[i].begin];
                if (is_opener(c))
                    ++depth;
                else if (is_closer(c) && --depth == 0)
                    return i + 1;
            }
            return toks_.size();
        }

        // Index of the ',' at the initialiser's own level that ends it, or `end`.
        size_t initializer_end(size_t i, size_t end) const {
            int depth = 0;
            for (; i < end; ++i) {
                if (toks_[i].kind != tok_kind_t::Punct) continue;
                const char c = src_[toks_[i].begin];
                if (is_opener(c))
                    ++depth;
                else if (is_closer(c)) {
                    if (depth > 0) --depth;
                } else if (c == ',' && depth == 0)
                    return i;
            }
            return end;
        }

        // Qualifiers between a storage keyword and its type; a layout qualifier takes
        // its parenthesised list with it.
        size_t skip_qualifiers(size_t i) const {
            while (is_ident(i) && is_qualifier(text(i))) {
                if (text(i) == "layout" && is_punct(i + 1, '('))
                    i = skip_group(i + 1);
                else
                    ++i;
            }
            return i;
        }

        // `[N]` / `[]` suffixes. A size that is not a literal is recorded as unsized:
        // the declaration still parses and the initialiser decides the length.
        bool parse_dims(size_t& i, std::vector<int>& dims) const {
            while (is_punct(i, '[')) {
                if (is_punct(i + 1, ']')) {
                    dims.push_back(-1);
                    i += 2;
                } else if (is_number(i + 1) && is_punct(i + 2, ']')) {
                    dims.push_back(static_cast<int>(std::strtol(text(i + 1).c_str(), nullptr, 0)));
                    i += 3;
                } else {
                    const size_t after = skip_group(i);
                    if (after == toks_.size()) return false;
                    dims.push_back(-1);
                    i = after;
                }
            }
            return true;
        }

        // -- global declarations --

        struct pending_t {
            size_t cut_begin, cut_end; // bytes to delete
            type_ref_t type;
            std::string name;
            size_t init_begin, init_end; // tokens of the expression
        };

        // Parses the declarator list shared by uniform and const declarations:
        //   name [dims] [= expr] {, name [dims] [= expr]} up to `end`.
        // Returns false on anything unexpected -- an interface block lands here on its
        // '{' -- and the caller then leaves the statement untouched.
        bool parse_declarators(size_t& i, size_t end, const type_ref_t& type, std::vector<pending_t>& found) const {
            while (true) {
                if (i >= end || !is_ident(i)) return false;
                pending_t p;
                p.name = text(i);
                ++i;
                // `float[2] x[3]` is x: 3 arrays of 2, so the declarator's dimensions are
                // the outer ones.
                std::vector<int> outer;
                if (!parse_dims(i, outer)) return false;
                p.type = type;
                p.type.dims.insert(p.type.dims.begin(), outer.begin(), outer.end());
                p.cut_begin = p.cut_end = p.init_begin = p.init_end = 0;
                if (is_punct(i, '=')) {
                    p.init_begin = i + 1;
                    p.init_end = initializer_end(p.init_begin, end);
                    if (p.init_end == p.init_begin) return false;
                    p.cut_begin = toks_[i - 1].end;
                    p.cut_end = toks_[p.init_end - 1].end;
                    i = p.init_end;
                }
                found.push_back(p);
                if (i == end) return true;
                if (!is_punct(i, ',')) return false;
                ++i;
            }
        }

        size_t handle_uniform(size_t i) {
            const size_t end = statement_end(i);
            const size_t next = end < toks_.size() ? end + 1 : end;
            size_t j = skip_qualifiers(i + 1);
            if (j >= end || !is_ident(j)) return next;
            type_ref_t type;
            type.name = text(j);
            ++j;
            if (!parse_dims(j, type.dims)) return next;
            std::vector<pending_t> found;
            if (!parse_declarators(j, end, type, found)) return next;
            for (const auto& p : found) {
                if (p.init_end == 0) continue;
                removals_.emplace_back(p.cut_begin, p.cut_end);
                if (out_) record(p);
            }
            return next;
        }

        // `struct Name { members };` -- remembered so a struct-typed uniform's default
        // can be split into member records.
        size_t handle_struct(size_t i) {
            const size_t end = statement_end(i);
            const size_t next = end < toks_.size() ? end + 1 : end;
            size_t j = i + 1;
            if (!is_ident(j)) return next;
            const std::string name = text(j);
            ++j;
            if (!is_punct(j, '{')) return next;
            ++j;
            struct_def_t def;
            while (j < end && !is_punct(j, '}')) {
                j = skip_qualifiers(j);
                if (j >= end || !is_ident(j)) return next;
                type_ref_t type;
                type.name = text(j);
                ++j;
                if (!parse_dims(j, type.dims)) return next;
                while (true) {
                    if (j >= end || !is_ident(j)) return next;
                    member_t m;
                    m.type = type;
                    m.name = text(j);
                    ++j;
                    std::vector<int> outer;
                    if (!parse_dims(j, outer)) return next;
                    m.type.dims.insert(m.type.dims.begin(), outer.begin(), outer.end());
                    def.members.push_back(m);
                    if (is_punct(j, ',')) {
                        ++j;
                        continue;
                    }
                    if (is_punct(j, ';')) {
                        ++j;
                        break;
                    }
                    return next;
                }
            }
            if (!is_punct(j, '}')) return next;
            structs_[name] = def;
            return next;
        }

        // `const T name = expr;` at global scope, for initialisers that name it.
        size_t handle_const(size_t i) {
            const size_t end = statement_end(i);
            const size_t next = end < toks_.size() ? end + 1 : end;
            size_t j = skip_qualifiers(i);
            if (j >= end || !is_ident(j)) return next;
            type_ref_t type;
            type.name = text(j);
            ++j;
            if (!parse_dims(j, type.dims)) return next;
            std::vector<pending_t> found;
            if (!parse_declarators(j, end, type, found)) return next;
            for (const auto& p : found)
                if (p.init_end != 0) consts_[p.name] = {p.init_begin, p.init_end};
            return next;
        }

        // -- constant evaluation --

        bool literal(size_t i, value_t& out) const {
            std::string t = text(i);
            out.kind = value_t::kind_t::Basic;
            out.shape = {uniform_base_t::Float, 1, 1};
            out.comps.assign(1, 0.0);
            const bool hex = t.size() > 1 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X');
            const bool has_u = !t.empty() && (t.back() == 'u' || t.back() == 'U');
            if (has_u) t.pop_back();
            bool is_float = false;
            if (!hex) {
                if (t.size() >= 2 && (t.compare(t.size() - 2, 2, "lf") == 0 || t.compare(t.size() - 2, 2, "LF") == 0)) {
                    t.erase(t.size() - 2);
                    is_float = true;
                } else if (!t.empty() && (t.back() == 'f' || t.back() == 'F')) {
                    t.pop_back();
                    is_float = true;
                }
                if (t.find_first_of(".eE") != std::string::npos) is_float = true;
            }
            char* stop = nullptr;
            if (is_float) {
                out.comps[0] = std::strtod(t.c_str(), &stop);
            } else {
                // Base 0 reads the 0x and leading-0 octal forms GLSL has.
                const unsigned long long v = std::strtoull(t.c_str(), &stop, 0);
                out.shape.base = has_u ? uniform_base_t::Uint : uniform_base_t::Int;
                out.comps[0] = convert(static_cast<double>(v), uniform_base_t::Uint, out.shape.base);
            }
            return stop != nullptr && *stop == '\0' && stop != t.c_str();
        }

        // Splits the argument list inside the parentheses [open, close] on the
        // commas at its own level.
        std::vector<std::pair<size_t, size_t>> split_args(size_t open, size_t close) const {
            std::vector<std::pair<size_t, size_t>> args;
            size_t b = open + 1;
            while (b < close) {
                const size_t e = initializer_end(b, close);
                args.emplace_back(b, e);
                b = e + 1;
            }
            return args;
        }

        // `T(args)` for a transparent type, with GLSL's constructor rules: a scalar
        // splats a vector and fills a matrix diagonal, a matrix argument is copied
        // over an identity, anything else is flattened column-major and consumed in
        // order.
        static bool construct_basic(const shape_t& target, const std::vector<value_t>& args, value_t& out) {
            out.kind = value_t::kind_t::Basic;
            out.shape = target;
            const int n = target.rows * target.columns;
            out.comps.assign(static_cast<size_t>(n), 0.0);
            for (const auto& a : args)
                if (a.kind != value_t::kind_t::Basic) return false;

            if (args.size() == 1) {
                const value_t& a = args[0];
                const bool scalar = a.comps.size() == 1;
                const bool matrix = a.shape.columns > 1;
                if (target.columns > 1 && scalar) {
                    for (int c = 0; c < target.columns; ++c)
                        for (int r = 0; r < target.rows; ++r)
                            out.comps[static_cast<size_t>(c * target.rows + r)] =
                                (r == c) ? convert(a.comps[0], a.shape.base, target.base) : 0.0;
                    return true;
                }
                if (target.columns > 1 && matrix) {
                    for (int c = 0; c < target.columns; ++c)
                        for (int r = 0; r < target.rows; ++r) {
                            double v = (r == c) ? 1.0 : 0.0;
                            if (c < a.shape.columns && r < a.shape.rows)
                                v = convert(a.comps[static_cast<size_t>(c * a.shape.rows + r)], a.shape.base,
                                            target.base);
                            out.comps[static_cast<size_t>(c * target.rows + r)] = v;
                        }
                    return true;
                }
                if (target.columns == 1 && scalar) {
                    for (int r = 0; r < target.rows; ++r)
                        out.comps[static_cast<size_t>(r)] = convert(a.comps[0], a.shape.base, target.base);
                    return true;
                }
            }

            std::vector<std::pair<double, uniform_base_t>> flat;
            for (const auto& a : args)
                for (double v : a.comps)
                    flat.emplace_back(v, a.shape.base);
            if (flat.size() < static_cast<size_t>(n)) return false;
            for (int k = 0; k < n; ++k)
                out.comps[static_cast<size_t>(k)] =
                    convert(flat[static_cast<size_t>(k)].first, flat[static_cast<size_t>(k)].second, target.base);
            return true;
        }

        // Bit reinterpretation between float and int/uint, which SPIRV-Cross uses to
        // spell infinities and NaNs.
        static bool bit_cast_call(const std::string& fn, const std::vector<value_t>& args, value_t& out) {
            if (args.size() != 1 || args[0].kind != value_t::kind_t::Basic || args[0].comps.size() != 1) return false;
            const value_t& a = args[0];
            out.kind = value_t::kind_t::Basic;
            out.comps.assign(1, 0.0);
            if (fn == "uintBitsToFloat" || fn == "intBitsToFloat") {
                const auto bits = static_cast<uint32_t>(convert(a.comps[0], a.shape.base, uniform_base_t::Uint));
                float f = 0.0f;
                std::memcpy(&f, &bits, sizeof f);
                out.shape = {uniform_base_t::Float, 1, 1};
                out.comps[0] = static_cast<double>(f);
                return true;
            }
            if (fn == "floatBitsToUint" || fn == "floatBitsToInt") {
                const auto f = static_cast<float>(convert(a.comps[0], a.shape.base, uniform_base_t::Float));
                uint32_t bits = 0;
                std::memcpy(&bits, &f, sizeof bits);
                const uniform_base_t to = fn == "floatBitsToUint" ? uniform_base_t::Uint : uniform_base_t::Int;
                out.shape = {to, 1, 1};
                out.comps[0] = convert(static_cast<double>(bits), uniform_base_t::Uint, to);
                return true;
            }
            return false;
        }

        // Evaluates the expression in tokens [b, e). Handles what SPIRV-Cross emits
        // for a constant: literals, true/false, unary sign, parentheses, constructors
        // of transparent types, arrays and structs, the float/int bit casts, and the
        // name of a global const. Arithmetic is not folded; a false return simply
        // means no default is recorded.
        bool eval(size_t b, size_t e, value_t& out, int depth) const {
            if (b >= e || depth > 16) return false;

            if (is_punct(b, '-') || is_punct(b, '+')) {
                if (!eval(b + 1, e, out, depth + 1)) return false;
                if (is_punct(b, '+')) return true;
                if (out.kind != value_t::kind_t::Basic) return false;
                for (double& v : out.comps) {
                    if (out.shape.base == uniform_base_t::Uint)
                        v = convert(-v, uniform_base_t::Int, uniform_base_t::Uint);
                    else if (out.shape.base == uniform_base_t::Bool)
                        return false;
                    else
                        v = -v;
                }
                return true;
            }

            if (is_punct(b, '(')) {
                if (skip_group(b) != e) return false;
                return eval(b + 1, e - 1, out, depth + 1);
            }

            if (is_number(b)) return b + 1 == e && literal(b, out);

            if (!is_ident(b)) return false;
            const std::string name = text(b);

            if (b + 1 == e) {
                if (name == "true" || name == "false") {
                    out.kind = value_t::kind_t::Basic;
                    out.shape = {uniform_base_t::Bool, 1, 1};
                    out.comps.assign(1, name == "true" ? 1.0 : 0.0);
                    return true;
                }
                const auto it = consts_.find(name);
                if (it == consts_.end()) return false;
                return eval(it->second.init_begin, it->second.init_end, out, depth + 1);
            }

            // T[N]...(args): an array constructor, one element per argument.
            if (is_punct(b + 1, '[')) {
                size_t j = b + 1;
                std::vector<int> dims;
                if (!parse_dims(j, dims) || dims.empty() || !is_punct(j, '(') || skip_group(j) != e) return false;
                out.kind = value_t::kind_t::Array;
                out.comps.clear();
                out.elems.clear();
                for (const auto& arg : split_args(j, e - 1)) {
                    value_t v;
                    if (!eval(arg.first, arg.second, v, depth + 1)) return false;
                    out.elems.push_back(std::move(v));
                }
                if (dims[0] >= 0 && static_cast<size_t>(dims[0]) != out.elems.size()) return false;
                return true;
            }

            if (!is_punct(b + 1, '(') || skip_group(b + 1) != e) return false;
            std::vector<value_t> args;
            for (const auto& arg : split_args(b + 1, e - 1)) {
                value_t v;
                if (!eval(arg.first, arg.second, v, depth + 1)) return false;
                args.push_back(std::move(v));
            }

            shape_t shape;
            if (basic_shape(name, shape)) return construct_basic(shape, args, out);
            const auto st = structs_.find(name);
            if (st != structs_.end()) {
                if (args.size() != st->second.members.size()) return false;
                out.kind = value_t::kind_t::Struct;
                out.comps.clear();
                out.elems = std::move(args);
                return true;
            }
            return bit_cast_call(name, args, out);
        }

        // -- turning a value into records --

        static bool same_shape(const value_t& v, const shape_t& s) {
            return v.kind == value_t::kind_t::Basic && v.shape.rows == s.rows && v.shape.columns == s.columns;
        }

        static void append_converted(const value_t& v, uniform_base_t to, std::vector<double>& into) {
            for (double c : v.comps)
                into.push_back(convert(c, v.shape.base, to));
        }

        // Walks the declared type and the value side by side and emits one record
        // per addressable leaf. Any mismatch between the two means the initialiser was
        // not what the declaration says, and nothing is recorded for that branch.
        void expand(const type_ref_t& type, const value_t& v, const std::string& name) {
            if (!type.dims.empty()) {
                if (v.kind != value_t::kind_t::Array) return;
                const int declared = type.dims[0];
                if (declared >= 0 && static_cast<size_t>(declared) != v.elems.size()) return;
                if (v.elems.empty()) return;
                type_ref_t elem = type;
                elem.dims.erase(elem.dims.begin());
                shape_t s;
                if (elem.dims.empty() && basic_shape(elem.name, s)) {
                    // The innermost array of a transparent type is one record, set
                    // with a single glUniform*v call.
                    uniform_default_t d;
                    d.name = name;
                    d.base = s.base;
                    d.rows = s.rows;
                    d.columns = s.columns;
                    d.count = static_cast<int>(v.elems.size());
                    for (const auto& el : v.elems) {
                        if (!same_shape(el, s)) return;
                        append_converted(el, s.base, d.values);
                    }
                    out_->push_back(std::move(d));
                    return;
                }
                for (size_t k = 0; k < v.elems.size(); ++k)
                    expand(elem, v.elems[k], name + "[" + std::to_string(k) + "]");
                return;
            }

            shape_t s;
            if (basic_shape(type.name, s)) {
                if (!same_shape(v, s)) return;
                uniform_default_t d;
                d.name = name;
                d.base = s.base;
                d.rows = s.rows;
                d.columns = s.columns;
                d.count = 1;
                append_converted(v, s.base, d.values);
                out_->push_back(std::move(d));
                return;
            }

            const auto st = structs_.find(type.name);
            if (st == structs_.end() || v.kind != value_t::kind_t::Struct) return;
            const auto& members = st->second.members;
            if (v.elems.size() != members.size()) return;
            for (size_t k = 0; k < members.size(); ++k)
                expand(members[k].type, v.elems[k], name + "." + members[k].name);
        }

        void record(const pending_t& p) {
            value_t v;
            if (!eval(p.init_begin, p.init_end, v, 0)) return;
            expand(p.type, v, p.name);
        }
    };

} // namespace

std::string process_uniform_declarations(const std::string& essl, std::vector<uniform_default_t>* defaults) {
    if (defaults) defaults->clear();
    return parser_t(essl, defaults).run();
}
