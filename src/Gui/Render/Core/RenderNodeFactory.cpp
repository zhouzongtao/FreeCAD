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
#include "RenderNodeFactory.h"

#include <Base/Console.h>

FC_LOG_LEVEL_INIT("RenderNodeFactory", true, true)

namespace Gui {
namespace Render {

//=============================================================================
// RenderNodeFactory 默认实现 / RenderNodeFactory Default Implementations
//=============================================================================

std::shared_ptr<RenderNode> RenderNodeFactory::createViewProviderRoot(
    std::shared_ptr<RenderNode>* transform,
    std::shared_ptr<RenderNode>* modeSwitch)
{
    // 创建根节点 / Create root node
    auto root = createSeparator();
    if (!root) {
        FC_ERR("RenderNodeFactory: Failed to create root separator");
        return nullptr;
    }

    // 尝试将 root 转换为 RenderGroup 以添加子节点
    // Try to cast root to RenderGroup to add children
    auto* rootGroup = dynamic_cast<RenderGroup*>(root.get());
    if (!rootGroup) {
        FC_WARN("RenderNodeFactory: Root node does not support addChild");
        return root;
    }

    // 创建变换节点 / Create transform node
    auto transformNode = createTransform();
    if (transformNode) {
        rootGroup->addChild(transformNode);
        if (transform) {
            *transform = transformNode;
        }
    }

    // 创建模式切换节点 / Create mode switch node
    auto switchNode = createSwitch();
    if (switchNode) {
        rootGroup->addChild(switchNode);
        if (modeSwitch) {
            *modeSwitch = switchNode;
        }
    }

    return root;
}

std::shared_ptr<RenderNode> RenderNodeFactory::createMaterialGroup(const Material& material)
{
    auto group = createSeparator();
    if (!group) {
        return nullptr;
    }

    // 尝试将 group 转换为 RenderGroup 以添加子节点
    // Try to cast group to RenderGroup to add children
    auto* groupPtr = dynamic_cast<RenderGroup*>(group.get());
    if (!groupPtr) {
        return group;
    }

    // 创建材质节点 / Create material node
    auto matNode = createMaterial();
    if (matNode) {
        // 材质设置将由派生类的具体实现处理
        // Material setup will be handled by derived class implementations
        groupPtr->addChild(matNode);
    }

    return group;
}

//=============================================================================
// RenderNodeFactoryRegistry 实现 / RenderNodeFactoryRegistry Implementation
//=============================================================================

RenderNodeFactoryRegistry& RenderNodeFactoryRegistry::instance()
{
    static RenderNodeFactoryRegistry instance;
    return instance;
}

void RenderNodeFactoryRegistry::registerFactory(BackendType type, RenderNodeFactory::Ptr factory)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_factories.find(type) != _factories.end()) {
        FC_WARN("RenderNodeFactoryRegistry: Replacing existing factory for backend "
                << static_cast<int>(type));
    }

    _factories[type] = std::move(factory);

    FC_LOG("RenderNodeFactoryRegistry: Registered factory for backend "
           << static_cast<int>(type) << " (" << _factories[type]->getBackendName() << ")");
}

void RenderNodeFactoryRegistry::unregisterFactory(BackendType type)
{
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _factories.find(type);
    if (it != _factories.end()) {
        FC_LOG("RenderNodeFactoryRegistry: Unregistered factory for backend "
               << static_cast<int>(type));
        _factories.erase(it);
    }
}

RenderNodeFactory::Ptr RenderNodeFactoryRegistry::getFactory(BackendType type) const
{
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _factories.find(type);
    if (it != _factories.end()) {
        return it->second;
    }

    return nullptr;
}

RenderNodeFactory::Ptr RenderNodeFactoryRegistry::getDefaultFactory() const
{
    return getFactory(_defaultBackend);
}

void RenderNodeFactoryRegistry::setDefaultBackend(BackendType type)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_factories.find(type) == _factories.end()) {
        FC_WARN("RenderNodeFactoryRegistry: Setting default to unregistered backend "
                << static_cast<int>(type));
    }

    _defaultBackend = type;
    FC_LOG("RenderNodeFactoryRegistry: Default backend set to " << static_cast<int>(type));
}

bool RenderNodeFactoryRegistry::hasFactory(BackendType type) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _factories.find(type) != _factories.end();
}

std::vector<BackendType> RenderNodeFactoryRegistry::getRegisteredBackends() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    std::vector<BackendType> backends;
    backends.reserve(_factories.size());

    for (const auto& pair : _factories) {
        backends.push_back(pair.first);
    }

    return backends;
}

BackendInfo RenderNodeFactoryRegistry::getBackendInfo(BackendType type) const
{
    std::lock_guard<std::mutex> lock(_mutex);

    BackendInfo info;
    info.type = type;

    auto it = _factories.find(type);
    if (it != _factories.end()) {
        info.name = it->second->getBackendName();
        // 其他信息由具体后端填充 / Other info filled by specific backends
    }

    return info;
}

} // namespace Render
} // namespace Gui
