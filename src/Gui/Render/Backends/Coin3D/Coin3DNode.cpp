/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Development Team                             *
 *                                                                         *
 *   This file is part of the FreeCAD CAD development system.              *
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

#include "Coin3DNode.h"
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/misc/SoChildList.h>
#include <Base/Console.h>

namespace Gui {
namespace Render {

//===========================================================================
// Coin3DNode - 基础包装器类 / Base Wrapper Class
//===========================================================================

Coin3DNode::Coin3DNode(SoNode* coinNode, bool ownsNode, NodeType type)
    : RenderNode(type)
    , _coinNode(coinNode)
    , _ownsNode(ownsNode)
{
    setBackendType(BackendType::Coin3D);
}

Coin3DNode::~Coin3DNode()
{
    if (_ownsNode && _coinNode) {
        _coinNode->unref();
    }
}

void Coin3DNode::setCoinNode(SoNode* node, bool ownsNode)
{
    if (_ownsNode && _coinNode && (_coinNode != node)) {
        _coinNode->unref();
    }

    _coinNode = node;
    _ownsNode = ownsNode;

    if (_coinNode && _ownsNode) {
        _coinNode->ref();
    }

    touch();
}

void Coin3DNode::touch()
{
    RenderNode::touch();

    // 触发 Coin3D 节点的 touch() 方法
    // Trigger Coin3D node's touch() method
    if (_coinNode) {
        _coinNode->touch();
    }
}

//===========================================================================
// Coin3DNode Selection Support
//===========================================================================

void Coin3DNode::setSelected(bool selected, const std::string& element)
{
    // Update base class state
    RenderNode::setSelected(selected, element);

    // Coin3D selection synchronization is handled through SoFCSelection nodes
    // in the actual scene graph. This method updates the internal state,
    // and the View3DInventorViewer/SoFCUnifiedSelection handles the visual update.
    //
    // Note: For full Coin3D integration, we would need to find the SoFCSelection
    // parent node and call its setSelected() method. However, that's typically
    // handled by the selection system itself (SelectionSingleton -> SoFCSelectionAction).

    if (_coinNode) {
        _coinNode->touch();  // Trigger redraw
    }
}

void Coin3DNode::setHighlighted(bool highlighted, const std::string& element)
{
    // Update base class state
    RenderNode::setHighlighted(highlighted, element);

    // Similar to selection, highlighting is typically handled through
    // SoFCUnifiedSelection and SoFCPreselectionAction in Coin3D.

    if (_coinNode) {
        _coinNode->touch();  // Trigger redraw
    }
}

void Coin3DNode::setHighlightColor(float r, float g, float b, float a)
{
    // Update base class state
    RenderNode::setHighlightColor(r, g, b, a);

    // Note: In Coin3D, highlight colors are typically managed globally
    // through SoFCUnifiedSelection::colorHighlight field.
    // This method updates the per-node state which can be used by
    // custom rendering or alternative backends.
}

void Coin3DNode::setSelectionColor(float r, float g, float b, float a)
{
    // Update base class state
    RenderNode::setSelectionColor(r, g, b, a);

    // Note: In Coin3D, selection colors are typically managed globally
    // through SoFCUnifiedSelection::colorSelection field.
}

//===========================================================================
// Coin3DGroup - SoGroup 包装器 / SoGroup Wrapper
//===========================================================================

Coin3DGroup::Coin3DGroup()
    : Coin3DNode(new SoGroup(), true, NodeType::Group)
{
    _coinNode->ref();
}

Coin3DGroup::Coin3DGroup(SoGroup* group, bool ownsNode)
    : Coin3DNode(group, ownsNode, NodeType::Group)
{
}

Coin3DGroup::~Coin3DGroup()
{
    // 清理包装的子节点 / Clean up wrapped children
    _children.clear();
}

void Coin3DGroup::addChild(RenderNode::Ptr child)
{
    if (!child) {
        return;
    }

    if (child->getParent()) {
        Base::Console().warning("Coin3DGroup::addChild: Node already has a parent\n");
        return;
    }

    _children.push_back(child);
    child->setParent(this);

    // 添加到 SoGroup / Add to SoGroup
    if (auto* coin3DChild = dynamic_cast<Coin3DNode*>(child.get())) {
        getSoGroup()->addChild(coin3DChild->getCoinNode());
    } else {
        // 对于非 Coin3D 节点，需要额外处理
        // For non-Coin3D nodes, additional handling is needed
        Base::Console().warning("Coin3DGroup::addChild: Adding non-Coin3D node not yet supported\n");
    }

    touch();
}

void Coin3DGroup::insertChild(size_t index, RenderNode::Ptr child)
{
    if (!child) {
        return;
    }

    if (child->getParent()) {
        Base::Console().warning("Coin3DGroup::insertChild: Node already has a parent\n");
        return;
    }

    if (index >= _children.size()) {
        index = _children.size();
    }

    _children.insert(_children.begin() + index, child);
    child->setParent(this);

    if (auto* coin3DChild = dynamic_cast<Coin3DNode*>(child.get())) {
        getSoGroup()->insertChild(coin3DChild->getCoinNode(), static_cast<int>(index));
    }

    touch();
}

bool Coin3DGroup::removeChild(RenderNode* child)
{
    if (!child) {
        return false;
    }

    for (auto it = _children.begin(); it != _children.end(); ++it) {
        if (it->get() == child) {
            child->setParent(nullptr);

            if (auto* coin3DChild = dynamic_cast<Coin3DNode*>(child)) {
                getSoGroup()->removeChild(coin3DChild->getCoinNode());
            }

            _children.erase(it);
            touch();
            return true;
        }
    }
    return false;
}

void Coin3DGroup::removeAllChildren()
{
    for (auto& child : _children) {
        child->setParent(nullptr);
    }
    _children.clear();
    getSoGroup()->removeAllChildren();
    touch();
}

bool Coin3DGroup::replaceChild(RenderNode* oldChild, RenderNode::Ptr newChild)
{
    if (!oldChild || !newChild) {
        return false;
    }

    for (size_t i = 0; i < _children.size(); ++i) {
        if (_children[i].get() == oldChild) {
            oldChild->setParent(nullptr);

            if (auto* oldCoin3D = dynamic_cast<Coin3DNode*>(oldChild)) {
                if (auto* newCoin3D = dynamic_cast<Coin3DNode*>(newChild.get())) {
                    getSoGroup()->replaceChild(oldCoin3D->getCoinNode(), newCoin3D->getCoinNode());
                }
            }

            _children[i] = newChild;
            newChild->setParent(this);
            touch();
            return true;
        }
    }
    return false;
}

RenderNode* Coin3DGroup::getChild(size_t index) const
{
    if (index >= _children.size()) {
        return nullptr;
    }
    return _children[index].get();
}

RenderNode::Ptr Coin3DGroup::getChildPtr(size_t index) const
{
    if (index >= _children.size()) {
        return nullptr;
    }
    return _children[index];
}

RenderNode* Coin3DGroup::findChild(const std::string& name) const
{
    for (const auto& child : _children) {
        if (child->getName() == name) {
            return child.get();
        }
    }
    return nullptr;
}

int Coin3DGroup::findChildIndex(const RenderNode* child) const
{
    if (!child) {
        return -1;
    }
    for (size_t i = 0; i < _children.size(); ++i) {
        if (_children[i].get() == child) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void Coin3DGroup::addCoinChild(SoNode* child)
{
    if (child) {
        getSoGroup()->addChild(child);
        touch();
    }
}

SoGroup* Coin3DGroup::getSoGroup() const
{
    return static_cast<SoGroup*>(_coinNode);
}

//===========================================================================
// Coin3DSeparator - SoSeparator 包装器 / SoSeparator Wrapper
//===========================================================================

Coin3DSeparator::Coin3DSeparator()
    : Coin3DGroup(new SoSeparator(), true)
{
    // 父类构造会设置 NodeType::Group，需要修正
    // Parent constructor sets NodeType::Group, need to fix
    _nodeType = NodeType::Separator;
}

Coin3DSeparator::Coin3DSeparator(SoSeparator* sep, bool ownsNode)
    : Coin3DGroup(sep, ownsNode)
{
    _nodeType = NodeType::Separator;
}

Coin3DSeparator::~Coin3DSeparator() = default;

SoSeparator* Coin3DSeparator::getSoSeparator() const
{
    return static_cast<SoSeparator*>(_coinNode);
}

//===========================================================================
// Coin3DTransform - SoTransform 包装器 / SoTransform Wrapper
//===========================================================================

Coin3DTransform::Coin3DTransform()
    : Coin3DNode(new SoTransform(), true, NodeType::Transform)
{
    _coinNode->ref();
}

Coin3DTransform::Coin3DTransform(SoTransform* transform, bool ownsNode)
    : Coin3DNode(transform, ownsNode, NodeType::Transform)
{
}

Coin3DTransform::~Coin3DTransform() = default;

SoTransform* Coin3DTransform::getSoTransform() const
{
    return static_cast<SoTransform*>(_coinNode);
}

void Coin3DTransform::setTranslation(const Vec3f& translation)
{
    if (auto* t = getSoTransform()) {
        t->translation.setValue(translation.x, translation.y, translation.z);
        touch();
    }
}

Vec3f Coin3DTransform::getTranslation() const
{
    if (auto* t = getSoTransform()) {
        const SbVec3f& v = t->translation.getValue();
        return Vec3f(v[0], v[1], v[2]);
    }
    return Vec3f();
}

void Coin3DTransform::setRotation(const Vec3f& axis, float angle)
{
    if (auto* t = getSoTransform()) {
        t->rotation.setValue(axis.x, axis.y, axis.z, angle);
        touch();
    }
}

void Coin3DTransform::setRotation(const Quaternion& rotation)
{
    if (auto* t = getSoTransform()) {
        // Coin3D 使用轴-角表示 / Coin3D uses axis-angle representation
        // TODO: 将四元数转换为轴-角 / Convert quaternion to axis-angle
        t->rotation.setValue(rotation.x, rotation.y, rotation.z, rotation.w);
        touch();
    }
}

Quaternion Coin3DTransform::getRotation() const
{
    if (auto* t = getSoTransform()) {
        const SbRotation& rot = t->rotation.getValue();
        float x, y, z, w;
        rot.getValue(x, y, z, w);
        return Quaternion(x, y, z, w);
    }
    return Quaternion();
}

void Coin3DTransform::setScale(const Vec3f& scale)
{
    if (auto* t = getSoTransform()) {
        t->scaleFactor.setValue(scale.x, scale.y, scale.z);
        touch();
    }
}

Vec3f Coin3DTransform::getScale() const
{
    if (auto* t = getSoTransform()) {
        const SbVec3f& s = t->scaleFactor.getValue();
        return Vec3f(s[0], s[1], s[2]);
    }
    return Vec3f(1.0f, 1.0f, 1.0f);
}

void Coin3DTransform::setScaleOrientation(const Vec3f& axis, float angle)
{
    if (auto* t = getSoTransform()) {
        t->scaleOrientation.setValue(axis.x, axis.y, axis.z, angle);
        touch();
    }
}

void Coin3DTransform::setCenter(const Vec3f& center)
{
    if (auto* t = getSoTransform()) {
        t->center.setValue(center.x, center.y, center.z);
        touch();
    }
}

Vec3f Coin3DTransform::getCenter() const
{
    if (auto* t = getSoTransform()) {
        const SbVec3f& c = t->center.getValue();
        return Vec3f(c[0], c[1], c[2]);
    }
    return Vec3f();
}

void Coin3DTransform::applyTransform(const Matrix4& matrix)
{
    // TODO: 实现矩阵应用 / Implement matrix application
    // 需要将 FreeCAD Matrix4 转换为 SbMatrix 并应用到 SoTransform
    // Need to convert FreeCAD Matrix4 to SbMatrix and apply to SoTransform
    touch();
}

void Coin3DTransform::reset()
{
    if (auto* t = getSoTransform()) {
        t->translation.setValue(0.0f, 0.0f, 0.0f);
        t->rotation.setValue(0.0f, 0.0f, 1.0f, 0.0f);
        t->scaleFactor.setValue(1.0f, 1.0f, 1.0f);
        t->scaleOrientation.setValue(0.0f, 0.0f, 1.0f, 0.0f);
        t->center.setValue(0.0f, 0.0f, 0.0f);
        touch();
    }
}

//===========================================================================
// Coin3DSwitch - SoSwitch 包装器 / SoSwitch Wrapper
//===========================================================================

// Coin3DSwitch - SoSwitch wrapper
// Note: Constants moved inline to avoid syntax issues

Coin3DSwitch::Coin3DSwitch()
    : Coin3DGroup(new SoSwitch(), true)
{
    // 父类构造会设置 NodeType::Group，需要修正为 Switch
    // Parent constructor sets NodeType::Group, need to fix to Switch
    _nodeType = NodeType::Switch;
}

Coin3DSwitch::Coin3DSwitch(SoSwitch* sw, bool ownsNode)
    : Coin3DGroup(sw, ownsNode)
{
    _nodeType = NodeType::Switch;
}

Coin3DSwitch::~Coin3DSwitch() = default;

SoSwitch* Coin3DSwitch::getSoSwitch() const
{
    return static_cast<SoSwitch*>(_coinNode);
}

void Coin3DSwitch::setWhichChild(int index)
{
    if (auto* sw = getSoSwitch()) {
        sw->whichChild.setValue(index);
        touch();
    }
}

int Coin3DSwitch::getWhichChild() const
{
    if (auto* sw = getSoSwitch()) {
        return sw->whichChild.getValue();
    }
    return -1;  // SO_SWITCH_NONE
}

} // namespace Render
} // namespace Gui
