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

#ifndef GUI_RENDER_RENDERENGINE_H
#define GUI_RENDER_RENDERENGINE_H

#include <memory>
#include <string>
#include <functional>
#include <vector>

#include <FCGlobal.h>
#include "RenderTypes.h"
#include "RenderNode.h"

namespace Gui {
namespace Render {

// 前向声明 / Forward declarations
class RenderViewer;
class RenderAction;

/**
 * @brief 渲染引擎接口 / Rendering engine interface
 *
 * 定义了所有渲染后端必须实现的核心接口。
 * Defines the core interface that all rendering backends must implement.
 *
 * 设计原则 / Design Principles:
 * - 接口稳定性：新功能通过扩展方法添加，不修改现有接口
 * - 后端无关性：FreeCAD 核心代码不依赖具体后端实现
 * - 性能优先：最小化虚函数开销
 * - 线程安全：引擎实例可以在多线程环境下使用
 */
class GuiExport RenderEngine {
public:
    using Ptr = std::shared_ptr<RenderEngine>;
    using WeakPtr = std::weak_ptr<RenderEngine>;

    /**
     * @brief 虚析构函数 / Virtual destructor
     */
    virtual ~RenderEngine() = default;

    //-----------------------------------------------------------------------
    // 引擎信息 / Engine Information
    //-----------------------------------------------------------------------

    /**
     * @brief 获取引擎类型 / Get engine type
     */
    virtual BackendType getType() const = 0;

    /**
     * @brief 获取引擎名称 / Get engine name
     */
    virtual std::string getName() const = 0;

    /**
     * @brief 获取引擎版本 / Get engine version
     */
    virtual std::string getVersion() const = 0;

    /**
     * @brief 获取引擎信息 / Get engine information
     */
    virtual BackendInfo getInfo() const = 0;

    /**
     * @brief 检查引擎是否可用 / Check if engine is available
     */
    virtual bool isAvailable() const = 0;

    /**
     * @brief 初始化引擎 / Initialize engine
     *
     * 在首次使用引擎前调用，执行必要的初始化操作。
     * Called before first use of the engine to perform necessary initialization.
     *
     * @return true 初始化成功 / Initialization successful
     * @return false 初始化失败 / Initialization failed
     */
    virtual bool initialize() = 0;

    /**
     * @brief 关闭引擎 / Shutdown engine
     *
     * 释放引擎资源，在不再使用引擎时调用。
     * Release engine resources, called when the engine is no longer needed.
     */
    virtual void shutdown() = 0;

    //-----------------------------------------------------------------------
    // 节点创建 / Node Creation
    //-----------------------------------------------------------------------

    /**
     * @brief 创建分组节点 / Create group node
     */
    virtual RenderGroup::Ptr createGroup() = 0;

    /**
     * @brief 创建分隔节点 / Create separator node
     */
    virtual RenderSeparator::Ptr createSeparator() = 0;

    /**
     * @brief 创建变换节点 / Create transform node
     */
    virtual RenderNode::Ptr createTransform() = 0;

    /**
     * @brief 创建切换节点 / Create switch node
     */
    virtual RenderNode::Ptr createSwitch() = 0;

    /**
     * @brief 创建材质节点 / Create material node
     */
    virtual RenderNode::Ptr createMaterial() = 0;

    /**
     * @brief 创建几何节点 / Create geometry node
     */
    virtual RenderNode::Ptr createGeometry() = 0;

    /**
     * @brief 创建相机节点 / Create camera node
     */
    virtual RenderNode::Ptr createCamera() = 0;

    /**
     * @brief 创建灯光节点 / Create light node
     */
    virtual RenderNode::Ptr createLight(LightType type) = 0;

    //-----------------------------------------------------------------------
    // 场景管理 / Scene Management
    //-----------------------------------------------------------------------

    /**
     * @brief 设置场景根节点 / Set scene root node
     */
    virtual void setSceneRoot(RenderNode::Ptr root) = 0;

    /**
     * @brief 获取场景根节点 / Get scene root node
     */
    virtual RenderNode::Ptr getSceneRoot() const = 0;

    /**
     * @brief 更新场景 / Update scene
     *
     * 在场景图发生变化后调用，触发必要的更新。
     * Called after scene graph changes to trigger necessary updates.
     */
    virtual void updateScene() = 0;

    //-----------------------------------------------------------------------
    // 渲染控制 / Rendering Control
    //-----------------------------------------------------------------------

    /**
     * @brief 渲染帧 / Render a frame
     *
     * 执行一帧的渲染操作。
     * Perform rendering for a single frame.
     */
    virtual void render() = 0;

    /**
     * @brief 设置渲染模式 / Set render mode
     */
    virtual void setRenderMode(RenderMode mode) = 0;

    /**
     * @brief 获取渲染模式 / Get render mode
     */
    virtual RenderMode getRenderMode() const = 0;

    /**
     * @brief 设置背景色 / Set background color
     */
    virtual void setBackgroundColor(const Color& color) = 0;

    /**
     * @brief 获取背景色 / Get background color
     */
    virtual Color getBackgroundColor() const = 0;

    /**
     * @brief 获取渲染统计信息 / Get rendering statistics
     */
    virtual RenderStats getStats() const = 0;

    /**
     * @brief 重置统计信息 / Reset statistics
     */
    virtual void resetStats() = 0;

    //-----------------------------------------------------------------------
    // 资源管理 / Resource Management
    //-----------------------------------------------------------------------

    /**
     * @brief 释放未使用的资源 / Release unused resources
     */
    virtual void releaseUnusedResources() = 0;

    /**
     * @brief 获取显存使用量 / Get GPU memory usage (bytes)
     */
    virtual size_t getMemoryUsage() const = 0;

    //-----------------------------------------------------------------------
    // 兼容性接口 / Compatibility Interface
    //-----------------------------------------------------------------------

    /**
     * @brief 获取原生后端指针 / Get native backend pointer
     *
     * 用于与现有 Coin3D 代码兼容。
     * For compatibility with existing Coin3D code.
     */
    virtual void* getNativePointer() const = 0;

    /**
     * @brief 设置事件回调 / Set event callback
     */
    using EventCallback = std::function<void(const std::string& event, void* data)>;
    virtual void setEventCallback(EventCallback callback) = 0;
};

//-------------------------------------------------------------------------
// 渲染引擎工厂 / Render Engine Factory
//-------------------------------------------------------------------------

/**
 * @brief 渲染引擎工厂 / Render engine factory
 *
 * 单例工厂类，负责创建和管理渲染引擎实例。
 * Singleton factory class responsible for creating and managing
 * render engine instances.
 *
 * 设计模式 / Design Pattern:
 * - 工厂模式：封装引擎创建逻辑
 * - 单例模式：全局唯一的工厂实例
 * - 注册模式：动态注册新的后端类型
 */
class GuiExport RenderEngineFactory {
public:
    /**
     * @brief 获取工厂单例 / Get factory singleton
     */
    static RenderEngineFactory& instance();

    /**
     * @brief 引擎创建函数类型 / Engine creation function type
     */
    using EngineCreator = std::function<RenderEngine::Ptr()>;

    /**
     * @brief 注册引擎类型 / Register engine type
     *
     * @param type 后端类型 / Backend type
     * @param creator 创建函数 / Creation function
     * @return true 注册成功 / Registration successful
     * @return false 注册失败 / Registration failed
     */
    bool registerEngine(BackendType type, EngineCreator creator);

    /**
     * @brief 取消注册引擎类型 / Unregister engine type
     */
    bool unregisterEngine(BackendType type);

    /**
     * @brief 检查引擎类型是否已注册 / Check if engine type is registered
     */
    bool isEngineRegistered(BackendType type) const;

    /**
     * @brief 创建引擎实例 / Create engine instance
     *
     * @param type 后端类型 / Backend type
     * @return 引擎指针，失败返回 nullptr / Engine pointer, nullptr on failure
     */
    RenderEngine::Ptr create(BackendType type);

    /**
     * @brief 创建默认引擎 / Create default engine
     *
     * 通常是 Coin3D 引擎，因为它是最稳定的后端。
     * Usually the Coin3D engine as it's the most stable backend.
     */
    RenderEngine::Ptr createDefault();

    /**
     * @brief 获取所有已注册的引擎类型 / Get all registered engine types
     */
    std::vector<BackendType> getRegisteredTypes() const;

    /**
     * @brief 获取引擎信息 / Get engine information
     */
    BackendInfo getEngineInfo(BackendType type) const;

    /**
     * @brief 设置默认引擎类型 / Set default engine type
     */
    void setDefaultType(BackendType type) { _defaultType = type; }

    /**
     * @brief 获取默认引擎类型 / Get default engine type
     */
    BackendType getDefaultType() const { return _defaultType; }

    /**
     * @brief 自动选择最佳引擎 / Auto-select best available engine
     *
     * 根据当前环境和硬件配置选择最合适的引擎。
     * Select the most suitable engine based on current environment.
     */
    BackendType selectBestEngine() const;

    /**
     * @brief 析构函数 / Destructor
     */
    ~RenderEngineFactory() = default;

private:
    RenderEngineFactory();

    // 禁止拷贝和移动 / Disable copy and move
    FC_DISABLE_COPY_MOVE(RenderEngineFactory)

    // 引擎创建器映射 / Engine creator map
    std::unordered_map<BackendType, EngineCreator> _creators;

    // 默认引擎类型 / Default engine type
    BackendType _defaultType{BackendType::Coin3D};
};

//-------------------------------------------------------------------------
// 引擎注册助手 / Engine Registration Helper
//-------------------------------------------------------------------------

/**
 * @brief 引擎自动注册助手 / Engine auto-registration helper
 *
 * RAII 类，用于在全局构造时自动注册引擎类型。
 * RAII class for automatic engine type registration during global construction.
 *
 * 使用示例 / Usage Example:
 * @code
 * namespace MyBackend {
 *     static RenderEngineRegistration<MyEngine> registration(BackendType::Custom);
 * }
 * @endcode
 */
template<typename EngineType>
class RenderEngineRegistration {
public:
    explicit RenderEngineRegistration(BackendType type)
    {
        RenderEngineFactory::instance().registerEngine(
            type,
            []() -> RenderEngine::Ptr {
                return std::make_shared<EngineType>();
            }
        );
    }
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_RENDERENGINE_H
