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

#include "RenderManager.h"
#include <Base/Console.h>
#include <mutex>

namespace Gui {
namespace Core {

// 引入 Render 命名空间作为别名 / Import Render namespace as alias

namespace {
    // 单例实例 / Singleton instance
    RenderManager* g_instance = nullptr;
    std::mutex g_instanceMutex;
}

//===========================================================================
// RenderManager Implementation
//===========================================================================

RenderManager& RenderManager::instance()
{
    std::lock_guard<std::mutex> lock(g_instanceMutex);

    if (!g_instance) {
        g_instance = new RenderManager();
    }

    return *g_instance;
}

RenderManager::RenderManager() = default;

RenderManager::~RenderManager()
{
    shutdown();
}

//---------------------------------------------------------------------------
// 初始化和清理 / Initialization and Cleanup
//---------------------------------------------------------------------------

bool RenderManager::initialize()
{
    if (_initialized) {
        return true;
    }

    Base::Console().log("RenderManager::initialize: Initializing render manager\n");

    // 注册默认引擎（如果尚未注册）
    // Register default engines (if not already registered)
    auto& factory = Render::RenderEngineFactory::instance();

    // Coin3D 通常已经通过 Coin3DEngine.cpp 中的自动注册注册
    // Coin3D is usually already registered via auto-registration in Coin3DEngine.cpp

    // 创建默认引擎 / Create default engine
    _currentEngine = factory.createDefault();
    if (_currentEngine) {
        _currentBackend = _currentEngine->getType();
        if (_currentEngine->initialize()) {
            _initialized = true;
            Base::Console().log("RenderManager::initialize: Initialized with backend: %s\n",
                                _currentEngine->getName().c_str());
        } else {
            Base::Console().error("RenderManager::initialize: Failed to initialize default engine\n");
        }
    } else {
        Base::Console().warning("RenderManager::initialize: No default engine available\n");
        // 即使没有引擎也标记为已初始化（允许后续注册）
        // Mark as initialized even without engine (allows later registration)
        _initialized = true;
    }

    return _initialized;
}

void RenderManager::shutdown()
{
    if (!_initialized) {
        return;
    }

    Base::Console().log("RenderManager::shutdown: Shutting down render manager\n");

    // 释放当前引擎 / Release current engine
    if (_currentEngine) {
        _currentEngine->shutdown();
        _currentEngine.reset();
    }

    _currentBackend = Render::BackendType::None;
    _initialized = false;
}

//---------------------------------------------------------------------------
// 后端管理 / Backend Management
//---------------------------------------------------------------------------

bool RenderManager::switchBackend(Render::BackendType type, bool force)
{
    if (!force && type == _currentBackend) {
        return true;  // 已经是目标后端 / Already on target backend
    }

    Base::Console().log("RenderManager::switchBackend: Switching from %d to %d\n",
                        static_cast<int>(_currentBackend), static_cast<int>(type));

    // 保存旧后端 / Save old backend
    Render::BackendType oldBackend = _currentBackend;

    // 关闭当前引擎 / Shutdown current engine
    if (_currentEngine) {
        _currentEngine->shutdown();
    }

    // 创建新引擎 / Create new engine
    _currentEngine = getEngine(type);

    if (_currentEngine) {
        _currentBackend = type;

        // 初始化新引擎 / Initialize new engine
        if (!_currentEngine->initialize()) {
            Base::Console().error("RenderManager::switchBackend: Failed to initialize new engine\n");
            // 尝试回退到旧引擎 / Try to fallback to old engine
            if (oldBackend != Render::BackendType::None) {
                _currentEngine = getEngine(oldBackend);
                if (_currentEngine) {
                    _currentEngine->initialize();
                    _currentBackend = oldBackend;
                }
            }
            return false;
        }

        // 应用全局设置 / Apply global settings
        _currentEngine->setRenderMode(_globalRenderMode);
        _currentEngine->setBackgroundColor(_backgroundColor);

        // 触发切换事件 / Fire switch event
        fireBackendSwitchEvent(oldBackend, type);

        Base::Console().log("RenderManager::switchBackend: Successfully switched to %s\n",
                            _currentEngine->getName().c_str());
        return true;
    } else {
        Base::Console().error("RenderManager::switchBackend: Failed to create engine for type %d\n",
                              static_cast<int>(type));
        return false;
    }
}

bool RenderManager::isBackendAvailable(Render::BackendType type) const
{
    auto& factory = Render::RenderEngineFactory::instance();
    return factory.isEngineRegistered(type);
}

Render::RenderEngine::Ptr RenderManager::getEngine(Render::BackendType type)
{
    // 首先检查本地注册的创建器 / First check locally registered creators
    auto it = _engineCreators.find(type);
    if (it != _engineCreators.end()) {
        return it->second();
    }

    // 然后检查工厂 / Then check factory
    auto& factory = Render::RenderEngineFactory::instance();
    return factory.create(type);
}

//---------------------------------------------------------------------------
// 引擎注册 / Engine Registration
//---------------------------------------------------------------------------

void RenderManager::registerEngine(Render::BackendType type, EngineCreator creator)
{
    _engineCreators[type] = std::move(creator);
    Base::Console().log("RenderManager::registerEngine: Registered engine type %d\n",
                        static_cast<int>(type));
}

void RenderManager::unregisterEngine(Render::BackendType type)
{
    _engineCreators.erase(type);
    Base::Console().log("RenderManager::unregisterEngine: Unregistered engine type %d\n",
                        static_cast<int>(type));
}

//---------------------------------------------------------------------------
// 全局渲染设置 / Global Render Settings
//---------------------------------------------------------------------------

void RenderManager::setRenderMode(Render::RenderMode mode)
{
    _globalRenderMode = mode;
    if (_currentEngine) {
        _currentEngine->setRenderMode(mode);
    }
}

void RenderManager::setBackgroundColor(const Render::Color& color)
{
    _backgroundColor = color;
    if (_currentEngine) {
        _currentEngine->setBackgroundColor(color);
    }
}

//---------------------------------------------------------------------------
// 事件处理 / Event Handling
//---------------------------------------------------------------------------

void RenderManager::addBackendSwitchCallback(BackendSwitchCallback callback)
{
    _backendSwitchCallbacks.push_back(std::move(callback));
}

void RenderManager::fireBackendSwitchEvent(Render::BackendType oldBackend, Render::BackendType newBackend)
{
    for (const auto& callback : _backendSwitchCallbacks) {
        try {
            callback(oldBackend, newBackend);
        } catch (const std::exception& e) {
            Base::Console().error("RenderManager::fireBackendSwitchEvent: Callback exception: %s\n",
                                  e.what());
        } catch (...) {
            Base::Console().error("RenderManager::fireBackendSwitchEvent: Unknown callback exception\n");
        }
    }
}

//---------------------------------------------------------------------------
// 状态查询 / Status Query
//---------------------------------------------------------------------------

Render::RenderStats RenderManager::getStats() const
{
    if (_currentEngine) {
        return _currentEngine->getStats();
    }
    return Render::RenderStats{};
}

void RenderManager::resetStats()
{
    if (_currentEngine) {
        _currentEngine->resetStats();
    }
}

std::string RenderManager::getRendererInfo() const
{
    if (_currentEngine) {
        auto info = _currentEngine->getInfo();
        return info.name + " " + info.version;
    }
    return "No renderer";
}

} // namespace Core
} // namespace Gui
