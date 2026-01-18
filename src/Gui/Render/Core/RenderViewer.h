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

#ifndef GUI_RENDER_CORE_RENDERVIEWER_H
#define GUI_RENDER_CORE_RENDERVIEWER_H

#include <memory>
#include <string>

#include <FCGlobal.h>
#include "RenderTypes.h"
#include "RenderNode.h"

// 前向声明 Qt 类 / Forward declarations for Qt classes
class QWidget;
class QImage;
class QOpenGLWidget;
class QSurfaceFormat;

namespace Gui {
namespace Render {

/**
 * @brief 渲染查看器接口 / Rendering viewer interface
 *
 * 定义了3D查看器的核心功能，使渲染后端可以独立于具体查看器实现。
 * Defines the core functionality of a 3D viewer, allowing rendering backends
 * to be independent of specific viewer implementations.
 *
 * 设计目标 / Design Goals:
 * - 抽象 Coin3D/Qt 相关的查看器功能
 * - 支持运行时后端切换
 * - 保持与现有 View3DInventorViewer 的兼容性
 *
 * 使用方式 / Usage:
 * ```cpp
 * RenderViewer* viewer = RenderManager::instance().getRenderViewer();
 * viewer->setSceneRoot(rootNode);
 * viewer->render();
 * ```
 */
class GuiExport RenderViewer {
public:
    virtual ~RenderViewer() = default;

    //-----------------------------------------------------------------------
    // 场景图管理 / Scene Graph Management
    //-----------------------------------------------------------------------

    /**
     * @brief 设置场景根节点 / Set scene root node
     *
     * @param root 场景图的根节点 / Root node of the scene graph
     */
    virtual void setSceneRoot(RenderNode::Ptr root) = 0;

    /**
     * @brief 获取场景根节点 / Get scene root node
     */
    virtual RenderNode::Ptr getSceneRoot() const = 0;

    /**
     * @brief 更新场景 / Update scene
     *
     * 通知查看器场景图已更改，需要重新渲染。
     * Notifies the viewer that the scene graph has changed and needs re-rendering.
     */
    virtual void updateScene() = 0;

    //-----------------------------------------------------------------------
    // 渲染控制 / Rendering Control
    //-----------------------------------------------------------------------

    /**
     * @brief 触发渲染 / Trigger rendering
     *
     * 执行一次渲染操作。
     * Perform a single rendering operation.
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

    //-----------------------------------------------------------------------
    // 相机控制 / Camera Control
    //-----------------------------------------------------------------------

    /**
     * @brief 设置相机参数 / Set camera parameters
     */
    virtual void setCamera(const CameraParams& params) = 0;

    /**
     * @brief 获取相机参数 / Get camera parameters
     */
    virtual CameraParams getCamera() const = 0;

    /**
     * @brief 重置相机视图 / Reset camera view
     */
    virtual void resetCamera() = 0;

    /**
     * @brief 适合到整个场景 / Fit to entire scene
     */
    virtual void fitAll() = 0;

    /**
     * @brief 适合到选中的对象 / Fit to selected objects
     */
    virtual void fitSelection() = 0;

    //-----------------------------------------------------------------------
    // 灯光控制 / Light Control
    //-----------------------------------------------------------------------

    /**
     * @brief 设置环境光强度 / Set ambient light intensity
     */
    virtual void setAmbientIntensity(float intensity) = 0;

    /**
     * @brief 获取环境光强度 / Get ambient light intensity
     */
    virtual float getAmbientIntensity() const = 0;

    /**
     * @brief 启用/禁用方向光（背光）/ Enable/disable directional light (backlight)
     */
    virtual void setBacklightEnabled(bool enabled) = 0;

    /**
     * @brief 检查背光是否启用 / Check if backlight is enabled
     */
    virtual bool isBacklightEnabled() const = 0;

    //-----------------------------------------------------------------------
    // 窗口集成 / Window Integration
    //-----------------------------------------------------------------------

    /**
     * @brief 获取 Qt 窗口部件 / Get Qt widget
     *
     * 返回嵌入的 Qt 窗口部件，用于集成到主界面。
     * Returns the embedded Qt widget for integration into the main window.
     */
    virtual QWidget* getWidget() const = 0;

    /**
     * @brief 调整大小 / Resize
     */
    virtual void resize(int width, int height) = 0;

    /**
     * @brief 调整大小事件 / Resize event
     */
    virtual void onResize(int width, int height) = 0;

    //-----------------------------------------------------------------------
    // 截图功能 / Screenshot Functionality
    //-----------------------------------------------------------------------

    /**
     * @brief 截取当前视图为图像 / Capture current view as image
     *
     * @param width 图像宽度 / Image width
     * @param height 图像高度 / Image height
     * @return 捕获的图像 / Captured image
     */
    virtual QImage grabImage(int width, int height) = 0;

    /**
     * @brief 保存截图 / Save screenshot
     *
     * @param filename 文件名 / Filename
     * @param width 图像宽度 / Image width
     * @param height 图像高度 / Image height
     * @return 是否成功 / Whether successful
     */
    virtual bool saveScreenshot(const QString& filename, int width, int height) = 0;

    //-----------------------------------------------------------------------
    // 统计信息 / Statistics
    //-----------------------------------------------------------------------

    /**
     * @brief 获取渲染统计信息 / Get rendering statistics
     */
    virtual RenderStats getStats() const = 0;

    /**
     * @brief 重置统计信息 / Reset statistics
     */
    virtual void resetStats() const = 0;

    //-----------------------------------------------------------------------
    // 兼容性接口 / Compatibility Interface
    //-----------------------------------------------------------------------

    /**
     * @brief 获取原生后端指针 / Get native backend pointer
     *
     * 用于与现有 View3DInventorViewer 代码兼容。
     * For compatibility with existing View3DInventorViewer code.
     *
     * 对于 Coin3D 后端，返回 SoSceneManager*
     * 对于 OsgVerse 后端，返回 osgViewer::Viewer*
     */
    virtual void* getNativePointer() const = 0;

    /**
     * @brief 获取后端类型 / Get backend type
     */
    virtual BackendType getBackendType() const = 0;

    /**
     * @brief 设置事件回调 / Set event callback
     */
    using EventCallback = std::function<void(const std::string& event, void* data)>;
    virtual void setEventCallback(EventCallback callback) = 0;

    //-----------------------------------------------------------------------
    // 查看器模式 / Viewer Modes
    //-----------------------------------------------------------------------

    /**
     * @brief 查看器模式枚举 / Viewer mode enumeration
     */
    enum ViewerMode {
        Idle = 0,           ///< 空闲模式 / Idle mode
        Interaction = 1,    ///< 交互模式 / Interaction mode
        Spinning = 2,       ///< 旋转动画模式 / Spinning animation mode
        Zooming = 3,        ///< 缩放模式 / Zooming mode
        Panning = 4         ///< 平移模式 / Panning mode
    };

    /**
     * @brief 获取当前查看器模式 / Get current viewer mode
     */
    virtual ViewerMode getViewerMode() const = 0;

    /**
     * @brief 设置查看器模式 / Set viewer mode
     */
    virtual void setViewerMode(ViewerMode mode) = 0;

    /**
     * @brief 检查是否正在动画 / Check if animating
     */
    virtual bool isAnimating() const = 0;

    /**
     * @brief 停止动画 / Stop animation
     */
    virtual void stopAnimation() = 0;
};

//-------------------------------------------------------------------------
// 类型别名 / Type Aliases
//-------------------------------------------------------------------------

using RenderViewerPtr = std::shared_ptr<RenderViewer>;
using RenderViewerWeakPtr = std::weak_ptr<RenderViewer>;

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_CORE_RENDERVIEWER_H
