// Host harness for process_uniform_declarations. Links gl/glsl/uniform_defaults.cpp
// itself rather than a copy, so the test cannot drift away from the code it checks.
//
// The inputs are what SPIRV-Cross emits in ES mode, plus the shapes the previous
// implementation got wrong: identifiers that merely contain "uniform", arrays,
// second declarators, interface blocks, and type names that start with a
// precision keyword.
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>

#include "gl/glsl/uniform_defaults.h"

static int fails = 0;

static void eq(const char* what, const std::string& got, const std::string& want) {
    if (got != want) {
        printf("  FAIL %s\n    got : %s\n    want: %s\n", what, got.c_str(), want.c_str());
        ++fails;
    }
}

static void eq(const char* what, long got, long want) {
    if (got != want) {
        printf("  FAIL %-50s got %ld want %ld\n", what, got, want);
        ++fails;
    }
}

// Text is unchanged, and no defaults are reported.
static void untouched(const char* what, const char* src) {
    std::vector<uniform_default_t> d;
    eq(what, process_uniform_declarations(src, &d), src);
    eq((std::string(what) + " reports nothing").c_str(), static_cast<long>(d.size()), 0);
}

static std::string values(const uniform_default_t& d) {
    std::string s;
    for (double v : d.values) {
        char buf[64];
        if (d.base == uniform_base_t::Float)
            snprintf(buf, sizeof buf, "%g", v);
        else
            snprintf(buf, sizeof buf, "%lld", static_cast<long long>(v));
        if (!s.empty()) s += ' ';
        s += buf;
    }
    return s;
}

static const char* base_name(uniform_base_t b) {
    switch (b) {
    case uniform_base_t::Float:
        return "float";
    case uniform_base_t::Int:
        return "int";
    case uniform_base_t::Uint:
        return "uint";
    case uniform_base_t::Bool:
        return "bool";
    }
    return "?";
}

// One record, printed as "name base rows columns count: values".
static std::string describe(const uniform_default_t& d) {
    return d.name + " " + base_name(d.base) + " " + std::to_string(d.rows) + "x" + std::to_string(d.columns) + " [" +
           std::to_string(d.count) + "]: " + values(d);
}

static void defaults(const char* what, const char* src, const std::vector<std::string>& want) {
    std::vector<uniform_default_t> d;
    process_uniform_declarations(src, &d);
    std::string got, exp;
    for (const auto& r : d)
        got += describe(r) + "\n";
    for (const auto& w : want)
        exp += w + "\n";
    eq(what, got, exp);
}

int main() {
    printf("stripping initialisers\n");
    eq("scalar", process_uniform_declarations("uniform float x = 1.0;"), "uniform float x;");
    eq("precision", process_uniform_declarations("uniform highp float x = 1.0;"), "uniform highp float x;");
    eq("layout before", process_uniform_declarations("layout(location = 0) uniform highp vec3 t = vec3(1.0);"),
       "layout(location = 0) uniform highp vec3 t;");
    eq("layout after", process_uniform_declarations("uniform layout(location = 1) float x = 1.0;"),
       "uniform layout(location = 1) float x;");
    eq("array keeps its size", process_uniform_declarations("uniform highp float w[3] = float[](0.25, 0.5, 0.25);"),
       "uniform highp float w[3];");
    eq("array on the type", process_uniform_declarations("uniform float[3] w = float[](0.25, 0.5, 0.25);"),
       "uniform float[3] w;");
    eq("two declarators", process_uniform_declarations("uniform float a = 1.0, b = 2.0;"), "uniform float a, b;");
    eq("second declarator only", process_uniform_declarations("uniform float a, b = 2.0;"), "uniform float a, b;");
    eq("nested constructor with commas",
       process_uniform_declarations("uniform mat2 m = mat2(vec2(1.0, 2.0), vec2(3.0, 4.0)), n = mat2(1.0);"),
       "uniform mat2 m, n;");
    eq("type starting with a precision word", process_uniform_declarations("uniform lowpoly_t p = lowpoly_t(1.0);"),
       "uniform lowpoly_t p;");
    eq("no trailing semicolon", process_uniform_declarations("uniform float x = 1.0"), "uniform float x");
    eq("newlines inside", process_uniform_declarations("uniform\n  highp vec2\n  v\n  =\n  vec2(1.0,\n 2.0)\n;"),
       "uniform\n  highp vec2\n  v\n;");

    printf("leaving other text alone\n");
    untouched("no initialiser", "uniform highp sampler2D tex;");
    untouched("identifier containing uniform", "void main() { float nonuniformScale = 2.0; }");
    untouched("nonuniformEXT lvalue", "void main() { data[nonuniformEXT(i)] = 1.0; }");
    untouched("global named uniforms", "vec4 uniforms = vec4(0.0);");
    untouched("global named uniformScale", "float uniformScale = 2.0;");
    untouched("interface block with layout(offset=)",
              "layout(std140) uniform B { layout(offset = 16) float x; float y; } b;");
    untouched("interface block instance named uniforms", "layout(std140) uniform Uniforms { mat4 m; } uniforms;");
    untouched("line comment", "// uniform note = 1;\nuniform float x;");
    untouched("block comment", "/* uniform note = 1; */ uniform float x;");
    untouched("preprocessor line", "#define X uniform float x = 1.0;\nuniform float y;");
    untouched("continued preprocessor line", "#define X uniform \\\n float x = 1.0;\nuniform float y;");
    untouched("assignment in a body", "uniform float x;\nvoid main() { float k = 1.0; }");
    untouched("function parameter named const", "float f(const in float uniform_in) { return uniform_in; }");

    printf("reported defaults\n");
    defaults("float", "uniform float exposure = 1.5;", {"exposure float 1x1 [1]: 1.5"});
    defaults("vec3", "uniform highp vec3 tint = vec3(1.0, 0.5, 0.25);", {"tint float 3x1 [1]: 1 0.5 0.25"});
    defaults("splat", "uniform vec4 c = vec4(2.0);", {"c float 4x1 [1]: 2 2 2 2"});
    defaults("int and uint", "uniform int i = -3; uniform uint u = 7u;", {"i int 1x1 [1]: -3", "u uint 1x1 [1]: 7"});
    defaults("hex and octal", "uniform uint h = 0xffu; uniform int o = 010;",
             {"h uint 1x1 [1]: 255", "o int 1x1 [1]: 8"});
    defaults("bool", "uniform bool b = true; uniform bvec2 bb = bvec2(true, false);",
             {"b bool 1x1 [1]: 1", "bb bool 2x1 [1]: 1 0"});
    defaults("float array", "uniform highp float weights[3] = float[](0.25, 0.5, 0.25);",
             {"weights float 1x1 [3]: 0.25 0.5 0.25"});
    defaults("sized array constructor", "uniform vec2 pts[2] = vec2[2](vec2(1.0, 2.0), vec2(3.0, 4.0));",
             {"pts float 2x1 [2]: 1 2 3 4"});
    defaults("unsized declaration", "uniform float w[] = float[](1.0, 2.0);", {"w float 1x1 [2]: 1 2"});
    defaults("matrix from columns", "uniform mat2 m = mat2(vec2(1.0, 2.0), vec2(3.0, 4.0));",
             {"m float 2x2 [1]: 1 2 3 4"});
    defaults("matrix diagonal", "uniform mat3 m = mat3(2.0);", {"m float 3x3 [1]: 2 0 0 0 2 0 0 0 2"});
    defaults("mat2x3 from scalars", "uniform mat2x3 m = mat2x3(1.0, 2.0, 3.0, 4.0, 5.0, 6.0);",
             {"m float 3x2 [1]: 1 2 3 4 5 6"});
    defaults("matrix from matrix", "uniform mat3 m = mat3(mat2(vec2(1.0, 2.0), vec2(3.0, 4.0)));",
             {"m float 3x3 [1]: 1 2 0 3 4 0 0 0 1"});
    defaults("vec4 from vec3 and scalar", "uniform vec4 c = vec4(vec3(1.0, 2.0, 3.0), 4.0);",
             {"c float 4x1 [1]: 1 2 3 4"});
    defaults("conversions",
             "uniform ivec2 v = ivec2(vec2(1.9, -2.9)); uniform vec2 f = vec2(ivec2(1, 2)); uniform uint w = uint(-1);",
             {"v int 2x1 [1]: 1 -2", "f float 2x1 [1]: 1 2", "w uint 1x1 [1]: 4294967295"});
    defaults("negative float", "uniform float x = -0.5;", {"x float 1x1 [1]: -0.5"});
    defaults("exponent", "uniform float x = 1e-3;", {"x float 1x1 [1]: 0.001"});
    defaults("parenthesised", "uniform float x = (2.5);", {"x float 1x1 [1]: 2.5"});
    defaults("second declarator", "uniform float a = 1.0, b = 2.0;", {"a float 1x1 [1]: 1", "b float 1x1 [1]: 2"});
    defaults("global const reference", "const vec3 _22 = vec3(1.0, 0.5, 0.25);\nuniform vec3 tint = _22;",
             {"tint float 3x1 [1]: 1 0.5 0.25"});
    defaults("struct split into members",
             "struct Light\n{\n    highp vec3 color;\n    highp float intensity;\n};\n"
             "uniform Light light = Light(vec3(1.0, 0.9, 0.8), 2.0);",
             {"light.color float 3x1 [1]: 1 0.9 0.8", "light.intensity float 1x1 [1]: 2"});
    defaults("array of structs",
             "struct S { float a; vec2 b[2]; };\n"
             "uniform S s[2] = S[](S(1.0, vec2[](vec2(1.0), vec2(2.0))), S(3.0, vec2[](vec2(4.0), vec2(5.0))));",
             {"s[0].a float 1x1 [1]: 1", "s[0].b float 2x1 [2]: 1 1 2 2", "s[1].a float 1x1 [1]: 3",
              "s[1].b float 2x1 [2]: 4 4 5 5"});
    defaults("nested struct", "struct A { float x; }; struct B { A a; int n; };\nuniform B b = B(A(1.0), 2);",
             {"b.a.x float 1x1 [1]: 1", "b.n int 1x1 [1]: 2"});
    defaults("array of arrays", "uniform float g[2][2] = float[][](float[](1.0, 2.0), float[](3.0, 4.0));",
             {"g[0] float 1x1 [2]: 1 2", "g[1] float 1x1 [2]: 3 4"});

    printf("initialisers that cannot be evaluated are still removed\n");
    {
        std::vector<uniform_default_t> d;
        eq("arithmetic stripped", process_uniform_declarations("uniform float x = 1.0 + 2.0;", &d), "uniform float x;");
        eq("arithmetic not reported", static_cast<long>(d.size()), 0);
        eq("unknown function stripped", process_uniform_declarations("uniform float x = sin(1.0);", &d),
           "uniform float x;");
        eq("unknown function not reported", static_cast<long>(d.size()), 0);
        eq("array length mismatch stripped",
           process_uniform_declarations("uniform float w[3] = float[](1.0, 2.0);", &d), "uniform float w[3];");
        eq("array length mismatch not reported", static_cast<long>(d.size()), 0);
        eq("shape mismatch stripped", process_uniform_declarations("uniform vec3 v = vec2(1.0);", &d),
           "uniform vec3 v;");
        eq("shape mismatch not reported", static_cast<long>(d.size()), 0);
    }

    printf("bit casts\n");
    {
        std::vector<uniform_default_t> d;
        process_uniform_declarations("uniform float inf = uintBitsToFloat(0x7f800000u);", &d);
        eq("uintBitsToFloat reported", static_cast<long>(d.size()), 1);
        if (d.size() == 1) eq("uintBitsToFloat is inf", std::isinf(d[0].values[0]) ? 1 : 0, 1);
    }

    printf("a whole SPIRV-Cross fragment shader\n");
    {
        const char* essl = "#version 320 es\n"
                           "precision mediump float;\n"
                           "precision highp int;\n"
                           "\n"
                           "layout(location = 5) uniform highp float nonuniformScale;\n"
                           "layout(location = 2) uniform highp float weights[3] = float[](0.25, 0.5, 0.25);\n"
                           "layout(location = 6, binding = 0) uniform highp sampler2D tex;\n"
                           "layout(location = 1) uniform highp vec3 tint = vec3(1.0, 0.5, 0.25);\n"
                           "layout(location = 0) uniform highp float exposure = 1.5;\n"
                           "\n"
                           "layout(location = 0) out highp vec4 fragColor;\n"
                           "layout(location = 0) in highp vec2 uv;\n"
                           "\n"
                           "void main()\n"
                           "{\n"
                           "    highp float s = nonuniformScale;\n"
                           "    highp float w = (weights[0] + weights[1]) + weights[2];\n"
                           "    fragColor = (((texture(tex, uv) * vec4(tint, 1.0)) * exposure) * w) * s;\n"
                           "}\n";
        const char* want = "#version 320 es\n"
                           "precision mediump float;\n"
                           "precision highp int;\n"
                           "\n"
                           "layout(location = 5) uniform highp float nonuniformScale;\n"
                           "layout(location = 2) uniform highp float weights[3];\n"
                           "layout(location = 6, binding = 0) uniform highp sampler2D tex;\n"
                           "layout(location = 1) uniform highp vec3 tint;\n"
                           "layout(location = 0) uniform highp float exposure;\n"
                           "\n"
                           "layout(location = 0) out highp vec4 fragColor;\n"
                           "layout(location = 0) in highp vec2 uv;\n"
                           "\n"
                           "void main()\n"
                           "{\n"
                           "    highp float s = nonuniformScale;\n"
                           "    highp float w = (weights[0] + weights[1]) + weights[2];\n"
                           "    fragColor = (((texture(tex, uv) * vec4(tint, 1.0)) * exposure) * w) * s;\n"
                           "}\n";
        std::vector<uniform_default_t> d;
        eq("shader text", process_uniform_declarations(essl, &d), want);
        std::string got;
        for (const auto& r : d)
            got += describe(r) + "\n";
        eq("shader defaults", got,
           "weights float 1x1 [3]: 0.25 0.5 0.25\n"
           "tint float 3x1 [1]: 1 0.5 0.25\n"
           "exposure float 1x1 [1]: 1.5\n");
        eq("idempotent", process_uniform_declarations(want), want);
    }

    if (fails) {
        printf("%d failure(s)\n", fails);
        return 1;
    }
    printf("all passed\n");
    return 0;
}
