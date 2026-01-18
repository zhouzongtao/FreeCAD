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

#ifndef GUI_RENDER_UTILS_MATERIALCONVERTER_H
#define GUI_RENDER_UTILS_MATERIALCONVERTER_H

#include <string>
#include <vector>

#include <FCGlobal.h>
#include "../Core/RenderTypes.h"

// 前向声明 / Forward declarations
namespace App {
class Material;
class PropertyColor;
}

namespace Gui {
namespace Render {
namespace Utils {

/**
 * @brief 材质转换工具 / Material conversion utilities
 *
 * 在 FreeCAD App::Material 与渲染层 Material 之间转换。
 * Converts between FreeCAD App::Material and render layer Material.
 */
class GuiExport MaterialConverter {
public:
    //-----------------------------------------------------------------------
    // 基础转换 / Basic Conversion
    //-----------------------------------------------------------------------

    /**
     * @brief 从 App::Material 转换 / Convert from App::Material
     *
     * @param appMaterial FreeCAD App 层材质 / FreeCAD App layer material
     * @param renderMaterial 输出渲染材质 / Output render material
     */
    static void fromAppMaterial(
        const App::Material& appMaterial,
        Material& renderMaterial
    );

    /**
     * @brief 转换为 App::Material / Convert to App::Material
     *
     * @param renderMaterial 渲染材质 / Render material
     * @param appMaterial 输出 App 层材质 / Output App layer material
     */
    static void toAppMaterial(
        const Material& renderMaterial,
        App::Material& appMaterial
    );

    //-----------------------------------------------------------------------
    // 属性转换 / Property Conversion
    //-----------------------------------------------------------------------

    /**
     * @brief 从属性颜色转换 / Convert from property color
     */
    static Color fromPropertyColor(const App::PropertyColor& prop);

    /**
     * @brief 转换为属性颜色 / Convert to property color
     */
    static App::PropertyColor* toPropertyColor(const Color& color);

    //-----------------------------------------------------------------------
    // 预设材质 / Preset Materials
    //-----------------------------------------------------------------------

    /**
     * @brief 材质预设类型 / Material preset types
     */
    enum class Preset {
        Default,
        Metal,
        Plastic,
        Glass,
        Wood,
        Stone,
        Fabric,
        Ceramic,
        Liquid,
        Emissive,
        Transparent,
        Mirror,
        Gold,
        Silver,
        Copper,
        Bronze,
        Steel,
        Aluminum,
        Concrete,
        Brick,
        Leather,
        Paper,
        Rubber,
        Wax,
        Custom
    };

    /**
     * @brief 获取预设材质 / Get preset material
     *
     * @param preset 预设类型 / Preset type
     * @return 材质结构 / Material structure
     */
    static Material getPreset(Preset preset);

    /**
     * @brief 获取预设材质名称 / Get preset material name
     */
    static std::string getPresetName(Preset preset);

    /**
     * @brief 从名称获取预设 / Get preset from name
     */
    static Preset getPresetByName(const std::string& name);

    //-----------------------------------------------------------------------
    // PBR 材质转换 / PBR Material Conversion
    //-----------------------------------------------------------------------

    /**
     * @brief 从传统材质转换为 PBR 材质 / Convert from traditional to PBR
     *
     * 使用近似算法从环境光/漫反射/镜面反射计算 PBR 参数。
     * Uses approximation to calculate PBR parameters from ambient/diffuse/specular.
     *
     * @param traditional 传统材质 / Traditional material
     * @param pmr 输出 PBR 材质 / Output PBR material
     */
    static void traditionalToPBR(
        const Material& traditional,
        Material& pbr
    );

    /**
     * @brief 从 PBR 材质转换为传统材质 / Convert from PBR to traditional
     */
    static void pbrToTraditional(
        const Material& pbr,
        Material& traditional
    );

    //-----------------------------------------------------------------------
    // 纹理处理 / Texture Processing
    //-----------------------------------------------------------------------

    /**
     * @brief 纹理参数 / Texture parameters
     */
    struct TextureParam {
        std::string filename;       ///< 文件路径 / File path
        float scale{1.0f};          ///< 纹理缩放 / Texture scale
        float rotation{0.0f};       ///< 纹理旋转（度）/ Texture rotation (degrees)
        Vec2f offset{0.0f, 0.0f};   ///< 纹理偏移 / Texture offset
        bool repeatU{true};         ///< U 方向重复 / Repeat in U direction
        bool repeatV{true};         ///< V 方向重复 / Repeat in V direction
    };

    /**
     * @brief 设置纹理参数 / Set texture parameters
     */
    static void setTextureParam(
        Material& material,
        Material::TextureType type,
        const TextureParam& param
    );

    /**
     * @brief 获取纹理参数 / Get texture parameters
     */
    static TextureParam getTextureParam(
        const Material& material,
        Material::TextureType type
    );

    //-----------------------------------------------------------------------
    // 材质验证 / Material Validation
    //-----------------------------------------------------------------------

    /**
     * @brief 验证材质参数 / Validate material parameters
     *
     * 确保所有值在有效范围内。
     * Ensures all values are within valid ranges.
     *
     * @param material 要验证的材质 / Material to validate
     * @return 是否有效 / Whether valid
     */
    static bool validate(const Material& material);

    /**
     * @brief 修复材质参数 / Fix material parameters
     *
     * 将超出范围的值调整到有效范围内。
     * Clamps out-of-range values to valid ranges.
     */
    static void fix(Material& material);

    //-----------------------------------------------------------------------
    // 材质混合 / Material Blending
    //-----------------------------------------------------------------------

    /**
     * @brief 混合两种材质 / Blend two materials
     *
     * @param mat1 第一种材质 / First material
     * @param mat2 第二种材质 / Second material
     * @param factor 混合因子 [0-1] / Blend factor [0-1]
     * @return 混合后的材质 / Blended material
     */
    static Material blend(
        const Material& mat1,
        const Material& mat2,
        float factor
    );

    /**
     * @brief 材质叠加 / Layer materials
     *
     * 将 topMaterial 叠加到 baseMaterial 上。
     * Layers topMaterial over baseMaterial.
     */
    static Material layer(
        const Material& baseMaterial,
        const Material& topMaterial
    );

    //-----------------------------------------------------------------------
    // 材质序列化 / Material Serialization
    //-----------------------------------------------------------------------

    /**
     * @brief 序列化材质为 JSON / Serialize material to JSON
     */
    static std::string serializeToJson(const Material& material);

    /**
     * @brief 从 JSON 反序列化材质 / Deserialize material from JSON
     */
    static Material deserializeFromJson(const std::string& json);

    /**
     * @brief 序列化材质字典 / Serialize material dictionary
     */
    static std::string serializeDict(
        const std::unordered_map<std::string, Material>& materials
    );

    /**
     * @brief 反序列化材质字典 / Deserialize material dictionary
     */
    static std::unordered_map<std::string, Material> deserializeDict(
        const std::string& json
    );
};

//-------------------------------------------------------------------------
// 辅助函数 / Helper Functions
//-------------------------------------------------------------------------

/**
 * @brief 创建随机材质 / Create random material
 *
 * @param randomizeColor 是否随机化颜色 / Whether to randomize color
 * @return 随机材质 / Random material
 */
Material createRandomMaterial(bool randomizeColor = true);

/**
 * @brief 材质颜色转换工具 / Material color conversion utilities
 */
namespace ColorUtils {
    /**
     * @brief RGB 转 HSV / RGB to HSV
     */
    void rgbToHsv(float r, float g, float b, float& h, float& s, float& v);

    /**
     * @brief HSV 转 RGB / HSV to RGB
     */
    void hsvToRgb(float h, float s, float v, float& r, float& g, float& b);

    /**
     * @brief 调整亮度 / Adjust brightness
     */
    Color adjustBrightness(const Color& color, float factor);

    /**
     * @brief 调整饱和度 / Adjust saturation
     */
    Color adjustSaturation(const Color& color, float factor);

    /**
     * @brief 混合颜色 / Blend colors
     */
    Color blendColors(const Color& c1, const Color& c2, float factor);
}

} // namespace Utils
} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_UTILS_MATERIALCONVERTER_H
