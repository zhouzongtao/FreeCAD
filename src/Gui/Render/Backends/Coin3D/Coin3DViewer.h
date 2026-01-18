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

#ifndef GUI_RENDER_BACKENDS_COIN3D_COIN3DVIEWER_H
#define GUI_RENDER_BACKENDS_COIN3D_COIN3DVIEWER_H

#include <memory>

#include <FCGlobal.h>
#include "../../Core/RenderViewer.h"

// 前向声明 / Forward declarations
class QWidget;
class QImage;
class QOpenGLWidget;
class QSurfaceFormat;
class SoNode;
class SoSceneManager;
class SoGLRenderAction;

namespace Gui {

class View3DInventorViewer;

namespace Render {

/**
 * @brief Coin3D 渲染查看器包装器 / Coin3D rendering viewer wrapper
 *
 * 将现有的 View3DInventorViewer 包装为 RenderViewer 接口，
 * 使其可以通过抽象层访问。
 *
 * Wraps the existing View3DInventorViewer to implement the RenderViewer interface,
 * allowing it to be accessed through the abstraction layer.
 *
 * 设计原则 / Design Principles:
 * - 不修改现有 View3DInventorViewer 实现
 * - 提供透明的接口转换
 * - 支持运行时与其他后端切换
 */
class GuiExport Coin3DViewer : public RenderViewer {
public:
    /**
     * @brief 构造函数 / Constructor
     *
     * @param viewer 现有的 View3DInventorViewer 实例 / Existing View3DInventorViewer instance
     * @param takeOwnership 是否获取所有权 / Whether to take ownership
     */
    explicit Coin3DViewer(View3DInventorViewer* viewer, bool takeOwnership = false);

    /**
     * @brief 析构函数 / Destructor
     */
    ~Coin3DViewer() override;

    // 禁止拷贝，允许移动 / Disable copy, allow move
    FC_DISABLE_COPY(Coin3DViewer)
    FC_DEFAULT_MOVE(Coin3DViewer)

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
    BackendType getBackendType() const override { return BackendType::Coin3D; }
    void setEventCallback(EventCallback callback override;

    //-----------------------------------------------------------------------
    // 查看器模式 / Viewer Modes
    //-----------------------------------------------------------------------

    ViewerMode getViewerMode() const override;
    void setViewerMode(ViewerMode mode) override;
    bool isAnimating() const override;
    void stopAnimation() override;

    //-----------------------------------------------------------------------
    // Coin3D 特定接口 / Coin3D-specific Interface
    //-----------------------------------------------------------------------

    /**
     * @brief 获取底层 View3DInventorViewer / Get underlying View3DInventorViewer
     *
     * @return View3DInventorViewer 指针 / Pointer to View3DInventorViewer
     */
    View3DInventorViewer* getViewer() const { return _viewer; }

    /**
     * @brief 设置场景图（Coin3D 节点）/ Set scene graph (Coin3D node)
     *
     * @param root Coin3D 场景根节点 / Coin3D scene root node
     */
    void setSceneGraph(SoNode* root);

    /**
     * @brief 获取场景管理器 / Get scene manager
     *
     * @return SoSceneManager 指针 / Pointer to SoSceneManager
     */
    SoSceneManager* getSceneManager() const;

    /**
     * @brief 设置渲染动作 / Set render action
     *
     * @param action SoGLRenderAction 指针 / Pointer to SoGLRenderAction
     */
    void setRenderAction(SoGLRenderAction* action);

    /**
     * @brief 获取渲染动作 / Get render action
     *
     * @return SoGLRenderAction 指针 / Pointer to SoGLRenderAction
     */
    SoGLRenderAction* getRenderAction() const;

    /**
     * @brief 同步渲染根节点 / Sync render root node
     *
     * 将抽象的渲染根节点同步到 Coin3D 场景图。
     * Syncs the abstract render root node to Coin3D scene graph.
     */
    void syncSceneRoot();

private:
    View3DInventorViewer* _viewer;     ///< 底层查看器 / Underlying viewer
    bool _ownsViewer;                  ///< 所有权标志 / Ownership flag
    RenderNode::Ptr _renderRoot;        ///< 抽象渲染根节点 / Abstract render root node
    EventCallback _eventCallback;       ///< 事件回调 / Event callback
    mutable RenderStats _stats;          ///< 统计信息 / Statistics
    ViewerMode _viewerMode{ViewerMode::Idle};
};

//-------------------------------------------------------------------------
// 便利函数 / Convenience Functions
//-------------------------------------------------------------------------

/**
 * @brief 创建 Coin3D 查看器包装器 / Create Coin3D viewer wrapper
 *
 * @param viewer View3DInventorViewer 实例 / View3DInventorViewer instance
 * @param takeOwnership 是否获取所有权 / Whether to take ownership
 * @return Coin3DViewer 指针 / Pointer to Coin3DViewer
 */
inline RenderViewerPtr createCoin3DViewer(View3DInventorViewer* viewer, bool takeOwnership = false)
{
    return std::make_shared<Coin3DViewer>(viewer, takeOwnership);
}

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_COIN3D_COIN3DVIEWER_H
