/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Development Team                             *
 *                                                                         *
 *   This file is part of the FreeCAD CaD development system.              *
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

#ifndef GUI_RENDER_BACKENDS_COIN3D_COIN3DNODE_H
#define GUI_RENDER_BACKENDS_COIN3D_COIN3DNODE_H

#include <memory>
#include <vector>

#include <Inventor/nodes/SoNode.h>
#include <FCGlobal.h>
#include "../../Core/RenderNode.h"

// 前向声明 Coin3D 节点类型 / Forward declarations for Coin3D node types
class SoGroup;
class SoSeparator;
class SoTransform;
class SoSwitch;
class SoMaterial;
class SoCamera;
class SoLight;
class SoDirectionalLight;
class SoPointLight;
class SoSpotLight;

namespace Gui {
namespace Render {

/**
 * @brief Coin3D 节点包装器基类 / Coin3D node wrapper base class
 *
 * 将 Coin3D SoNode 包装到 RenderNode 抽象接口中。
 * Wraps a Coin3D SoNode into the RenderNode abstract interface.
 *
 * 设计原则 / Design Principles:
 * 1. 保持与现有 FreeCAD Coin3D 代码的兼容性
 * 2. 提供直接访问底层 SoNode 的能力
 * 3. 支持可选的所有权管理
 * 4. 零开销抽象
 */
class GuiExport Coin3DNode : public RenderNode {
public:
    /**
     * @brief 构造函数 / Constructor
     *
     * @param coinNode 底层 Coin3D 节点 / Underlying Coin3D node
     * @param ownsNode 是否拥有节点所有权 / Whether to own the node
     * @param type 节点类型 / Node type
     */
    Coin3DNode(SoNode* coinNode, bool ownsNode, NodeType type);

    /**
     * @brief 虚析构函数 / Virtual destructor
     */
    ~Coin3DNode() override;

    // 禁止拷贝，允许移动 / Disable copy, allow move
    FC_DISABLE_COPY(Coin3DNode)
    FC_DEFAULT_MOVE(Coin3DNode)

    /**
     * @brief 获取底层 Coin3D 节点 / Get underlying Coin3D node
     */
    SoNode* getCoinNode() const { return _coinNode; }

    /**
     * @brief 设置 Coin3D 节点 / Set Coin3D node
     *
     * @param node 新的 Coin3D 节点 / New Coin3D node
     * @param ownsNode 是否拥有节点所有权 / Whether to own the node
     */
    void setCoinNode(SoNode* node, bool ownsNode);

    /**
     * @brief 检查是否拥有节点 / Check if owns the node
     */
    bool ownsNode() const { return _ownsNode; }

    /**
     * @brief 触发节点修改 / Trigger node modification
     */
    void touch() override;

protected:
    void* getBackendNodeImpl() override { return _coinNode; }
    const void* getBackendNodeImpl() const override { return _coinNode; }

    SoNode* _coinNode;      ///< 底层 Coin3D 节点 / Underlying Coin3D node
    bool _ownsNode;         ///< 所有权标志 / Ownership flag
};

//-------------------------------------------------------------------------
// Coin3D 分组节点包装器 / Coin3D Group Node Wrapper
//-------------------------------------------------------------------------

/**
 * @brief Coin3D SoGroup 包装器 / Coin3D SoGroup wrapper
 *
 * 包装 SoGroup 节点，提供子节点管理功能。
 * Wraps SoGroup node, providing child node management.
 */
class GuiExport Coin3DGroup : public Coin3DNode {
public:
    RENDER_NODE_STATIC_TYPE(NodeType::Group)

    /**
     * @brief 构造函数 - 创建新 SoGroup / Constructor - Create new SoGroup
     */
    Coin3DGroup();

    /**
     * @brief 从现有 SoGroup 创建 / Create from existing SoGroup
     *
     * @param group 现有的 SoGroup 节点 / Existing SoGroup node
     * @param ownsNode 是否拥有节点所有权 / Whether to own the node
     */
    explicit Coin3DGroup(SoGroup* group, bool ownsNode = false);

    /**
     * @brief 析构函数 / Destructor
     */
    ~Coin3DGroup() override;

    /**
     * @brief 添加子节点 / Add child node
     */
    void addChild(RenderNode::Ptr child);

    /**
     * @brief 插入子节点 / Insert child node at position
     */
    void insertChild(size_t index, RenderNode::Ptr child);

    /**
     * @brief 移除子节点 / Remove child node
     */
    bool removeChild(RenderNode* child);

    /**
     * @brief 移除所有子节点 / Remove all children
     */
    void removeAllChildren();

    /**
     * @brief 替换子节点 / Replace child node
     */
    bool replaceChild(RenderNode* oldChild, RenderNode::Ptr newChild);

    /**
     * @brief 获取子节点数量 / Get child count
     */
    size_t getNumChildren() const { return _children.size(); }

    /**
     * @brief 获取子节点 / Get child node
     */
    RenderNode* getChild(size_t index) const;
    RenderNode::Ptr getChildPtr(size_t index) const;

    /**
     * @brief 查找子节点 / Find child by name
     */
    RenderNode* findChild(const std::string& name) const;

    /**
     * @brief 获取子节点索引 / Get child index
     */
    int findChildIndex(const RenderNode* child) const;

    /**
     * @brief 直接添加 Coin3D 子节点 / Add Coin3D child node directly
     *
     * @param child Coin3D 节点 / Coin3D node
     */
    void addCoinChild(SoNode* child);

    /**
     * @brief 获取 SoGroup 指针 / Get SoGroup pointer
     */
    SoGroup* getSoGroup() const;

protected:
    /// 包装的子节点（用于跟踪）/ Wrapped children (for tracking)
    std::vector<RenderNode::Ptr> _children;
};

//-------------------------------------------------------------------------
// Coin3D 分隔节点包装器 / Coin3D Separator Node Wrapper
//-------------------------------------------------------------------------

/**
 * @brief Coin3D SoSeparator 包装器 / Coin3D SoSeparator wrapper
 *
 * SoSeparator 继承自 SoGroup，并对子节点的渲染状态进行隔离。
 * SoSeparator inherits from SoGroup and isolates rendering state for children.
 */
class GuiExport Coin3DSeparator : public Coin3DGroup {
public:
    RENDER_NODE_STATIC_TYPE(NodeType::Separator)

    /**
     * @brief 构造函数 - 创建新 SoSeparator / Constructor - Create new SoSeparator
     */
    Coin3DSeparator();

    /**
     * @brief 从现有 SoSeparator 创建 / Create from existing SoSeparator
     *
     * @param sep 现有的 SoSeparator 节点 / Existing SoSeparator node
     * @param ownsNode 是否拥有节点所有权 / Whether to own the node
     */
    explicit Coin3DSeparator(SoSeparator* sep, bool ownsNode = false);

    /**
     * @brief 析构函数 / Destructor
     */
    ~Coin3DSeparator() override;

    /**
     * @brief 获取 SoSeparator 指针 / Get SoSeparator pointer
     */
    SoSeparator* getSoSeparator() const;
};

//-------------------------------------------------------------------------
// Coin3D 变换节点包装器 / Coin3D Transform Node Wrapper
//-------------------------------------------------------------------------

/**
 * @brief Coin3D SoTransform 包装器 / Coin3D SoTransform wrapper
 *
 * 管理节点的变换（平移、旋转、缩放）。
 * Manages node transformation (translation, rotation, scale).
 */
class GuiExport Coin3DTransform : public Coin3DNode {
public:
    RENDER_NODE_STATIC_TYPE(NodeType::Transform)

    /**
     * @brief 构造函数 - 创建新 SoTransform / Constructor - Create new SoTransform
     */
    Coin3DTransform();

    /**
     * @brief 从现有 SoTransform 创建 / Create from existing SoTransform
     */
    explicit Coin3DTransform(SoTransform* transform, bool ownsNode = false);

    /**
     * @brief 析构函数 / Destructor
     */
    ~Coin3DTransform() override;

    /**
     * @brief 获取 SoTransform 指针 / Get SoTransform pointer
     */
    SoTransform* getSoTransform() const;

    // 变换操作 / Transformation operations

    /**
     * @brief 设置平移 / Set translation
     */
    void setTranslation(const Vec3f& translation);

    /**
     * @brief 获取平移 / Get translation
     */
    Vec3f getTranslation() const;

    /**
     * @brief 设置旋转轴和角度 / Set rotation axis and angle
     */
    void setRotation(const Vec3f& axis, float angle);

    /**
     * @brief 设置旋转（四元数）/ Set rotation (quaternion)
     */
    void setRotation(const Quaternion& rotation);

    /**
     * @brief 获取旋转 / Get rotation
     */
    Quaternion getRotation() const;

    /**
     * @brief 设置缩放 / Set scale
     */
    void setScale(const Vec3f& scale);

    /**
     * @brief 获取缩放 / Get scale
     */
    Vec3f getScale() const;

    /**
     * @brief 设置缩放方向 / Set scale orientation
     */
    void setScaleOrientation(const Vec3f& axis, float angle);

    /**
     * @brief 设置中心点 / Set center point
     */
    void setCenter(const Vec3f& center);

    /**
     * @brief 获取中心点 / Get center
     */
    Vec3f getCenter() const;

    /**
     * @brief 组合变换 / Combine transformations
     *
     * 将矩阵应用到当前变换。
     * Apply matrix to current transformation.
     */
    void applyTransform(const Matrix4& matrix);

    /**
     * @brief 重置变换为单位 / Reset transformation to identity
     */
    void reset();
};

//-------------------------------------------------------------------------
// Coin3D 切换节点包装器 / Coin3D Switch Node Wrapper
//-------------------------------------------------------------------------

/**
 * @brief Coin3D SoSwitch 包装器 / Coin3D SoSwitch wrapper
 *
 * 管理显示模式切换（如线框/着色）。
 * Manages display mode switching (e.g., wireframe/shaded).
 *
 * 注意：SoSwitch 继承自 SoGroup，因此 Coin3DSwitch 继承自 Coin3DGroup
 * Note: SoSwitch inherits from SoGroup, so Coin3DSwitch inherits from Coin3DGroup
 */
class GuiExport Coin3DSwitch : public Coin3DGroup {
public:
    RENDER_NODE_STATIC_TYPE(NodeType::Switch)

    /**
     * @brief 构造函数 - 创建新 SoSwitch / Constructor - Create new SoSwitch
     */
    Coin3DSwitch();

    /**
     * @brief 从现有 SoSwitch 创建 / Create from existing SoSwitch
     */
    explicit Coin3DSwitch(SoSwitch* sw, bool ownsNode = false);

    /**
     * @brief 析构函数 / Destructor
     */
    ~Coin3DSwitch() override;

    /**
     * @brief 获取 SoSwitch 指针 / Get SoSwitch pointer
     */
    SoSwitch* getSoSwitch() const;

    /**
     * @brief 设置活动子节点索引 / Set active child index
     *
     * @param index 子节点索引，-1 表示不显示任何子节点 / Child index, -1 to show none
     * @param whichChild SO_SWITCH_ALL / SO_SWITCH_NONE / SO_SWITCH_INVERT
     */
    void setWhichChild(int index);

    /**
     * @brief 获取活动子节点索引 / Get active child index
     */
    int getWhichChild() const;

    /**
     * @brief 获取子节点数量 / Get child count
     * (继承自 Coin3DGroup / Inherited from Coin3DGroup)
     */
    using Coin3DGroup::getNumChildren;

    /**
     * @brief 获取子节点 / Get child node
     * (继承自 Coin3DGroup / Inherited from Coin3DGroup)
     */
    using Coin3DGroup::getChild;

    /**
     * @brief 添加子节点 / Add child node
     * (继承自 Coin3DGroup / Inherited from Coin3DGroup)
     */
    using Coin3DGroup::addChild;
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_COIN3D_COIN3DNODE_H
