/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Development Team                           *
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

#ifndef GUI_RENDER_GEOMETRYNODEBUILDER_H
#define GUI_RENDER_GEOMETRYNODEBUILDER_H

#include <memory>

#include <FCGlobal.h>
#include "RenderNode.h"
#include "RenderNodeFactory.h"
#include "GeometryData.h"

namespace Gui {
namespace Render {

/**
 * @brief 几何节点构建选项 / Geometry node build options
 */
struct GuiExport BuildOptions {
    /// 默认材质颜色 / Default material color
    Color diffuseColor{0.8f, 0.8f, 0.8f, 1.0f};

    /// 线框颜色 / Wireframe color
    Color lineColor{0.0f, 0.0f, 0.0f, 1.0f};

    /// 点颜色 / Point color
    Color pointColor{0.0f, 0.0f, 0.0f, 1.0f};

    /// 线宽 / Line width
    float lineWidth{1.0f};

    /// 点大小 / Point size
    float pointSize{2.0f};

    /// 透明度 / Transparency
    float transparency{0.0f};

    /// 是否使用顶点颜色（如果可用）/ Use vertex colors if available
    bool useVertexColors{true};

    /// 是否使用顶点法线（如果可用）/ Use vertex normals if available
    bool useVertexNormals{true};

    /// 是否启用背面剔除 / Enable back-face culling
    bool enableCulling{true};

    /// 是否启用光照 / Enable lighting
    bool enableLighting{true};
};

/**
 * @brief 几何节点构建器 / Geometry node builder
 *
 * 从 GeometryData 构建各种显示模式的渲染节点。
 * Builds render nodes for various display modes from GeometryData.
 *
 * 使用方式 / Usage:
 * ```cpp
 * GeometryData data;
 * // ... 填充数据 ...
 *
 * auto factory = RenderNodeFactoryRegistry::instance().getDefaultFactory();
 * GeometryNodeBuilder builder(factory.get());
 *
 * // 创建着色模式节点
 * auto shadedNode = builder.buildShadedNode(data);
 *
 * // 创建线框模式节点
 * auto wireframeNode = builder.buildWireframeNode(data);
 * ```
 */
class GuiExport GeometryNodeBuilder {
public:
    /**
     * @brief 构造函数 / Constructor
     *
     * @param factory 渲染节点工厂 / Render node factory
     */
    explicit GeometryNodeBuilder(RenderNodeFactory* factory);

    /**
     * @brief 析构函数 / Destructor
     */
    ~GeometryNodeBuilder() = default;

    /**
     * @brief 设置构建选项 / Set build options
     */
    void setOptions(const BuildOptions& options) { _options = options; }

    /**
     * @brief 获取当前选项 / Get current options
     */
    const BuildOptions& getOptions() const { return _options; }

    //=========================================================================
    // 单一模式节点构建 / Single Mode Node Building
    //=========================================================================

    /**
     * @brief 构建着色模式节点 / Build shaded mode node
     *
     * 创建带有材质的三角形面渲染节点。
     * Creates triangle face render node with material.
     *
     * @param data 几何数据 / Geometry data
     * @return 渲染节点，失败返回 nullptr / Render node, nullptr on failure
     */
    RenderNode::Ptr buildShadedNode(const GeometryData& data);

    /**
     * @brief 构建线框模式节点 / Build wireframe mode node
     *
     * 创建边线渲染节点。
     * Creates edge line render node.
     *
     * @param data 几何数据 / Geometry data
     * @return 渲染节点，失败返回 nullptr / Render node, nullptr on failure
     */
    RenderNode::Ptr buildWireframeNode(const GeometryData& data);

    /**
     * @brief 构建点模式节点 / Build points mode node
     *
     * 创建点集渲染节点。
     * Creates point set render node.
     *
     * @param data 几何数据 / Geometry data
     * @return 渲染节点，失败返回 nullptr / Render node, nullptr on failure
     */
    RenderNode::Ptr buildPointsNode(const GeometryData& data);

    //=========================================================================
    // 组合模式节点构建 / Combined Mode Node Building
    //=========================================================================

    /**
     * @brief 构建 Flat Lines 模式节点 / Build flat lines mode node
     *
     * 创建着色面 + 边线的组合节点（FreeCAD 默认显示模式）。
     * Creates shaded faces + edge lines combined node (FreeCAD default display mode).
     *
     * @param data 几何数据 / Geometry data
     * @return 渲染节点，失败返回 nullptr / Render node, nullptr on failure
     */
    RenderNode::Ptr buildFlatLinesNode(const GeometryData& data);

    /**
     * @brief 构建所有标准显示模式 / Build all standard display modes
     *
     * 创建包含所有标准显示模式的 Switch 节点。
     * Creates a Switch node containing all standard display modes.
     *
     * Switch 子节点顺序：
     * Switch child order:
     * 0 - Flat Lines (面+线)
     * 1 - Shaded (纯着色)
     * 2 - Wireframe (纯线框)
     * 3 - Points (纯点)
     *
     * @param data 几何数据 / Geometry data
     * @return Switch 节点，失败返回 nullptr / Switch node, nullptr on failure
     */
    RenderNode::Ptr buildAllModes(const GeometryData& data);

    //=========================================================================
    // 辅助方法 / Helper Methods
    //=========================================================================

    /**
     * @brief 创建材质节点 / Create material node
     *
     * @param color 漫反射颜色 / Diffuse color
     * @param transparency 透明度 / Transparency
     * @return 材质节点 / Material node
     */
    RenderNode::Ptr createMaterialNode(const Color& color, float transparency = 0.0f);

    /**
     * @brief 创建绘制样式节点 / Create draw style node
     *
     * @param lineWidth 线宽 / Line width
     * @param pointSize 点大小 / Point size
     * @return 绘制样式节点 / Draw style node
     */
    RenderNode::Ptr createDrawStyleNode(float lineWidth, float pointSize);

    /**
     * @brief 创建坐标节点 / Create coordinate node
     *
     * @param data 几何数据 / Geometry data
     * @return 坐标节点 / Coordinate node
     */
    RenderNode::Ptr createCoordinateNode(const GeometryData& data);

    /**
     * @brief 创建法线节点 / Create normal node
     *
     * @param data 几何数据 / Geometry data
     * @return 法线节点 / Normal node
     */
    RenderNode::Ptr createNormalNode(const GeometryData& data);

private:
    RenderNodeFactory* _factory;
    BuildOptions _options;
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_GEOMETRYNODEBUILDER_H
