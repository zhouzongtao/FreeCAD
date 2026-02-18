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
    Base::Console().log("OsgVerseShaderManager: Creating standard Phong shader (GLSL 1.20)\n");

    // GLSL 1.20 Phong shader using a custom uniform u_baseColor for the material color.
    // This is the most reliable approach on macOS GL 2.1 because:
    // - gl_FrontMaterial might not be forwarded to shader on macOS
    // - gl_Color (vertex colors) + BIND_OVERALL might not work with shaders
    // - osg::Uniform is always reliably passed to the shader
    const char* vertexSource =
        "#version 120\n"
        "varying vec3 vPosition;\n"
        "varying vec3 vNormal;\n"
        "varying vec4 vColor;\n"
        "void main() {\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "    vPosition = (gl_ModelViewMatrix * gl_Vertex).xyz;\n"
        "    vNormal = normalize(gl_NormalMatrix * gl_Normal);\n"
        "    vColor = gl_Color;\n"
        "}\n";

    const char* fragmentSource =
        "#version 120\n"
        "varying vec3 vPosition;\n"
        "varying vec3 vNormal;\n"
        "varying vec4 vColor;\n"
        "uniform vec4 u_baseColor;\n"
        "uniform int u_colorMode;\n"
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
        "    vec4 baseColor = (u_colorMode == 1) ? vColor : u_baseColor;\n"
        "    // Ambient\n"
        "    vec3 ambient = baseColor.rgb * 0.3;\n"
        "    // Diffuse (Lambertian)\n"
        "    float NdotL = max(dot(N, L), 0.0);\n"
        "    vec3 diffuse = baseColor.rgb * NdotL * 0.7;\n"
        "    // Specular (Blinn-Phong)\n"
        "    float NdotH = max(dot(N, H), 0.0);\n"
        "    float spec = pow(NdotH, 40.0);\n"
        "    vec3 specular = vec3(0.3) * spec;\n"
        "    // Backlight from gl_LightSource[1]\n"
        "    vec3 backPos = gl_LightSource[1].position.xyz;\n"
        "    float backContrib = 0.0;\n"
        "    if (dot(backPos, backPos) > 0.001) {\n"
        "        vec3 L1 = normalize(backPos);\n"
        "        backContrib = max(dot(N, L1), 0.0) * 0.2;\n"
        "    }\n"
        "    vec3 backDiffuse = baseColor.rgb * backContrib;\n"
        "    gl_FragColor = vec4(ambient + diffuse + specular + backDiffuse, baseColor.a);\n"
        "}\n";

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
        const int MAX_LIGHTS = 8;

        struct Light {
            int type;              // 0=Directional, 1=Point, 2=Spot
            vec3 color;            // 光源颜色*强度 / Light color * intensity
            vec3 position;         // 位置（点光源和聚光灯）/ Position (point and spot)
            vec3 direction;        // 方向（方向光和聚光灯）/ Direction (directional and spot)
            vec3 attenuation;      // 衰减参数 (constant, linear, quadratic)
            float range;           // 范围 / Range
            vec2 coneAngles;       // 聚光灯锥角 (inner, outer) cosine values
        };

        uniform int u_numLights;
        uniform Light u_lights[MAX_LIGHTS];
        uniform vec3 u_ambientLight;
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

        // 计算单个光源的贡献 / Calculate contribution from a single light
        vec3 calculateLightContribution(Light light, vec3 N, vec3 V, vec3 albedo, float metallicValue, float roughnessValue, vec3 F0)
        {
            vec3 L;
            float attenuation = 1.0;

            // 根据光源类型计算光照方向和衰减 / Calculate light direction and attenuation based on light type
            if (light.type == 0) {
                // 方向光 / Directional light
                L = normalize(-light.direction);
            }
            else if (light.type == 1) {
                // 点光源 / Point light
                vec3 lightVec = light.position - vWorldPos;
                float distance = length(lightVec);
                L = normalize(lightVec);

                // 衰减计算 / Attenuation calculation
                if (distance < light.range) {
                    attenuation = 1.0 / (light.attenuation.x +
                                        light.attenuation.y * distance +
                                        light.attenuation.z * distance * distance);
                } else {
                    attenuation = 0.0;
                }
            }
            else if (light.type == 2) {
                // 聚光灯 / Spot light
                vec3 lightVec = light.position - vWorldPos;
                float distance = length(lightVec);
                L = normalize(lightVec);

                // 衰减计算 / Attenuation calculation
                if (distance < light.range) {
                    attenuation = 1.0 / (light.attenuation.x +
                                        light.attenuation.y * distance +
                                        light.attenuation.z * distance * distance);

                    // 聚光灯锥形衰减 / Spot cone attenuation
                    float theta = dot(L, normalize(-light.direction));
                    float epsilon = light.coneAngles.x - light.coneAngles.y;
                    float intensity = clamp((theta - light.coneAngles.y) / epsilon, 0.0, 1.0);
                    attenuation *= intensity;
                } else {
                    attenuation = 0.0;
                }
            }

            if (attenuation <= 0.0) {
                return vec3(0.0);
            }

            vec3 H = normalize(V + L);

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
            vec3 diffuse = kD * albedo / PI;

            // 最终光照 / Final lighting
            vec3 radiance = light.color * attenuation;
            return (diffuse + specular) * radiance * NdotL;
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

            // 计算视线向量 / Calculate view vector
            vec3 V = normalize(cameraPosition - vWorldPos);

            // 计算F0（表面反射率）/ Calculate F0 (surface reflectance)
            vec3 F0 = vec3(0.04);
            F0 = mix(F0, albedo.rgb, metallicValue);

            // 累积所有光源的贡献 / Accumulate contributions from all lights
            vec3 Lo = vec3(0.0);
            for (int i = 0; i < u_numLights && i < MAX_LIGHTS; ++i) {
                Lo += calculateLightContribution(u_lights[i], N, V, albedo.rgb, metallicValue, roughnessValue, F0);
            }

            // 环境光 / Ambient
            vec3 ambient = u_ambientLight * albedo.rgb * ao;

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
