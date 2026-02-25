/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Development Team                             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <fstream>
# include <sstream>
#endif

#include "OsgVerseShaderManager.h"
#include <Base/Console.h>
#include <App/Application.h>

using namespace Gui::Render;

//===========================================================================
// ShaderSnippets Implementation
//===========================================================================

std::string ShaderSnippets::processIncludes(const std::string& source)
{
    std::string result = source;
    // Map of snippet names to their code
    static const std::unordered_map<std::string, std::string> snippets = {
        {"toneMapReinhard", toneMapReinhard()},
        {"gammaCorrect", gammaCorrect()},
        {"lambertian", lambertian()},
        {"blinnPhong", blinnPhong()},
    };

    // Replace #pragma include "name" with actual code
    const std::string pragma = "#pragma include \"";
    size_t pos = 0;
    while ((pos = result.find(pragma, pos)) != std::string::npos) {
        size_t endQuote = result.find('"', pos + pragma.size());
        if (endQuote == std::string::npos) break;

        std::string name = result.substr(pos + pragma.size(), endQuote - pos - pragma.size());
        size_t lineEnd = result.find('\n', endQuote);
        if (lineEnd == std::string::npos) lineEnd = result.size();

        auto it = snippets.find(name);
        if (it != snippets.end()) {
            result.replace(pos, lineEnd - pos, it->second);
        } else {
            Base::Console().warning("ShaderSnippets: Unknown include '%s'\n", name.c_str());
            pos = lineEnd;
        }
    }
    return result;
}

//===========================================================================
// OsgVerseShaderManager Implementation
//===========================================================================

OsgVerseShaderManager& OsgVerseShaderManager::instance()
{
    static OsgVerseShaderManager instance;
    return instance;
}

OsgVerseShaderManager::OsgVerseShaderManager()
{
    // 设置默认shader路径 / Set default shader path
    _shaderPath = App::Application::getResourceDir() + "Gui/Render/Backends/OsgVerse/shaders/";

    Base::Console().log("OsgVerseShaderManager: Initializing shader manager\n");
    Base::Console().log("OsgVerseShaderManager: Shader path: %s\n", _shaderPath.c_str());

    // 初始化内置shader / Initialize built-in shaders
    initializeBuiltinShaders();
}

void OsgVerseShaderManager::initializeBuiltinShaders()
{
    Base::Console().log("OsgVerseShaderManager: Initializing built-in shaders\n");

    // 预创建所有内置shader类型
    // Pre-create all built-in shader types
    // 注意：实际编译延迟到首次使用
    // Note: Actual compilation is deferred until first use
}

osg::Program* OsgVerseShaderManager::getProgram(ShaderType type)
{
    // 检查缓存 / Check cache
    auto it = _programs.find(type);
    if (it != _programs.end() && it->second.compiled) {
        return it->second.program.get();
    }

    // 创建新程序 / Create new program
    osg::Program* program = nullptr;

    switch (type) {
        case ShaderType::Standard:
            program = createStandardShader();
            break;
        case ShaderType::PBR:
            program = createPBRShader();
            break;
        case ShaderType::Wireframe:
            program = createWireframeShader();
            break;
        case ShaderType::Flat:
            program = createFlatShader();
            break;
        case ShaderType::Unlit:
            program = createUnlitShader();
            break;
        case ShaderType::ToneMap:
            program = createToneMapShader();
            break;
        case ShaderType::GammaCorrection:
            program = createGammaCorrectionShader();
            break;
        case ShaderType::BloomBrightExtract:
            program = createBloomBrightExtractShader();
            break;
        case ShaderType::BloomBlur:
            program = createBloomBlurShader();
            break;
        case ShaderType::BloomComposite:
            program = createBloomCompositeShader();
            break;
        case ShaderType::TAA:
            program = createTAAShader();
            break;
        case ShaderType::SSAO:
            program = createSSAOShader();
            break;
        case ShaderType::SSAOBlur:
            program = createSSAOBlurShader();
            break;
        case ShaderType::SSAOComposite:
            program = createSSAOCompositeShader();
            break;
        default:
            Base::Console().error("OsgVerseShaderManager: Unknown shader type\n");
            return nullptr;
    }

    if (program) {
        ShaderProgramInfo info;
        info.program = program;
        info.type = type;
        info.compiled = true;
        _programs[type] = info;
        _compileErrors.erase(type);

        Base::Console().log("OsgVerseShaderManager: Created shader program for type %d\n",
                           static_cast<int>(type));
    }
    else {
        _compileErrors[type] = "Failed to create shader program for type "
                               + std::to_string(static_cast<int>(type));
        Base::Console().error("OsgVerseShaderManager: %s\n", _compileErrors[type].c_str());
    }

    return program;
}

osg::Program* OsgVerseShaderManager::createProgram(const std::string& name,
                                                   const std::string& vertexSource,
                                                   const std::string& fragmentSource)
{
    auto program = new osg::Program();
    program->setName(name);

    // 创建顶点着色器 / Create vertex shader
    auto vertexShader = createShaderFromSource(osg::Shader::VERTEX, vertexSource, name + "_vertex");
    if (!vertexShader) {
        Base::Console().error("OsgVerseShaderManager: Failed to create vertex shader for %s\n",
                             name.c_str());
        return nullptr;
    }
    program->addShader(vertexShader);

    // 创建片段着色器 / Create fragment shader
    auto fragmentShader = createShaderFromSource(osg::Shader::FRAGMENT, fragmentSource, name + "_fragment");
    if (!fragmentShader) {
        Base::Console().error("OsgVerseShaderManager: Failed to create fragment shader for %s\n",
                             name.c_str());
        return nullptr;
    }
    program->addShader(fragmentShader);

    return program;
}

osg::Program* OsgVerseShaderManager::loadProgramFromFiles(const std::string& name,
                                                          const std::string& vertexFile,
                                                          const std::string& fragmentFile)
{
    // 读取顶点着色器 / Read vertex shader
    std::string vertexSource = readShaderFile(vertexFile);
    if (vertexSource.empty()) {
        Base::Console().error("OsgVerseShaderManager: Failed to read vertex shader: %s\n",
                             vertexFile.c_str());
        return nullptr;
    }

    // 读取片段着色器 / Read fragment shader
    std::string fragmentSource = readShaderFile(fragmentFile);
    if (fragmentSource.empty()) {
        Base::Console().error("OsgVerseShaderManager: Failed to read fragment shader: %s\n",
                             fragmentFile.c_str());
        return nullptr;
    }

    return createProgram(name, vertexSource, fragmentSource);
}

bool OsgVerseShaderManager::applyShader(osg::StateSet* stateSet, ShaderType type)
{
    if (!stateSet) {
        return false;
    }

    osg::Program* program = getProgram(type);
    if (!program) {
        Base::Console().error("OsgVerseShaderManager: Failed to get shader program\n");
        return false;
    }

    stateSet->setAttributeAndModes(program, osg::StateAttribute::ON);
    return true;
}

void OsgVerseShaderManager::reloadAll()
{
    Base::Console().log("OsgVerseShaderManager: Reloading all shaders\n");

    // 清除缓存 / Clear cache
    _programs.clear();
    _customPrograms.clear();
    _compileErrors.clear();

    // 重新初始化 / Reinitialize
    initializeBuiltinShaders();
}

void OsgVerseShaderManager::clearCache()
{
    _programs.clear();
    _customPrograms.clear();
    _compileErrors.clear();
}

bool OsgVerseShaderManager::isCompiled(ShaderType type) const
{
    auto it = _programs.find(type);
    return it != _programs.end() && it->second.compiled;
}

std::string OsgVerseShaderManager::getCompileError(ShaderType type) const
{
    auto it = _compileErrors.find(type);
    if (it != _compileErrors.end()) {
        return it->second;
    }
    return "";
}

//-----------------------------------------------------------------------
// Private Methods
//-----------------------------------------------------------------------

osg::Shader* OsgVerseShaderManager::createShaderFromSource(osg::Shader::Type type,
                                                           const std::string& source,
                                                           const std::string& name)
{
    // Validate that source starts with #version
    if (source.empty()) {
        Base::Console().error("OsgVerseShaderManager: Empty shader source for '%s'\n", name.c_str());
        return nullptr;
    }

    // Find first non-whitespace character and verify it's #version
    size_t firstNonSpace = source.find_first_not_of(" \t\r\n");
    if (firstNonSpace == std::string::npos || source.substr(firstNonSpace, 8) != "#version") {
        Base::Console().warning("OsgVerseShaderManager: Shader '%s' does not start with #version directive. "
                               "This may cause issues on some drivers.\n", name.c_str());
    }

    auto shader = new osg::Shader(type);
    shader->setName(name);
    shader->setShaderSource(source);
    return shader;
}

std::string OsgVerseShaderManager::readShaderFile(const std::string& filename)
{
    std::string fullPath = _shaderPath + filename;

    std::ifstream file(fullPath);
    if (!file.is_open()) {
        Base::Console().warning("OsgVerseShaderManager: Failed to open shader file: %s\n",
                               fullPath.c_str());
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

//-----------------------------------------------------------------------
// Shader Creation Methods
//-----------------------------------------------------------------------

osg::Program* OsgVerseShaderManager::createStandardShader()
{
    Base::Console().log("OsgVerseShaderManager: Creating standard Phong shader (GLSL 1.20)\n");

    // GLSL 1.20 Phong shader using a custom uniform baseColor for the material color.
    // This is the most reliable approach on macOS GL 2.1 because:
    // - gl_FrontMaterial might not be forwarded to shader on macOS
    // - gl_Color (vertex colors) + BIND_OVERALL might not work with shaders
    // - osg::Uniform is always reliably passed to the shader
    const char* vertexSource =
        "#version 120\n"
        "varying vec3 vPosition;\n"
        "varying vec3 vNormal;\n"
        "varying vec4 vColor;\n"
        "uniform mat4 u_shadowMatrix;\n"
        "varying vec4 vShadowCoord;\n"
        "void main() {\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "    vPosition = (gl_ModelViewMatrix * gl_Vertex).xyz;\n"
        "    vNormal = normalize(gl_NormalMatrix * gl_Normal);\n"
        "    vColor = gl_Color;\n"
        "    vShadowCoord = u_shadowMatrix * gl_ModelViewMatrix * gl_Vertex;\n"
        "}\n";

    const char* fragmentSource =
        "#version 120\n"
        "varying vec3 vPosition;\n"
        "varying vec3 vNormal;\n"
        "varying vec4 vColor;\n"
        "varying vec4 vShadowCoord;\n"
        "uniform vec4 baseColor;\n"
        "uniform int u_colorMode;\n"
        "uniform bool u_shadowEnabled;\n"
        "uniform sampler2D u_shadowMap;\n"
        "uniform float u_shadowBias;\n"
        "float calcShadow(vec4 fragPosLightSpace) {\n"
        "    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;\n"
        "    if (projCoords.z > 1.0) return 1.0;\n"
        "    float currentDepth = projCoords.z;\n"
        "    float bias = u_shadowBias;\n"
        "    float shadow = 0.0;\n"
        "    float texelSize = 1.0 / 2048.0;\n"
        "    for (int x = -1; x <= 1; x++) {\n"
        "        for (int y = -1; y <= 1; y++) {\n"
        "            float pcfDepth = texture2D(u_shadowMap, projCoords.xy + vec2(float(x), float(y)) * texelSize).r;\n"
        "            shadow += (currentDepth - bias > pcfDepth) ? 0.0 : 1.0;\n"
        "        }\n"
        "    }\n"
        "    return shadow / 9.0;\n"
        "}\n"
        "void main() {\n"
        "    vec3 N = normalize(vNormal);\n"
        "    // Light direction from gl_LightSource[0] with hardcoded fallback\n"
        "    vec3 lightPos = gl_LightSource[0].position.xyz;\n"
        "    vec3 L;\n"
        "    if (dot(lightPos, lightPos) > 0.001) {\n"
        "        L = normalize(lightPos);\n"
        "    } else {\n"
        "        L = normalize(vec3(0.3, -0.5, 0.8));\n"
        "    }\n"
        "    vec3 V = normalize(-vPosition);\n"
        "    vec3 H = normalize(L + V);\n"
        "    vec4 matColor = (u_colorMode == 1) ? vColor : baseColor;\n"
        "    // Ambient\n"
        "    vec3 ambient = matColor.rgb * 0.3;\n"
        "    // Diffuse (Lambertian)\n"
        "    float NdotL = max(dot(N, L), 0.0);\n"
        "    vec3 diffuse = matColor.rgb * NdotL * 0.7;\n"
        "    // Specular (Blinn-Phong)\n"
        "    float NdotH = max(dot(N, H), 0.0);\n"
        "    float spec = pow(NdotH, 40.0);\n"
        "    vec3 specular = vec3(0.3) * spec;\n"
        "    // Shadow factor\n"
        "    float shadow = 1.0;\n"
        "    if (u_shadowEnabled) {\n"
        "        shadow = calcShadow(vShadowCoord);\n"
        "    }\n"
        "    // Backlight from gl_LightSource[1]\n"
        "    vec3 backPos = gl_LightSource[1].position.xyz;\n"
        "    float backContrib = 0.0;\n"
        "    if (dot(backPos, backPos) > 0.001) {\n"
        "        vec3 L1 = normalize(backPos);\n"
        "        backContrib = max(dot(N, L1), 0.0) * 0.2;\n"
        "    }\n"
        "    vec3 backDiffuse = matColor.rgb * backContrib;\n"
        "    gl_FragColor = vec4(ambient + (diffuse + specular) * shadow + backDiffuse, matColor.a);\n"
        "}\n";

    return createProgram("StandardPhong", vertexSource, fragmentSource);
}

osg::Program* OsgVerseShaderManager::createPBRShader()
{
    Base::Console().log("OsgVerseShaderManager: Creating PBR shader (GLSL 1.20)\n");

    // PBR vertex shader — GLSL 1.20 for macOS GL 2.1 compatibility
    // Uses gl_Vertex/gl_Normal instead of osg_Vertex/osg_Normal
    // Uses varying instead of out, no mat3 varying (split into 3 vec3)
    const std::string vertexSource =
        "#version 120\n"
        "// View-space position for lighting\n"
        "varying vec3 vViewPos;\n"
        "varying vec3 vNormal;\n"
        "varying vec2 vTexCoord;\n"
        "varying vec4 vColor;\n"
        "// TBN matrix columns (GLSL 1.20 has no varying mat3)\n"
        "varying vec3 vTangent;\n"
        "varying vec3 vBitangent;\n"
        "varying vec3 vTBNNormal;\n"
        "uniform mat4 u_shadowMatrix;\n"
        "varying vec4 vShadowCoord;\n"
        "void main() {\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "    // View-space position for lighting calculations\n"
        "    vViewPos = (gl_ModelViewMatrix * gl_Vertex).xyz;\n"
        "    // Transform normal to view space\n"
        "    vNormal = normalize(gl_NormalMatrix * gl_Normal);\n"
        "    // TBN for normal mapping — tangent from attribute 6\n"
        "    vec3 T = normalize(gl_NormalMatrix * gl_MultiTexCoord6.xyz);\n"
        "    vec3 N = vNormal;\n"
        "    vec3 B = cross(N, T);\n"
        "    vTangent = T;\n"
        "    vBitangent = B;\n"
        "    vTBNNormal = N;\n"
        "    // Pass texture coordinates and vertex color\n"
        "    vTexCoord = gl_MultiTexCoord0.xy;\n"
        "    vColor = gl_Color;\n"
        "    vShadowCoord = u_shadowMatrix * gl_ModelViewMatrix * gl_Vertex;\n"
        "}\n";

    // PBR fragment shader — GLSL 1.20 for macOS GL 2.1 compatibility
    // Key differences from GLSL 3.30 version:
    // - No struct arrays (unreliable in GLSL 1.20), use individual uniforms per light
    // - varying instead of in, gl_FragColor instead of out FragColor
    // - texture2D() instead of texture()
    // - All pow() inputs clamped to prevent NaN on macOS
    // - View-space lighting (no world-space, avoids needing ViewMatrixInverse)
    // - MAX_LIGHTS reduced to 4 for GL 2.1 uniform count limits
    const std::string fragmentSource =
        "#version 120\n"
        "varying vec3 vViewPos;\n"
        "varying vec3 vNormal;\n"
        "varying vec2 vTexCoord;\n"
        "varying vec4 vColor;\n"
        "varying vec3 vTangent;\n"
        "varying vec3 vBitangent;\n"
        "varying vec3 vTBNNormal;\n"
        "varying vec4 vShadowCoord;\n"
        "\n"
        "// Shadow mapping\n"
        "uniform bool u_shadowEnabled;\n"
        "uniform sampler2D u_shadowMap;\n"
        "uniform float u_shadowBias;\n"
        "\n"
        "// PBR material parameters (via osg::Uniform)\n"
        "uniform vec4 baseColor;\n"
        "uniform float metallic;\n"
        "uniform float roughness;\n"
        "uniform vec4 emissive;\n"
        "uniform float ambientOcclusion;\n"
        "\n"
        "// Textures\n"
        "uniform sampler2D baseColorTexture;\n"
        "uniform sampler2D normalTexture;\n"
        "uniform sampler2D metallicRoughnessTexture;\n"
        "uniform sampler2D occlusionTexture;\n"
        "uniform sampler2D emissiveTexture;\n"
        "\n"
        "uniform bool hasBaseColorTexture;\n"
        "uniform bool hasNormalTexture;\n"
        "uniform bool hasMetallicRoughnessTexture;\n"
        "uniform bool hasOcclusionTexture;\n"
        "uniform bool hasEmissiveTexture;\n"
        "\n"
        "// Light uniforms — individual per light (no struct arrays in GLSL 1.20)\n"
        "// Each light has: type(int), color(vec3), position(vec3), direction(vec3),\n"
        "//                  attenuation(vec3), range(float), coneAngles(vec2)\n"
        "uniform int u_numLights;\n"
        "uniform int u_lights_type[4];\n"
        "uniform vec3 u_lights_color[4];\n"
        "uniform vec3 u_lights_position[4];\n"
        "uniform vec3 u_lights_direction[4];\n"
        "uniform vec3 u_lights_attenuation[4];\n"
        "uniform float u_lights_range[4];\n"
        "uniform vec2 u_lights_coneAngles[4];\n"
        "uniform vec3 u_ambientLight;\n"
        "\n"
        "const float PI = 3.14159265359;\n"
        "\n"
        "// GGX Normal Distribution Function\n"
        "float DistributionGGX(vec3 N, vec3 H, float r) {\n"
        "    float a = r * r;\n"
        "    float a2 = a * a;\n"
        "    float NdotH = max(dot(N, H), 0.0);\n"
        "    float NdotH2 = NdotH * NdotH;\n"
        "    float denom = NdotH2 * (a2 - 1.0) + 1.0;\n"
        "    denom = PI * denom * denom + 0.0001;\n"
        "    return a2 / denom;\n"
        "}\n"
        "\n"
        "// Smith Geometry Shadowing\n"
        "float GeometrySchlickGGX(float NdotV, float r) {\n"
        "    float k = ((r + 1.0) * (r + 1.0)) / 8.0;\n"
        "    return NdotV / (NdotV * (1.0 - k) + k + 0.0001);\n"
        "}\n"
        "float GeometrySmith(float NdotV, float NdotL, float r) {\n"
        "    return GeometrySchlickGGX(NdotV, r) * GeometrySchlickGGX(NdotL, r);\n"
        "}\n"
        "\n"
        "// Schlick Fresnel\n"
        "vec3 fresnelSchlick(float cosTheta, vec3 F0) {\n"
        "    float t = clamp(1.0 - cosTheta, 0.0, 1.0);\n"
        "    // pow(t, 5.0) with clamped input to prevent NaN\n"
        "    float t2 = t * t;\n"
        "    float t5 = t2 * t2 * t;\n"
        "    return F0 + (1.0 - F0) * t5;\n"
        "}\n"
        "\n"
        "// Calculate single light contribution (view-space)\n"
        "vec3 calcLight(int ltype, vec3 lcol, vec3 lpos, vec3 ldir,\n"
        "               vec3 latt, float lrange, vec2 lcone,\n"
        "               vec3 N, vec3 V, vec3 albedo, float met, float rough, vec3 F0) {\n"
        "    vec3 L;\n"
        "    float atten = 1.0;\n"
        "    if (ltype == 0) {\n"
        "        // Directional: direction is already in view space via gl_LightSource\n"
        "        L = normalize(-ldir);\n"
        "    } else if (ltype == 1) {\n"
        "        // Point light\n"
        "        vec3 lv = lpos - vViewPos;\n"
        "        float d = length(lv);\n"
        "        L = normalize(lv);\n"
        "        if (d < lrange) {\n"
        "            atten = 1.0 / (latt.x + latt.y * d + latt.z * d * d + 0.0001);\n"
        "        } else { return vec3(0.0); }\n"
        "    } else {\n"
        "        // Spot light\n"
        "        vec3 lv = lpos - vViewPos;\n"
        "        float d = length(lv);\n"
        "        L = normalize(lv);\n"
        "        if (d < lrange) {\n"
        "            atten = 1.0 / (latt.x + latt.y * d + latt.z * d * d + 0.0001);\n"
        "            float theta = dot(L, normalize(-ldir));\n"
        "            float eps = lcone.x - lcone.y;\n"
        "            atten *= clamp((theta - lcone.y) / (eps + 0.0001), 0.0, 1.0);\n"
        "        } else { return vec3(0.0); }\n"
        "    }\n"
        "    if (atten <= 0.0) return vec3(0.0);\n"
        "\n"
        "    vec3 H = normalize(V + L);\n"
        "    float NdotV = max(dot(N, V), 0.001);\n"
        "    float NdotL = max(dot(N, L), 0.0);\n"
        "    float HdotV = max(dot(H, V), 0.0);\n"
        "\n"
        "    // Cook-Torrance BRDF\n"
        "    float NDF = DistributionGGX(N, H, rough);\n"
        "    float G = GeometrySmith(NdotV, NdotL, rough);\n"
        "    vec3 F = fresnelSchlick(HdotV, F0);\n"
        "\n"
        "    vec3 spec = (NDF * G * F) / (4.0 * NdotV * NdotL + 0.0001);\n"
        "\n"
        "    // Energy conservation\n"
        "    vec3 kD = (vec3(1.0) - F) * (1.0 - met);\n"
        "    vec3 diff = kD * albedo / PI;\n"
        "\n"
        "    return (diff + spec) * lcol * atten * NdotL;\n"
        "}\n"
        "\n"
        "float calcShadow(vec4 fragPosLightSpace) {\n"
        "    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;\n"
        "    if (projCoords.z > 1.0) return 1.0;\n"
        "    float currentDepth = projCoords.z;\n"
        "    float bias = u_shadowBias;\n"
        "    float shadow = 0.0;\n"
        "    float texelSize = 1.0 / 2048.0;\n"
        "    for (int x = -1; x <= 1; x++) {\n"
        "        for (int y = -1; y <= 1; y++) {\n"
        "            float pcfDepth = texture2D(u_shadowMap, projCoords.xy + vec2(float(x), float(y)) * texelSize).r;\n"
        "            shadow += (currentDepth - bias > pcfDepth) ? 0.0 : 1.0;\n"
        "        }\n"
        "    }\n"
        "    return shadow / 9.0;\n"
        "}\n"
        "\n"
        "void main() {\n"
        "    // Base color\n"
        "    vec4 albedo = baseColor;\n"
        "    if (hasBaseColorTexture) {\n"
        "        albedo = albedo * texture2D(baseColorTexture, vTexCoord);\n"
        "    }\n"
        "\n"
        "    // Normal (view-space)\n"
        "    vec3 N = normalize(vNormal);\n"
        "    if (hasNormalTexture) {\n"
        "        vec3 nm = texture2D(normalTexture, vTexCoord).xyz * 2.0 - 1.0;\n"
        "        mat3 TBN = mat3(normalize(vTangent), normalize(vBitangent), normalize(vTBNNormal));\n"
        "        N = normalize(TBN * nm);\n"
        "    }\n"
        "\n"
        "    // Metallic / Roughness\n"
        "    float met = metallic;\n"
        "    float rough = roughness;\n"
        "    if (hasMetallicRoughnessTexture) {\n"
        "        vec4 mr = texture2D(metallicRoughnessTexture, vTexCoord);\n"
        "        met = met * mr.b;\n"
        "        rough = rough * mr.g;\n"
        "    }\n"
        "    rough = clamp(rough, 0.04, 1.0);\n"
        "\n"
        "    // Ambient occlusion\n"
        "    float ao = ambientOcclusion;\n"
        "    if (hasOcclusionTexture) {\n"
        "        ao = ao * texture2D(occlusionTexture, vTexCoord).r;\n"
        "    }\n"
        "\n"
        "    // View vector (view-space: camera is at origin)\n"
        "    vec3 V = normalize(-vViewPos);\n"
        "\n"
        "    // F0 (surface reflectance at normal incidence)\n"
        "    vec3 F0 = mix(vec3(0.04), albedo.rgb, met);\n"
        "\n"
        "    // Accumulate light contributions (max 4 lights for GL 2.1)\n"
        "    vec3 Lo = vec3(0.0);\n"
        "    int numL = u_numLights;\n"
        "    if (numL > 4) numL = 4;\n"
        "    // Unrolled loop for GLSL 1.20 compatibility\n"
        "    if (numL > 0) Lo += calcLight(u_lights_type[0], u_lights_color[0],\n"
        "        u_lights_position[0], u_lights_direction[0], u_lights_attenuation[0],\n"
        "        u_lights_range[0], u_lights_coneAngles[0], N, V, albedo.rgb, met, rough, F0);\n"
        "    if (numL > 1) Lo += calcLight(u_lights_type[1], u_lights_color[1],\n"
        "        u_lights_position[1], u_lights_direction[1], u_lights_attenuation[1],\n"
        "        u_lights_range[1], u_lights_coneAngles[1], N, V, albedo.rgb, met, rough, F0);\n"
        "    if (numL > 2) Lo += calcLight(u_lights_type[2], u_lights_color[2],\n"
        "        u_lights_position[2], u_lights_direction[2], u_lights_attenuation[2],\n"
        "        u_lights_range[2], u_lights_coneAngles[2], N, V, albedo.rgb, met, rough, F0);\n"
        "    if (numL > 3) Lo += calcLight(u_lights_type[3], u_lights_color[3],\n"
        "        u_lights_position[3], u_lights_direction[3], u_lights_attenuation[3],\n"
        "        u_lights_range[3], u_lights_coneAngles[3], N, V, albedo.rgb, met, rough, F0);\n"
        "\n"
        "    // Ambient\n"
        "    vec3 ambient = u_ambientLight * albedo.rgb * ao;\n"
        "\n"
        "    // Emissive\n"
        "    vec3 emissiveColor = emissive.rgb;\n"
        "    if (hasEmissiveTexture) {\n"
        "        emissiveColor = emissiveColor * texture2D(emissiveTexture, vTexCoord).rgb;\n"
        "    }\n"
        "\n"
        "    // Shadow factor\n"
        "    float shadow = 1.0;\n"
        "    if (u_shadowEnabled) {\n"
        "        shadow = calcShadow(vShadowCoord);\n"
        "    }\n"
        "\n"
        "    vec3 color = ambient + Lo * shadow + emissiveColor;\n"
        "\n"
        "    // Reinhard tone mapping\n"
        "    color = color / (color + vec3(1.0));\n"
        "\n"
        "    // Gamma correction\n"
        "    color.r = pow(max(color.r, 0.0), 1.0/2.2);\n"
        "    color.g = pow(max(color.g, 0.0), 1.0/2.2);\n"
        "    color.b = pow(max(color.b, 0.0), 1.0/2.2);\n"
        "\n"
        "    gl_FragColor = vec4(color, albedo.a);\n"
        "}\n";

    return createProgram("PBR", vertexSource, fragmentSource);
}

osg::Program* OsgVerseShaderManager::createWireframeShader()
{
    Base::Console().log("OsgVerseShaderManager: Creating wireframe shader\n");

    const std::string vertexSource =
        "#version 120\n"
        "void main() {\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "}\n";

    const std::string fragmentSource =
        "#version 120\n"
        "uniform vec4 wireframeColor;\n"
        "void main() {\n"
        "    gl_FragColor = wireframeColor;\n"
        "}\n";

    return createProgram("Wireframe", vertexSource, fragmentSource);
}

osg::Program* OsgVerseShaderManager::createFlatShader()
{
    Base::Console().log("OsgVerseShaderManager: Creating flat shading shader\n");

    const std::string vertexSource =
        "#version 120\n"
        "varying vec3 vNormal;\n"
        "void main() {\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "    vNormal = gl_NormalMatrix * gl_Normal;\n"
        "}\n";

    const std::string fragmentSource =
        "#version 120\n"
        "varying vec3 vNormal;\n"
        "uniform vec4 diffuseColor;\n"
        "uniform vec3 lightDirection;\n"
        "void main() {\n"
        "    vec3 N = normalize(vNormal);\n"
        "    vec3 L = normalize(lightDirection);\n"
        "    float NdotL = max(dot(N, L), 0.0);\n"
        "    vec3 color = diffuseColor.rgb * (0.3 + 0.7 * NdotL);\n"
        "    gl_FragColor = vec4(color, diffuseColor.a);\n"
        "}\n";

    return createProgram("Flat", vertexSource, fragmentSource);
}

osg::Program* OsgVerseShaderManager::createUnlitShader()
{
    Base::Console().log("OsgVerseShaderManager: Creating unlit shader\n");

    const std::string vertexSource =
        "#version 120\n"
        "varying vec4 vColor;\n"
        "varying vec2 vTexCoord;\n"
        "void main() {\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "    vColor = gl_Color;\n"
        "    vTexCoord = gl_MultiTexCoord0.xy;\n"
        "}\n";

    const std::string fragmentSource =
        "#version 120\n"
        "varying vec4 vColor;\n"
        "varying vec2 vTexCoord;\n"
        "uniform vec4 baseColor;\n"
        "uniform sampler2D baseColorTexture;\n"
        "uniform bool hasBaseColorTexture;\n"
        "void main() {\n"
        "    vec4 color = baseColor * vColor;\n"
        "    if (hasBaseColorTexture) {\n"
        "        color = color * texture2D(baseColorTexture, vTexCoord);\n"
        "    }\n"
        "    gl_FragColor = color;\n"
        "}\n";

    return createProgram("Unlit", vertexSource, fragmentSource);
}

osg::Program* OsgVerseShaderManager::createToneMapShader()
{
    Base::Console().log("OsgVerseShaderManager: Creating tone mapping shader (GLSL 1.20)\n");

    // Simple pass-through vertex shader for full-screen quad
    const std::string vertexSource =
        "#version 120\n"
        "varying vec2 vTexCoord;\n"
        "void main() {\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "    vTexCoord = gl_MultiTexCoord0.xy;\n"
        "}\n";

    // Tone mapping fragment shader with 3 operators
    // u_toneMapMode: 0=Reinhard, 1=ACES Filmic, 2=Exposure
    const std::string fragmentSource =
        "#version 120\n"
        "varying vec2 vTexCoord;\n"
        "uniform sampler2D u_inputTexture;\n"
        "uniform int u_toneMapMode;\n"
        "uniform float u_exposure;\n"
        "\n"
        "// ACES filmic tone mapping curve\n"
        "vec3 acesFilmic(vec3 x) {\n"
        "    float a = 2.51;\n"
        "    float b = 0.03;\n"
        "    float c = 2.43;\n"
        "    float d = 0.59;\n"
        "    float e = 0.14;\n"
        "    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);\n"
        "}\n"
        "\n"
        "void main() {\n"
        "    vec3 color = texture2D(u_inputTexture, vTexCoord).rgb;\n"
        "\n"
        "    if (u_toneMapMode == 0) {\n"
        "        // Reinhard\n"
        "        color = color / (color + vec3(1.0));\n"
        "    } else if (u_toneMapMode == 1) {\n"
        "        // ACES Filmic\n"
        "        color = acesFilmic(color);\n"
        "    } else {\n"
        "        // Exposure-based\n"
        "        color = vec3(1.0) - vec3(\n"
        "            exp(-color.r * u_exposure),\n"
        "            exp(-color.g * u_exposure),\n"
        "            exp(-color.b * u_exposure));\n"
        "    }\n"
        "\n"
        "    gl_FragColor = vec4(color, 1.0);\n"
        "}\n";

    return createProgram("ToneMap", vertexSource, fragmentSource);
}

osg::Program* OsgVerseShaderManager::createGammaCorrectionShader()
{
    Base::Console().log("OsgVerseShaderManager: Creating gamma correction shader (GLSL 1.20)\n");

    const std::string vertexSource =
        "#version 120\n"
        "varying vec2 vTexCoord;\n"
        "void main() {\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "    vTexCoord = gl_MultiTexCoord0.xy;\n"
        "}\n";

    // Per-component pow to avoid NaN on macOS
    const std::string fragmentSource =
        "#version 120\n"
        "varying vec2 vTexCoord;\n"
        "uniform sampler2D u_inputTexture;\n"
        "uniform float u_gamma;\n"
        "void main() {\n"
        "    vec3 color = texture2D(u_inputTexture, vTexCoord).rgb;\n"
        "    float invGamma = 1.0 / u_gamma;\n"
        "    color.r = pow(max(color.r, 0.0), invGamma);\n"
        "    color.g = pow(max(color.g, 0.0), invGamma);\n"
        "    color.b = pow(max(color.b, 0.0), invGamma);\n"
        "    gl_FragColor = vec4(color, 1.0);\n"
        "}\n";

    return createProgram("GammaCorrection", vertexSource, fragmentSource);
}

osg::Program* OsgVerseShaderManager::createBloomBrightExtractShader()
{
    Base::Console().log("OsgVerseShaderManager: Creating bloom bright extract shader (GLSL 1.20)\n");

    const std::string vertexSource =
        "#version 120\n"
        "varying vec2 vTexCoord;\n"
        "void main() {\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "    vTexCoord = gl_MultiTexCoord0.xy;\n"
        "}\n";

    const std::string fragmentSource =
        "#version 120\n"
        "varying vec2 vTexCoord;\n"
        "uniform sampler2D u_inputTexture;\n"
        "uniform float u_bloomThreshold;\n"
        "void main() {\n"
        "    vec3 color = texture2D(u_inputTexture, vTexCoord).rgb;\n"
        "    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));\n"
        "    if (brightness > u_bloomThreshold) {\n"
        "        gl_FragColor = vec4(color, 1.0);\n"
        "    } else {\n"
        "        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
        "    }\n"
        "}\n";

    return createProgram("BloomBrightExtract", vertexSource, fragmentSource);
}

osg::Program* OsgVerseShaderManager::createBloomBlurShader()
{
    Base::Console().log("OsgVerseShaderManager: Creating bloom blur shader (GLSL 1.20)\n");

    const std::string vertexSource =
        "#version 120\n"
        "varying vec2 vTexCoord;\n"
        "void main() {\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "    vTexCoord = gl_MultiTexCoord0.xy;\n"
        "}\n";

    // 9-tap separable Gaussian blur
    // GLSL 1.20: no array initializers, assign element by element
    const std::string fragmentSource =
        "#version 120\n"
        "varying vec2 vTexCoord;\n"
        "uniform sampler2D u_inputTexture;\n"
        "uniform int u_horizontal;\n"
        "uniform vec2 u_texelSize;\n"
        "void main() {\n"
        "    float weights[5];\n"
        "    weights[0] = 0.227027;\n"
        "    weights[1] = 0.1945946;\n"
        "    weights[2] = 0.1216216;\n"
        "    weights[3] = 0.054054;\n"
        "    weights[4] = 0.016216;\n"
        "    vec3 result = texture2D(u_inputTexture, vTexCoord).rgb * weights[0];\n"
        "    vec2 offset;\n"
        "    if (u_horizontal == 1) {\n"
        "        offset = vec2(u_texelSize.x, 0.0);\n"
        "    } else {\n"
        "        offset = vec2(0.0, u_texelSize.y);\n"
        "    }\n"
        "    for (int i = 1; i < 5; i++) {\n"
        "        result += texture2D(u_inputTexture, vTexCoord + offset * float(i)).rgb * weights[i];\n"
        "        result += texture2D(u_inputTexture, vTexCoord - offset * float(i)).rgb * weights[i];\n"
        "    }\n"
        "    gl_FragColor = vec4(result, 1.0);\n"
        "}\n";

    return createProgram("BloomBlur", vertexSource, fragmentSource);
}

osg::Program* OsgVerseShaderManager::createBloomCompositeShader()
{
    Base::Console().log("OsgVerseShaderManager: Creating bloom composite shader (GLSL 1.20)\n");

    const std::string vertexSource =
        "#version 120\n"
        "varying vec2 vTexCoord;\n"
        "void main() {\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "    vTexCoord = gl_MultiTexCoord0.xy;\n"
        "}\n";

    const std::string fragmentSource =
        "#version 120\n"
        "varying vec2 vTexCoord;\n"
        "uniform sampler2D u_inputTexture;\n"
        "uniform sampler2D u_sceneTexture;\n"
        "uniform float u_bloomIntensity;\n"
        "void main() {\n"
        "    vec3 scene = texture2D(u_sceneTexture, vTexCoord).rgb;\n"
        "    vec3 bloom = texture2D(u_inputTexture, vTexCoord).rgb;\n"
        "    gl_FragColor = vec4(scene + bloom * u_bloomIntensity, 1.0);\n"
        "}\n";

    return createProgram("BloomComposite", vertexSource, fragmentSource);
}

osg::Program* OsgVerseShaderManager::createSSAOShader()
{
    Base::Console().log("OsgVerseShaderManager: Creating SSAO shader (GLSL 1.20)\n");

    const std::string vertexSource =
        "#version 120\n"
        "varying vec2 vTexCoord;\n"
        "void main() {\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "    vTexCoord = gl_MultiTexCoord0.xy;\n"
        "}\n";

    const std::string fragmentSource =
        "#version 120\n"
        "varying vec2 vTexCoord;\n"
        "\n"
        "uniform sampler2D u_inputTexture;\n"
        "uniform sampler2D u_depthTexture;\n"
        "uniform sampler2D u_noiseTexture;\n"
        "\n"
        "uniform vec3 u_ssaoKernel[32];\n"
        "uniform int u_kernelSize;\n"
        "uniform float u_ssaoRadius;\n"
        "uniform float u_ssaoBias;\n"
        "uniform float u_ssaoIntensity;\n"
        "uniform vec2 u_screenSize;\n"
        "uniform mat4 u_projMatrix;\n"
        "uniform mat4 u_invProjMatrix;\n"
        "\n"
        "// Reconstruct view-space position from depth\n"
        "vec3 viewPosFromDepth(vec2 uv, float depth) {\n"
        "    // Convert to NDC [-1, 1]\n"
        "    vec4 ndc = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);\n"
        "    vec4 viewPos = u_invProjMatrix * ndc;\n"
        "    return viewPos.xyz / viewPos.w;\n"
        "}\n"
        "\n"
        "void main() {\n"
        "    float depth = texture2D(u_depthTexture, vTexCoord).r;\n"
        "    // Skip background (depth == 1.0)\n"
        "    if (depth >= 1.0) {\n"
        "        gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
        "        return;\n"
        "    }\n"
        "\n"
        "    vec3 fragPos = viewPosFromDepth(vTexCoord, depth);\n"
        "\n"
        "    // Reconstruct normal from depth using dFdx/dFdy\n"
        "    vec3 dPdx = dFdx(fragPos);\n"
        "    vec3 dPdy = dFdy(fragPos);\n"
        "    vec3 normal = normalize(cross(dPdx, dPdy));\n"
        "\n"
        "    // Tile noise texture over screen (4x4 noise)\n"
        "    vec2 noiseScale = u_screenSize / 4.0;\n"
        "    vec3 randomVec = texture2D(u_noiseTexture, vTexCoord * noiseScale).xyz;\n"
        "\n"
        "    // Gram-Schmidt to build TBN\n"
        "    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));\n"
        "    vec3 bitangent = cross(normal, tangent);\n"
        "    // TBN transforms from tangent space to view space\n"
        "    // mat3 in GLSL 1.20: columns are tangent, bitangent, normal\n"
        "\n"
        "    float occlusion = 0.0;\n"
        "    // Loop with constant upper bound for GLSL 1.20\n"
        "    for (int i = 0; i < 32; i++) {\n"
        "        if (i >= u_kernelSize) break;\n"
        "\n"
        "        // Transform sample from tangent to view space\n"
        "        vec3 s = u_ssaoKernel[i];\n"
        "        vec3 samplePos = tangent * s.x + bitangent * s.y + normal * s.z;\n"
        "        samplePos = fragPos + samplePos * u_ssaoRadius;\n"
        "\n"
        "        // Project sample to screen space\n"
        "        vec4 offset = u_projMatrix * vec4(samplePos, 1.0);\n"
        "        offset.xy /= offset.w;\n"
        "        offset.xy = offset.xy * 0.5 + 0.5;\n"
        "\n"
        "        // Sample depth at projected position\n"
        "        float sampleDepth = texture2D(u_depthTexture, offset.xy).r;\n"
        "        vec3 sampleViewPos = viewPosFromDepth(offset.xy, sampleDepth);\n"
        "\n"
        "        // Range check and occlusion test\n"
        "        float rangeCheck = smoothstep(0.0, 1.0,\n"
        "            u_ssaoRadius / max(abs(fragPos.z - sampleViewPos.z), 0.0001));\n"
        "        occlusion += (sampleViewPos.z >= samplePos.z + u_ssaoBias ? 1.0 : 0.0) * rangeCheck;\n"
        "    }\n"
        "\n"
        "    occlusion = occlusion / float(u_kernelSize);\n"
        "    float ao = 1.0 - occlusion * u_ssaoIntensity;\n"
        "    ao = clamp(ao, 0.0, 1.0);\n"
        "    gl_FragColor = vec4(ao, ao, ao, 1.0);\n"
        "}\n";

    return createProgram("SSAO", vertexSource, fragmentSource);
}

osg::Program* OsgVerseShaderManager::createSSAOBlurShader()
{
    Base::Console().log("OsgVerseShaderManager: Creating SSAO blur shader (GLSL 1.20)\n");

    const std::string vertexSource =
        "#version 120\n"
        "varying vec2 vTexCoord;\n"
        "void main() {\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "    vTexCoord = gl_MultiTexCoord0.xy;\n"
        "}\n";

    // Bilateral 4x4 box blur: depth-aware to preserve edges
    const std::string fragmentSource =
        "#version 120\n"
        "varying vec2 vTexCoord;\n"
        "uniform sampler2D u_inputTexture;\n"
        "uniform sampler2D u_depthTexture;\n"
        "uniform vec2 u_screenSize;\n"
        "\n"
        "void main() {\n"
        "    vec2 texelSize = 1.0 / u_screenSize;\n"
        "    float centerDepth = texture2D(u_depthTexture, vTexCoord).r;\n"
        "    float result = 0.0;\n"
        "    float totalWeight = 0.0;\n"
        "\n"
        "    for (int x = -2; x <= 1; x++) {\n"
        "        for (int y = -2; y <= 1; y++) {\n"
        "            vec2 offset = vec2(float(x), float(y)) * texelSize;\n"
        "            vec2 sampleUV = vTexCoord + offset;\n"
        "            float sampleAO = texture2D(u_inputTexture, sampleUV).r;\n"
        "            float sampleDepth = texture2D(u_depthTexture, sampleUV).r;\n"
        "\n"
        "            // Bilateral weight: reject samples with very different depth\n"
        "            float depthDiff = abs(centerDepth - sampleDepth);\n"
        "            float w = (depthDiff < 0.001) ? 1.0 : 0.0;\n"
        "\n"
        "            result += sampleAO * w;\n"
        "            totalWeight += w;\n"
        "        }\n"
        "    }\n"
        "\n"
        "    result = (totalWeight > 0.0) ? result / totalWeight : texture2D(u_inputTexture, vTexCoord).r;\n"
        "    gl_FragColor = vec4(result, result, result, 1.0);\n"
        "}\n";

    return createProgram("SSAOBlur", vertexSource, fragmentSource);
}

osg::Program* OsgVerseShaderManager::createSSAOCompositeShader()
{
    Base::Console().log("OsgVerseShaderManager: Creating SSAO composite shader (GLSL 1.20)\n");

    const std::string vertexSource =
        "#version 120\n"
        "varying vec2 vTexCoord;\n"
        "void main() {\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "    vTexCoord = gl_MultiTexCoord0.xy;\n"
        "}\n";

    // Composite: multiply scene color by AO factor
    // u_inputTexture = blurred AO (grayscale), u_sceneTexture = scene color
    const std::string fragmentSource =
        "#version 120\n"
        "varying vec2 vTexCoord;\n"
        "uniform sampler2D u_inputTexture;\n"
        "uniform sampler2D u_sceneTexture;\n"
        "\n"
        "void main() {\n"
        "    vec3 sceneColor = texture2D(u_sceneTexture, vTexCoord).rgb;\n"
        "    float ao = texture2D(u_inputTexture, vTexCoord).r;\n"
        "    gl_FragColor = vec4(sceneColor * ao, 1.0);\n"
        "}\n";

    return createProgram("SSAOComposite", vertexSource, fragmentSource);
}

osg::Program* OsgVerseShaderManager::createTAAShader()
{
    Base::Console().log("OsgVerseShaderManager: Creating TAA resolve shader (GLSL 1.20)\n");

    const std::string vertexSource =
        "#version 120\n"
        "varying vec2 vTexCoord;\n"
        "void main() {\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "    vTexCoord = gl_MultiTexCoord0.xy;\n"
        "}\n";

    // TAA resolve fragment shader — GLSL 1.20
    // Reprojects history via depth-based motion vectors, clamps to 3x3 neighborhood
    // Uses int uniform for bool (GLSL 1.20 bool uniform support is spotty)
    const std::string fragmentSource =
        "#version 120\n"
        "varying vec2 vTexCoord;\n"
        "\n"
        "uniform sampler2D u_inputTexture;\n"
        "uniform sampler2D u_historyTexture;\n"
        "uniform sampler2D u_depthTexture;\n"
        "\n"
        "uniform mat4 u_currentMVP;\n"
        "uniform mat4 u_prevMVP;\n"
        "uniform mat4 u_invCurrentMVP;\n"
        "\n"
        "uniform vec2 u_screenSize;\n"
        "uniform float u_blendFactor;\n"
        "uniform int u_historyValid;\n"
        "uniform vec2 u_jitterOffset;\n"
        "\n"
        "void main() {\n"
        "    vec2 unjitteredUV = vTexCoord - u_jitterOffset / u_screenSize;\n"
        "    vec3 currentColor = texture2D(u_inputTexture, unjitteredUV).rgb;\n"
        "\n"
        "    if (u_historyValid == 0) {\n"
        "        gl_FragColor = vec4(currentColor, 1.0);\n"
        "        return;\n"
        "    }\n"
        "\n"
        "    // Reconstruct world position from depth for reprojection\n"
        "    float depth = texture2D(u_depthTexture, vTexCoord).r;\n"
        "    vec4 ndcPos = vec4(vTexCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);\n"
        "    vec4 worldPos = u_invCurrentMVP * ndcPos;\n"
        "    worldPos = worldPos / worldPos.w;\n"
        "\n"
        "    // Reproject to previous frame\n"
        "    vec4 prevClip = u_prevMVP * worldPos;\n"
        "    vec2 prevUV = (prevClip.xy / prevClip.w) * 0.5 + 0.5;\n"
        "\n"
        "    // Off-screen: use current frame only\n"
        "    if (prevUV.x < 0.0 || prevUV.x > 1.0 || prevUV.y < 0.0 || prevUV.y > 1.0) {\n"
        "        gl_FragColor = vec4(currentColor, 1.0);\n"
        "        return;\n"
        "    }\n"
        "\n"
        "    vec3 historyColor = texture2D(u_historyTexture, prevUV).rgb;\n"
        "\n"
        "    // Neighborhood clamping — unrolled 3x3 (GLSL 1.20 safe)\n"
        "    vec2 texelSize = 1.0 / u_screenSize;\n"
        "    vec3 minColor = currentColor;\n"
        "    vec3 maxColor = currentColor;\n"
        "    vec3 n;\n"
        "    n = texture2D(u_inputTexture, unjitteredUV + vec2(-1.0, -1.0) * texelSize).rgb;\n"
        "    minColor = min(minColor, n); maxColor = max(maxColor, n);\n"
        "    n = texture2D(u_inputTexture, unjitteredUV + vec2( 0.0, -1.0) * texelSize).rgb;\n"
        "    minColor = min(minColor, n); maxColor = max(maxColor, n);\n"
        "    n = texture2D(u_inputTexture, unjitteredUV + vec2( 1.0, -1.0) * texelSize).rgb;\n"
        "    minColor = min(minColor, n); maxColor = max(maxColor, n);\n"
        "    n = texture2D(u_inputTexture, unjitteredUV + vec2(-1.0,  0.0) * texelSize).rgb;\n"
        "    minColor = min(minColor, n); maxColor = max(maxColor, n);\n"
        "    n = texture2D(u_inputTexture, unjitteredUV + vec2( 1.0,  0.0) * texelSize).rgb;\n"
        "    minColor = min(minColor, n); maxColor = max(maxColor, n);\n"
        "    n = texture2D(u_inputTexture, unjitteredUV + vec2(-1.0,  1.0) * texelSize).rgb;\n"
        "    minColor = min(minColor, n); maxColor = max(maxColor, n);\n"
        "    n = texture2D(u_inputTexture, unjitteredUV + vec2( 0.0,  1.0) * texelSize).rgb;\n"
        "    minColor = min(minColor, n); maxColor = max(maxColor, n);\n"
        "    n = texture2D(u_inputTexture, unjitteredUV + vec2( 1.0,  1.0) * texelSize).rgb;\n"
        "    minColor = min(minColor, n); maxColor = max(maxColor, n);\n"
        "\n"
        "    // Clamp history to neighborhood\n"
        "    historyColor = clamp(historyColor, minColor, maxColor);\n"
        "\n"
        "    // Blend: low alpha = more history, high alpha = more current\n"
        "    vec3 result = mix(historyColor, currentColor, u_blendFactor);\n"
        "    gl_FragColor = vec4(result, 1.0);\n"
        "}\n";

    return createProgram("TAAResolve", vertexSource, fragmentSource);
}
