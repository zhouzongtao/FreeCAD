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

#ifndef GUI_RENDER_BACKENDS_COIN3D_COIN3DENGINE_H
#define GUI_RENDER_BACKENDS_COIN3D_COIN3DENGINE_H

#include <memory>
#include <string>

#include <FCGlobal.h>
#include "../../Core/RenderEngine.h"
#include "../../Core/RenderNode.h"

// Coin3D / Open Inventor 前向声明 / Forward declarations
class SoNode;
class SoGroup;
class SoSeparator;
class SoTransform;
class SoSwitch;
class SoMaterial;
class SoCamera;
class SoLight;

namespace Gui {
namespace Render {

/**
 * @brief Coin3D 渲染引擎 / Coin3D rendering engine
 *
 * Coin3D (Open Inventor) 渲染后端的包装器类。
 * Wrapper class for the Coin3D (Open Inventor) rendering backend.
 *
 * 设计目标 / Design Goals:
 * - 与现有 FreeCAD Coin3D 代码完全兼容
 * - 提供抽象接口，支持未来切换到其他后端
 * - 最小化包装开销，接近原生性能
 * - 支持渐进式迁移，新旧代码可共存
 *
 * 兼容性策略 / Compatibility Strategy:
 * - 保留对 SoNode 的直接访问能力
 * - 提供 Coin3DNode 与 RenderNode 的双向转换
 * - 支持现有 ViewProvider 代码无需修改即可工作
 */
class GuiExport Coin3DEngine : public RenderEngine {
public:
    /**
     * @brief 构造函数 / Constructor
     */
    Coin3DEngine();

    /**
     * @brief 析构函数 / Destructor
     */
    ~Coin3DEngine() override;

    //-----------------------------------------------------------------------
    // 引擎信息 / Engine Information
    //-----------------------------------------------------------------------

    BackendType getType() const override { return BackendType::Coin3D; }
    std::string getName() const override;
    std::string getVersion() const override;
    BackendInfo getInfo() const override;
    bool isAvailable() const override;

    bool initialize() override;
    void shutdown() override;

    //-----------------------------------------------------------------------
    // 节点创建 / Node Creation
    //-----------------------------------------------------------------------

    RenderGroup::Ptr createGroup() override;
    RenderSeparator::Ptr createSeparator() override;
    RenderNode::Ptr createTransform() override;
    RenderNode::Ptr createSwitch() override;
    RenderNode::Ptr createMaterial() override;
    RenderNode::Ptr createGeometry() override;
    RenderNode::Ptr createCamera() override;
    RenderNode::Ptr createLight(LightType type) override;

    //-----------------------------------------------------------------------
    // 场景管理 / Scene Management
    //-----------------------------------------------------------------------

    void setSceneRoot(RenderNode::Ptr root) override;
    RenderNode::Ptr getSceneRoot() const override;
    void updateScene() override;

    //-----------------------------------------------------------------------
    // 渲染控制 / Rendering Control
    //-----------------------------------------------------------------------

    void render() override;
    void setRenderMode(RenderMode mode) override;
    RenderMode getRenderMode() const override;
    void setBackgroundColor(const Color& color) override;
    Color getBackgroundColor() const override;
    RenderStats getStats() const override;
    void resetStats() override;

    //-----------------------------------------------------------------------
    // 资源管理 / Resource Management
    //-----------------------------------------------------------------------

    void releaseUnusedResources() override;
    size_t getMemoryUsage() const override;

    //-----------------------------------------------------------------------
    // 兼容性接口 / Compatibility Interface
    //-----------------------------------------------------------------------

    /**
     * @brief 获取原生 Coin3D 指针 / Get native Coin3D pointer
     *
     * 返回底层 SoSceneManager 或 SoGLRenderAction
     * Returns underlying SoSceneManager or SoGLRenderAction
     */
    void* getNativePointer() const override { return _nativePointer; }

    void setEventCallback(EventCallback callback) override { _eventCallback = std::move(callback); }

    //-----------------------------------------------------------------------
    // Coin3D 特定接口 / Coin3D-specific Interface
    //-----------------------------------------------------------------------

    /**
     * @brief 从 SoNode 创建包装的 RenderNode / Create wrapped RenderNode from SoNode
     *
     * 将现有的 Coin3D 节点包装到抽象接口中。
     * Wraps an existing Coin3D node into the abstract interface.
     *
     * @param coinNode Coin3D 节点指针 / Coin3D node pointer
     * @param takeOwnership 是否获取所有权 / Whether to take ownership
     * @return 包装后的 RenderNode / Wrapped RenderNode
     */
    RenderNode::Ptr wrapNode(SoNode* coinNode, bool takeOwnership = false);

    /**
     * @brief 从 RenderNode 提取 SoNode / Extract SoNode from RenderNode
     *
     * 获取包装器内的底层 Coin3D 节点。
     * Get the underlying Coin3D node inside the wrapper.
     *
     * @param renderNode 抽象渲染节点 / Abstract render node
     * @return Coin3D 节点指针，失败返回 nullptr / Coin3D node pointer, nullptr on failure
     */
    SoNode* unwrapNode(RenderNode* renderNode) const;

    /**
     * @brief 设置 Coin3D 场景管理器 / Set Coin3D scene manager
     *
     * 用于与现有 View3DInventorViewer 集成。
     * For integration with existing View3DInventorViewer.
     */
    void setSceneManager(void* sceneManager);

    /**
     * @brief 获取 Coin3D 场景管理器 / Get Coin3D scene manager
     */
    void* getSceneManager() const { return _sceneManager; }

    /**
     * @brief 设置渲染动作 / Set render action
     */
    void setRenderAction(void* renderAction);

    /**
     * @brief 获取渲染动作 / Get render action
     */
    void* getRenderAction() const { return _renderAction; }

private:
    // Coin3D 内部状态 / Coin3D internal state
    void* _nativePointer{nullptr};      ///< 原生指针（SoSceneManager 等）/ Native pointer
    void* _sceneManager{nullptr};       ///< SoSceneManager 指针 / SoSceneManager pointer
    void* _renderAction{nullptr};       ///< SoGLRenderAction 指针 / SoGLRenderAction pointer

    // 渲染状态 / Rendering state
    RenderMode _renderMode{RenderMode::Default};
    Color _backgroundColor{0.2f, 0.2f, 0.3f, 1.0f};
    RenderNode::Ptr _sceneRoot;

    // 统计信息 / Statistics
    mutable RenderStats _stats;

    // 事件回调 / Event callback
    EventCallback _eventCallback;

    // 初始化状态 / Initialization state
    bool _initialized{false};
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_COIN3D_COIN3DENGINE_H
