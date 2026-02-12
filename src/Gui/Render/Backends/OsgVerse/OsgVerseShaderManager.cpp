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

        Base::Console().log("OsgVerseShaderManager: Created shader program for type %d\n",
                           static_cast<int>(type));
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
    Base::Console().log("OsgVerseShaderManager: Creating standard Phong shader\n");

    // 标准Phong顶点着色器 / Standard Phong vertex shader
    const char* vertexSource = R"(
        #version 330 core

        // 输入属性 / Input attributes
        in vec3 osg_Vertex;
        in vec3 osg_Normal;
        in vec4 osg_Color;
        in vec2 osg_MultiTexCoord0;

        // 输出到片段着色器 / Output to fragment shader
        out vec3 vPosition;
        out vec3 vNormal;
        out vec4 vColor;
        out vec2 vTexCoord;

        // Uniforms
        uniform mat4 osg_ModelViewProjectionMatrix;
        uniform mat4 osg_ModelViewMatrix;
        uniform mat3 osg_NormalMatrix;

        void main()
        {
            // 变换顶点位置 / Transform vertex position
            gl_Position = osg_ModelViewProjectionMatrix * vec4(osg_Vertex, 1.0);

            // 视图空间位置 / View space position
            vPosition = (osg_ModelViewMatrix * vec4(osg_Vertex, 1.0)).xyz;

            // 变换法线 / Transform normal
            vNormal = normalize(osg_NormalMatrix * osg_Normal);

            // 传递颜色和纹理坐标 / Pass color and texture coordinates
            vColor = osg_Color;
            vTexCoord = osg_MultiTexCoord0;
        }
    )";

    // 标准Phong片段着色器 / Standard Phong fragment shader
    const char* fragmentSource = R"(
        #version 330 core

        // 输入 / Inputs
        in vec3 vPosition;
        in vec3 vNormal;
        in vec4 vColor;
        in vec2 vTexCoord;

        // 输出 / Output
        out vec4 FragColor;

        // 材质参数 / Material parameters
        uniform vec4 ambientColor;
        uniform vec4 diffuseColor;
        uniform vec4 specularColor;
        uniform vec4 emissiveColor;
        uniform float shininess;
        uniform float opacity;

        // 纹理 / Textures
        uniform sampler2D baseColorTexture;
        uniform bool hasBaseColorTexture;

        // 光照参数 / Lighting parameters
        uniform vec3 lightDirection;
        uniform vec4 lightColor;
        uniform float lightIntensity;

        void main()
        {
            // 归一化法线 / Normalize normal
            vec3 N = normalize(vNormal);
            vec3 L = normalize(lightDirection);
            vec3 V = normalize(-vPosition);
            vec3 H = normalize(L + V);

            // 基础颜色 / Base color
            vec4 baseColor = diffuseColor;
            if (hasBaseColorTexture) {
                baseColor *= texture(baseColorTexture, vTexCoord);
            }

            // 环境光 / Ambient
            vec3 ambient = ambientColor.rgb * baseColor.rgb;

            // 漫反射 / Diffuse
            float NdotL = max(dot(N, L), 0.0);
            vec3 diffuse = NdotL * lightColor.rgb * baseColor.rgb * lightIntensity;

            // 镜面反射 / Specular
            float NdotH = max(dot(N, H), 0.0);
            float spec = pow(NdotH, shininess);
            vec3 specular = spec * specularColor.rgb * lightColor.rgb * lightIntensity;

            // 自发光 / Emissive
            vec3 emissive = emissiveColor.rgb;

            // 最终颜色 / Final color
            vec3 finalColor = ambient + diffuse + specular + emissive;
            FragColor = vec4(finalColor, baseColor.a * opacity);
        }
    )";

    return createProgram("StandardPhong", vertexSource, fragmentSource);
}

osg::Program* OsgVerseShaderManager::createPBRShader()
{
    Base::Console().log("OsgVerseShaderManager: Creating PBR shader\n");

    // PBR顶点着色器 / PBR vertex shader
    const char* vertexSource = R"(
        #version 330 core

        // 输入属性 / Input attributes
        in vec3 osg_Vertex;
        in vec3 osg_Normal;
        in vec4 osg_Color;
        in vec2 osg_MultiTexCoord0;
        in vec3 osg_Tangent;

        // 输出到片段着色器 / Output to fragment shader
        out vec3 vWorldPos;
        out vec3 vNormal;
        out vec2 vTexCoord;
        out vec4 vColor;
        out mat3 vTBN;

        // Uniforms
        uniform mat4 osg_ModelViewProjectionMatrix;
        uniform mat4 osg_ModelViewMatrix;
        uniform mat3 osg_NormalMatrix;
        uniform mat4 osg_ViewMatrixInverse;

        void main()
        {
            // 变换顶点位置 / Transform vertex position
            gl_Position = osg_ModelViewProjectionMatrix * vec4(osg_Vertex, 1.0);

            // 世界空间位置 / World space position
            vec4 worldPos = osg_ViewMatrixInverse * osg_ModelViewMatrix * vec4(osg_Vertex, 1.0);
            vWorldPos = worldPos.xyz;

            // 变换法线 / Transform normal
            vNormal = normalize(osg_NormalMatrix * osg_Normal);

            // 计算TBN矩阵（用于法线贴图）/ Calculate TBN matrix (for normal mapping)
            vec3 T = normalize(osg_NormalMatrix * osg_Tangent);
            vec3 N = vNormal;
            vec3 B = cross(N, T);
            vTBN = mat3(T, B, N);

            // 传递纹理坐标和颜色 / Pass texture coordinates and color
            vTexCoord = osg_MultiTexCoord0;
            vColor = osg_Color;
        }
    )";

    // PBR片段着色器 / PBR fragment shader
    const char* fragmentSource = R"(
        #version 330 core

        // 输入 / Inputs
        in vec3 vWorldPos;
        in vec3 vNormal;
        in vec2 vTexCoord;
        in vec4 vColor;
        in mat3 vTBN;

        // 输出 / Output
        out vec4 FragColor;

        // PBR材质参数 / PBR material parameters
        uniform vec4 baseColor;
        uniform float metallic;
        uniform float roughness;
        uniform vec4 emissive;
        uniform float ambientOcclusion;

        // 纹理 / Textures
        uniform sampler2D baseColorTexture;
        uniform sampler2D normalTexture;
        uniform sampler2D metallicRoughnessTexture;
        uniform sampler2D occlusionTexture;
        uniform sampler2D emissiveTexture;

        uniform bool hasBaseColorTexture;
        uniform bool hasNormalTexture;
        uniform bool hasMetallicRoughnessTexture;
        uniform bool hasOcclusionTexture;
        uniform bool hasEmissiveTexture;

        // 光照参数 / Lighting parameters
        uniform vec3 lightDirection;
        uniform vec4 lightColor;
        uniform float lightIntensity;
        uniform vec3 cameraPosition;

        // 常量 / Constants
        const float PI = 3.14159265359;

        // PBR函数 / PBR functions

        // 法线分布函数 (GGX/Trowbridge-Reitz) / Normal Distribution Function
        float DistributionGGX(vec3 N, vec3 H, float roughness)
        {
            float a = roughness * roughness;
            float a2 = a * a;
            float NdotH = max(dot(N, H), 0.0);
            float NdotH2 = NdotH * NdotH;

            float nom = a2;
            float denom = (NdotH2 * (a2 - 1.0) + 1.0);
            denom = PI * denom * denom;

            return nom / denom;
        }

        // 几何遮蔽函数 (Smith) / Geometry Shadowing Function
        float GeometrySchlickGGX(float NdotV, float roughness)
        {
            float r = (roughness + 1.0);
            float k = (r * r) / 8.0;

            float nom = NdotV;
            float denom = NdotV * (1.0 - k) + k;

            return nom / denom;
        }

        float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
        {
            float NdotV = max(dot(N, V), 0.0);
            float NdotL = max(dot(N, L), 0.0);
            float ggx2 = GeometrySchlickGGX(NdotV, roughness);
            float ggx1 = GeometrySchlickGGX(NdotL, roughness);

            return ggx1 * ggx2;
        }

        // Fresnel方程 (Schlick近似) / Fresnel Equation (Schlick approximation)
        vec3 fresnelSchlick(float cosTheta, vec3 F0)
        {
            return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
        }

        void main()
        {
            // 获取基础颜色 / Get base color
            vec4 albedo = baseColor;
            if (hasBaseColorTexture) {
                albedo *= texture(baseColorTexture, vTexCoord);
            }

            // 获取法线 / Get normal
            vec3 N = normalize(vNormal);
            if (hasNormalTexture) {
                vec3 normalMap = texture(normalTexture, vTexCoord).xyz * 2.0 - 1.0;
                N = normalize(vTBN * normalMap);
            }

            // 获取金属度和粗糙度 / Get metallic and roughness
            float metallicValue = metallic;
            float roughnessValue = roughness;
            if (hasMetallicRoughnessTexture) {
                vec4 mr = texture(metallicRoughnessTexture, vTexCoord);
                metallicValue *= mr.b;  // 金属度在蓝色通道 / Metallic in blue channel
                roughnessValue *= mr.g; // 粗糙度在绿色通道 / Roughness in green channel
            }

            // 获取环境光遮蔽 / Get ambient occlusion
            float ao = ambientOcclusion;
            if (hasOcclusionTexture) {
                ao *= texture(occlusionTexture, vTexCoord).r;
            }

            // 计算光照向量 / Calculate lighting vectors
            vec3 V = normalize(cameraPosition - vWorldPos);
            vec3 L = normalize(lightDirection);
            vec3 H = normalize(V + L);

            // 计算F0（表面反射率）/ Calculate F0 (surface reflectance)
            vec3 F0 = vec3(0.04);
            F0 = mix(F0, albedo.rgb, metallicValue);

            // Cook-Torrance BRDF
            float NDF = DistributionGGX(N, H, roughnessValue);
            float G = GeometrySmith(N, V, L, roughnessValue);
            vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

            vec3 numerator = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
            vec3 specular = numerator / denominator;

            // 能量守恒 / Energy conservation
            vec3 kS = F;
            vec3 kD = vec3(1.0) - kS;
            kD *= 1.0 - metallicValue;

            // 漫反射 / Diffuse
            float NdotL = max(dot(N, L), 0.0);
            vec3 diffuse = kD * albedo.rgb / PI;

            // 最终光照 / Final lighting
            vec3 radiance = lightColor.rgb * lightIntensity;
            vec3 Lo = (diffuse + specular) * radiance * NdotL;

            // 环境光 / Ambient
            vec3 ambient = vec3(0.03) * albedo.rgb * ao;

            // 自发光 / Emissive
            vec3 emissiveColor = emissive.rgb;
            if (hasEmissiveTexture) {
                emissiveColor *= texture(emissiveTexture, vTexCoord).rgb;
            }

            // 最终颜色 / Final color
            vec3 color = ambient + Lo + emissiveColor;

            // HDR色调映射 (简单的Reinhard) / HDR tone mapping (simple Reinhard)
            color = color / (color + vec3(1.0));

            // Gamma校正 / Gamma correction
            color = pow(color, vec3(1.0/2.2));

            FragColor = vec4(color, albedo.a);
        }
    )";

    return createProgram("PBR", vertexSource, fragmentSource);
}

osg::Program* OsgVerseShaderManager::createWireframeShader()
{
    Base::Console().log("OsgVerseShaderManager: Creating wireframe shader\n");

    const char* vertexSource = R"(
        #version 330 core

        in vec3 osg_Vertex;
        uniform mat4 osg_ModelViewProjectionMatrix;

        void main()
        {
            gl_Position = osg_ModelViewProjectionMatrix * vec4(osg_Vertex, 1.0);
        }
    )";

    const char* fragmentSource = R"(
        #version 330 core

        out vec4 FragColor;
        uniform vec4 wireframeColor;

        void main()
        {
            FragColor = wireframeColor;
        }
    )";

    return createProgram("Wireframe", vertexSource, fragmentSource);
}

osg::Program* OsgVerseShaderManager::createFlatShader()
{
    Base::Console().log("OsgVerseShaderManager: Creating flat shading shader\n");

    const char* vertexSource = R"(
        #version 330 core

        in vec3 osg_Vertex;
        in vec3 osg_Normal;

        out vec3 vNormal;

        uniform mat4 osg_ModelViewProjectionMatrix;
        uniform mat3 osg_NormalMatrix;

        void main()
        {
            gl_Position = osg_ModelViewProjectionMatrix * vec4(osg_Vertex, 1.0);
            vNormal = osg_NormalMatrix * osg_Normal;
        }
    )";

    const char* fragmentSource = R"(
        #version 330 core

        in vec3 vNormal;
        out vec4 FragColor;

        uniform vec4 diffuseColor;
        uniform vec3 lightDirection;

        void main()
        {
            vec3 N = normalize(vNormal);
            vec3 L = normalize(lightDirection);
            float NdotL = max(dot(N, L), 0.0);

            vec3 color = diffuseColor.rgb * (0.3 + 0.7 * NdotL);
            FragColor = vec4(color, diffuseColor.a);
        }
    )";

    return createProgram("Flat", vertexSource, fragmentSource);
}

osg::Program* OsgVerseShaderManager::createUnlitShader()
{
    Base::Console().log("OsgVerseShaderManager: Creating unlit shader\n");

    const char* vertexSource = R"(
        #version 330 core

        in vec3 osg_Vertex;
        in vec4 osg_Color;
        in vec2 osg_MultiTexCoord0;

        out vec4 vColor;
        out vec2 vTexCoord;

        uniform mat4 osg_ModelViewProjectionMatrix;

        void main()
        {
            gl_Position = osg_ModelViewProjectionMatrix * vec4(osg_Vertex, 1.0);
            vColor = osg_Color;
            vTexCoord = osg_MultiTexCoord0;
        }
    )";

    const char* fragmentSource = R"(
        #version 330 core

        in vec4 vColor;
        in vec2 vTexCoord;

        out vec4 FragColor;

        uniform vec4 baseColor;
        uniform sampler2D baseColorTexture;
        uniform bool hasBaseColorTexture;

        void main()
        {
            vec4 color = baseColor * vColor;
            if (hasBaseColorTexture) {
                color *= texture(baseColorTexture, vTexCoord);
            }
            FragColor = color;
        }
    )";

    return createProgram("Unlit", vertexSource, fragmentSource);
}
