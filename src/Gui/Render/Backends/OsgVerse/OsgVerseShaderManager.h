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

#ifndef GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSESHADERMANAGER_H
#define GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSESHADERMANAGER_H

#include <memory>
#include <string>
#include <unordered_map>

#include <osg/Program>
#include <osg/Shader>
#include <osg/StateSet>

#include <FCGlobal.h>

namespace Gui {
namespace Render {

/**
 * @brief Common GLSL 1.20 shader code snippets for reuse across shaders
 *
 * All snippets are GLSL 1.20 compatible for macOS GL 2.1.
 * Use by concatenating into shader source strings.
 */
namespace ShaderSnippets {

/// Reinhard tone mapping: color / (color + 1)
inline const char* toneMapReinhard() {
    return
        "vec3 toneMapReinhard(vec3 color) {\n"
        "    return color / (color + vec3(1.0));\n"
        "}\n";
}

/// Per-component gamma correction (avoids pow(vec3) which can NaN on macOS)
inline const char* gammaCorrect() {
    return
        "vec3 gammaCorrect(vec3 color) {\n"
        "    return vec3(\n"
        "        pow(max(color.r, 0.0), 1.0/2.2),\n"
        "        pow(max(color.g, 0.0), 1.0/2.2),\n"
        "        pow(max(color.b, 0.0), 1.0/2.2));\n"
        "}\n";
}

/// Simple Lambertian diffuse factor
inline const char* lambertian() {
    return
        "float lambertian(vec3 N, vec3 L) {\n"
        "    return max(dot(N, L), 0.0);\n"
        "}\n";
}

/// Blinn-Phong specular factor
inline const char* blinnPhong() {
    return
        "float blinnPhong(vec3 N, vec3 H, float shininess) {\n"
        "    return pow(max(dot(N, H), 0.0), shininess);\n"
        "}\n";
}

/// Simple string-based include preprocessor
/// Replaces occurrences of `#pragma include "snippet_name"` with actual code
std::string processIncludes(const std::string& source);

} // namespace ShaderSnippets

/**
 * @brief Shader类型枚举 / Shader type enumeration
 */
enum class ShaderType {
    Standard,           ///< 标准Phong着色 / Standard Phong shading
    PBR,               ///< PBR金属/粗糙度工作流 / PBR metallic/roughness workflow
    Wireframe,         ///< 线框渲染 / Wireframe rendering
    Flat,              ///< 平面着色 / Flat shading
    Unlit,             ///< 无光照 / Unlit
    ToneMap,           ///< HDR tone mapping post-process
    GammaCorrection,   ///< Final gamma correction post-process
    BloomBrightExtract,///< Bloom bright area extraction
    BloomBlur,         ///< Bloom Gaussian blur (separable)
    BloomComposite,    ///< Bloom composite (scene + bloom)
    TAA,               ///< Temporal Anti-Aliasing resolve
    SSAO,              ///< Screen-space ambient occlusion
    SSAOBlur,          ///< SSAO bilateral blur
    SSAOComposite      ///< SSAO composite (scene * AO)
};

/**
 * @brief Shader程序信息 / Shader program information
 */
struct ShaderProgramInfo {
    osg::ref_ptr<osg::Program> program;
    std::string name;
    ShaderType type;
    bool compiled{false};
};

/**
 * @brief OsgVerse Shader管理器 / OsgVerse Shader Manager
 *
 * 负责管理和缓存shader程序，提供统一的shader访问接口。
 * Manages and caches shader programs, provides unified shader access interface.
 *
 * 设计原则 / Design Principles:
 * - 单例模式：全局唯一的shader管理器
 * - 延迟加载：shader在首次使用时才编译
 * - 缓存机制：避免重复编译相同的shader
 * - 热重载：支持运行时重新加载shader（开发模式）
 */
class GuiExport OsgVerseShaderManager {
public:
    /**
     * @brief 获取单例实例 / Get singleton instance
     */
    static OsgVerseShaderManager& instance();

    /**
     * @brief 获取shader程序 / Get shader program
     *
     * @param type Shader类型 / Shader type
     * @return osg::Program指针，失败返回nullptr / osg::Program pointer, nullptr on failure
     */
    osg::Program* getProgram(ShaderType type);

    /**
     * @brief 创建自定义shader程序 / Create custom shader program
     *
     * @param name 程序名称 / Program name
     * @param vertexSource 顶点着色器源码 / Vertex shader source
     * @param fragmentSource 片段着色器源码 / Fragment shader source
     * @return osg::Program指针 / osg::Program pointer
     */
    osg::Program* createProgram(const std::string& name,
                               const std::string& vertexSource,
                               const std::string& fragmentSource);

    /**
     * @brief 从文件加载shader程序 / Load shader program from files
     *
     * @param name 程序名称 / Program name
     * @param vertexFile 顶点着色器文件路径 / Vertex shader file path
     * @param fragmentFile 片段着色器文件路径 / Fragment shader file path
     * @return osg::Program指针 / osg::Program pointer
     */
    osg::Program* loadProgramFromFiles(const std::string& name,
                                       const std::string& vertexFile,
                                       const std::string& fragmentFile);

    /**
     * @brief 应用shader到StateSet / Apply shader to StateSet
     *
     * @param stateSet 目标StateSet / Target StateSet
     * @param type Shader类型 / Shader type
     * @return 是否成功 / Whether successful
     */
    bool applyShader(osg::StateSet* stateSet, ShaderType type);

    /**
     * @brief 重新加载所有shader / Reload all shaders
     *
     * 用于开发时热重载shader。
     * Used for hot-reloading shaders during development.
     */
    void reloadAll();

    /**
     * @brief 清除所有缓存的shader / Clear all cached shaders
     */
    void clearCache();

    /**
     * @brief 获取shader源码路径 / Get shader source path
     */
    std::string getShaderPath() const { return _shaderPath; }

    /**
     * @brief 设置shader源码路径 / Set shader source path
     */
    void setShaderPath(const std::string& path) { _shaderPath = path; }

    /**
     * @brief 检查shader是否已编译 / Check if shader is compiled
     */
    bool isCompiled(ShaderType type) const;

    /**
     * @brief 获取编译错误信息 / Get compilation error message
     */
    std::string getCompileError(ShaderType type) const;

private:
    OsgVerseShaderManager();
    ~OsgVerseShaderManager() = default;

    // 禁止拷贝和移动 / Disable copy and move
    OsgVerseShaderManager(const OsgVerseShaderManager&) = delete;
    OsgVerseShaderManager& operator=(const OsgVerseShaderManager&) = delete;

    /**
     * @brief 初始化内置shader / Initialize built-in shaders
     */
    void initializeBuiltinShaders();

    /**
     * @brief 创建标准Phong shader / Create standard Phong shader
     */
    osg::Program* createStandardShader();

    /**
     * @brief 创建PBR shader / Create PBR shader
     */
    osg::Program* createPBRShader();

    /**
     * @brief 创建线框shader / Create wireframe shader
     */
    osg::Program* createWireframeShader();

    /**
     * @brief 创建平面着色shader / Create flat shading shader
     */
    osg::Program* createFlatShader();

    /**
     * @brief 创建无光照shader / Create unlit shader
     */
    osg::Program* createUnlitShader();

    /**
     * @brief Create HDR tone mapping post-process shader
     */
    osg::Program* createToneMapShader();

    /**
     * @brief Create gamma correction post-process shader
     */
    osg::Program* createGammaCorrectionShader();

    /**
     * @brief Create bloom bright extraction shader
     */
    osg::Program* createBloomBrightExtractShader();

    /**
     * @brief Create bloom Gaussian blur shader (separable)
     */
    osg::Program* createBloomBlurShader();

    /**
     * @brief Create bloom composite shader
     */
    osg::Program* createBloomCompositeShader();

    /**
     * @brief Create TAA resolve shader
     */
    osg::Program* createTAAShader();

    /**
     * @brief Create SSAO shader
     */
    osg::Program* createSSAOShader();

    /**
     * @brief Create SSAO bilateral blur shader
     */
    osg::Program* createSSAOBlurShader();

    /**
     * @brief Create SSAO composite shader
     */
    osg::Program* createSSAOCompositeShader();

    /**
     * @brief 从字符串创建shader / Create shader from string
     */
    osg::Shader* createShaderFromSource(osg::Shader::Type type,
                                        const std::string& source,
                                        const std::string& name);

    /**
     * @brief 从文件读取shader源码 / Read shader source from file
     */
    std::string readShaderFile(const std::string& filename);

    // Shader程序缓存 / Shader program cache
    std::unordered_map<ShaderType, ShaderProgramInfo> _programs;
    std::unordered_map<std::string, osg::ref_ptr<osg::Program>> _customPrograms;

    // Shader源码路径 / Shader source path
    std::string _shaderPath;

    // 编译错误信息 / Compilation error messages
    std::unordered_map<ShaderType, std::string> _compileErrors;
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSESHADERMANAGER_H
