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

#ifndef GUI_RENDER_BACKENDS_COIN3D_COIN3DNODEFACTORY_H
#define GUI_RENDER_BACKENDS_COIN3D_COIN3DNODEFACTORY_H

#include <FCGlobal.h>
#include "../../Core/RenderNodeFactory.h"

namespace Gui {
namespace Render {

/**
 * @brief Coin3D 渲染节点工厂 / Coin3D render node factory
 *
 * 创建 Coin3D 后端的渲染节点实现。
 * Creates Coin3D backend render node implementations.
 *
 * 所有创建的节点都是 Coin3DXxx 类型，这些类型包装了对应的 SoXxx 节点。
 * All created nodes are Coin3DXxx types, which wrap corresponding SoXxx nodes.
 */
class GuiExport Coin3DNodeFactory : public RenderNodeFactory {
public:
    /**
     * @brief 构造函数 / Constructor
     */
    Coin3DNodeFactory();

    /**
     * @brief 析构函数 / Destructor
     */
    ~Coin3DNodeFactory() override;

    //=========================================================================
    // RenderNodeFactory 接口实现 / RenderNodeFactory Interface Implementation
    //=========================================================================

    BackendType getBackendType() const override { return BackendType::Coin3D; }
    std::string getBackendName() const override { return "Coin3D"; }

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
    // Coin3D 特有方法 / Coin3D Specific Methods
    //=========================================================================

    /**
     * @brief 从现有 SoNode 创建包装器 / Create wrapper from existing SoNode
     *
     * 将现有的 Coin3D 节点包装到抽象层中。
     * Wraps an existing Coin3D node into the abstraction layer.
     *
     * @param node 现有的 SoNode / Existing SoNode
     * @param ownsNode 是否拥有节点所有权 / Whether to take ownership
     * @return 包装后的 RenderNode / Wrapped RenderNode
     */
    std::shared_ptr<RenderNode> wrapCoinNode(class SoNode* node, bool ownsNode = false);

    /**
     * @brief 从现有 SoSeparator 创建包装器 / Create wrapper from existing SoSeparator
     */
    std::shared_ptr<RenderNode> wrapCoinSeparator(class SoSeparator* sep, bool ownsNode = false);

    /**
     * @brief 从现有 SoGroup 创建包装器 / Create wrapper from existing SoGroup
     */
    std::shared_ptr<RenderNode> wrapCoinGroup(class SoGroup* group, bool ownsNode = false);
};

/**
 * @brief 获取 Coin3D 节点工厂实例 / Get Coin3D node factory instance
 *
 * 便捷函数，等价于 RenderNodeFactoryRegistry::instance().getFactory(BackendType::Coin3D)
 * Convenience function, equivalent to RenderNodeFactoryRegistry::instance().getFactory(BackendType::Coin3D)
 */
inline RenderNodeFactory::Ptr getCoin3DFactory()
{
    return RenderNodeFactoryRegistry::instance().getFactory(BackendType::Coin3D);
}

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_COIN3D_COIN3DNODEFACTORY_H
