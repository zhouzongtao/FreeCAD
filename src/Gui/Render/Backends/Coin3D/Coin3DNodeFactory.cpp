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
#include "Coin3DNodeFactory.h"

#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoPickStyle.h>

#include <Base/Console.h>

#include "Coin3DNode.h"
#include "Coin3DMaterial.h"
#include "Coin3DGeometry.h"

FC_LOG_LEVEL_INIT("Coin3DNodeFactory", true, true)

namespace Gui {
namespace Render {

//=============================================================================
// 静态工厂注册 / Static Factory Registration
//=============================================================================

// 自动注册 Coin3D 工厂 / Auto-register Coin3D factory
static RenderNodeFactoryRegistrar<Coin3DNodeFactory> coin3dFactoryRegistrar(BackendType::Coin3D);

//=============================================================================
// Coin3DNodeFactory 实现 / Coin3DNodeFactory Implementation
//=============================================================================

Coin3DNodeFactory::Coin3DNodeFactory()
{
    FC_LOG("Coin3DNodeFactory: Created");
}

Coin3DNodeFactory::~Coin3DNodeFactory()
{
    FC_LOG("Coin3DNodeFactory: Destroyed");
}

//-----------------------------------------------------------------------------
// 容器节点创建 / Container Node Creation
//-----------------------------------------------------------------------------

std::shared_ptr<RenderNode> Coin3DNodeFactory::createGroup()
{
    return std::make_shared<Coin3DGroup>();
}

std::shared_ptr<RenderNode> Coin3DNodeFactory::createSeparator()
{
    return std::make_shared<Coin3DSeparator>();
}

std::shared_ptr<RenderNode> Coin3DNodeFactory::createSwitch()
{
    return std::make_shared<Coin3DSwitch>();
}

std::shared_ptr<RenderNode> Coin3DNodeFactory::createTransform()
{
    return std::make_shared<Coin3DTransform>();
}

//-----------------------------------------------------------------------------
// 几何节点创建 / Geometry Node Creation
//-----------------------------------------------------------------------------

std::shared_ptr<RenderNode> Coin3DNodeFactory::createCoordinate()
{
    // 创建 SoCoordinate3 节点
    SoCoordinate3* coord = new SoCoordinate3();
    return std::make_shared<Coin3DNode>(coord, true, NodeType::Coordinate);
}

std::shared_ptr<RenderNode> Coin3DNodeFactory::createIndexedFaceSet()
{
    return std::make_shared<Coin3DIndexedFaceSet>();
}

std::shared_ptr<RenderNode> Coin3DNodeFactory::createIndexedLineSet()
{
    return std::make_shared<Coin3DLineSet>();
}

std::shared_ptr<RenderNode> Coin3DNodeFactory::createPointSet()
{
    return std::make_shared<Coin3DPointSet>();
}

std::shared_ptr<RenderNode> Coin3DNodeFactory::createNormal()
{
    // 创建 SoNormal 节点
    SoNormal* normal = new SoNormal();
    return std::make_shared<Coin3DNode>(normal, true, NodeType::Node);
}

//-----------------------------------------------------------------------------
// 外观节点创建 / Appearance Node Creation
//-----------------------------------------------------------------------------

std::shared_ptr<RenderNode> Coin3DNodeFactory::createMaterial()
{
    return std::make_shared<Coin3DMaterial>();
}

std::shared_ptr<RenderNode> Coin3DNodeFactory::createDrawStyle()
{
    // 创建 SoDrawStyle 节点
    SoDrawStyle* style = new SoDrawStyle();
    return std::make_shared<Coin3DNode>(style, true, NodeType::Node);
}

std::shared_ptr<RenderNode> Coin3DNodeFactory::createPickStyle()
{
    // 创建 SoPickStyle 节点
    SoPickStyle* style = new SoPickStyle();
    return std::make_shared<Coin3DNode>(style, true, NodeType::Node);
}

//-----------------------------------------------------------------------------
// 选择节点创建 / Selection Node Creation
//-----------------------------------------------------------------------------

std::shared_ptr<RenderNode> Coin3DNodeFactory::createSelectionRoot()
{
    // TODO: 创建 SoFCSelectionRoot 包装器
    // 暂时返回普通 Separator
    // For now, return a regular Separator
    // This should be replaced with SoFCSelectionRoot wrapper when implemented
    FC_WARN("Coin3DNodeFactory::createSelectionRoot: "
            "Using SoSeparator as placeholder for SoFCSelectionRoot");
    return std::make_shared<Coin3DSeparator>();
}

std::shared_ptr<RenderNode> Coin3DNodeFactory::createBoundingBox()
{
    // TODO: 创建 SoFCBoundingBox 包装器
    // 暂时返回普通 Separator
    FC_WARN("Coin3DNodeFactory::createBoundingBox: "
            "Using SoSeparator as placeholder for SoFCBoundingBox");
    return std::make_shared<Coin3DSeparator>();
}

//-----------------------------------------------------------------------------
// Coin3D 特有方法 / Coin3D Specific Methods
//-----------------------------------------------------------------------------

std::shared_ptr<RenderNode> Coin3DNodeFactory::wrapCoinNode(SoNode* node, bool ownsNode)
{
    if (!node) {
        return nullptr;
    }

    // 根据节点类型创建适当的包装器
    // Create appropriate wrapper based on node type
    if (node->isOfType(SoSeparator::getClassTypeId())) {
        return std::make_shared<Coin3DSeparator>(
            static_cast<SoSeparator*>(node), ownsNode);
    }
    else if (node->isOfType(SoSwitch::getClassTypeId())) {
        return std::make_shared<Coin3DSwitch>(
            static_cast<SoSwitch*>(node), ownsNode);
    }
    else if (node->isOfType(SoGroup::getClassTypeId())) {
        return std::make_shared<Coin3DGroup>(
            static_cast<SoGroup*>(node), ownsNode);
    }
    else if (node->isOfType(SoTransform::getClassTypeId())) {
        return std::make_shared<Coin3DTransform>(
            static_cast<SoTransform*>(node), ownsNode);
    }
    else if (node->isOfType(SoMaterial::getClassTypeId())) {
        return std::make_shared<Coin3DMaterial>(
            static_cast<SoMaterial*>(node), ownsNode);
    }
    else if (node->isOfType(SoIndexedFaceSet::getClassTypeId())) {
        return std::make_shared<Coin3DIndexedFaceSet>(
            static_cast<SoIndexedFaceSet*>(node), ownsNode);
    }
    // 默认包装 / Default wrapper
    return std::make_shared<Coin3DNode>(node, ownsNode, NodeType::Node);
}

std::shared_ptr<RenderNode> Coin3DNodeFactory::wrapCoinSeparator(SoSeparator* sep, bool ownsNode)
{
    if (!sep) {
        return nullptr;
    }
    return std::make_shared<Coin3DSeparator>(sep, ownsNode);
}

std::shared_ptr<RenderNode> Coin3DNodeFactory::wrapCoinGroup(SoGroup* group, bool ownsNode)
{
    if (!group) {
        return nullptr;
    }
    return std::make_shared<Coin3DGroup>(group, ownsNode);
}

} // namespace Render
} // namespace Gui
