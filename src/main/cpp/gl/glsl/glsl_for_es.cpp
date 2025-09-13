#include "glsl_for_es.h"

#include <glslang/Public/ShaderLang.h>
#include <glslang/Include/Types.h>
#include <glslang/Public/ShaderLang.h>
#include <spirv_cross/spirv_cross_c.h>
#include <iostream>
#include <fstream>
#include "../log.h"
#include "glslang/SPIRV/GlslangToSpv.h"
#include <string>
#include <regex>
#include <strstream>
#include <algorithm>
#include <sstream>
#include "cache.h"
#include "../../version.h"

#define DEBUG 0	

const char* atomicCounterEmulatedWatermark = "// Non-opaque atomic uniform converted to SSBO";

#if !defined(__APPLE__)
char* (*MesaConvertShader)(const char *src, unsigned int type, unsigned int glsl, unsigned int essl);
#endif

static TBuiltInResource InitResources()
{
    TBuiltInResource Resources{};
    Resources.maxLights = 32;
    Resources.maxClipPlanes                             = 6;
    Resources.maxTextureUnits                           = 32;
    Resources.maxTextureCoords                          = 32;
    Resources.maxVertexAttribs                          = 64;
    Resources.maxVertexUniformComponents                = 4096;
    Resources.maxVaryingFloats                          = 64;
    Resources.maxVertexTextureImageUnits                = 32;
    Resources.maxCombinedTextureImageUnits              = 80;
    Resources.maxTextureImageUnits                      = 32;
    Resources.maxFragmentUniformComponents              = 4096;
    Resources.maxDrawBuffers                            = 32;
    Resources.maxVertexUniformVectors                   = 128;
    Resources.maxVaryingVectors                         = 8;
    Resources.maxFragmentUniformVectors                 = 16;
    Resources.maxVertexOutputVectors                    = 16;
    Resources.maxFragmentInputVectors                   = 15;
    Resources.minProgramTexelOffset                     = -8;
    Resources.maxProgramTexelOffset                     = 7;
    Resources.maxClipDistances                          = 8;
    Resources.maxComputeWorkGroupCountX                 = 65535;
    Resources.maxComputeWorkGroupCountY                 = 65535;
    Resources.maxComputeWorkGroupCountZ                 = 65535;
    Resources.maxComputeWorkGroupSizeX                  = 1024;
    Resources.maxComputeWorkGroupSizeY                  = 1024;
    Resources.maxComputeWorkGroupSizeZ                  = 64;
    Resources.maxComputeUniformComponents               = 1024;
    Resources.maxComputeTextureImageUnits               = 16;
    Resources.maxComputeImageUniforms                   = 8;
    Resources.maxComputeAtomicCounters                  = 8;
    Resources.maxComputeAtomicCounterBuffers            = 1;
    Resources.maxVaryingComponents                      = 60;
    Resources.maxVertexOutputComponents                 = 64;
    Resources.maxGeometryInputComponents                = 64;
    Resources.maxGeometryOutputComponents               = 128;
    Resources.maxFragmentInputComponents                = 128;
    Resources.maxImageUnits                             = 8;
    Resources.maxCombinedImageUnitsAndFragmentOutputs   = 8;
    Resources.maxCombinedShaderOutputResources          = 8;
    Resources.maxImageSamples                           = 0;
    Resources.maxVertexImageUniforms                    = 0;
    Resources.maxTessControlImageUniforms               = 0;
    Resources.maxTessEvaluationImageUniforms            = 0;
    Resources.maxGeometryImageUniforms                  = 0;
    Resources.maxFragmentImageUniforms                  = 8;
    Resources.maxCombinedImageUniforms                  = 8;
    Resources.maxGeometryTextureImageUnits              = 16;
    Resources.maxGeometryOutputVertices                 = 256;
    Resources.maxGeometryTotalOutputComponents          = 1024;
    Resources.maxGeometryUniformComponents              = 1024;
    Resources.maxGeometryVaryingComponents              = 64;
    Resources.maxTessControlInputComponents             = 128;
    Resources.maxTessControlOutputComponents            = 128;
    Resources.maxTessControlTextureImageUnits           = 16;
    Resources.maxTessControlUniformComponents           = 1024;
    Resources.maxTessControlTotalOutputComponents       = 4096;
    Resources.maxTessEvaluationInputComponents          = 128;
    Resources.maxTessEvaluationOutputComponents         = 128;
    Resources.maxTessEvaluationTextureImageUnits        = 16;
    Resources.maxTessEvaluationUniformComponents        = 1024;
    Resources.maxTessPatchComponents                    = 120;
    Resources.maxPatchVertices                          = 32;
    Resources.maxTessGenLevel                           = 64;
    Resources.maxViewports                              = 16;
    Resources.maxVertexAtomicCounters                   = 0;
    Resources.maxTessControlAtomicCounters              = 0;
    Resources.maxTessEvaluationAtomicCounters           = 0;
    Resources.maxGeometryAtomicCounters                 = 0;
    Resources.maxFragmentAtomicCounters                 = 8;
    Resources.maxCombinedAtomicCounters                 = 8;
    Resources.maxAtomicCounterBindings                  = 1;
    Resources.maxVertexAtomicCounterBuffers             = 0;
    Resources.maxTessControlAtomicCounterBuffers        = 0;
    Resources.maxTessEvaluationAtomicCounterBuffers     = 0;
    Resources.maxGeometryAtomicCounterBuffers           = 0;
    Resources.maxFragmentAtomicCounterBuffers           = 1;
    Resources.maxCombinedAtomicCounterBuffers           = 1;
    Resources.maxAtomicCounterBufferSize                = 16384;
    Resources.maxTransformFeedbackBuffers               = 4;
    Resources.maxTransformFeedbackInterleavedComponents = 64;
    Resources.maxCullDistances                          = 8;
    Resources.maxCombinedClipAndCullDistances           = 8;
    Resources.maxSamples                                = 4;
    Resources.maxMeshOutputVerticesNV                   = 256;
    Resources.maxMeshOutputPrimitivesNV                 = 512;
    Resources.maxMeshWorkGroupSizeX_NV                  = 32;
    Resources.maxMeshWorkGroupSizeY_NV                  = 1;
    Resources.maxMeshWorkGroupSizeZ_NV                  = 1;
    Resources.maxTaskWorkGroupSizeX_NV                  = 32;
    Resources.maxTaskWorkGroupSizeY_NV                  = 1;
    Resources.maxTaskWorkGroupSizeZ_NV                  = 1;
    Resources.maxMeshViewCountNV                        = 4;
    Resources.limits.nonInductiveForLoops = true;
    Resources.limits.whileLoops = true;
    Resources.limits.doWhileLoops = true;
    Resources.limits.generalUniformIndexing = true;
    Resources.limits.generalAttributeMatrixVectorIndexing = true;
    Resources.limits.generalVaryingIndexing = true;
    Resources.limits.generalSamplerIndexing = true;
    Resources.limits.generalVariableIndexing = true;
    Resources.limits.generalConstantMatrixVectorIndexing = true;
    return Resources;
}

inline int getGLSLVersion(const char* glsl_code) {
    static const std::regex version_pattern(R"(#version\s+(\d{3}))", std::regex::optimize);
    std::string code(glsl_code);
    std::smatch match;
    if (std::regex_search(code, match, version_pattern)) {
        return std::stoi(match[1].str());
    }
    return -1;
}

inline std::string forceSupporterOutput(const std::string& glslCode) {
    bool hasPrecisionFloat = glslCode.find("precision ") != std::string::npos &&
                             glslCode.find("float;") != std::string::npos;
    bool hasPrecisionInt = glslCode.find("precision ") != std::string::npos &&
                           glslCode.find("int;") != std::string::npos;

    std::string result = glslCode;
    std::string precisionFloat, precisionInt;

    if (hasPrecisionFloat && hasPrecisionInt) {
        std::istringstream iss(result);
        std::vector<std::string> lines;
        std::string line;
        lines.reserve(128);
        while (std::getline(iss, line)) {
            bool isPrecisionLine = (line.find("precision ") != std::string::npos) &&
                                   (line.find("float;") != std::string::npos || line.find("int;") != std::string::npos);
            if (!isPrecisionLine) lines.push_back(line);
        }
        result.clear();
        for (size_t i = 0; i < lines.size(); ++i) {
            if (i != 0) result += '\n';
            result += lines[i];
        }
        precisionFloat = "precision highp float;\n";
        precisionInt = "precision highp int;\n";
    } else {
        precisionFloat = hasPrecisionFloat ? "" : "precision highp float;\n";
        precisionInt = hasPrecisionInt ? "" : "precision highp int;\n";
    }
    size_t lastExtensionPos = result.rfind("#extension");
    size_t insertionPos = 0;
    if (lastExtensionPos != std::string::npos) {
        size_t nextNewline = result.find('\n', lastExtensionPos);
        insertionPos = (nextNewline != std::string::npos) ? nextNewline + 1 : result.length();
    } else {
        size_t firstNewline = result.find('\n');
        if (firstNewline != std::string::npos) {
            insertionPos = firstNewline + 1;
        } else {
            result = precisionFloat + precisionInt + result;
            return result;
        }
    }
    result.insert(insertionPos, precisionFloat + precisionInt);
    return result;
}

inline std::string removeLayoutBinding(const std::string& glslCode) {
    static const std::regex bindingRegex1(R"(layout\s*\(\s*binding\s*=\s*\d+\s*\)\s*)", std::regex::optimize);
    static const std::regex bindingRegex2(R"(layout\s*\(\s*binding\s*=\s*\d+\s*,)", std::regex::optimize);
    std::string result = std::regex_replace(glslCode, bindingRegex1, "");
    result = std::regex_replace(result, bindingRegex2, "layout(");
    return result;
}

// 更高效的 trim
inline void trim(std::string& str) {
    auto left = std::find_if_not(str.begin(), str.end(), ::isspace);
    auto right = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();
    if (left < right) str = std::string(left, right); else str.clear();
}

// uniform 处理
std::string process_uniform_declarations(const std::string& glslCode) {
    static const std::vector<std::string> precision_kws = {"highp", "lowp", "mediump"};
    std::string result;
    size_t scan_pos = 0, chunk_start = 0, length = glslCode.length();
    result.reserve(length);

    while (scan_pos < length) {
        if (glslCode.compare(scan_pos, 7, "uniform") == 0) {
            if (scan_pos > chunk_start)
                result.append(glslCode, chunk_start, scan_pos - chunk_start);

            const size_t decl_start = scan_pos;
            scan_pos += 7;  // Skip "uniform"

            std::string precision, type;
            bool found_precision = false;
            while (scan_pos < length && std::isspace(glslCode[scan_pos])) ++scan_pos;

            for (const auto& kw : precision_kws) {
                if (glslCode.compare(scan_pos, kw.length(), kw) == 0) {
                    precision = " " + kw;
                    scan_pos += kw.length();
                    found_precision = true;
                    break;
                }
            }
            if (!found_precision) {
                const size_t type_start = scan_pos;
                while (scan_pos < length && (std::isalnum(glslCode[scan_pos]) || glslCode[scan_pos] == '_')) ++scan_pos;
                type = glslCode.substr(type_start, scan_pos - type_start);
            }
            // 第二轮解析精度
            while (scan_pos < length && std::isspace(glslCode[scan_pos])) ++scan_pos;
            for (const auto& kw : precision_kws) {
                if (glslCode.compare(scan_pos, kw.length(), kw) == 0) {
                    if (precision.empty()) precision = " " + kw;
                    scan_pos += kw.length();
                    break;
                }
            }
            if (type.empty()) {
                const size_t type_start = scan_pos;
                while (scan_pos < length && (std::isalnum(glslCode[scan_pos]) || glslCode[scan_pos] == '_')) ++scan_pos;
                type = glslCode.substr(type_start, scan_pos - type_start);
            }
            while (scan_pos < length && std::isspace(glslCode[scan_pos])) ++scan_pos;
            const size_t name_start = scan_pos;
            while (scan_pos < length && (std::isalnum(glslCode[scan_pos]) || glslCode[scan_pos] == '_')) ++scan_pos;
            const std::string name = glslCode.substr(name_start, scan_pos - name_start);

            size_t decl_end = glslCode.find(';', scan_pos);
            if (decl_end == std::string::npos) decl_end = length; else ++decl_end;
            bool has_initializer = (glslCode.find('=', scan_pos) < decl_end);
            if (has_initializer) {
                result.append("uniform").append(precision).append(" ").append(type).append(" ").append(name).append(";");
            } else {
                result.append(glslCode, decl_start, decl_end - decl_start);
            }
            scan_pos = chunk_start = decl_end;
        } else {
            ++scan_pos;
        }
    }
    if (chunk_start < length) result.append(glslCode, chunk_start, length - chunk_start);
    return result;
}

std::string processOutColorLocations(const std::string& glslCode) {
    static const std::regex pattern(R"(\n(out highp vec4 outColor)(\d+);)", std::regex::optimize);
    static const std::string replacement = "\nlayout(location=$2) $1$2;";
    return std::regex_replace(glslCode, pattern, replacement);
}

inline bool checkIfAtomicCounterBufferEmulated(const std::string& glslCode) {
    return glslCode.find(atomicCounterEmulatedWatermark) != std::string::npos;
}

std::string GLSLtoGLSLES(const char* glsl_code, GLenum glsl_type, uint essl_version, uint glsl_version, int& return_code) {
    std::string sha256_string(glsl_code);
    sha256_string += "\n//" + std::to_string(MAJOR) + "." + std::to_string(MINOR) + "." + std::to_string(REVISION) + "|" + std::to_string(essl_version);
    const char* cachedESSL = Cache::get_instance().get(sha256_string.c_str());
    if (cachedESSL) {
        LOG_D("GLSL Hit Cache:\n%s\n-->\n%s", glsl_code, cachedESSL)
        bool atomicCounterEmulated = checkIfAtomicCounterBufferEmulated(std::string(cachedESSL));
        return_code = atomicCounterEmulated ? 1 : 0;
        return (char*)cachedESSL;
    }
    return_code = -1;
    std::string converted = glsl_version < 140 ? GLSLtoGLSLES_1(glsl_code, glsl_type, essl_version, return_code)
                                               : GLSLtoGLSLES_2(glsl_code, glsl_type, essl_version, return_code);
    if (return_code >= 0 && !converted.empty()) {
        converted = process_uniform_declarations(converted);
        Cache::get_instance().put(sha256_string.c_str(), converted.c_str());
    }
    return (return_code >= 0) ? converted : glsl_code;
}

// 行替换优化
inline std::string replace_line_starting_with(const std::string& glslCode, const std::string& starting, const std::string& substitution = "") {
    std::istringstream in(glslCode);
    std::ostringstream out;
    std::string line;
    while (std::getline(in, line)) {
        if (line.find(starting) == 0) {
            if (!substitution.empty()) out << substitution << "\n";
        } else {
            out << line << "\n";
        }
    }
    return out.str();
}

inline void replace_all(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

inline size_t find_insertion_point(const std::string& glsl) {
    size_t pos = 0, insertion_point = 0;
    size_t version_pos = glsl.find("#version");
    if (version_pos != std::string::npos) {
        size_t version_end = glsl.find('\n', version_pos);
        insertion_point = (version_end == std::string::npos) ? glsl.length() : version_end + 1;
        pos = insertion_point;
    }
    while (pos < glsl.length()) {
        size_t line_begin = pos;
        while (pos < glsl.length() && std::isspace(glsl[pos])) ++pos;
        if (pos >= glsl.length()) break;
        if (glsl[pos] == '#') {
            ++pos;
            while (pos < glsl.length() && std::isspace(glsl[pos])) ++pos;
            if (glsl.compare(pos, 9, "extension") == 0) {
                size_t ext_end = glsl.find('\n', pos);
                insertion_point = (ext_end == std::string::npos) ? glsl.length() : ext_end + 1;
                pos = insertion_point;
            } else break;
        } else break;
    }
    return insertion_point;
}

// atomicCounter优化
bool process_non_opaque_atomic_to_ssbo(std::string& source) {
    if (source.find("atomicCounter") == std::string::npos) return false;
    static const std::regex decl_rx(R"(layout\s*\(\s*binding\s*=\s*(\d+)\s*(?:,\s*offset\s*=\s*(\d+)\s*)?\)\s*uniform\s+atomic_uint\s+(\w+)\s*;)", std::regex::icase | std::regex::optimize);

    std::set<std::string> atomic_vars;
    std::map<std::string, std::string> binding_map;
    std::smatch m;
    auto it = source.cbegin();
    std::string result;
    size_t last_pos = 0;

    while (std::regex_search(it, source.cend(), m, decl_rx)) {
        size_t match_pos = std::distance(source.cbegin(), it) + m.position(0);
        result.append(source, last_pos, match_pos - last_pos);
        std::string binding = m[1].str(), var = m[3].str();
        atomic_vars.insert(var);
        binding_map[var] = binding;
        std::ostringstream repl;
        repl << "layout(std430, binding=" << binding << ") buffer AtomicCounterSSBO_" << binding << " {\n"
             << "    uint " << var << ";\n};\n";
        result.append(repl.str());
        it = source.cbegin() + match_pos + m.length(0);
        last_pos = match_pos + m.length(0);
    }
    result.append(source, last_pos, source.length() - last_pos);
    if (atomic_vars.empty()) return true;

    for (auto& var : atomic_vars) {
        static const std::regex inc_rx(R"(\batomicCounterIncrement\s*\(\s*)" + var + R"(\s*\))", std::regex::icase | std::regex::optimize);
        static const std::regex dec_rx(R"(\batomicCounterDecrement\s*\(\s*)" + var + R"(\s*\))", std::regex::icase | std::regex::optimize);
        static const std::regex add_rx(R"(\batomicCounterAdd\s*\(\s*)" + var + R"(\s*,\s*([^)]+)\s*\))", std::regex::icase | std::regex::optimize);
        static const std::regex val_rx(R"(\batomicCounter\s*\(\s*)" + var + R"(\s*\))", std::regex::icase | std::regex::optimize);
        result = std::regex_replace(result, inc_rx, "atomicAdd(" + var + ", 1u)");
        result = std::regex_replace(result, dec_rx, "atomicAdd(" + var + ", uint(-1))");
        result = std::regex_replace(result, add_rx, "atomicAdd(" + var + ", $1)");
        result = std::regex_replace(result, val_rx, var);
    }

    // memoryBarrierBuffer插入
    {
        static const std::regex rx_barrier(R"(([ \t]*\batomicAdd\b[^;]*;))", std::regex::icase | std::regex::optimize);
        std::string new_result;
        size_t last_pos = 0;
        for (auto it = std::sregex_iterator(result.begin(), result.end(), rx_barrier);
            it != std::sregex_iterator(); ++it) {
            size_t start_pos = it->position();
            new_result.append(result, last_pos, start_pos - last_pos);
            new_result.append(it->str());
            new_result.append("\n    memoryBarrierBuffer();");
            last_pos = start_pos + it->length();
        }
        new_result.append(result, last_pos, result.length() - last_pos);
        result.swap(new_result);
    }
    result += "\n" + std::string(atomicCounterEmulatedWatermark);
    source.swap(result);
    return true;
}

// SamplerBuffer更高效处理
void process_sampler_buffer(std::string& source) {
    if (source.find("isamplerBuffer") == std::string::npos) return;
    static const std::regex buf_rx(R"(isamplerBuffer)", std::regex::optimize);
    source = std::regex_replace(source, buf_rx, "isampler2D");
    static const std::regex fetch_rx(R"(texelFetch\s*\(\s*(\w+)\s*,\s*([^)]+?)\s*\))", std::regex::optimize);
    source = std::regex_replace(source, fetch_rx, "texelFetch($1, ivec2(($2) % u_BufferTexWidth, ($2) / u_BufferTexWidth), 0)");
    const char* boundaryProtection = R"(
ivec2 bufferCoords(int index) {
    int width = u_BufferTexWidth;
    int x = index % width;
    int y = index / width;
    if (y >= u_BufferTexHeight) {
        y = u_BufferTexHeight - 1;
        x = width - 1;
    }
    return ivec2(x, y);
}
)";
    size_t insertion_point = find_insertion_point(source);
    if (insertion_point != std::string::npos)
        source.insert(insertion_point, boundaryProtection);

    const char* uniformDecl = R"(
uniform int u_BufferTexWidth;
uniform int u_BufferTexHeight;
)";
    insertion_point = find_insertion_point(source);
    if (insertion_point != std::string::npos) {
        insertion_point = source.find('\n', insertion_point);
        if (insertion_point != std::string::npos)
            source.insert(insertion_point + 1, uniformDecl);
    }
}

static void inject_textureQueryLod(std::string& glsl) {
    static const std::regex defRegex(R"(vec2\s+mg_textureQueryLod\s*\()", std::regex::optimize);
    if (glsl.find("textureQueryLod") == std::string::npos) return;
    if (std::regex_search(glsl, defRegex)) return;
    const std::string textureQueryLodImpl = R"(
#define textureQueryLod mg_textureQueryLod
vec2 mg_textureQueryLod(sampler2D tex, vec2 uv) {
    vec2 texSizeF = vec2(textureSize(tex, 0));
    vec2 dFdx_uv = dFdx(uv * texSizeF);
    vec2 dFdy_uv = dFdy(uv * texSizeF);
    float maxDerivative = max(length(dFdx_uv), length(dFdy_uv));
    float lod = log2(maxDerivative);
    return vec2(lod);
}
)";
    size_t insertPos = find_insertion_point(glsl);
    glsl.insert(insertPos, "\n" + textureQueryLodImpl + "\n");
}

static inline void inject_temporal_filter(std::string& glsl) {
    static const std::regex defRegex(R"(vec4\s+GI_TemporalFilter\s*\()", std::regex::optimize);
    if (glsl.find("GI_TemporalFilter") == std::string::npos) return;
    if (std::regex_search(glsl, defRegex)) return;
    static const std::regex uniformRegex(R"(^\s*(?:layout\s*\([^)]*\)\s*)?uniform\s+\w+(?:\s*\[\s*\d+\s*\])?\s+\w+(?:\s*\[\s*\d+\s*\])?\s*;.*$)", std::regex::ECMAScript | std::regex::multiline | std::regex::optimize);
    std::sregex_iterator it(glsl.begin(), glsl.end(), uniformRegex), end;
    size_t insertPos = 0;
    for (; it != end; ++it) insertPos = it->position() + it->length();
    const std::string GI_TemporalFilterImpl = R"(
vec4 GI_TemporalFilter() {
    vec2 uv = gl_FragCoord.xy / screenSize;
    uv += taaJitter * pixelSize;
    vec4 currentGI = texture(colortex0, uv);
    float depth = texture(depthtex0, uv).r;
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 viewPos = gbufferProjectionInverse * clipPos;
    viewPos /= viewPos.w;
    vec4 worldPos = gbufferModelViewInverse * viewPos;
    vec4 prevClipPos = gbufferPreviousProjection * (gbufferPreviousModelView * worldPos);
    prevClipPos /= prevClipPos.w;
    vec2 prevUV = prevClipPos.xy * 0.5 + 0.5;
    vec4 historyGI = texture(colortex1, prevUV);
    float difference = length(currentGI.rgb - historyGI.rgb);
    float thresholdValue = 0.1;
    float adaptiveBlend = mix(0.9, 0.0, smoothstep(thresholdValue, thresholdValue * 2.0, difference));
    vec4 filteredGI = mix(currentGI, historyGI, adaptiveBlend);
    if (difference > thresholdValue * 2.0) {
        filteredGI = currentGI;
    }
    return filteredGI;
}
)";
    glsl.insert(insertPos, "\n" + GI_TemporalFilterImpl + "\n");
}
#define xstr(s) str(s)
#define str(s) #s

void inject_mg_macro_definition(std::string& glslCode) {
    std::string macro_definitions =
            "\n#define MG_MOBILEGLUES\n"
            "#define MG_MOBILEGLUES_VERSION " xstr(MAJOR) xstr(MINOR) xstr(REVISION) xstr(PATCH) "\n";
    size_t versionPos = glslCode.rfind("#version");
    size_t insertionPos = 0;
    if (versionPos != std::string::npos) {
        size_t nextNewline = glslCode.find('\n', versionPos);
        insertionPos = (nextNewline != std::string::npos) ? nextNewline + 1 : glslCode.length();
    } else {
        size_t firstNewline = glslCode.find('\n');
        insertionPos = (firstNewline != std::string::npos) ? firstNewline + 1 : 0;
    }
    glslCode.insert(insertionPos, macro_definitions);
}

std::string preprocess_glsl(const std::string& glsl, GLenum shaderType, bool* atomicCounterEmulated) {
    std::string ret = glsl;
    ret = replace_line_starting_with(ret, "#line");
    replace_all(ret, "#ifdef GL_ARB_derivative_control", "#if 0");
    replace_all(ret, "#ifndef GL_ARB_derivative_control", "#if 1");
    replace_all(ret,
                "const mat3 rotInverse = transpose(rot);",
                "const mat3 rotInverse = mat3(rot[0][0], rot[1][0], rot[2][0], rot[0][1], rot[1][1], rot[2][1], rot[0][2], rot[1][2], rot[2][2]);");
    inject_temporal_filter(ret);
    if (!g_gles_caps.GL_EXT_texture_query_lod) inject_textureQueryLod(ret);
    inject_mg_macro_definition(ret);
    if (hardware->emulate_texture_buffer) process_sampler_buffer(ret);
    *atomicCounterEmulated = process_non_opaque_atomic_to_ssbo(ret);
    return ret;
}

inline int get_or_add_glsl_version(std::string& glsl) {
    int glsl_version = getGLSLVersion(glsl.c_str());
    if (glsl_version == -1) {
        glsl_version = 140;
        glsl.insert(0, "#version 140\n");
    }
    LOG_D("GLSL version: %d", glsl_version)
    return glsl_version;
}

std::vector<unsigned int> glsl_to_spirv(GLenum shader_type, int glsl_version, const char * const *shader_src, int& errc) {
    EShLanguage shader_language;
    switch (shader_type) {
        case GL_VERTEX_SHADER: shader_language = EShLangVertex; break;
        case GL_FRAGMENT_SHADER: shader_language = EShLangFragment; break;
        case GL_COMPUTE_SHADER: shader_language = EShLangCompute; break;
        case GL_TESS_CONTROL_SHADER: shader_language = EShLangTessControl; break;
        case GL_TESS_EVALUATION_SHADER: shader_language = EShLangTessEvaluation; break;
        case GL_GEOMETRY_SHADER: shader_language = EShLangGeometry; break;
        default: LOG_D("GLSL type not supported!"); errc = -1; return {};
    }
    using namespace glslang;
    glslang::TShader shader(shader_language);
    shader.setStrings(shader_src, 1);
    shader.setEnvInput(EShSourceGlsl, shader_language, EShClientVulkan, glsl_version);
    shader.setEnvClient(EShClientOpenGL, EShTargetOpenGL_450);
    shader.setEnvTarget(EShTargetSpv, EShTargetSpv_1_6);
    shader.setAutoMapLocations(true);
    shader.setAutoMapBindings(true);
    TBuiltInResource TBuiltInResource_resources = InitResources();
    if (!shader.parse(&TBuiltInResource_resources, glsl_version, true, EShMsgDefault)) {
        LOG_D("GLSL Compiling ERROR: \n%s", shader.getInfoLog())
        errc = -1; return {};
    }
    LOG_D("GLSL Compiled.")
    glslang::TProgram program; program.addShader(&shader);
    if (!program.link(EShMsgDefault)) {
        LOG_D("Shader Linking ERROR: %s", program.getInfoLog())
        errc = -1; return {};
    }
    LOG_D("Shader Linked.")
    std::vector<unsigned int> spirv_code;
    glslang::SpvOptions spvOptions; spvOptions.disableOptimizer = false;
    glslang::GlslangToSpv(*program.getIntermediate(shader_language), spirv_code, &spvOptions);
    errc = 0; return spirv_code;
}

std::string spirv_to_essl(std::vector<unsigned int> spirv, uint essl_version, int& errc) {
    spvc_context context = nullptr;
    spvc_parsed_ir ir = nullptr;
    spvc_compiler compiler_glsl = nullptr;
    spvc_compiler_options options = nullptr;
    const char *result = nullptr;
    LOG_D("spirv_code.size(): %d", spirv.size())
    spvc_context_create(&context);
    spvc_context_parse_spirv(context, spirv.data(), spirv.size(), &ir);
    spvc_context_create_compiler(context, SPVC_BACKEND_GLSL, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler_glsl);
    spvc_compiler_create_compiler_options(compiler_glsl, &options);
    spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_GLSL_VERSION, essl_version >= 300 ? essl_version : 300);
    spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_ES, SPVC_TRUE);
    spvc_compiler_install_compiler_options(compiler_glsl, options);
    spvc_compiler_compile(compiler_glsl, &result);
    if (!result) {
        LOG_E("Error: unexpected error in spirv-cross.")
        errc = -1;
        spvc_context_destroy(context); return "";
    }
    std::string essl(result);
    spvc_context_destroy(context);
    errc = 0;
    return essl;
}

// 只初始化一次
static bool glslang_inited = false;
std::string GLSLtoGLSLES_2(const char *glsl_code, GLenum glsl_type, uint essl_version, int& return_code) {
    bool atomicCounterEmulated = false;
    std::string correct_glsl_str = preprocess_glsl(glsl_code, glsl_type, &atomicCounterEmulated);
    LOG_D("Firstly converted GLSL:\n%s", correct_glsl_str.c_str())
    int glsl_version = get_or_add_glsl_version(correct_glsl_str);
    if (!glslang_inited) { glslang::InitializeProcess(); glslang_inited = true; }
    const char* s[] = { correct_glsl_str.c_str() };
    int errc = 0;
    std::vector<unsigned int> spirv_code = glsl_to_spirv(glsl_type, glsl_version, s, errc);
    if (errc != 0) { return_code = -1; return ""; }
    errc = 0;
    std::string essl = spirv_to_essl(spirv_code, essl_version, errc);
    if (errc != 0) { return_code = -2; return ""; }
    // Post-processing ESSL
    if (glsl_type != GL_COMPUTE_SHADER) essl = removeLayoutBinding(essl);
    essl = processOutColorLocations(essl);
    essl = forceSupporterOutput(essl);
    LOG_D("Originally GLSL to GLSL ES Complete: \n%s", essl.c_str())
    return_code = errc;
    if (return_code == 0) return_code = atomicCounterEmulated ? 1 : 0;
    return essl;
}

std::string GLSLtoGLSLES_1(const char *glsl_code, GLenum glsl_type, uint esversion, int& return_code) {
#if !defined(__APPLE__)
    LOG_W("Warning: use glsl optimizer to convert shader.")
    if (esversion < 300) esversion = 300;
    std::string result = MesaConvertShader(glsl_code, glsl_type == GL_VERTEX_SHADER ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER, 460LL, esversion);
    return_code = 0;
    return result;
#else
    LOG_W_FORCE("Cannot convert glsl with version %d in MacOS/iOS", esversion);
    return std::string(glsl_code);
#endif
}