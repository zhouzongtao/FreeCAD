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

#include "RenderEngine.h"
#include <Base/Console.h>
#include <mutex>

FC_LOG_LEVEL_INIT("Render")

namespace Gui {
namespace Render {

//===========================================================================
// RenderEngineFactory Implementation
//===========================================================================

namespace {
    // 工厂单例实例 / Factory singleton instance
    std::unique_ptr<RenderEngineFactory> g_factoryInstance;
    std::mutex g_factoryMutex;
}

RenderEngineFactory& RenderEngineFactory::instance()
{
    std::lock_guard<std::mutex> lock(g_factoryMutex);

    if (!g_factoryInstance) {
        g_factoryInstance = std::unique_ptr<RenderEngineFactory>(new RenderEngineFactory());
    }

    return *g_factoryInstance;
}

RenderEngineFactory::RenderEngineFactory()
{
    // 默认注册 Coin3D 引擎 / Register Coin3D engine by default
    // 注意：实际注册在 Coin3DEngine.cpp 中完成
    // Note: Actual registration is done in Coin3DEngine.cpp
}

bool RenderEngineFactory::registerEngine(BackendType type, EngineCreator creator)
{
    if (!creator) {
        FC_WARN("RenderEngineFactory: Cannot register null creator for backend type " << static_cast<int>(type));
        return false;
    }

    if (_creators.find(type) != _creators.end()) {
        FC_WARN("RenderEngineFactory: Engine type " << static_cast<int>(type) << " already registered, overriding");
    }

    _creators[type] = std::move(creator);
    FC_LOG("RenderEngineFactory: Registered engine type " << static_cast<int>(type));
    return true;
}

bool RenderEngineFactory::unregisterEngine(BackendType type)
{
    auto it = _creators.find(type);
    if (it == _creators.end()) {
        return false;
    }

    _creators.erase(it);
    FC_LOG("RenderEngineFactory: Unregistered engine type " << static_cast<int>(type));
    return true;
}

bool RenderEngineFactory::isEngineRegistered(BackendType type) const
{
    return _creators.find(type) != _creators.end();
}

RenderEngine::Ptr RenderEngineFactory::create(BackendType type)
{
    auto it = _creators.find(type);
    if (it == _creators.end()) {
        FC_ERR("RenderEngineFactory: No creator registered for backend type " << static_cast<int>(type));
        return nullptr;
    }

    try {
        auto engine = it->second();
        if (engine) {
            FC_LOG("RenderEngineFactory: Created engine of type " << static_cast<int>(type));
        }
        return engine;
    }
    catch (const std::exception& e) {
        FC_ERR("RenderEngineFactory: Exception while creating engine type " << static_cast<int>(type) << ": " << e.what());
        return nullptr;
    }
}

RenderEngine::Ptr RenderEngineFactory::createDefault()
{
    // 使用 selectBestEngine 自动选择最佳引擎
    // Use selectBestEngine to automatically select the best engine
    BackendType bestType = selectBestEngine();
    
    if (bestType != BackendType::None) {
        auto engine = create(bestType);
        if (engine) {
            Base::Console().log("RenderEngineFactory: Using best engine: %d\n", static_cast<int>(bestType));
            return engine;
        }
    }

    // 如果自动选择失败，尝试默认类型
    // If auto-selection fails, try default type
    auto engine = create(_defaultType);
    if (engine) {
        return engine;
    }

    // 如果默认类型失败，尝试其他已注册的引擎
    // If default type fails, try other registered engines
    for (const auto& [type, creator] : _creators) {
        if (type != _defaultType && type != bestType) {
            engine = creator();
            if (engine) {
                FC_WARN("RenderEngineFactory: Default engine unavailable, using type " << static_cast<int>(type));
                return engine;
            }
        }
    }

    FC_ERR("RenderEngineFactory: No available engine found");
    return nullptr;
}

std::vector<BackendType> RenderEngineFactory::getRegisteredTypes() const
{
    std::vector<BackendType> types;
    types.reserve(_creators.size());

    for (const auto& [type, creator] : _creators) {
        types.push_back(type);
    }

    return types;
}

BackendInfo RenderEngineFactory::getEngineInfo(BackendType type) const
{
    BackendInfo info;
    info.type = type;

    // 尝试创建临时引擎实例以获取信息
    // Try to create temporary engine instance to get information
    auto it = _creators.find(type);
    if (it != _creators.end()) {
        try {
            if (auto engine = it->second()) {
                info = engine->getInfo();
            }
        }
        catch (...) {
            // 忽略异常，使用默认信息 / Ignore exception, use default info
        }
    }

    // 设置默认名称 / Set default name
    if (info.name.empty()) {
        switch (type) {
            case BackendType::Coin3D:
                info.name = "Coin3D";
                info.description = "Open Inventor compatible rendering engine";
                break;
            case BackendType::OsgVerse:
                info.name = "OsgVerse";
                info.description = "OpenSceneGraph based PBR rendering engine";
                break;
            default:
                info.name = "Unknown";
                info.description = "Unknown rendering backend";
                break;
        }
    }

    return info;
}

BackendType RenderEngineFactory::selectBestEngine() const
{
    // 优先级顺序 / Priority order:
    // 1. Coin3D（稳定的默认后端）
    // 2. OsgVerse（实验性后端，需手动切换）
    // 3. None（无可用引擎）

    Base::Console().log("RenderEngineFactory::selectBestEngine: Selecting best engine...\n");

    // 优先使用 Coin3D / Prefer Coin3D (stable default)
    if (_creators.find(BackendType::Coin3D) != _creators.end()) {
        Base::Console().log("RenderEngineFactory::selectBestEngine: Using Coin3D as default\n");
        return BackendType::Coin3D;
    }

    // 检查 OsgVerse 是否可用 / Check if OsgVerse is available
    auto osgIt = _creators.find(BackendType::OsgVerse);
    if (osgIt != _creators.end()) {
        Base::Console().log("RenderEngineFactory::selectBestEngine: Coin3D not available, using OsgVerse\n");
        return BackendType::OsgVerse;
    }

    Base::Console().warning("RenderEngineFactory::selectBestEngine: No engines available!\n");
    return BackendType::None;
}

} // namespace Render
} // namespace Gui
