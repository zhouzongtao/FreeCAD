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
 *   but WITHOUT ANY WARRANTY; without even the implied of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEMATERIAL_H
#define GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEMATERIAL_H

#include <memory>
#include <string>
#include <unordered_map>

#include <osg/StateSet>
#include <osg/Material>
#include <osg/Texture2D>
#include <osg/Image>

#include <FCGlobal.h>
#include "../../Core/RenderTypes.h"

namespace Gui {
namespace Render {

/**
 * @brief OsgVerse 材质系统 / OsgVerse material system
 *
 * 提供 PBR（物理基础渲染）材质支持，通过 OSG StateSet 和 Uniforms 实现。
 * Provides PBR (Physically Based Rendering) material support via OSG StateSet and Uniforms.
 *
 * 设计原则 / Design Principles:
 * 1. 支持 PBR 金属/粗糙度工作流
 * 2. 兼容 Coin3D 固定管线材质
 * 3. 支持多种纹理贴图
 * 4. 高效的状态管理
 */
class GuiExport OsgVerseMaterial {
public:
    /**
     * @brief 构造函数 / Constructor
     */
    OsgVerseMaterial();

    /**
     * @brief 析构函数 / Destructor
     */
    ~OsgVerseMaterial();

    // 禁止拷贝，允许移动 / Disable copy, allow move
    OsgVerseMaterial(const OsgVerseMaterial&) = delete;
    OsgVerseMaterial& operator=(const OsgVerseMaterial&) = delete;
    OsgVerseMaterial(OsgVerseMaterial&&) = default;
    OsgVerseMaterial& operator=(OsgVerseMaterial&&) = default;

    /**
     * @brief 获取 OSG StateSet / Get OSG StateSet
     */
    osg::StateSet* getStateSet() const { return _stateSet.get(); }

    //-----------------------------------------------------------------------
    // PBR 材质参数 / PBR Material Parameters
    //-----------------------------------------------------------------------

    /**
     * @brief 设置基础颜色 / Set base color
     *
     * PBR 中的 albedo/diffuse 颜色。
     * Albedo/diffuse color in PBR.
     */
    void setBaseColor(const Color& color);
    Color getBaseColor() const;

    /**
     * @brief 设置金属度 / Set metallic
     *
     * 0.0 = 非金属（电介质），1.0 = 金属
     * 0.0 = non-metallic (dielectric), 1.0 = metallic
     */
    void setMetallic(float metallic);
    float getMetallic() const;

    /**
     * @brief 设置粗糙度 / Set roughness
     *
     * 0.0 = 光滑（镜面），1.0 = 粗糙（漫反射）
     * 0.0 = smooth (specular), 1.0 = rough (diffuse)
     */
    void setRoughness(float roughness);
    float getRoughness() const;

    /**
     * @brief 设置自发光颜色 / Set emissive color
     */
    void setEmissive(const Color& color);
    Color getEmissive() const;

    /**
     * @brief 设置透明度 / Set transparency
     *
     * 0.0 = 完全透明，1.0 = 完全不透明
     * 0.0 = fully transparent, 1.0 = fully opaque
     */
    void setOpacity(float opacity);
    float getOpacity() const;

    /**
     * @brief 设置环境光遮蔽 / Set ambient occlusion
     */
    void setAmbientOcclusion(float ao);
    float getAmbientOcclusion() const;

    //-----------------------------------------------------------------------
    // 纹理贴图 / Texture Maps
    //-----------------------------------------------------------------------

    /**
     * @brief 纹理类型 / Texture type
     */
    enum class TextureType {
        BaseColor,          ///< 基础颜色贴图 / Base color map
        Normal,             ///< 法线贴图 / Normal map
        MetallicRoughness,  ///< 金属度/粗糙度贴图 / Metallic/Roughness map
        Occlusion,          ///< 环境光遮蔽贴图 / Ambient occlusion map
        Emissive,           ///< 自发光贴图 / Emissive map
        Height,             ///< 高度贴图 / Height map
        Opacity             ///< 透明度贴图 / Opacity map
    };

    /**
     * @brief 设置纹理 / Set texture
     *
     * @param type 纹理类型 / Texture type
     * @param filename 纹理文件路径 / Texture file path
     * @return 是否成功 / Whether successful
     */
    bool setTexture(TextureType type, const std::string& filename);

    /**
     * @brief 设置纹理（从图像数据）/ Set texture (from image data)
     */
    bool setTexture(TextureType type, osg::Image* image);

    /**
     * @brief 移除纹理 / Remove texture
     */
    void removeTexture(TextureType type);

    /**
     * @brief 检查是否有纹理 / Check if has texture
     */
    bool hasTexture(TextureType type) const;

    /**
     * @brief 获取纹理 / Get texture
     */
    osg::Texture2D* getTexture(TextureType type) const;

    //-----------------------------------------------------------------------
    // Coin3D 兼容性 / Coin3D Compatibility
    //-----------------------------------------------------------------------

    /**
     * @brief 从 Coin3D 材质转换 / Convert from Coin3D material
     *
     * 将 Coin3D 固定管线材质参数转换为 PBR 参数。
     * Convert Coin3D fixed-function material parameters to PBR parameters.
     */
    void setFromCoin3DMaterial(const Material& coinMaterial);

    /**
     * @brief 设置环境光颜色（Coin3D 兼容）/ Set ambient color (Coin3D compatibility)
     */
    void setAmbientColor(const Color& color);
    Color getAmbientColor() const;

    /**
     * @brief 设置漫反射颜色（Coin3D 兼容）/ Set diffuse color (Coin3D compatibility)
     */
    void setDiffuseColor(const Color& color);
    Color getDiffuseColor() const;

    /**
     * @brief 设置镜面反射颜色（Coin3D 兼容）/ Set specular color (Coin3D compatibility)
     */
    void setSpecularColor(const Color& color);
    Color getSpecularColor() const;

    /**
     * @brief 设置光泽度（Coin3D 兼容）/ Set shininess (Coin3D compatibility)
     *
     * 自动转换为粗糙度：roughness = 1.0 - (shininess / 128.0)
     * Automatically converted to roughness: roughness = 1.0 - (shininess / 128.0)
     */
    void setShininess(float shininess);
    float getShininess() const;

    //-----------------------------------------------------------------------
    // 渲染状态 / Rendering State
    //-----------------------------------------------------------------------

    /**
     * @brief 设置双面渲染 / Set two-sided rendering
     */
    void setTwoSided(bool twoSided);
    bool isTwoSided() const;

    /**
     * @brief 设置线框模式 / Set wireframe mode
     */
    void setWireframe(bool wireframe);
    bool isWireframe() const;

    /**
     * @brief 设置混合模式 / Set blend mode
     */
    enum class BlendMode {
        Opaque,         ///< 不透明 / Opaque
        Transparent,    ///< 透明 / Transparent
        Additive,       ///< 加法混合 / Additive blending
        Multiply        ///< 乘法混合 / Multiply blending
    };

    void setBlendMode(BlendMode mode);
    BlendMode getBlendMode() const;

    /**
     * @brief 启用/禁用深度测试 / Enable/disable depth test
     */
    void setDepthTest(bool enable);
    bool isDepthTestEnabled() const;

    /**
     * @brief 启用/禁用深度写入 / Enable/disable depth write
     */
    void setDepthWrite(bool enable);
    bool isDepthWriteEnabled() const;

    //-----------------------------------------------------------------------
    // Uniform 管理 / Uniform Management
    //-----------------------------------------------------------------------

    /**
     * @brief 设置自定义 uniform / Set custom uniform
     */
    void setUniform(const std::string& name, float value);
    void setUniform(const std::string& name, const osg::Vec2f& value);
    void setUniform(const std::string& name, const osg::Vec3f& value);
    void setUniform(const std::string& name, const osg::Vec4f& value);

    /**
     * @brief 获取自定义 uniform / Get custom uniform
     */
    bool getUniform(const std::string& name, float& value) const;
    bool getUniform(const std::string& name, osg::Vec2f& value) const;
    bool getUniform(const std::string& name, osg::Vec3f& value) const;
    bool getUniform(const std::string& name, osg::Vec4f& value) const;

    /**
     * @brief 移除自定义 uniform / Remove custom uniform
     */
    void removeUniform(const std::string& name);

    //-----------------------------------------------------------------------
    // 工具方法 / Utility Methods
    //-----------------------------------------------------------------------

    /**
     * @brief 应用到节点 / Apply to node
     *
     * 将材质应用到 OSG 节点。
     * Apply material to OSG node.
     */
    void applyToNode(osg::Node* node);

    /**
     * @brief 克隆材质 / Clone material
     */
    std::unique_ptr<OsgVerseMaterial> clone() const;

    /**
     * @brief 重置为默认值 / Reset to default values
     */
    void reset();

private:
    /**
     * @brief 更新 StateSet / Update StateSet
     */
    void updateStateSet();

    /**
     * @brief 创建纹理单元 / Create texture unit
     */
    int getTextureUnit(TextureType type) const;

    /**
     * @brief Coin3D 到 PBR 转换辅助函数 / Coin3D to PBR conversion helpers
     */
    float shininessToRoughness(float shininess) const;
    float roughnessToShininess(float roughness) const;

    osg::ref_ptr<osg::StateSet> _stateSet;  ///< OSG 状态集 / OSG state set

    // PBR 参数 / PBR parameters
    Color _baseColor{0.8f, 0.8f, 0.8f, 1.0f};
    float _metallic{0.0f};
    float _roughness{0.5f};
    Color _emissive{0.0f, 0.0f, 0.0f, 0.0f};
    float _opacity{1.0f};
    float _ambientOcclusion{1.0f};

    // Coin3D 兼容参数 / Coin3D compatibility parameters
    Color _ambientColor{0.2f, 0.2f, 0.2f, 1.0f};
    Color _diffuseColor{0.8f, 0.8f, 0.8f, 1.0f};
    Color _specularColor{0.0f, 0.0f, 0.0f, 1.0f};
    float _shininess{20.0f};

    // 纹理 / Textures
    std::unordered_map<TextureType, osg::ref_ptr<osg::Texture2D>> _textures;

    // 渲染状态 / Rendering state
    bool _twoSided{false};
    bool _wireframe{false};
    BlendMode _blendMode{BlendMode::Opaque};
    bool _depthTest{true};
    bool _depthWrite{true};
};

/**
 * @brief 材质管理器 / Material manager
 *
 * 管理材质缓存和共享。
 * Manages material caching and sharing.
 */
class GuiExport OsgVerseMaterialManager {
public:
    /**
     * @brief 获取单例实例 / Get singleton instance
     */
    static OsgVerseMaterialManager& instance();

    /**
     * @brief 创建材质 / Create material
     */
    std::shared_ptr<OsgVerseMaterial> createMaterial(const std::string& name = "");

    /**
     * @brief 获取材质 / Get material
     */
    std::shared_ptr<OsgVerseMaterial> getMaterial(const std::string& name) const;

    /**
     * @brief 注册材质 / Register material
     */
    void registerMaterial(const std::string& name, std::shared_ptr<OsgVerseMaterial> material);

    /**
     * @brief 移除材质 / Remove material
     */
    void removeMaterial(const std::string& name);

    /**
     * @brief 清除所有材质 / Clear all materials
     */
    void clear();

    /**
     * @brief 获取材质数量 / Get material count
     */
    size_t getMaterialCount() const;

private:
    OsgVerseMaterialManager() = default;
    ~OsgVerseMaterialManager() = default;

    std::unordered_map<std::string, std::shared_ptr<OsgVerseMaterial>> _materials;
    int _nextId{0};
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEMATERIAL_H
