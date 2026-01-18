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
 *   write to the FreeCAD Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "Coin3DViewer.h"
#include "Coin3DNode.h"
#include "Coin3DUtils.h"

#include <View3DInventorViewer.h>
#include <Quarter/SoQTQuarterAdaptor.h>

#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/SoSceneManager.h>
#include <Inventor/actions/SoGLRenderAction.h>

#include <QWidget>
#include <QImage>
#include <QOpenGLWidget>

#include <Base/Console.h>

FC_LOG_LEVEL_INIT("Render.Coin3DViewer")

namespace Gui {
namespace Render {

//===========================================================================
// Coin3DViewer Implementation
//===========================================================================

Coin3DViewer::Coin3DViewer(View3DInventorViewer* viewer, bool takeOwnership)
    : _viewer(viewer)
    , _ownsViewer(takeOwnership)
    , _renderRoot(nullptr)
    , _stats{}
    , _viewerMode(ViewerMode::Idle)
{
    if (!_viewer) {
        FC_ERR("Coin3DViewer: Created with null View3DInventorViewer");
    }
}

Coin3DViewer::~Coin3DViewer()
{
    if (_ownsViewer && _viewer) {
        delete _viewer;
    }
}

//---------------------------------------------------------------------------
// 场景图管理 / Scene Graph Management
//---------------------------------------------------------------------------

void Coin3DViewer::setSceneRoot(RenderNode::Ptr root)
{
    _renderRoot = root;
    syncSceneRoot();
}

RenderNode::Ptr Coin3DViewer::getSceneRoot() const
{
    return _renderRoot;
}

void Coin3DViewer::updateScene()
{
    if (_viewer) {
        // 触发场景图更新
        // Trigger scene graph update
        if (auto* root = _viewer->getSceneGraph()) {
            root->touch();
        }
    }
}

//---------------------------------------------------------------------------
// 渲染控制 / Rendering Control
//---------------------------------------------------------------------------

void Coin3DViewer::render()
{
    if (_viewer) {
        // View3DInventorViewer 会自动渲染，这里可以手动触发重绘
        // View3DInventorViewer auto-renders, here we can manually trigger repaint
        _viewer->getWidget()->update();
        _stats.frameCount++;
    }
}

void Coin3DViewer::setRenderMode(RenderMode mode)
{
    if (_viewer) {
        // 将 RenderMode 转换为 View3DInventorViewer 的渲染类型
        // Convert RenderMode to View3DInventorViewer's render type
        // TODO: 实现渲染模式转换
        (void)mode;
    }
}

RenderMode Coin3DViewer::getRenderMode() const
{
    // TODO: 从 View3DInventorViewer 获取当前渲染模式
    return RenderMode::Default;
}

void Coin3DViewer::setBackgroundColor(const Color& color)
{
    if (_viewer) {
        // TODO: 设置背景色
        (void)color;
    }
}

Color Coin3DViewer::getBackgroundColor() const
{
    // TODO: 获取背景色
    return Color(0.2f, 0.2f, 0.3f, 1.0f);
}

//---------------------------------------------------------------------------
// 相机控制 / Camera Control
//---------------------------------------------------------------------------

void Coin3DViewer::setCamera(const CameraParams& params)
{
    if (_viewer) {
        // TODO: 设置相机参数
        (void)params;
    }
}

CameraParams Coin3DViewer::getCamera() const
{
    // TODO: 获取相机参数
    return CameraParams{};
}

void Coin3DViewer::resetCamera()
{
    if (_viewer) {
        _viewer->resetView();
    }
}

void Coin3DViewer::fitAll()
{
    if (_viewer) {
        _viewer->viewAll();
    }
}

void Coin3DViewer::fitSelection()
{
    if (_viewer) {
        _viewer->viewSelection();
    }
}

//---------------------------------------------------------------------------
// 灯光控制 / Light Control
//---------------------------------------------------------------------------

void Coin3DViewer::setAmbientIntensity(float intensity)
{
    if (auto* env = getEnvironment()) {
        // TODO: 设置环境光强度
        (void)intensity;
    }
}

float Coin3DViewer::getAmbientIntensity() const
{
    // TODO: 获取环境光强度
    return 0.2f;
}

void Coin3DViewer::setBacklightEnabled(bool enabled)
{
    if (_viewer) {
        _viewer->setBacklightEnabled(enabled);
    }
}

bool Coin3DViewer::isBacklightEnabled() const
{
    return _viewer ? _viewer->isBacklightEnabled() : false;
}

//---------------------------------------------------------------------------
// 窗口集成 / Window Integration
//---------------------------------------------------------------------------

QWidget* Coin3DViewer::getWidget() const
{
    return _viewer ? _viewer->getWidget() : nullptr;
}

void Coin3DViewer::resize(int width, int height)
{
    if (auto* widget = getWidget()) {
        widget->resize(width, height);
    }
}

void Coin3DViewer::onResize(int width, int height)
{
    if (_viewer) {
        // View3DInventorViewer 会自动处理 resize
        // View3DInventorViewer handles resize automatically
        (void)width;
        (void)height;
    }
}

//---------------------------------------------------------------------------
// 截图功能 / Screenshot Functionality
//---------------------------------------------------------------------------

QImage Coin3DViewer::grabImage(int width, int height)
{
    if (_viewer) {
        return _viewer->grabFramebuffer();
    }
    return QImage();
}

bool Coin3DViewer::saveScreenshot(const QString& filename, int width, int height)
{
    QImage img = grabImage(width, height);
    if (!img.isNull()) {
        return img.save(filename);
    }
    return false;
}

//---------------------------------------------------------------------------
// 统计信息 / Statistics
//---------------------------------------------------------------------------

RenderStats Coin3DViewer::getStats() const
{
    return _stats;
}

void Coin3DViewer::resetStats() const
{
    _stats = RenderStats{};
}

//---------------------------------------------------------------------------
// 兼容性接口 / Compatibility Interface
//---------------------------------------------------------------------------

void* Coin3DViewer::getNativePointer() const
{
    return static_cast<void*>(getSceneManager());
}

void Coin3DViewer::setEventCallback(EventCallback callback)
{
    _eventCallback = std::move(callback);
}

//---------------------------------------------------------------------------
// 查看器模式 / Viewer Modes
//---------------------------------------------------------------------------

ViewerMode Coin3DViewer::getViewerMode() const
{
    return _viewerMode;
}

void Coin3DViewer::setViewerMode(ViewerMode mode)
{
    _viewerMode = mode;
}

bool Coin3DViewer::isAnimating() const
{
    return _viewer ? _viewer->isAnimating() : false;
}

void Coin3DViewer::stopAnimation()
{
    if (_viewer) {
        _viewer->stopAnimating();
    }
}

//---------------------------------------------------------------------------
// Coin3D 特定接口 / Coin3D-specific Interface
//---------------------------------------------------------------------------

void Coin3DViewer::setSceneGraph(SoNode* root)
{
    if (_viewer) {
        _viewer->setSceneGraph(root);
    }
}

SoSceneManager* Coin3DViewer::getSceneManager() const
{
    // TODO: 从 View3DInventorViewer 获取场景管理器
    return nullptr;
}

void Coin3DViewer::setRenderAction(SoGLRenderAction* action)
{
    // TODO: 设置渲染动作
    (void)action;
}

SoGLRenderAction* Coin3DViewer::getRenderAction() const
{
    // TODO: 获取渲染动作
    return nullptr;
}

void Coin3DViewer::syncSceneRoot()
{
    if (!_renderRoot || !_viewer) {
        return;
    }

    // 将抽象渲染根节点转换为 Coin3D 节点
    // Convert abstract render root node to Coin3D node
    if (auto* coin3DNode = dynamic_cast<Coin3DNode*>(_renderRoot.get())) {
        SoNode* coinNode = coin3DNode->getCoinNode();
        if (coinNode) {
            _viewer->setSceneGraph(coinNode);
        }
    }
}

} // namespace Render
} // namespace Gui
