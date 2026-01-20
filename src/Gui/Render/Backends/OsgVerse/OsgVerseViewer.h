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

#ifndef GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEVIEWER_H
#define GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEVIEWER_H

#include <memory>
#include <QOpenGLWidget>
#include <osg/ref_ptr>

#include <FCGlobal.h>
#include "../../Core/RenderViewer.h"
#include "OsgVerseEngine.h"

// OsgVerse / OSG 前向声明 / Forward declarations
namespace osgViewer {
    class Viewer;
    class GraphicsWindowEmbedded;
}

namespace Gui {
namespace Render {

/**
 * @brief OsgVerse 查看器 / OsgVerse viewer
 *
 * 基于 OpenSceneGraph 的 3D 查看器实现。
 * 3D viewer implementation based on OpenSceneGraph.
 *
 * 设计目标 / Design Goals:
 * - 与 Qt 无缝集成
 * - 提供与 View3DInventorViewer 相似的交互体验
 * - 支持现代渲染特性（PBR、HDR 等）
 * - 高性能渲染和流畅的交互
 *
 * 技术特性 / Technical Features:
 * - 基于 QOpenGLWidget 的 Qt 集成
 * - 支持多线程渲染
 * - 内置相机操纵器
 * - 事件处理和交互
 */
class GuiExport OsgVerseViewer : public RenderViewer {
public:
    /**
     * @brief 构造函数 / Constructor
     */
    OsgVerseViewer();

    /**
     * @brief 析构函数 / Destructor
     */
    ~OsgVerseViewer() override;

    //-----------------------------------------------------------------------
    // 场景图管理 / Scene Graph Management
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

    //-----------------------------------------------------------------------
    // 相机控制 / Camera Control
    //-----------------------------------------------------------------------

    void setCamera(const CameraParams& params) override;
    CameraParams getCamera() const override;
    void resetCamera() override;
    void fitAll() override;
    void fitSelection() override;

    //-----------------------------------------------------------------------
    // 灯光控制 / Light Control
    //-----------------------------------------------------------------------

    void setAmbientIntensity(float intensity) override;
    float getAmbientIntensity() const override;
    void setBacklightEnabled(bool enabled) override;
    bool isBacklightEnabled() const override;

    //-----------------------------------------------------------------------
    // 窗口集成 / Window Integration
    //-----------------------------------------------------------------------

    QWidget* getWidget() const override;
    void resize(int width, int height) override;
    void onResize(int width, int height) override;

    //-----------------------------------------------------------------------
    // 截图功能 / Screenshot Functionality
    //-----------------------------------------------------------------------

    QImage grabImage(int width, int height) override;
    bool saveScreenshot(const QString& filename, int width, int height) override;

    //-----------------------------------------------------------------------
    // 统计信息 / Statistics
    //-----------------------------------------------------------------------

    RenderStats getStats() const override;
    void resetStats() const override;

    //-----------------------------------------------------------------------
    // 兼容性接口 / Compatibility Interface
    //-----------------------------------------------------------------------

    void* getNativePointer() const override;
    BackendType getBackendType() const override { return BackendType::OsgVerse; }
    void setEventCallback(EventCallback callback) override;

    //-----------------------------------------------------------------------
    // 查看器模式 / Viewer Modes
    //-----------------------------------------------------------------------

    ViewerMode getViewerMode() const override;
    void setViewerMode(ViewerMode mode) override;
    bool isAnimating() const override;
    void stopAnimation() override;

    //-----------------------------------------------------------------------
    // OsgVerse 特定接口 / OsgVerse-specific Interface
    //-----------------------------------------------------------------------

    /**
     * @brief 获取 OSG 查看器 / Get OSG viewer
     */
    osgViewer::Viewer* getOsgViewer() const { return _viewer; }

    /**
     * @brief 获取渲染引擎 / Get render engine
     */
    OsgVerseEngine* getEngine() const { return _engine.get(); }

    /**
     * @brief 设置相机操纵器 / Set camera manipulator
     */
    void setCameraManipulator(void* manipulator);

    /**
     * @brief 启用/禁用统计显示 / Enable/disable statistics display
     */
    void setStatsEnabled(bool enabled);

    /**
     * @brief 检查统计显示是否启用 / Check if statistics display is enabled
     */
    bool isStatsEnabled() const { return _statsEnabled; }

private:
    /**
     * @brief 初始化查看器 / Initialize viewer
     */
    void initializeViewer();

    /**
     * @brief 初始化 Qt 窗口 / Initialize Qt widget
     */
    void initializeWidget();

    /**
     * @brief 设置默认相机 / Setup default camera
     */
    void setupDefaultCamera();

    /**
     * @brief 设置默认光照 / Setup default lighting
     */
    void setupDefaultLighting();

    /**
     * @brief 确保已初始化 / Ensure initialized
     * 
     * 延迟初始化模式：只在第一次使用时才初始化
     * Lazy initialization: only initialize on first use
     */
    void ensureInitialized();

    /**
     * @brief 检查前置条件 / Check prerequisites
     * 
     * @return true 如果所有前置条件满足 / if all prerequisites are met
     */
    bool checkPrerequisites();

    /**
     * @brief Qt 窗口类 / Qt widget class
     */
    class ViewerWidget;

    // 核心组件 / Core components
    std::unique_ptr<OsgVerseEngine> _engine;    ///< 渲染引擎 / Render engine
    osgViewer::Viewer* _viewer{nullptr};        ///< OSG 查看器 / OSG viewer
    ViewerWidget* _widget{nullptr};             ///< Qt 窗口 / Qt widget
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> _graphicsWindow; ///< 嵌入式图形窗口 / Embedded graphics window

    // 初始化状态 / Initialization state
    bool _initialized{false};                   ///< 是否已初始化 / Whether initialized
    bool _initializationFailed{false};          ///< 初始化是否失败 / Whether initialization failed
    bool _firstFrame{true};                     ///< 是否是第一帧 / Whether first frame

    // 场景状态 / Scene state
    RenderNode::Ptr _sceneRoot;
    RenderMode _renderMode{RenderMode::Default};
    Color _backgroundColor{0.2f, 0.2f, 0.3f, 1.0f};

    // 相机状态 / Camera state
    CameraParams _cameraParams;
    float _ambientIntensity{0.3f};
    bool _backlightEnabled{true};

    // 查看器状态 / Viewer state
    ViewerMode _viewerMode{ViewerMode::Idle};
    bool _isAnimating{false};
    bool _statsEnabled{false};

    // 事件回调 / Event callback
    EventCallback _eventCallback;
};

/**
 * @brief OsgVerse 查看器的 Qt 窗口部件 / Qt widget for OsgVerse viewer
 *
 * 集成 OSG 渲染到 Qt 窗口系统。
 * Integrates OSG rendering into Qt widget system.
 */
class OsgVerseViewer::ViewerWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    ViewerWidget(osgViewer::Viewer* viewer, 
                 osgViewer::GraphicsWindowEmbedded* graphicsWindow,
                 QWidget* parent = nullptr);
    ~ViewerWidget() override;

    osgViewer::GraphicsWindowEmbedded* getGraphicsWindow() const { return _graphicsWindow.get(); }

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    // 鼠标事件 / Mouse events
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    // 键盘事件 / Keyboard events
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    osgViewer::Viewer* _viewer;
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> _graphicsWindow;
    bool _firstFrame{true};
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEVIEWER_H
