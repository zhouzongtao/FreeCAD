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

#ifndef GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSENODE_H
#define GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSENODE_H

#include <memory>
#include <vector>
#include <string>

#include <osg/Node>
#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/Switch>

#include <FCGlobal.h>
#include "../../Core/RenderNode.h"

namespace Gui {
namespace Render {

/**
 * @brief OsgVerse 节点包装器基类 / OsgVerse node wrapper base class
 *
 * 将 OSG/OsgVerse 节点包装到 RenderNode 抽象接口中。
 * Wraps OSG/OsgVerse nodes into the RenderNode abstract interface.
 *
 * 设计原则 / Design Principles:
 * 1. 使用 osg::ref_ptr 管理 OSG 节点生命周期
 * 2. 提供直接访问底层 OSG 节点的能力
 * 3. 支持可选的所有权管理
 * 4. 零开销抽象
 */
class GuiExport OsgVerseNode : public RenderNode {
public:
    /**
     * @brief 构造函数 / Constructor
     *
     * @param osgN层 OSG 节点 / Underlying OSG node
     * @param ownsNode 是否拥有节点所有权 / Whether to own the node
     * @param type 节点类型 / Node type
     */
    OsgVerseNode(osg::Node* osgNode, bool ownsNode, NodeType type);

    /**
     * @brief 虚析构函数 / Virtual destructor
     */
    ~OsgVerseNode() override;

    // 禁止拷贝，允许移动 / Disable copy, allow move
    FC_DISABLE_COPY(OsgVerseNode)
    FC_DEFAULT_MOVE(OsgVerseNode)

    /**
     * @brief 获取底层 OSG 节点 / Get underlying OSG node
     */
    osg::Node* getOsgNode() const { return _osgNode.get(); }

    /**
     * @brief 设置 OSG 节点 / Set OSG node
     *
     * @param node 新的 OSG 节点 / New OSG node
     * @param ownsNode 是否拥有节点所有权 / Whether to own the node
     */
    void setOsgNode(osg::Node* node, bool ownsNode);

    /**
     * @brief 检查是否拥有节点 / Check if owns the node
     */
    bool ownsNode() const { return _ownsNode; }

    /**
     * @brief 触发节点修改 / Trigger node modification
     */
    void touch() override;

    /**
     * @brief 设置节点名称 / Set node name
     */
    void setNodeName(const std::string& name);

    /**
     * @brief 获取节点名称 / Get node name
     */
    std::string getNodeName() const;

protected:
    void* getBackendNodeImpl() override { return _osgNode.get(); }
    const void* getBackendNodeImpl() const override { return _osgNode.get(); }

    osg::ref_ptr<osg::Node> _osgNode;  ///< 底层 OSG 节点 / Underlying OSG node
    bool _ownsNode;                     ///< 所有权标志 / Ownership flag
};

//-------------------------------------------------------------------------
// OsgVerse 分组节点包装器 / OsgVerse Group Node Wrapper
//-------------------------------------------------------------------------

/**
 * @brief OsgVerse osg::Group 包装器 / OsgVerse osg::Group wrapper
 *
 * 包装 osg::Group 节点，提供子节点管理功能。
 * Wraps osg::Group node, providing child node management.
 */
class GuiExport OsgVerseGroup : public OsgVerseNode {
public:
    RENDER_NODE_STATIC_TYPE(NodeType::Group)

    /**
     * @brief 构造函数 - 创建新 osg::Group / Constructor - Create new osg::Group
     */
    OsgVerseGroup();

    /**
     * @brief 从现有 osg::Group 创建 / Create from existing osg::Group
     *
     * @param group 现有的 osg::Group 节点 / Existing osg::Group node
     * @param ownsNode 是否拥有节点所有权 / Whether to own the node
     */
    explicit OsgVerseGroup(osg::Group* group, bool ownsNode = false);

    /**
     * @brief 析构函数 / Destructor
     */
    ~OsgVerseGroup() override;

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
     * @brief 直接添加 OSG 子节点 / Add OSG child node directly
     *
     * @param child OSG 节点 / OSG node
     */
    void addOsgChild(osg::Node* child);

    /**
     * @brief 获取 osg::Group 指针 / Get osg::Group pointer
     */
    osg::Group* getOsgGroup() const;

protected:
    /// 包装的子节点（用于跟踪）/ Wrapped children (for tracking)
    std::vector<RenderNode::Ptr> _children;
};

//-------------------------------------------------------------------------
// OsgVerse 分隔节点包装器 / OsgVerse Separator Node Wrapper
//-------------------------------------------------------------------------

/**
 * @brief OsgVerse Separator 包装器 / OsgVerse Separator wrapper
 *
 * 在 OSG 中，Separator 通过 osg::Group + StateSet 实现状态隔离。
 * In OSG, Separator is implemented via osg::Group + StateSet for state isolation.
 */
class GuiExport OsgVerseSeparator : public OsgVerseGroup {
public:
    RENDER_NODE_STATIC_TYPE(NodeType::Separator)

    /**
     * @brief 构造函数 - 创建新 Separator / Constructor - Create new Separator
     */
    OsgVerseSeparator();

    /**
     * @brief 从现有 osg::Group 创建 / Create from existing osg::Group
     *
     * @param group 现有的 osg::Group 节点 / Existing osg::Group node
     * @param ownsNode 是否拥有节点所有权 / Whether to own the node
     */
    explicit OsgVerseSeparator(osg::Group* group, bool ownsNode = false);

    /**
     * @brief 析构函数 / Destructor
     */
    ~OsgVerseSeparator() override;
};

//-------------------------------------------------------------------------
// OsgVerse 变换节点包装器 / OsgVerse Transform Node Wrapper
//-------------------------------------------------------------------------

/**
 * @brief OsgVerse osg::MatrixTransform 包装器 / OsgVerse osg::MatrixTransform wrapper
 *
 * 管理节点的变换（平移、旋转、缩放）。
 * Manages node transformation (translation, rotation, scale).
 */
class GuiExport OsgVerseTransform : public OsgVerseNode {
public:
    RENDER_NODE_STATIC_TYPE(NodeType::Transform)

    /**
     * @brief 构造函数 - 创建新 osg::MatrixTransform / Constructor - Create new osg::MatrixTransform
     */
    OsgVerseTransform();

    /**
     * @brief 从现有 osg::MatrixTransform 创建 / Create from existing osg::MatrixTransform
     */
    explicit OsgVerseTransform(osg::MatrixTransform* transform, bool ownsNode = false);

    /**
     * @brief 析构函数 / Destructor
     */
    ~OsgVerseTransform() override;

    /**
     * @brief 获取 osg::MatrixTransform 指针 / Get osg::MatrixTransform pointer
     */
    osg::MatrixTransform* getOsgTransform() const;

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
     * @brief 设置变换矩阵 / Set transformation matrix
     */
    void setMatrix(const Matrix4& matrix);

    /**
     * @brief 获取变换矩阵 / Get transformation matrix
     */
    Matrix4 getMatrix() const;

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
// OsgVerse 切换节点包装器 / OsgVerse Switch Node Wrapper
//-------------------------------------------------------------------------

/**
 * @brief OsgVerse osg::Switch 包装器 / OsgVerse osg::Switch wrapper
 *
 * 管理显示模式切换（如线框/着色）。
 * Manages display mode switching (e.g., wireframe/shaded).
 */
class GuiExport OsgVerseSwitch : public OsgVerseGroup {
public:
    RENDER_NODE_STATIC_TYPE(NodeType::Switch)

    /**
     * @brief 构造函数 - 创建新 osg::Switch / Constructor - Create new osg::Switch
     */
    OsgVerseSwitch();

    /**
     * @brief 从现有 osg::Switch 创建 / Create from existing osg::Switch
     */
    explicit OsgVerseSwitch(osg::Switch* sw, bool ownsNode = false);

    /**
     * @brief 析构函数 / Destructor
     */
    ~OsgVerseSwitch() override;

    /**
     * @brief 获取 osg::Switch 指针 / Get osg::Switch pointer
     */
    osg::Switch* getOsgSwitch() const;

    /**
     * @brief 设置活动子节点索引 / Set active child index
     *
     * @param index 子节点索引，-1 表示不显示任何子节点 / Child index, -1 to show none
     */
    void setWhichChild(int index);

    /**
     * @brief 获取活动子节点索引 / Get active child index
     */
    int getWhichChild() const;

    /**
     * @brief 设置所有子节点可见性 / Set all children visibility
     */
    void setAllChildrenOn();

    /**
     * @brief 设置所有子节点不可见 / Set all children off
     */
    void setAllChildrenOff();

    /**
     * @brief 设置单个子节点可见性 / Set single child visibility
     */
    void setValue(size_t index, bool value);

    /**
     * @brief 获取单个子节点可见性 / Get single child visibility
     */
    bool getValue(size_t index) const;
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSENODE_H
