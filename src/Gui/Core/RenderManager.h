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

#ifndef GUI_CORE_RENDERMANAGER_H
#define GUI_CORE_RENDERMANAGER_H

#include <memory>
#include <string>
#include <functional>

#include <FCGlobal.h>
#include "../Render/Core/RenderTypes.h"
#include "../Render/Core/RenderEngine.h"

namespace Gui {
namespace Core {

/**
 * @brief 渲染管理器 / Rendering manager
 *
 * 管理运行时后端切换和渲染引擎生命周期。
 * Manages runtime backend switching and render engine lifecycle.
 *
 * 功能 / Features:
 * - 运行时切换渲染后端
 * - 管理当前活动的引擎
 * - 提供全局渲染设置
 * - 处理引擎初始化和清理
 *
 * 设计模式 / Design Patterns:
 * - 单例模式：全局唯一的管理器
 * - 观察者模式：后端切换事件通知
 */
class GuiExport RenderManager {
public:
    /**
     * @brief 获取管理器单例 / Get manager singleton
     */
    static RenderManager& instance();

    //-----------------------------------------------------------------------
    // 后端管理 / Backend Management
    //-----------------------------------------------------------------------

    /**
     * @brief 切换渲染后端 / Switch rendering backend
     *
     * @param type 目标后端类型 / Target backend type
     * @param force 是否强制切换（即使相同）/ Whether to force switch
     * @return 是否成功 / Whether successful
     */
    bool switchBackend(Render::BackendType type, bool force = false);

    /**
     * @brief 获取当前后端类型 / Get current backend type
     */
    Render::BackendType getCurrentBackend() const { return _currentBackend; }

    /**
     * @brief 检查后端是否可用 / Check if backend is available
     */
    bool isBackendAvailable(Render::BackendType type) const;

    /**
     * @brief 获取当前引擎 / Get current engine
     */
    Render::RenderEngine::Ptr getCurrentEngine() const { return _currentEngine; }

    /**
     * @brief 获取指定后端的引擎 / Get engine for specific backend
     *
     * 如果引擎未创建，会创建新实例。
     * Creates new instance if not already created.
     */
    Render::RenderEngine::Ptr getEngine(Render::BackendType type);

    //-----------------------------------------------------------------------
    // 引擎注册 / Engine Registration
    //-----------------------------------------------------------------------

    /**
     * @brief 注册引擎创建器 / Register engine creator
     *
     * @param type 后端类型 / Backend type
     * @param creator 创建函数 / Creation function
     */
    using EngineCreator = std::function<Render::RenderEngine::Ptr()>;
    void registerEngine(Render::BackendType type, EngineCreator creator);

    /**
     * @brief 取消注册引擎创建器 / Unregister engine creator
     */
    void unregisterEngine(Render::BackendType type);

    //-----------------------------------------------------------------------
    // 全局渲染设置 / Global Render Settings
    //-----------------------------------------------------------------------

    /**
     * @brief 设置全局渲染模式 / Set global render mode
     */
    void setRenderMode(Render::RenderMode mode);

    /**
     * @brief 获取全局渲染模式 / Get global render mode
     */
    Render::RenderMode getRenderMode() const { return _globalRenderMode; }

    /**
     * @brief 设置背景色 / Set background color
     */
    void setBackgroundColor(const Render::Color& color);

    /**
     * @brief 获取背景色 / Get background color
     */
    Render::Color getBackgroundColor() const { return _backgroundColor; }

    //-----------------------------------------------------------------------
    // 事件处理 / Event Handling
    //-----------------------------------------------------------------------

    /**
     * @brief 后端切换事件回调 / Backend switch event callback
     */
    using BackendSwitchCallback = std::function<void(Render::BackendType oldBackend, Render::BackendType newBackend)>;

    /**
     * @brief 注册后端切换回调 / Register backend switch callback
     */
    void addBackendSwitchCallback(BackendSwitchCallback callback);

    /**
     * @brief 触发后端切换事件 / Fire backend switch event
     */
    void fireBackendSwitchEvent(Render::BackendType oldBackend, Render::BackendType newBackend);

    //-----------------------------------------------------------------------
    // 状态查询 / Status Query
    //-----------------------------------------------------------------------

    /**
     * @brief 检查是否已初始化 / Check if initialized
     */
    bool isInitialized() const { return _initialized; }

    /**
     * @brief 获取渲染统计信息 / Get render statistics
     */
    Render::RenderStats getStats() const;

    /**
     * @brief 重置统计信息 / Reset statistics
     */
    void resetStats();

    /**
     * @brief 获取渲染器信息字符串 / Get renderer info string
     */
    std::string getRendererInfo() const;

    //-----------------------------------------------------------------------
    // 初始化和清理 / Initialization and Cleanup
    //-----------------------------------------------------------------------

    /**
     * @brief 初始化渲染管理器 / Initialize render manager
     *
     * 在应用程序启动时调用。
     * Called at application startup.
     */
    bool initialize();

    /**
     * @brief 关闭渲染管理器 / Shutdown render manager
     *
     * 在应用程序关闭时调用。
     * Called at application shutdown.
     */
    void shutdown();

    // 禁止拷贝和移动 / Disable copy and move
    FC_DISABLE_COPY_MOVE(RenderManager)

private:
    RenderManager();
    ~RenderManager();

    // 当前状态 / Current state
    Render::BackendType _currentBackend{Render::BackendType::None};
    Render::RenderEngine::Ptr _currentEngine;
    bool _initialized{false};

    // 全局设置 / Global settings
    Render::RenderMode _globalRenderMode{Render::RenderMode::Default};
    Render::Color _backgroundColor{0.2f, 0.2f, 0.3f, 1.0f};

    // 引擎创建器映射 / Engine creator map
    std::unordered_map<Render::BackendType, EngineCreator> _engineCreators;

    // 事件回调列表 / Event callback list
    std::vector<BackendSwitchCallback> _backendSwitchCallbacks;
};

//-------------------------------------------------------------------------
// 辅助函数 / Helper Functions
//-------------------------------------------------------------------------

/**
 * @brief 获取当前渲染引擎（便捷函数）/ Get current render engine (convenience function)
 */
inline Render::RenderEngine::Ptr getCurrentRenderEngine()
{
    return RenderManager::instance().getCurrentEngine();
}

/**
 * @brief 切换渲染后端（便捷函数）/ Switch render backend (convenience function)
 */
inline bool switchRenderBackend(Render::BackendType type, bool force = false)
{
    return RenderManager::instance().switchBackend(type, force);
}

/**
 * @brief 获取当前后端类型（便捷函数）/ Get current backend type (convenience function)
 */
inline Render::BackendType getCurrentRenderBackend()
{
    return RenderManager::instance().getCurrentBackend();
}

} // namespace Core
} // namespace Gui

#endif // GUI_CORE_RENDERMANAGER_H
