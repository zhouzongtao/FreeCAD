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

#include "ViewProviderRenderProxy.h"
#include "ViewProvider.h"
#include "Render/Core/RenderNode.h"
#include "Render/Backends/Coin3D/Coin3DNode.h"
#include "Core/RenderManager.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTransform.h>
#include <Base/Console.h>

namespace Gui {

//===========================================================================
// ViewProviderRenderProxy Implementation
//===========================================================================

ViewProviderRenderProxy::ViewProviderRenderProxy(ViewProvider* viewProvider)
    : _viewProvider(viewProvider)
{
    // 创建渲染根节点 / Create render root node
    _rootSeparator = std::make_shared<Render::RenderSeparator>();
    _renderRoot = _rootSeparator;

    // 根据当前后端创建相应的节点
    // Create appropriate nodes based on current backend
    using namespace Render;

    auto currentBackend = Core::RenderManager::instance().getCurrentBackend();

    if (currentBackend == BackendType::Coin3D) {
        // 使用 Coin3D 包装器 / Use Coin3D wrappers
        auto coinSep = std::make_shared<Render::Coin3DSeparator>();
        _rootSeparator = coinSep;
        _renderRoot = coinSep;

        // 创建模式切换节点 / Create mode switch node
        auto coinSwitch = std::make_shared<Render::Coin3DSwitch>();
        _modeSwitch = coinSwitch->getSoSwitch();
        if (auto* group = dynamic_cast<Render::RenderGroup*>(_rootSeparator.get())) {
            group->addChild(coinSwitch);
        }
    } else {
        // 使用通用节点 / Use generic nodes
        _rootSeparator = std::make_shared<Render::RenderSeparator>();
        _renderRoot = _rootSeparator;
    }
}

ViewProviderRenderProxy::~ViewProviderRenderProxy() = default;

//---------------------------------------------------------------------------
// 场景图根节点 / Scene Graph Root Node
//---------------------------------------------------------------------------

Render::RenderNode* ViewProviderRenderProxy::getRenderRoot() const
{
    return _renderRoot.get();
}

void ViewProviderRenderProxy::setRenderRoot(Render::RenderNodePtr root)
{
    _renderRoot = root;
    if (auto sep = std::dynamic_pointer_cast<Render::RenderSeparator>(root)) {
        _rootSeparator = sep;
    }
}

SoSeparator* ViewProviderRenderProxy::getCoinRoot() const
{
    if (auto* coinSep = dynamic_cast<Render::Coin3DSeparator*>(_rootSeparator.get())) {
        return coinSep->getSoSeparator();
    }
    return _coinRoot;
}

void ViewProviderRenderProxy::setCoinRoot(SoSeparator* root)
{
    _coinRoot = root;
    // 同时更新渲染根节点 / Also update render root node
    if (root) {
        // 包装现有 Coin3D 节点 / Wrap existing Coin3D node
        // TODO: 实现 wrapSoNode
        // _renderRoot = Render::Coin3DUtils::wrapSoNode(root, false);
    }
}

//---------------------------------------------------------------------------
// 显示模式 / Display Mode
//---------------------------------------------------------------------------

bool ViewProviderRenderProxy::setDisplayMode(const char* mode)
{
    if (!mode) {
        return false;
    }

    _currentDisplayMode = mode;

    // 查找对应的渲染节点 / Find corresponding render node
    auto it = _displayModes.find(mode);
    if (it != _displayModes.end()) {
        // TODO: 切换到对应的显示模式节点
        // TODO: Switch to corresponding display mode node
        if (_modeSwitch) {
            // 设置活动子节点 / Set active child
            // _modeSwitch->setWhichChild(childIndex);
        }
        return true;
    }

    // 如果使用 Coin3D，通知 ViewProvider
    // If using Coin3D, notify ViewProvider
    if (_viewProvider && isUsingCoin3D()) {
        // ViewProvider 可能需要处理特殊模式
        // ViewProvider may need to handle special modes
    }

    return false;
}

std::vector<std::string> ViewProviderRenderProxy::getDisplayModes() const
{
    std::vector<std::string> modes;
    modes.reserve(_displayModes.size());
    for (const auto& [mode, node] : _displayModes) {
        modes.push_back(mode);
    }
    return modes;
}

void ViewProviderRenderProxy::addDisplayMode(const std::string& mode, Render::RenderNodePtr node)
{
    _displayModes[mode] = node;
    // TODO: 添加到模式切换节点
    // TODO: Add to mode switch node
}

//---------------------------------------------------------------------------
// 可见性 / Visibility
//---------------------------------------------------------------------------

void ViewProviderRenderProxy::setVisible(bool visible)
{
    _visible = visible;
    if (_renderRoot) {
        _renderRoot->setVisible(visible);
    }
}

void ViewProviderRenderProxy::toggleVisibility()
{
    setVisible(!_visible);
}

//---------------------------------------------------------------------------
// 变换 / Transformation
//---------------------------------------------------------------------------

void ViewProviderRenderProxy::setTransform(const Base::Matrix4D& matrix)
{
    if (auto* coinTransform = dynamic_cast<Render::Coin3DTransform*>(
            _renderRoot->cast<Render::Coin3DTransform>())) {
        // coinTransform->applyTransform(matrix);
    }
    // TODO: 处理其他后端的变换
    // TODO: Handle transformation for other backends
}

Base::Matrix4D ViewProviderRenderProxy::getTransform() const
{
    // TODO: 从变换节点获取矩阵
    // TODO: Get matrix from transform node
    return Base::Matrix4D();
}

void ViewProviderRenderProxy::resetTransform()
{
    if (auto* coinTransform = dynamic_cast<Render::Coin3DTransform*>(
            _renderRoot->cast<Render::Coin3DTransform>())) {
        coinTransform->reset();
    }
}

//---------------------------------------------------------------------------
// 材质 / Material
//---------------------------------------------------------------------------

void ViewProviderRenderProxy::setMaterial(const Render::Material& material)
{
    // TODO: 设置材质节点
    // TODO: Set material node
    (void)material;
}

Render::Material ViewProviderRenderProxy::getMaterial() const
{
    // TODO: 从材质节点获取
    // TODO: Get from material node
    return Render::Material();
}

void ViewProviderRenderProxy::setMaterialStyle(const std::string& style)
{
    _materialStyle = style;
    // TODO: 应用材质样式
    // TODO: Apply material style
}

//---------------------------------------------------------------------------
// 选择状态 / Selection State
//---------------------------------------------------------------------------

void ViewProviderRenderProxy::setSelected(bool selected)
{
    _selected = selected;
    // TODO: 应用选择高亮
    // TODO: Apply selection highlight
}

void ViewProviderRenderProxy::toggleSelection()
{
    setSelected(!_selected);
}

void ViewProviderRenderProxy::setHighlightColor(const Render::Color& color)
{
    _highlightColor = color;
}

//---------------------------------------------------------------------------
// 边界框 / Bounding Box
//---------------------------------------------------------------------------

Render::BoundingBox ViewProviderRenderProxy::getBoundingBox() const
{
    if (_renderRoot) {
        return _renderRoot->getBoundingBox();
    }
    return Render::BoundingBox();
}

void ViewProviderRenderProxy::updateBoundingBox()
{
    // 触发边界框重新计算
    // Trigger bounding box recalculation
    if (_renderRoot) {
        _renderRoot->touch();
    }
}

//---------------------------------------------------------------------------
// 渲染状态 / Render State
//---------------------------------------------------------------------------

void ViewProviderRenderProxy::setRenderMode(Render::RenderMode mode)
{
    _renderMode = mode;
    // TODO: 应用渲染模式
    // TODO: Apply render mode
}

void ViewProviderRenderProxy::touch()
{
    if (_renderRoot) {
        _renderRoot->touch();
    }
}

//---------------------------------------------------------------------------
// 子节点管理 / Child Management
//---------------------------------------------------------------------------

void ViewProviderRenderProxy::addChild(Render::RenderNodePtr child)
{
    if (_rootSeparator && child) {
        if (auto* group = dynamic_cast<Render::RenderGroup*>(_rootSeparator.get())) {
            group->addChild(child);
        }
    }
}

bool ViewProviderRenderProxy::removeChild(Render::RenderNode* child)
{
    if (_rootSeparator && child) {
        if (auto* group = dynamic_cast<Render::RenderGroup*>(_rootSeparator.get())) {
            return group->removeChild(child);
        }
    }
    return false;
}

size_t ViewProviderRenderProxy::getChildCount() const
{
    if (_rootSeparator) {
        if (auto* group = dynamic_cast<Render::RenderGroup*>(_rootSeparator.get())) {
            return group->getNumChildren();
        }
    }
    return 0;
}

Render::RenderNode* ViewProviderRenderProxy::getChild(size_t index) const
{
    if (_rootSeparator) {
        if (auto* group = dynamic_cast<Render::RenderGroup*>(_rootSeparator.get())) {
            return group->getChild(index);
        }
    }
    return nullptr;
}

//---------------------------------------------------------------------------
// 兼容性方法 / Compatibility Methods
//---------------------------------------------------------------------------

bool ViewProviderRenderProxy::isUsingCoin3D() const
{
    return Core::RenderManager::instance().getCurrentBackend() == Render::BackendType::Coin3D;
}

bool ViewProviderRenderProxy::isUsingOsgVerse() const
{
    return Core::RenderManager::instance().getCurrentBackend() == Render::BackendType::OsgVerse;
}

} // namespace Gui
