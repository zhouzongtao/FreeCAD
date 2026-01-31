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

#include "PreCompiled.h"

#include "CoinViewer.h"
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProvider.h>
#include <Gui/View3D/ViewerFactory.h>
#include <Gui/Navigation/NavigationStyle.h>
#include <Gui/Render/Core/RenderTypes.h>
#include <Base/Console.h>

#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/SoDB.h>

#include <cmath>
#include <algorithm>

using namespace Gui::View3D::Coin;
using Gui::ViewProvider;
using Gui::View3D::SelectionMode;
using Gui::View3D::RenderMode;
using Gui::View3D::CameraParams;
using Gui::View3D::PickResult;
namespace Render = Gui::Render;

//===========================================================================
// CoinViewer Implementation
//===========================================================================

CoinViewer::CoinViewer(QWidget* parent, const QOpenGLWidget* shareWidget)
{
    Base::Console().log("CoinViewer: Creating Coin3D viewer\n");
    
    // 创建 View3DInventorViewer 实例
    _coinViewer = new View3DInventorViewer(parent, shareWidget);
    
    Base::Console().log("CoinViewer: Coin3D viewer created successfully\n");
}

CoinViewer::~CoinViewer()
{
    Base::Console().log("CoinViewer: Destroying Coin3D viewer\n");
    
    // View3DInventorViewer 会自动清理
    // 因为它是 QWidget，Qt 会处理删除
}

//-----------------------------------------------------------------------
// 基础渲染接口
//-----------------------------------------------------------------------

void CoinViewer::render()
{
    // View3DInventorViewer 通过 Qt 的 paintEvent 自动渲染
    // 这里只需要触发更新
    if (_coinViewer) {
        _coinViewer->update();
    }
}

void CoinViewer::resize(int width, int height)
{
    if (_coinViewer) {
        _coinViewer->resize(width, height);
    }
}

QWidget* CoinViewer::getWidget()
{
    return _coinViewer;
}

QOpenGLWidget* CoinViewer::getGLWidget()
{
    return qobject_cast<QOpenGLWidget*>(_coinViewer);
}

//-----------------------------------------------------------------------
// 场景管理
//-----------------------------------------------------------------------

void CoinViewer::setSceneGraph(void* root)
{
    if (_coinViewer && root) {
        auto* soNode = static_cast<SoNode*>(root);
        _coinViewer->setSceneGraph(soNode);
    }
}

void* CoinViewer::getSceneGraph()
{
    if (_coinViewer) {
        return _coinViewer->getSceneGraph();
    }
    return nullptr;
}

void CoinViewer::updateScene()
{
    if (_coinViewer) {
        _coinViewer->update();
    }
}

//-----------------------------------------------------------------------
// 相机控制
//-----------------------------------------------------------------------

void CoinViewer::setCamera(const CameraParams& params)
{
    if (!_coinViewer) {
        return;
    }
    
    SoCamera* camera = _coinViewer->getCamera();
    if (!camera) {
        return;
    }
    
    // 设置相机位置和方向
    SbVec3f pos(params.position.x, params.position.y, params.position.z);
    SbVec3f target(params.target.x, params.target.y, params.target.z);
    SbVec3f up(params.upVector.x, params.upVector.y, params.upVector.z);
    
    // 计算方向
    SbVec3f dir = target - pos;
    dir.normalize();
    
    // 设置相机方向（使用 pointAt）
    camera->position = pos;
    camera->pointAt(target, up);
    
    // 设置裁剪面
    camera->nearDistance = params.nearPlane;
    camera->farDistance = params.farPlane;
    
    // 设置投影参数
    if (params.orthographic) {
        if (auto* ortho = dynamic_cast<SoOrthographicCamera*>(camera)) {
            ortho->height = params.height;
        }
    } else {
        if (auto* persp = dynamic_cast<SoPerspectiveCamera*>(camera)) {
            persp->heightAngle = params.fieldOfView * M_PI / 180.0;  // 转换为弧度
        }
    }
}

CameraParams CoinViewer::getCamera() const
{
    CameraParams params;
    
    if (!_coinViewer) {
        return params;
    }
    
    SoCamera* camera = _coinViewer->getCamera();
    if (!camera) {
        return params;
    }
    
    // 获取相机位置
    SbVec3f pos = camera->position.getValue();
    params.position = Base::Vector3d(pos[0], pos[1], pos[2]);
    
    // 获取相机方向
    SbRotation rot = camera->orientation.getValue();
    SbVec3f dir(0, 0, -1);  // Coin3D 默认方向
    rot.multVec(dir, dir);
    
    // 计算目标点（假设距离为 10）
    params.target = params.position + Base::Vector3d(dir[0], dir[1], dir[2]) * 10.0;
    
    // 获取上向量
    SbVec3f up(0, 1, 0);
    rot.multVec(up, up);
    params.upVector = Base::Vector3d(up[0], up[1], up[2]);
    
    // 获取裁剪面
    params.nearPlane = camera->nearDistance.getValue();
    params.farPlane = camera->farDistance.getValue();
    
    // 获取投影参数
    params.orthographic = camera->isOfType(SoOrthographicCamera::getClassTypeId());
    
    if (params.orthographic) {
        if (auto* ortho = dynamic_cast<SoOrthographicCamera*>(camera)) {
            params.height = ortho->height.getValue();
        }
    } else {
        if (auto* persp = dynamic_cast<SoPerspectiveCamera*>(camera)) {
            params.fieldOfView = persp->heightAngle.getValue() * 180.0 / M_PI;  // 转换为度
        }
    }
    
    return params;
}

void CoinViewer::viewAll()
{
    if (_coinViewer) {
        _coinViewer->viewAll();
    }
}

void CoinViewer::fitSelection()
{
    if (_coinViewer) {
        _coinViewer->viewSelection();
    }
}

void CoinViewer::resetCamera()
{
    if (_coinViewer && _coinViewer->hasHomePosition()) {
        _coinViewer->resetToHomePosition();
    } else {
        viewAll();
    }
}

void CoinViewer::setCameraType(bool orthographic)
{
    if (_coinViewer) {
        SoType type = orthographic ? 
            SoOrthographicCamera::getClassTypeId() : 
            SoPerspectiveCamera::getClassTypeId();
        _coinViewer->setCameraType(type);
    }
}

bool CoinViewer::isCameraOrthographic() const
{
    if (_coinViewer) {
        SoCamera* camera = _coinViewer->getCamera();
        if (camera) {
            return camera->isOfType(SoOrthographicCamera::getClassTypeId());
        }
    }
    return false;
}

//-----------------------------------------------------------------------
// 事件处理
//-----------------------------------------------------------------------

bool CoinViewer::handleMouseEvent(QMouseEvent* event)
{
    // View3DInventorViewer 自动处理鼠标事件
    // 这里返回 false 让事件继续传播
    return false;
}

bool CoinViewer::handleKeyEvent(QKeyEvent* event)
{
    // View3DInventorViewer 自动处理键盘事件
    return false;
}

bool CoinViewer::handleWheelEvent(QWheelEvent* event)
{
    // View3DInventorViewer 自动处理滚轮事件
    return false;
}

//-----------------------------------------------------------------------
// 拾取和选择
//-----------------------------------------------------------------------

PickResult CoinViewer::pick(const QPoint& pos)
{
    PickResult result;
    
    if (!_coinViewer) {
        return result;
    }
    
    // 使用 View3DInventorViewer 的拾取功能
    SbVec2s sbPos(pos.x(), pos.y());
    SoPickedPoint* pp = _coinViewer->pickPoint(sbPos);
    
    if (pp) {
        result.valid = true;
        
        // 获取拾取点
        SbVec3f point = pp->getPoint();
        result.point = Base::Vector3d(point[0], point[1], point[2]);
        
        // 获取法线
        SbVec3f normal = pp->getNormal();
        result.normal = Base::Vector3d(normal[0], normal[1], normal[2]);
        
        // 获取 ViewProvider
        SoPath* path = pp->getPath();
        if (path) {
            result.viewProvider = _coinViewer->getViewProviderByPath(path);
        }
        
        delete pp;
    }
    
    return result;
}

void CoinViewer::setSelectionMode(SelectionMode mode)
{
    if (_coinViewer) {
        int coinMode = convertSelectionMode(mode);
        // View3DInventorViewer 使用不同的选择模式枚举
        // 这里需要根据实际情况调整
    }
}

SelectionMode CoinViewer::getSelectionMode() const
{
    // 从 View3DInventorViewer 获取当前选择模式
    return SelectionMode::None;
}

void CoinViewer::startSelection(SelectionMode mode)
{
    if (_coinViewer) {
        int coinMode = convertSelectionMode(mode);
        _coinViewer->startSelection(static_cast<View3DInventorViewer::SelectionMode>(coinMode));
    }
}

void CoinViewer::stopSelection()
{
    if (_coinViewer) {
        _coinViewer->stopSelection();
    }
}

void CoinViewer::abortSelection()
{
    if (_coinViewer) {
        _coinViewer->abortSelection();
    }
}

bool CoinViewer::isSelecting() const
{
    if (_coinViewer) {
        return _coinViewer->isSelecting();
    }
    return false;
}

//-----------------------------------------------------------------------
// ViewProvider 管理
//-----------------------------------------------------------------------

void CoinViewer::addViewProvider(ViewProvider* vp)
{
    if (_coinViewer && vp) {
        _coinViewer->addViewProvider(vp);
    }
}

void CoinViewer::removeViewProvider(ViewProvider* vp)
{
    if (_coinViewer && vp) {
        _coinViewer->removeViewProvider(vp);
    }
}

void CoinViewer::updateViewProvider(ViewProvider* vp)
{
    // Coin3D ViewProviders are updated automatically through the
    // ViewProvider::update() mechanism. No additional action needed here.
    (void)vp;
}

bool CoinViewer::hasViewProvider(ViewProvider* vp) const
{
    if (_coinViewer && vp) {
        return _coinViewer->hasViewProvider(vp);
    }
    return false;
}

std::vector<ViewProvider*> CoinViewer::getViewProviders() const
{
    std::vector<ViewProvider*> providers;
    
    if (_coinViewer) {
        // View3DInventorViewer 使用 set 存储 ViewProvider
        // 需要转换为 vector
        auto vpSet = _coinViewer->getViewProvidersOfType(ViewProvider::getClassTypeId());
        providers.assign(vpSet.begin(), vpSet.end());
    }
    
    return providers;
}

//-----------------------------------------------------------------------
// 渲染设置
//-----------------------------------------------------------------------

void CoinViewer::setRenderMode(RenderMode mode)
{
    if (!_coinViewer) {
        return;
    }

    // Map IViewer3D RenderMode to View3DInventorViewer override mode strings
    std::string overrideMode;
    switch (mode) {
        case RenderMode::Wireframe:
            overrideMode = "Wireframe";
            break;
        case RenderMode::Points:
            overrideMode = "Point";
            break;
        case RenderMode::HiddenLine:
            overrideMode = "Hidden Line";
            break;
        case RenderMode::NoShading:
            overrideMode = "No Shading";
            break;
        case RenderMode::Shaded:
            overrideMode = "Shaded";
            break;
        case RenderMode::FlatLines:
            overrideMode = "Flat Lines";
            break;
        case RenderMode::AsIs:
        default:
            overrideMode = "As Is";
            break;
    }

    _coinViewer->setOverrideMode(overrideMode);
}

RenderMode CoinViewer::getRenderMode() const
{
    if (!_coinViewer) {
        return RenderMode::Shaded;
    }

    // Map View3DInventorViewer override mode string to IViewer3D RenderMode
    std::string mode = _coinViewer->getOverrideMode();

    if (mode == "Wireframe") {
        return RenderMode::Wireframe;
    }
    else if (mode == "Point") {
        return RenderMode::Points;
    }
    else if (mode == "Hidden Line") {
        return RenderMode::HiddenLine;
    }
    else if (mode == "No Shading") {
        return RenderMode::NoShading;
    }
    else if (mode == "Shaded") {
        return RenderMode::Shaded;
    }
    else if (mode == "Flat Lines") {
        return RenderMode::FlatLines;
    }
    else {
        return RenderMode::AsIs;
    }
}

void CoinViewer::setBackgroundColor(const Base::Color& color)
{
    if (_coinViewer) {
        QColor qcolor(color.r * 255, color.g * 255, color.b * 255, color.a * 255);
        _coinViewer->setBackgroundColor(qcolor);
    }
}

Base::Color CoinViewer::getBackgroundColor() const
{
    if (_coinViewer) {
        QColor qcolor = _coinViewer->backgroundColor();
        return Base::Color(qcolor.redF(), qcolor.greenF(), qcolor.blueF(), qcolor.alphaF());
    }
    return Base::Color(0.2f, 0.2f, 0.3f, 1.0f);
}

void CoinViewer::setBacklightEnabled(bool enabled)
{
    if (_coinViewer) {
        _coinViewer->setBacklightEnabled(enabled);
    }
}

bool CoinViewer::isBacklightEnabled() const
{
    if (_coinViewer) {
        return _coinViewer->isBacklightEnabled();
    }
    return false;
}

void CoinViewer::setAmbientIntensity(float intensity)
{
    // Coin3D uses a different mechanism for ambient light
    // The ambient intensity is typically controlled through SoEnvironment or SoLightModel
    // For now, store the value and log it
    _ambientIntensity = std::clamp(intensity, 0.0f, 1.0f);

    // TODO: Implement proper Coin3D ambient intensity control
    // This would require:
    // 1. Finding or creating an SoEnvironment node
    // 2. Setting its ambientIntensity field
    Base::Console().log("CoinViewer: Ambient intensity set to %.2f (not fully implemented)\n",
                        _ambientIntensity);
}

float CoinViewer::getAmbientIntensity() const
{
    return _ambientIntensity;
}

//-----------------------------------------------------------------------
// 导航和交互
//-----------------------------------------------------------------------

void CoinViewer::setNavigationStyle(const std::string& style)
{
    if (!_coinViewer) {
        return;
    }

    // Map style string to full type name if needed
    std::string typeName = style;

    // Handle both short names and full names
    if (style.find("Gui::") == std::string::npos) {
        // Short name provided, convert to full name
        if (style == "CAD" || style == "CADNavigationStyle") {
            typeName = "Gui::CADNavigationStyle";
        }
        else if (style == "Blender" || style == "BlenderNavigationStyle") {
            typeName = "Gui::BlenderNavigationStyle";
        }
        else if (style == "Touchpad" || style == "TouchpadNavigationStyle") {
            typeName = "Gui::TouchpadNavigationStyle";
        }
        else if (style == "Gesture" || style == "GestureNavigationStyle") {
            typeName = "Gui::GestureNavigationStyle";
        }
        else if (style == "OpenInventor" || style == "InventorNavigationStyle") {
            typeName = "Gui::InventorNavigationStyle";
        }
        else if (style == "Maya" || style == "MayaGestureNavigationStyle") {
            typeName = "Gui::MayaGestureNavigationStyle";
        }
        else if (style == "OpenCascade" || style == "OpenCascadeNavigationStyle") {
            typeName = "Gui::OpenCascadeNavigationStyle";
        }
        else if (style == "OpenSCAD" || style == "OpenSCADNavigationStyle") {
            typeName = "Gui::OpenSCADNavigationStyle";
        }
        else if (style == "Revit" || style == "RevitNavigationStyle") {
            typeName = "Gui::RevitNavigationStyle";
        }
        else if (style == "TinkerCAD" || style == "TinkerCADNavigationStyle") {
            typeName = "Gui::TinkerCADNavigationStyle";
        }
        else if (style == "Trackball") {
            // Trackball maps to Blender style in FreeCAD
            typeName = "Gui::BlenderNavigationStyle";
        }
    }

    // Find and set the navigation type
    Base::Type navType = Base::Type::fromName(typeName.c_str());
    if (!navType.isBad()) {
        _coinViewer->setNavigationType(navType);
    }
}

std::string CoinViewer::getNavigationStyle() const
{
    if (_coinViewer) {
        NavigationStyle* nav = _coinViewer->navigationStyle();
        if (nav) {
            return nav->getTypeId().getName();
        }
    }
    return "Unknown";
}

void CoinViewer::setViewing(bool enable)
{
    if (_coinViewer) {
        _coinViewer->setViewing(enable);
    }
}

bool CoinViewer::isViewing() const
{
    if (_coinViewer) {
        return _coinViewer->isViewing();
    }
    return false;
}

//-----------------------------------------------------------------------
// 后端信息
//-----------------------------------------------------------------------

std::string CoinViewer::getBackendVersion() const
{
    return SoDB::getVersion();
}

//-----------------------------------------------------------------------
// 统计和调试
//-----------------------------------------------------------------------

Render::RenderStats CoinViewer::getStats() const
{
    Render::RenderStats stats;
    stats.frameCount = 0;
    stats.drawCalls = 0;
    stats.triangleCount = 0;
    stats.vertexCount = 0;
    stats.frameTime = 0.0;
    stats.fps = 0.0;

    if (!_coinViewer) {
        return stats;
    }

    // Count ViewProviders as a proxy for draw calls
    auto vpSet = _coinViewer->getViewProvidersOfType(ViewProvider::getClassTypeId());
    stats.drawCalls = static_cast<int>(vpSet.size());

    // Note: Detailed frame timing stats would require modifications to View3DInventorViewer
    // to expose the internal framesPerSecond array. For now, we return basic stats.

    return stats;
}

void CoinViewer::resetStats()
{
    // Coin3D manages stats internally per-frame
    // This is a placeholder for any custom stat tracking reset
}

void CoinViewer::setFPSEnabled(bool enabled)
{
    if (_coinViewer) {
        _coinViewer->setEnabledFPSCounter(enabled);
    }
}

bool CoinViewer::isFPSEnabled() const
{
    // Note: View3DInventorViewer doesn't expose a getter for fpsEnabled
    // Return false as default; enabling FPS counter still works via setFPSEnabled
    return false;
}

//-----------------------------------------------------------------------
// 高级功能
//-----------------------------------------------------------------------

QImage CoinViewer::grabImage(int width, int height)
{
    if (_coinViewer) {
        return _coinViewer->grabFramebuffer();
    }
    return QImage();
}

bool CoinViewer::saveScreenshot(const QString& filename, int width, int height)
{
    QImage img = grabImage(width, height);
    if (!img.isNull()) {
        return img.save(filename);
    }
    return false;
}

void CoinViewer::setEditingViewProvider(ViewProvider* vp, int mode)
{
    if (_coinViewer) {
        _coinViewer->setEditingViewProvider(vp, mode);
    }
}

ViewProvider* CoinViewer::getEditingViewProvider() const
{
    if (_coinViewer) {
        return _coinViewer->getEditingViewProvider();
    }
    return nullptr;
}

bool CoinViewer::isEditingViewProvider() const
{
    if (_coinViewer) {
        return _coinViewer->isEditingViewProvider();
    }
    return false;
}

void CoinViewer::resetEditingViewProvider()
{
    if (_coinViewer) {
        _coinViewer->resetEditingViewProvider();
    }
}

//-----------------------------------------------------------------------
// 辅助方法
//-----------------------------------------------------------------------

int CoinViewer::convertSelectionMode(SelectionMode mode)
{
    switch (mode) {
        case SelectionMode::Lasso:
            return View3DInventorViewer::Lasso;
        case SelectionMode::Rectangle:
            return View3DInventorViewer::Rectangle;
        case SelectionMode::Rubberband:
            return View3DInventorViewer::Rubberband;
        default:
            return View3DInventorViewer::Lasso;
    }
}

SelectionMode CoinViewer::convertSelectionMode(int mode)
{
    switch (mode) {
        case View3DInventorViewer::Lasso:
            return SelectionMode::Lasso;
        case View3DInventorViewer::Rectangle:
            return SelectionMode::Rectangle;
        case View3DInventorViewer::Rubberband:
            return SelectionMode::Rubberband;
        default:
            return SelectionMode::None;
    }
}

//===========================================================================
// CoinViewerRegistrar Implementation
//===========================================================================

CoinViewerRegistrar::CoinViewerRegistrar()
{
    Base::Console().log("CoinViewerRegistrar: Registering Coin3D viewer\n");
    
    // 注册 Coin3D 渲染器创建函数
    ViewerFactory::registerCreator(
        Render::BackendType::Coin3D,
        [](QWidget* parent, const QOpenGLWidget* shareWidget) -> std::unique_ptr<IViewer3D> {
            return std::make_unique<CoinViewer>(parent, shareWidget);
        }
    );
    
    Base::Console().log("CoinViewerRegistrar: Coin3D viewer registered successfully\n");
}

// 全局注册器实例（自动注册）
static CoinViewerRegistrar g_coinViewerRegistrar;
