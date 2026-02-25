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

#include "PreCompiled.h"
#include "OsgVerseNodeFactory.h"

#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/Switch>
#include <osg/Geode>
#include <osg/Geometry>

#include <Base/Console.h>

#include "OsgVerseNode.h"
#include "OsgVerseMaterial.h"
#include "OsgVerseGeometry.h"
#include "OsgVerseSelection.h"

FC_LOG_LEVEL_INIT("OsgVerseNodeFactory", true, true)

namespace Gui {
namespace Render {

//=============================================================================
// Factory Registration
//=============================================================================

// Note: OsgVerse factory is registered on-demand when OsgVerseGui module
// is loaded, not via static initialization. This avoids DLL load order
// issues on Windows.

//=============================================================================
// OsgVerseNodeFactory 实现 / OsgVerseNodeFactory Implementation
//=============================================================================

OsgVerseNodeFactory::OsgVerseNodeFactory()
{
    FC_LOG("OsgVerseNodeFactory: Created");
}

OsgVerseNodeFactory::~OsgVerseNodeFactory()
{
    FC_LOG("OsgVerseNodeFactory: Destroyed");
}

//-----------------------------------------------------------------------------
// 容器节点创建 / Container Node Creation
//-----------------------------------------------------------------------------

std::shared_ptr<RenderNode> OsgVerseNodeFactory::createGroup()
{
    return std::make_shared<OsgVerseGroup>();
}

std::shared_ptr<RenderNode> OsgVerseNodeFactory::createSeparator()
{
    return std::make_shared<OsgVerseSeparator>();
}

std::shared_ptr<RenderNode> OsgVerseNodeFactory::createSwitch()
{
    return std::make_shared<OsgVerseSwitch>();
}

std::shared_ptr<RenderNode> OsgVerseNodeFactory::createTransform()
{
    return std::make_shared<OsgVerseTransform>();
}

//-----------------------------------------------------------------------------
// 几何节点创建 / Geometry Node Creation
//-----------------------------------------------------------------------------

std::shared_ptr<RenderNode> OsgVerseNodeFactory::createCoordinate()
{
    // OSG 不使用独立的坐标节点，坐标数据直接存储在 Geometry 中
    // OSG doesn't use separate coordinate nodes, coordinates are stored directly in Geometry
    // 返回一个占位符节点 / Return a placeholder node
    auto group = std::make_shared<OsgVerseGroup>();
    group->setName("CoordinatePlaceholder");
    return group;
}

std::shared_ptr<RenderNode> OsgVerseNodeFactory::createIndexedFaceSet()
{
    // 创建 OsgVerseGeometry 并设置为三角形模式
    auto geometry = std::make_shared<OsgVerseGeometry>();
    geometry->setPrimitiveType(OsgVerseGeometry::PrimitiveType::Triangles);
    return geometry;
}

std::shared_ptr<RenderNode> OsgVerseNodeFactory::createIndexedLineSet()
{
    // 创建 OsgVerseGeometry 并设置为线段模式
    auto geometry = std::make_shared<OsgVerseGeometry>();
    geometry->setPrimitiveType(OsgVerseGeometry::PrimitiveType::Lines);
    return geometry;
}

std::shared_ptr<RenderNode> OsgVerseNodeFactory::createPointSet()
{
    // 创建 OsgVerseGeometry 并设置为点模式
    auto geometry = std::make_shared<OsgVerseGeometry>();
    geometry->setPrimitiveType(OsgVerseGeometry::PrimitiveType::Points);
    return geometry;
}

std::shared_ptr<RenderNode> OsgVerseNodeFactory::createNormal()
{
    // OSG 不使用独立的法线节点，法线数据直接存储在 Geometry 中
    // OSG doesn't use separate normal nodes, normals are stored directly in Geometry
    // 返回一个占位符节点 / Return a placeholder node
    auto group = std::make_shared<OsgVerseGroup>();
    group->setName("NormalPlaceholder");
    return group;
}

//-----------------------------------------------------------------------------
// 外观节点创建 / Appearance Node Creation
//-----------------------------------------------------------------------------

std::shared_ptr<RenderNode> OsgVerseNodeFactory::createMaterial()
{
    return std::make_shared<OsgVerseMaterial>();
}

std::shared_ptr<RenderNode> OsgVerseNodeFactory::createDrawStyle()
{
    // OSG 使用 StateSet 管理绘制样式
    // OSG uses StateSet for draw style management
    // 返回一个占位符节点，实际样式通过材质节点的 StateSet 设置
    auto group = std::make_shared<OsgVerseGroup>();
    group->setName("DrawStylePlaceholder");
    return group;
}

std::shared_ptr<RenderNode> OsgVerseNodeFactory::createPickStyle()
{
    // OSG 使用节点掩码管理拾取样式
    // OSG uses node masks for pick style management
    // 返回一个占位符节点，实际样式通过节点掩码设置
    auto group = std::make_shared<OsgVerseGroup>();
    group->setName("PickStylePlaceholder");
    return group;
}

//-----------------------------------------------------------------------------
// 选择节点创建 / Selection Node Creation
//-----------------------------------------------------------------------------

std::shared_ptr<RenderNode> OsgVerseNodeFactory::createSelectionRoot()
{
    // 创建选择根节点，支持选择高亮和预选高亮
    // Create selection root node with selection and preselection highlighting
    auto selectionRoot = std::make_shared<OsgVerseSelectionRoot>();

    // 设置默认高亮颜色 (与 FreeCAD 默认一致)
    // Set default highlight colors (consistent with FreeCAD defaults)
    selectionRoot->setSelectionColor(0.1f, 0.8f, 0.1f, 1.0f);     // 绿色 / Green
    selectionRoot->setPreselectionColor(0.88f, 0.88f, 0.1f, 1.0f); // 黄色 / Yellow
    selectionRoot->setHighlightMode(OsgVerseSelectionRoot::HighlightMode::Color);
    selectionRoot->setOutlineWidth(2.0f);

    FC_LOG("OsgVerseNodeFactory::createSelectionRoot: Created OsgVerseSelectionRoot");

    return selectionRoot;
}

std::shared_ptr<RenderNode> OsgVerseNodeFactory::createBoundingBox()
{
    // 创建边界框节点
    // Create bounding box node
    auto boundingBox = std::make_shared<OsgVerseBoundingBox>();

    // 设置默认外观 (白色线框)
    // Set default appearance (white wireframe)
    boundingBox->setColor(1.0f, 1.0f, 1.0f, 1.0f);
    boundingBox->setLineWidth(1.0f);

    FC_LOG("OsgVerseNodeFactory::createBoundingBox: Created OsgVerseBoundingBox");

    return boundingBox;
}

//-----------------------------------------------------------------------------
// OsgVerse 特有方法 / OsgVerse Specific Methods
//-----------------------------------------------------------------------------

std::shared_ptr<RenderNode> OsgVerseNodeFactory::wrapOsgNode(osg::Node* node, bool ownsNode)
{
    if (!node) {
        return nullptr;
    }

    // 根据节点类型创建适当的包装器
    // Create appropriate wrapper based on node type
    if (osg::Switch* sw = dynamic_cast<osg::Switch*>(node)) {
        return std::make_shared<OsgVerseSwitch>(sw, ownsNode);
    }
    else if (osg::MatrixTransform* mt = dynamic_cast<osg::MatrixTransform*>(node)) {
        return std::make_shared<OsgVerseTransform>(mt, ownsNode);
    }
    else if (osg::Group* group = dynamic_cast<osg::Group*>(node)) {
        return std::make_shared<OsgVerseGroup>(group, ownsNode);
    }
    else if (osg::Geometry* geom = dynamic_cast<osg::Geometry*>(node)) {
        return std::make_shared<OsgVerseGeometry>(geom, ownsNode);
    }

    // 默认包装 / Default wrapper
    return std::make_shared<OsgVerseNode>(node, ownsNode, NodeType::Node);
}

std::shared_ptr<RenderNode> OsgVerseNodeFactory::wrapOsgGroup(osg::Group* group, bool ownsNode)
{
    if (!group) {
        return nullptr;
    }
    return std::make_shared<OsgVerseGroup>(group, ownsNode);
}

std::shared_ptr<RenderNode> OsgVerseNodeFactory::createPBRMaterial()
{
    auto material = std::make_shared<OsgVerseMaterial>();
    // 设置默认 PBR 参数 / Set default PBR parameters
    material->setMetallic(0.0f);
    material->setRoughness(0.5f);
    return material;
}

std::shared_ptr<RenderNode> OsgVerseNodeFactory::createOptimizedGeometry()
{
    auto geometry = std::make_shared<OsgVerseGeometry>();
    // 默认使用三角形，后续可以优化为 VBO
    // Default to triangles, can be optimized to VBO later
    geometry->setPrimitiveType(OsgVerseGeometry::PrimitiveType::Triangles);
    return geometry;
}

} // namespace Render
} // namespace Gui
