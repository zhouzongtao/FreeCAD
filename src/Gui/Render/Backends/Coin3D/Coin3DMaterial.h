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

#ifndef GUI_RENDER_BACKENDS_COIN3D_COIN3DMATERIAL_H
#define GUI_RENDER_BACKENDS_COIN3DMATERIAL_H

#include <FCGlobal.h>
#include "../../Core/RenderTypes.h"
#include "Coin3DNode.h"

// 前向声明 / Forward declarations
class SoMaterial;
class SoMaterialBinding;
class SbColor;

namespace Gui {
namespace Render {

/**
 * @brief Coin3D SoMaterial 包装器 / Coin3D SoMaterial wrapper
 *
 * 管理材质属性，包括颜色、透明度、发光等。
 * Manages material properties including color, transparency, emission, etc.
 */
class GuiExport Coin3DMaterial : public Coin3DNode {
public:
    RENDER_NODE_STATIC_TYPE(NodeType::Material)

    /**
     * @brief 构造函数 - 创建新 SoMaterial / Constructor - Create new SoMaterial
     */
    Coin3DMaterial();

    /**
     * @brief 从现有 SoMaterial 创建 / Create from existing SoMaterial
     */
    explicit Coin3DMaterial(SoMaterial* material, bool ownsNode = false);

    /**
     * @brief 析构函数 / Destructor
     */
    ~Coin3DMaterial() override;

    /**
     * @brief 获取 SoMaterial 指针 / Get SoMaterial pointer
     */
    SoMaterial* getSoMaterial() const;

    //-----------------------------------------------------------------------
    // 环境光属性 / Ambient Properties
    //-----------------------------------------------------------------------

    /**
     * @brief 设置环境光强度 / Set ambient intensity
     * @param intensity 强度值 [0-1] / Intensity value [0-1]
     */
    void setAmbientIntensity(float intensity);

    /**
     * @brief 获取环境光强度 / Get ambient intensity
     */
    float getAmbientIntensity() const;

    //-----------------------------------------------------------------------
    // 漫反射颜色 / Diffuse Color
    //-----------------------------------------------------------------------

    /**
     * @brief 设置漫反射颜色 / Set diffuse color
     */
    void setDiffuseColor(const Color& color);

    /**
     * @brief 获取漫反射颜色 / Get diffuse color
     */
    Color getDiffuseColor() const;

    /**
     * @brief 设置漫反射颜色数组 / Set diffuse color array
     *
     * 用于多色几何体（如顶点着色）。
     * For multi-color geometry (e.g., vertex coloring).
     */
    void setDiffuseColors(const std::vector<Color>& colors);

    /**
     * @brief 获取漫反射颜色数组 / Get diffuse color array
     */
    std::vector<Color> getDiffuseColors() const;

    //-----------------------------------------------------------------------
    // 镜面反射属性 / Specular Properties
    //-----------------------------------------------------------------------

    /**
     * @brief 设置镜面反射颜色 / Set specular color
     */
    void setSpecularColor(const Color& color);

    /**
     * @brief 获取镜面反射颜色 / Get specular color
     */
    Color getSpecularColor() const;

    /**
     * @brief 设置发光颜色 / Set emissive color
     */
    void setEmissiveColor(const Color& color);

    /**
     * @brief 获取发光颜色 / Get emissive color
     */
    Color getEmissiveColor() const;

    //-----------------------------------------------------------------------
    // 光泽度 / Shininess
    //-----------------------------------------------------------------------

    /**
     * @brief 设置光泽度 / Set shininess
     * @param shininess 光泽度 [0-1] / Shininess [0-1]
     */
    void setShininess(float shininess);

    /**
     * @brief 获取光泽度 / Get shininess
     */
    float getShininess() const;

    //-----------------------------------------------------------------------
    // 透明度 / Transparency
    //-----------------------------------------------------------------------

    /**
     * @brief 设置透明度 / Set transparency
     * @param transparency 透明度 [0-1] / Transparency [0-1]
     */
    void setTransparency(float transparency);

    /**
     * @brief 获取透明度 / Get transparency
     */
    float getTransparency() const;

    //-----------------------------------------------------------------------
    // 批量操作 / Batch Operations
    //-----------------------------------------------------------------------

    /**
     * @brief 从 FreeCAD Material 设置材质 / Set material from FreeCAD Material
     *
     * @param material FreeCAD 材质对象 / FreeCAD material object
     */
    void setMaterial(const Material& material);

    /**
     * @brief 获取 FreeCAD Material 结构 / Get FreeCAD Material structure
     */
    Material getMaterial() const;

    /**
     * @brief 设置材质绑定方式 / Set material binding
     *
     * 控制材质如何应用到几何体（整体、面、顶点等）。
     * Controls how material is applied to geometry (overall, per-face, per-vertex, etc.).
     */
    enum class Binding {
        Overall,        ///< 整体应用 / Applied to whole
        PerPart,        ///< 每部分应用 / Applied per part
        PerPartIndexed, ///< 每部分索引应用 / Applied per part indexed
        PerFace,        ///< 每面应用 / Applied per face
        PerFaceIndexed, ///< 每面索引应用 / Applied per face indexed
        PerVertex,      ///< 每顶点应用 / Applied per vertex
        PerVertexIndexed, ///< 每顶点索引应用 / Applied per vertex indexed
        Default         ///< 默认 / Default
    };

    /**
     * @brief 设置材质绑定 / Set material binding
     */
    void setMaterialBinding(Binding binding);

    /**
     * @brief 获取材质绑定 / Get material binding
     */
    Binding getMaterialBinding() const;

private:
    /// 材质绑定节点 / Material binding node
    class SoMaterialBinding* _materialBinding{nullptr};
};

//-------------------------------------------------------------------------
// 材质转换工具 / Material Conversion Utilities
//-------------------------------------------------------------------------

namespace Coin3DMaterialUtils {
    /**
     * @brief Color 转 SbColor / Color to SbColor
     */
    SbColor colorToSbColor(const Color& color);

    /**
     * @brief SbColor 转 Color / SbColor to Color
     */
    Color sbColorToColor(const SbColor& sbColor);

    /**
     * @brief 从 FreeCAD App::Material 转换 / Convert from FreeCAD App::Material
     *
     * @param appMaterial FreeCAD App 层材质 / FreeCAD App layer material
     * @param output 输出材质结构 / Output material structure
     */
    void fromAppMaterial(const class App::Material& appMaterial, Material& output);

    /**
     * @brief 转换为 FreeCAD App::Material / Convert to FreeCAD App::Material
     *
     * @param input 输入材质结构 / Input material structure
     * @param appMaterial FreeCAD App 层材质 / FreeCAD App layer material
     */
    void toAppMaterial(const Material& input, class App::Material& appMaterial);

    /**
     * @brief 创建标准材质 / Create standard material
     *
     * 预定义的常用材质。
     * Predefined common materials.
     */
    enum class StandardMaterial {
        Default,
        Gold,
        Silver,
        Bronze,
        Copper,
        Plastic,
        Rubber,
        Glass,
        Metal,
        Wood,
        Stone
    };

    /**
     * @brief 获取标准材质 / Get standard material
     */
    Material getStandardMaterial(StandardMaterial type);
}

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_COIN3D_COIN3DMATERIAL_H
