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

#ifndef GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSENODEFACTORY_H
#define GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSENODEFACTORY_H

#include <FCGlobal.h>
#include "../../Core/RenderNodeFactory.h"

// 前向声明 OSG 类型 / Forward declarations for OSG types
namespace osg {
    class Node;
    class Group;
}

namespace Gui {
namespace Render {

/**
 * @brief OsgVerse 渲染节点工厂 / OsgVerse render node factory
 *
 * 创建 OsgVerse 后端的渲染节点实现。
 * Creates OsgVerse backend render node implementations.
 *
 * 所有创建的节点都是 OsgVerseXxx 类型，这些类型包装了对应的 osg::Xxx 节点。
 * All created nodes are OsgVerseXxx types, which wrap corresponding osg::Xxx nodes.
 */
class GuiExport OsgVerseNodeFactory : public RenderNodeFactory {
public:
    /**
     * @brief 构造函数 / Constructor
     */
    OsgVerseNodeFactory();

    /**
     * @brief 析构函数 / Destructor
     */
    ~OsgVerseNodeFactory() override;

    //=========================================================================
    // RenderNodeFactory 接口实现 / RenderNodeFactory Interface Implementation
    //=========================================================================

    BackendType getBackendType() const override { return BackendType::OsgVerse; }
    std::string getBackendName() const override { return "OsgVerse"; }

    // 容器节点 / Container nodes
    std::shared_ptr<RenderNode> createGroup() override;
    std::shared_ptr<RenderNode> createSeparator() override;
    std::shared_ptr<RenderNode> createSwitch() override;
    std::shared_ptr<RenderNode> createTransform() override;

    // 几何节点 / Geometry nodes
    std::shared_ptr<RenderNode> createCoordinate() override;
    std::shared_ptr<RenderNode> createIndexedFaceSet() override;
    std::shared_ptr<RenderNode> createIndexedLineSet() override;
    std::shared_ptr<RenderNode> createPointSet() override;
    std::shared_ptr<RenderNode> createNormal() override;

    // 外观节点 / Appearance nodes
    std::shared_ptr<RenderNode> createMaterial() override;
    std::shared_ptr<RenderNode> createDrawStyle() override;
    std::shared_ptr<RenderNode> createPickStyle() override;

    // 选择节点 / Selection nodes
    std::shared_ptr<RenderNode> createSelectionRoot() override;
    std::shared_ptr<RenderNode> createBoundingBox() override;

    //=========================================================================
    // OsgVerse 特有方法 / OsgVerse Specific Methods
    //=========================================================================

    /**
     * @brief 从现有 osg::Node 创建包装器 / Create wrapper from existing osg::Node
     *
     * 将现有的 OSG 节点包装到抽象层中。
     * Wraps an existing OSG node into the abstraction layer.
     *
     * @param node 现有的 osg::Node / Existing osg::Node
     * @param ownsNode 是否拥有节点所有权 / Whether to take ownership
     * @return 包装后的 RenderNode / Wrapped RenderNode
     */
    std::shared_ptr<RenderNode> wrapOsgNode(osg::Node* node, bool ownsNode = false);

    /**
     * @brief 从现有 osg::Group 创建包装器 / Create wrapper from existing osg::Group
     */
    std::shared_ptr<RenderNode> wrapOsgGroup(osg::Group* group, bool ownsNode = false);

    /**
     * @brief 创建 PBR 材质节点 / Create PBR material node
     *
     * 创建支持 PBR 渲染的高级材质节点。
     * Creates advanced material node with PBR rendering support.
     */
    std::shared_ptr<RenderNode> createPBRMaterial();

    /**
     * @brief 创建高效几何节点 / Create optimized geometry node
     *
     * 创建使用 VBO 的高效几何节点。
     * Creates geometry node using VBO for efficiency.
     */
    std::shared_ptr<RenderNode> createOptimizedGeometry();
};

/**
 * @brief 获取 OsgVerse 节点工厂实例 / Get OsgVerse node factory instance
 *
 * 便捷函数，等价于 RenderNodeFactoryRegistry::instance().getFactory(BackendType::OsgVerse)
 * Convenience function, equivalent to RenderNodeFactoryRegistry::instance().getFactory(BackendType::OsgVerse)
 */
inline RenderNodeFactory::Ptr getOsgVerseFactory()
{
    return RenderNodeFactoryRegistry::instance().getFactory(BackendType::OsgVerse);
}

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSENODEFACTORY_H
