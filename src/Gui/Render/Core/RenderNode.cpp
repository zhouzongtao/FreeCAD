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

#include "RenderNode.h"
#include <Base/Console.h>

FC_LOG_LEVEL_INIT("Render")

namespace Gui {
namespace Render {

//===========================================================================
// RenderNode
//===========================================================================

RenderNode::RenderNode(NodeType type)
    : _nodeType(type)
    , _refCount(0)
    , _parent(nullptr)
    , _visible(true)
    , _touched(false)
    , _renderMode(RenderMode::Default)
    , _backendType(BackendType::None)
{
}

RenderNode::~RenderNode() = default;

void RenderNode::ref() const
{
    ++_refCount;
}

void RenderNode::unref() const
{
    if (--_refCount <= 0) {
        delete this;
    }
}

std::vector<RenderNode*> RenderNode::getPath() const
{
    std::vector<RenderNode*> path;
    const RenderNode* current = this;
    while (current) {
        path.push_back(const_cast<RenderNode*>(current));
        current = current->_parent;
    }
    std::reverse(path.begin(), path.end());
    return path;
}

void RenderNode::onEvent(const std::string& event, void* data)
{
    // 默认空实现 / Default empty implementation
    // 派生类可以重写此方法来处理特定事件 / Derived classes can override to handle specific events
    (void)event;
    (void)data;
}

void RenderNode::touch()
{
    _touched = true;
    // 递归标记父节点为已修改 / Recursively mark parent as modified
    if (_parent) {
        _parent->touch();
    }
}

BoundingBox RenderNode::getBoundingBox() const
{
    // 默认返回空边界框 / Default empty bounding box
    return BoundingBox();
}

RenderNode::Ptr RenderNode::clone() const
{
    // 默认实现返回 nullptr，派生类应重写
    // Default implementation returns nullptr, derived classes should override
    return nullptr;
}

//===========================================================================
// RenderGroup
//===========================================================================

RenderGroup::RenderGroup()
    : RenderNode(NodeType::Group)
{
}

RenderGroup::~RenderGroup()
{
    removeAllChildren();
}

void RenderGroup::addChild(RenderNode::Ptr child)
{
    if (!child) {
        return;
    }

    // 检查是否已经是某个节点的子节点
    // Check if already a child of some node
    if (child->_parent) {
        FC_WARN("RenderGroup::addChild: Node already has a parent");
        return;
    }

    _children.push_back(child);
    child->setParent(this);
    touch();
}

void RenderGroup::insertChild(size_t index, RenderNode::Ptr child)
{
    if (!child) {
        return;
    }

    if (child->_parent) {
        FC_WARN("RenderGroup::insertChild: Node already has a parent");
        return;
    }

    if (index >= _children.size()) {
        _children.push_back(child);
    } else {
        _children.insert(_children.begin() + index, child);
    }
    child->setParent(this);
    touch();
}

bool RenderGroup::removeChild(RenderNode* child)
{
    if (!child) {
        return false;
    }

    for (auto it = _children.begin(); it != _children.end(); ++it) {
        if (it->get() == child) {
            child->setParent(nullptr);
            _children.erase(it);
            touch();
            return true;
        }
    }
    return false;
}

void RenderGroup::removeAllChildren()
{
    for (auto& child : _children) {
        child->setParent(nullptr);
    }
    _children.clear();
    touch();
}

bool RenderGroup::replaceChild(RenderNode* oldChild, RenderNode::Ptr newChild)
{
    if (!oldChild || !newChild) {
        return false;
    }

    for (size_t i = 0; i < _children.size(); ++i) {
        if (_children[i].get() == oldChild) {
            oldChild->setParent(nullptr);
            _children[i] = newChild;
            newChild->setParent(this);
            touch();
            return true;
        }
    }
    return false;
}

RenderNode* RenderGroup::getChild(size_t index) const
{
    if (index >= _children.size()) {
        return nullptr;
    }
    return _children[index].get();
}

RenderNode::Ptr RenderGroup::getChildPtr(size_t index) const
{
    if (index >= _children.size()) {
        return nullptr;
    }
    return _children[index];
}

RenderNode* RenderGroup::findChild(const std::string& name) const
{
    for (const auto& child : _children) {
        if (child->getName() == name) {
            return child.get();
        }
    }
    return nullptr;
}

RenderNode* RenderGroup::findChildRecursive(const std::string& name) const
{
    for (const auto& child : _children) {
        if (child->getName() == name) {
            return child.get();
        }
        // 递归搜索子组 / Recursively search child groups
        if (auto* group = child->cast<RenderGroup>()) {
            if (auto* found = group->findChildRecursive(name)) {
                return found;
            }
        }
    }
    return nullptr;
}

int RenderGroup::findChildIndex(const RenderNode* child) const
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

bool RenderGroup::hasChild(const RenderNode* child) const
{
    return findChildIndex(child) >= 0;
}

BoundingBox RenderGroup::getBoundingBox() const
{
    BoundingBox result;
    bool first = true;

    for (const auto& child : _children) {
        if (child->isVisible()) {
            BoundingBox childBox = child->getBoundingBox();
            if (childBox.IsValid()) {
                if (first) {
                    result = childBox;
                    first = false;
                } else {
                    result.Add(childBox);
                }
            }
        }
    }

    return result;
}

RenderNode::Ptr RenderGroup::clone() const
{
    auto newGroup = std::make_shared<RenderGroup>();
    newGroup->setName(_name);

    for (const auto& child : _children) {
        if (auto childClone = child->clone()) {
            newGroup->addChild(childClone);
        }
    }

    return newGroup;
}

//===========================================================================
// RenderSeparator
//===========================================================================

RenderSeparator::RenderSeparator()
    : RenderGroup()
{
    _nodeType = NodeType::Separator;
}

RenderSeparator::~RenderSeparator() = default;

RenderNode::Ptr RenderSeparator::clone() const
{
    auto newSeparator = std::make_shared<RenderSeparator>();
    newSeparator->setName(_name);

    for (const auto& child : _children) {
        if (auto childClone = child->clone()) {
            newSeparator->addChild(childClone);
        }
    }

    return newSeparator;
}

} // namespace Render
} // namespace Gui
