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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <osg/Node>
# include <osg/Group>
# include <osg/MatrixTransform>
# include <osg/Switch>
# include <osg/StateSet>
#endif

#include "OsgVerseNode.h"
#include <Base/Console.h>

using namespace Gui::Render;

//===========================================================================
// OsgVerseNode - 基础节点包装器 / Base Node Wrapper
//===========================================================================

OsgVerseNode::OsgVerseNode(osg::Node* osgNode, bool ownsNode, NodeType type)
    : RenderNode(type)
    , _osgNode(osgNode)
    , _ownsNode(ownsNode)
{
    if (!_osgNode) {
        Base::Console().warning("OsgVerseNode: Creating node with null OSG node\n");
    }
}

OsgVerseNode::~OsgVerseNode()
{
    // osg::ref_ptr 会自动管理引用计数
    // osg::ref_ptr automatically manages reference counting
}

void OsgVerseNode::setOsgNode(osg::Node* node, bool ownsNode)
{
    _osgNode = node;
    _ownsNode = ownsNode;
}

void OsgVerseNode::touch()
{
    if (_osgNode) {
        // OSG 使用 dirtyBound() 来标记节点需要更新
        // OSG uses dirtyBound() to mark node as needing update
        _osgNode->dirtyBound();
    }
}

void OsgVerseNode::setNodeName(const std::string& name)
{
    RenderNode::setName(name);
    if (_osgNode) {
        _osgNode->setName(name);
    }
}

std::string OsgVerseNode::getNodeName() const
{
    if (_osgNode) {
        return _osgNode->getName();
    }
    return RenderNode::getName();
}

//===========================================================================
// OsgVerseGroup - 分组节点包装器 / Group Node Wrapper
//===========================================================================

OsgVerseGroup::OsgVerseGroup()
    : OsgVerseNode(new osg::Group(), true, NodeType::Group)
{
}

OsgVerseGroup::OsgVerseGroup(osg::Group* group, bool ownsNode)
    : OsgVerseNode(group, ownsNode, NodeType::Group)
{
}

OsgVerseGroup::~OsgVerseGroup()
{
    // 清理子节点引用
    // Clean up child references
    _children.clear();
}

osg::Group* OsgVerseGroup::getOsgGroup() const
{
    return dynamic_cast<osg::Group*>(_osgNode.get());
}

void OsgVerseGroup::addChild(RenderNode::Ptr child)
{
    if (!child) {
        Base::Console().warning("OsgVerseGroup::addChild: Null child\n");
        return;
    }

    auto* group = getOsgGroup();
    if (!group) {
        Base::Console().error("OsgVerseGroup::addChild: Not a group node\n");
        return;
    }

    // 获取底层 OSG 节点
    // Get underlying OSG node
    auto* osgChild = child->getBackendNode<osg::Node>();
    if (!osgChild) {
        Base::Console().error("OsgVerseGroup::addChild: Child has no OSG node\n");
        return;
    }

    // 添加到 OSG 场景图
    // Add to OSG scene graph
    group->addChild(osgChild);

    // 保存引用
    // Save reference
    _children.push_back(child);
}

void OsgVerseGroup::insertChild(size_t index, RenderNode::Ptr child)
{
    if (!child) {
        Base::Console().warning("OsgVerseGroup::insertChild: Null child\n");
        return;
    }

    auto* group = getOsgGroup();
    if (!group) {
        Base::Console().error("OsgVerseGroup::insertChild: Not a group node\n");
        return;
    }

    auto* osgChild = child->getBackendNode<osg::Node>();
    if (!osgChild) {
        Base::Console().error("OsgVerseGroup::insertChild: Child has no OSG node\n");
        return;
    }

    // OSG 使用 insertChild
    // OSG uses insertChild
    if (index >= _children.size()) {
        group->addChild(osgChild);
        _children.push_back(child);
    }
    else {
        group->insertChild(index, osgChild);
        _children.insert(_children.begin() + index, child);
    }
}

bool OsgVerseGroup::removeChild(RenderNode* child)
{
    if (!child) {
        return false;
    }

    auto* group = getOsgGroup();
    if (!group) {
        return false;
    }

    // 查找子节点
    // Find child
    auto it = std::find_if(_children.begin(), _children.end(),
        [child](const RenderNode::Ptr& ptr) { return ptr.get() == child; });

    if (it == _children.end()) {
        return false;
    }

    // 从 OSG 场景图移除
    // Remove from OSG scene graph
    auto* osgChild = child->getBackendNode<osg::Node>();
    if (osgChild) {
        group->removeChild(osgChild);
    }

    // 移除引用
    // Remove reference
    _children.erase(it);
    return true;
}

void OsgVerseGroup::removeAllChildren()
{
    auto* group = getOsgGroup();
    if (group) {
        group->removeChildren(0, group->getNumChildren());
    }
    _children.clear();
}

bool OsgVerseGroup::replaceChild(RenderNode* oldChild, RenderNode::Ptr newChild)
{
    if (!oldChild || !newChild) {
        return false;
    }

    auto* group = getOsgGroup();
    if (!group) {
        return false;
    }

    // 查找旧子节点
    // Find old child
    auto it = std::find_if(_children.begin(), _children.end(),
        [oldChild](const RenderNode::Ptr& ptr) { return ptr.get() == oldChild; });

    if (it == _children.end()) {
        return false;
    }

    // 替换 OSG 节点
    // Replace OSG node
    auto* osgOldChild = oldChild->getBackendNode<osg::Node>();
    auto* osgNewChild = newChild->getBackendNode<osg::Node>();

    if (osgOldChild && osgNewChild) {
        group->replaceChild(osgOldChild, osgNewChild);
    }

    // 替换引用
    // Replace reference
    *it = newChild;
    return true;
}

RenderNode* OsgVerseGroup::getChild(size_t index) const
{
    if (index >= _children.size()) {
        return nullptr;
    }
    return _children[index].get();
}

RenderNode::Ptr OsgVerseGroup::getChildPtr(size_t index) const
{
    if (index >= _children.size()) {
        return nullptr;
    }
    return _children[index];
}

RenderNode* OsgVerseGroup::findChild(const std::string& name) const
{
    for (const auto& child : _children) {
        if (child->getName() == name) {
            return child.get();
        }
    }
    return nullptr;
}

int OsgVerseGroup::findChildIndex(const RenderNode* child) const
{
    auto it = std::find_if(_children.begin(), _children.end(),
        [child](const RenderNode::Ptr& ptr) { return ptr.get() == child; });

    if (it == _children.end()) {
        return -1;
    }
    return static_cast<int>(std::distance(_children.begin(), it));
}

void OsgVerseGroup::addOsgChild(osg::Node* child)
{
    auto* group = getOsgGroup();
    if (group && child) {
        group->addChild(child);
    }
}

//===========================================================================
// OsgVerseSeparator - 分隔节点包装器 / Separator Node Wrapper
//===========================================================================

OsgVerseSeparator::OsgVerseSeparator()
    : OsgVerseGroup(new osg::Group(), true)
{
    _nodeType = NodeType::Separator;

    // 在 OSG 中，Separator 通过 StateSet 实现状态隔离
    // In OSG, Separator is implemented via StateSet for state isolation
    auto* group = getOsgGroup();
    if (group) {
        // 创建新的 StateSet 以隔离渲染状态
        // Create new StateSet to isolate rendering state
        group->setStateSet(new osg::StateSet());
    }
}

OsgVerseSeparator::OsgVerseSeparator(osg::Group* group, bool ownsNode)
    : OsgVerseGroup(group, ownsNode)
{
    _nodeType = NodeType::Separator;

    // 确保有 StateSet
    // Ensure StateSet exists
    if (group && !group->getStateSet()) {
        group->setStateSet(new osg::StateSet());
    }
}

OsgVerseSeparator::~OsgVerseSeparator() = default;

//===========================================================================
// OsgVerseTransform - 变换节点包装器 / Transform Node Wrapper
//===========================================================================

OsgVerseTransform::OsgVerseTransform()
    : OsgVerseNode(new osg::MatrixTransform(), true, NodeType::Transform)
{
}

OsgVerseTransform::OsgVerseTransform(osg::MatrixTransform* transform, bool ownsNode)
    : OsgVerseNode(transform, ownsNode, NodeType::Transform)
{
}

OsgVerseTransform::~OsgVerseTransform() = default;

osg::MatrixTransform* OsgVerseTransform::getOsgTransform() const
{
    return dynamic_cast<osg::MatrixTransform*>(_osgNode.get());
}

void OsgVerseTransform::setTranslation(const Vec3f& translation)
{
    auto* transform = getOsgTransform();
    if (!transform) {
        return;
    }

    osg::Matrix matrix = transform->getMatrix();
    matrix.setTrans(osg::Vec3d(translation.x, translation.y, translation.z));
    transform->setMatrix(matrix);
}

Vec3f OsgVerseTransform::getTranslation() const
{
    auto* transform = getOsgTransform();
    if (!transform) {
        return Vec3f(0, 0, 0);
    }

    osg::Vec3d trans = transform->getMatrix().getTrans();
    return Vec3f(static_cast<float>(trans.x()),
                 static_cast<float>(trans.y()),
                 static_cast<float>(trans.z()));
}

void OsgVerseTransform::setRotation(const Vec3f& axis, float angle)
{
    auto* transform = getOsgTransform();
    if (!transform) {
        return;
    }

    osg::Matrix matrix = transform->getMatrix();
    osg::Quat rotation(angle, osg::Vec3d(axis.x, axis.y, axis.z));
    matrix.setRotate(rotation);
    transform->setMatrix(matrix);
}

void OsgVerseTransform::setRotation(const Quaternion& rotation)
{
    auto* transform = getOsgTransform();
    if (!transform) {
        return;
    }

    // 转换 FreeCAD Quaternion 到 OSG Quat
    // Convert FreeCAD Quaternion to OSG Quat
    double x, y, z, w;
    rotation.getValue(x, y, z, w);

    osg::Matrix matrix = transform->getMatrix();
    matrix.setRotate(osg::Quat(x, y, z, w));
    transform->setMatrix(matrix);
}

Quaternion OsgVerseTransform::getRotation() const
{
    auto* transform = getOsgTransform();
    if (!transform) {
        return Quaternion();
    }

    osg::Quat quat = transform->getMatrix().getRotate();
    return Quaternion(quat.x(), quat.y(), quat.z(), quat.w());
}

void OsgVerseTransform::setScale(const Vec3f& scale)
{
    auto* transform = getOsgTransform();
    if (!transform) {
        return;
    }

    // OSG doesn't have setScale(), need to rebuild matrix
    osg::Matrix oldMatrix = transform->getMatrix();
    osg::Vec3d trans = oldMatrix.getTrans();
    osg::Quat rot = oldMatrix.getRotate();
    
    osg::Matrix matrix = osg::Matrixd::scale(scale.x, scale.y, scale.z) *
                         osg::Matrixd::rotate(rot) *
                         osg::Matrixd::translate(trans);
    transform->setMatrix(matrix);
}

Vec3f OsgVerseTransform::getScale() const
{
    auto* transform = getOsgTransform();
    if (!transform) {
        return Vec3f(1, 1, 1);
    }

    osg::Vec3d scale = transform->getMatrix().getScale();
    return Vec3f(static_cast<float>(scale.x()),
                 static_cast<float>(scale.y()),
                 static_cast<float>(scale.z()));
}

void OsgVerseTransform::setMatrix(const Matrix4& matrix)
{
    auto* transform = getOsgTransform();
    if (!transform) {
        return;
    }

    // 转换 FreeCAD Matrix4D 到 OSG Matrix
    // Convert FreeCAD Matrix4D to OSG Matrix
    osg::Matrixd osgMatrix;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            osgMatrix(i, j) = matrix[i][j];
        }
    }

    transform->setMatrix(osgMatrix);
}

Matrix4 OsgVerseTransform::getMatrix() const
{
    auto* transform = getOsgTransform();
    if (!transform) {
        return Matrix4();
    }

    const osg::Matrixd& osgMatrix = transform->getMatrix();
    Matrix4 matrix;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            matrix[i][j] = osgMatrix(i, j);
        }
    }

    return matrix;
}

void OsgVerseTransform::applyTransform(const Matrix4& matrix)
{
    auto* transform = getOsgTransform();
    if (!transform) {
        return;
    }

    // 转换并组合矩阵
    // Convert and combine matrices
    osg::Matrixd osgMatrix;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            osgMatrix(i, j) = matrix[i][j];
        }
    }

    transform->setMatrix(transform->getMatrix() * osgMatrix);
}

void OsgVerseTransform::reset()
{
    auto* transform = getOsgTransform();
    if (transform) {
        transform->setMatrix(osg::Matrixd::identity());
    }
}

//===========================================================================
// OsgVerseSwitch - 切换节点包装器 / Switch Node Wrapper
//===========================================================================

OsgVerseSwitch::OsgVerseSwitch()
    : OsgVerseGroup(new osg::Switch(), true)
{
    _nodeType = NodeType::Switch;
}

OsgVerseSwitch::OsgVerseSwitch(osg::Switch* sw, bool ownsNode)
    : OsgVerseGroup(sw, ownsNode)
{
    _nodeType = NodeType::Switch;
}

OsgVerseSwitch::~OsgVerseSwitch() = default;

osg::Switch* OsgVerseSwitch::getOsgSwitch() const
{
    return dynamic_cast<osg::Switch*>(_osgNode.get());
}

void OsgVerseSwitch::setWhichChild(int index)
{
    auto* sw = getOsgSwitch();
    if (!sw) {
        return;
    }

    if (index < 0) {
        // 不显示任何子节点
        // Show no children
        sw->setAllChildrenOff();
    }
    else if (index == -3) {  // ALL_CHILDREN
        // 显示所有子节点
        // Show all children
        sw->setAllChildrenOn();
    }
    else {
        // 只显示指定子节点
        // Show only specified child
        sw->setSingleChildOn(index);
    }
}

int OsgVerseSwitch::getWhichChild() const
{
    auto* sw = getOsgSwitch();
    if (!sw) {
        return -1;
    }

    // OSG Switch 没有直接的 "whichChild" 概念
    // 需要检查哪个子节点是可见的
    // OSG Switch doesn't have direct "whichChild" concept
    // Need to check which child is visible
    for (unsigned int i = 0; i < sw->getNumChildren(); ++i) {
        if (sw->getValue(i)) {
            return static_cast<int>(i);
        }
    }

    return -1;  // 没有子节点可见 / No child visible
}

void OsgVerseSwitch::setAllChildrenOn()
{
    auto* sw = getOsgSwitch();
    if (sw) {
        sw->setAllChildrenOn();
    }
}

void OsgVerseSwitch::setAllChildrenOff()
{
    auto* sw = getOsgSwitch();
    if (sw) {
        sw->setAllChildrenOff();
    }
}

void OsgVerseSwitch::setValue(size_t index, bool value)
{
    auto* sw = getOsgSwitch();
    if (sw && index < sw->getNumChildren()) {
        sw->setValue(index, value);
    }
}

bool OsgVerseSwitch::getValue(size_t index) const
{
    auto* sw = getOsgSwitch();
    if (sw && index < sw->getNumChildren()) {
        return sw->getValue(index);
    }
    return false;
}
