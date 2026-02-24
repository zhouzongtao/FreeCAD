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

#ifdef RENDER_HAS_OSGVERSE_BACKEND

#include "PreCompiled.h"

#ifndef _PreComp_
# include <QOpenGLWidget>
# include <QApplication>
# include <QMouseEvent>
# include <QKeyEvent>
# include <QWheelEvent>
#endif

#include "OsgVerseViewerAdapter.h"
#include <Gui/ViewProvider.h>
#include <Gui/Render/Backends/OsgVerse/OsgVerseEngine.h>
#include <Base/Console.h>

#include <osg/Node>
#include <osgViewer/Viewer>
#include <osg/Camera>

using namespace Gui::View3D::OsgVerse;
using namespace Gui::View3D;

//===========================================================================
// Helper: Convert View3D::RenderMode ↔ Render::RenderMode
//===========================================================================

static Gui::Render::RenderMode toRenderMode(RenderMode mode)
{
    switch (mode) {
        case RenderMode::Wireframe:  return Gui::Render::RenderMode::Wireframe;
        case RenderMode::Points:     return Gui::Render::RenderMode::Points;
        case RenderMode::Shaded:     return Gui::Render::RenderMode::Shaded;
        case RenderMode::FlatLines:  return Gui::Render::RenderMode::Flat;
        case RenderMode::NoShading:  return Gui::Render::RenderMode::Flat;
        case RenderMode::HiddenLine: return Gui::Render::RenderMode::Wireframe;
        case RenderMode::AsIs:
        default:                     return Gui::Render::RenderMode::Default;
    }
}

static RenderMode fromRenderMode(Gui::Render::RenderMode mode)
{
    switch (mode) {
        case Gui::Render::RenderMode::Wireframe: return RenderMode::Wireframe;
        case Gui::Render::RenderMode::Points:    return RenderMode::Points;
        case Gui::Render::RenderMode::Shaded:    return RenderMode::Shaded;
        case Gui::Render::RenderMode::Flat:      return RenderMode::FlatLines;
        case Gui::Render::RenderMode::Gouraud:   return RenderMode::Shaded;
        case Gui::Render::RenderMode::Phong:     return RenderMode::Shaded;
        case Gui::Render::RenderMode::Default:
        default:                                 return RenderMode::AsIs;
    }
}

//===========================================================================
// Helper: Convert View3D::CameraParams ↔ Render::CameraParams
//===========================================================================

static Gui::Render::CameraParams toRenderCameraParams(const CameraParams& p)
{
    Gui::Render::CameraParams rp;
    rp.position = Gui::Render::Vec3f(
        static_cast<float>(p.position.x),
        static_cast<float>(p.position.y),
        static_cast<float>(p.position.z));
    rp.target = Gui::Render::Vec3f(
        static_cast<float>(p.target.x),
        static_cast<float>(p.target.y),
        static_cast<float>(p.target.z));
    rp.upVector = Gui::Render::Vec3f(
        static_cast<float>(p.upVector.x),
        static_cast<float>(p.upVector.y),
        static_cast<float>(p.upVector.z));
    rp.fieldOfView = static_cast<float>(p.fieldOfView);
    rp.aspectRatio = static_cast<float>(p.aspectRatio);
    rp.nearPlane = static_cast<float>(p.nearPlane);
    rp.farPlane = static_cast<float>(p.farPlane);
    rp.orthographic = p.orthographic;
    rp.height = static_cast<float>(p.height);
    return rp;
}

static CameraParams fromRenderCameraParams(const Gui::Render::CameraParams& rp)
{
    CameraParams p;
    p.position = Base::Vector3d(rp.position.x, rp.position.y, rp.position.z);
    p.target = Base::Vector3d(rp.target.x, rp.target.y, rp.target.z);
    p.upVector = Base::Vector3d(rp.upVector.x, rp.upVector.y, rp.upVector.z);
    p.fieldOfView = rp.fieldOfView;
    p.aspectRatio = rp.aspectRatio;
    p.nearPlane = rp.nearPlane;
    p.farPlane = rp.farPlane;
    p.orthographic = rp.orthographic;
    p.height = rp.height;
    return p;
}

//===========================================================================
// Constructor / Destructor
//===========================================================================

OsgVerseViewerAdapter::OsgVerseViewerAdapter(QWidget* parent, const QOpenGLWidget* shareWidget)
    : _viewer(std::make_unique<Gui::Render::OsgVerseViewer>())
    , _pickingService(std::make_unique<Gui::Render::OsgVersePickingService>())
{
    // Connect picking service to viewer
    _pickingService->setOsgVerseViewer(_viewer.get());
}

OsgVerseViewerAdapter::~OsgVerseViewerAdapter()
{
    _pickingService.reset();
    _viewer.reset();
}

//===========================================================================
// Basic rendering (already implemented)
//===========================================================================

void OsgVerseViewerAdapter::render()
{
    if (_viewer) {
        _viewer->render();
    }
}

void OsgVerseViewerAdapter::resize(int width, int height)
{
    if (_viewer) {
        _viewer->resize(width, height);
    }
}

QWidget* OsgVerseViewerAdapter::getWidget()
{
    return _viewer ? _viewer->getWidget() : nullptr;
}

QOpenGLWidget* OsgVerseViewerAdapter::getGLWidget()
{
    if (_viewer) {
        return qobject_cast<QOpenGLWidget*>(_viewer->getWidget());
    }
    return nullptr;
}

//===========================================================================
// Scene management (Phase A.3)
//===========================================================================

void OsgVerseViewerAdapter::setSceneGraph(void* root)
{
    if (!_viewer || !root) return;

    Gui::Render::OsgVerseEngine* engine = _viewer->getEngine();
    if (!engine) return;

    // Wrap the raw osg::Node* into a RenderNode::Ptr
    osg::Node* osgNode = static_cast<osg::Node*>(root);
    Gui::Render::RenderNode::Ptr wrapped = engine->wrapNode(osgNode);
    if (wrapped) {
        _viewer->setSceneRoot(wrapped);
    }
}

void* OsgVerseViewerAdapter::getSceneGraph()
{
    if (!_viewer) return nullptr;

    Gui::Render::OsgVerseEngine* engine = _viewer->getEngine();
    if (!engine) return nullptr;

    Gui::Render::RenderNode::Ptr root = _viewer->getSceneRoot();
    if (!root) return nullptr;

    // Unwrap to get the raw osg::Node*
    return static_cast<void*>(engine->unwrapNode(root.get()));
}

void OsgVerseViewerAdapter::updateScene()
{
    if (_viewer) {
        _viewer->updateScene();
    }
}

//===========================================================================
// Camera control (Phase A.2)
//===========================================================================

void OsgVerseViewerAdapter::setCamera(const CameraParams& params)
{
    if (!_viewer) return;

    _cameraOrthographic = params.orthographic;

    Gui::Render::CameraParams rp = toRenderCameraParams(params);
    _viewer->setCamera(rp);

    // Handle orthographic switching
    if (params.orthographic) {
        osgViewer::Viewer* osgViewer = _viewer->getOsgViewer();
        if (osgViewer && osgViewer->getCamera()) {
            double halfH = params.height * 0.5;
            double halfW = halfH * params.aspectRatio;
            osgViewer->getCamera()->setProjectionMatrixAsOrtho(
                -halfW, halfW, -halfH, halfH, params.nearPlane, params.farPlane);
        }
    }
}

CameraParams OsgVerseViewerAdapter::getCamera() const
{
    if (!_viewer) return CameraParams();

    Gui::Render::CameraParams rp = _viewer->getCamera();
    CameraParams p = fromRenderCameraParams(rp);
    p.orthographic = _cameraOrthographic;
    return p;
}

void OsgVerseViewerAdapter::viewAll()
{
    if (_viewer) {
        _viewer->fitAll();
    }
}

void OsgVerseViewerAdapter::fitSelection()
{
    if (_viewer) {
        _viewer->fitSelection();
    }
}

void OsgVerseViewerAdapter::resetCamera()
{
    if (_viewer) {
        _viewer->resetCamera();
    }
}

void OsgVerseViewerAdapter::setCameraType(bool orthographic)
{
    _cameraOrthographic = orthographic;

    if (!_viewer) return;

    osgViewer::Viewer* osgViewer = _viewer->getOsgViewer();
    if (!osgViewer || !osgViewer->getCamera()) return;

    osg::Camera* camera = osgViewer->getCamera();
    const osg::Viewport* vp = camera->getViewport();
    if (!vp) return;

    double aspect = vp->width() / vp->height();

    if (orthographic) {
        // Switch to orthographic: compute height from current perspective FOV and distance
        Gui::Render::CameraParams rp = _viewer->getCamera();
        double distance = (Base::Vector3f(rp.position) - Base::Vector3f(rp.target)).Length();
        double halfH = distance * std::tan(rp.fieldOfView * M_PI / 360.0);
        double halfW = halfH * aspect;
        camera->setProjectionMatrixAsOrtho(-halfW, halfW, -halfH, halfH,
                                           rp.nearPlane, rp.farPlane);
    } else {
        // Switch to perspective
        Gui::Render::CameraParams rp = _viewer->getCamera();
        camera->setProjectionMatrixAsPerspective(
            rp.fieldOfView, aspect, rp.nearPlane, rp.farPlane);
    }
}

bool OsgVerseViewerAdapter::isCameraOrthographic() const
{
    return _cameraOrthographic;
}

//===========================================================================
// Event handling (Phase A.1)
//===========================================================================

bool OsgVerseViewerAdapter::handleMouseEvent(QMouseEvent* event)
{
    if (!_viewer) return false;

    QWidget* widget = _viewer->getWidget();
    if (!widget) return false;

    // Forward the event to the ViewerWidget which handles OSG event queue
    QApplication::sendEvent(widget, event);
    return event->isAccepted();
}

bool OsgVerseViewerAdapter::handleKeyEvent(QKeyEvent* event)
{
    if (!_viewer) return false;

    QWidget* widget = _viewer->getWidget();
    if (!widget) return false;

    QApplication::sendEvent(widget, event);
    return event->isAccepted();
}

bool OsgVerseViewerAdapter::handleWheelEvent(QWheelEvent* event)
{
    if (!_viewer) return false;

    QWidget* widget = _viewer->getWidget();
    if (!widget) return false;

    QApplication::sendEvent(widget, event);
    return event->isAccepted();
}

//===========================================================================
// Picking and selection (Phase B.1 + selection state)
//===========================================================================

PickResult OsgVerseViewerAdapter::pick(const QPoint& pos)
{
    PickResult result;

    if (!_pickingService || !_viewer) return result;

    // Use the picking service to perform the pick
    Gui::Render::PickResults pickResults = _pickingService->pick(pos.x(), pos.y());

    if (!pickResults.hasHit()) return result;

    const Gui::Render::PickResult& closest = pickResults.closest();

    result.valid = closest.hit;
    result.point = Base::Vector3d(closest.point.x, closest.point.y, closest.point.z);
    result.normal = Base::Vector3d(closest.normal.x, closest.normal.y, closest.normal.z);
    result.distance = closest.distance;
    result.viewProvider = closest.viewProvider;
    result.subElementName = closest.elementName;
    result.faceIndex = closest.faceIndex;
    result.edgeIndex = closest.edgeIndex;
    result.vertexIndex = closest.vertexIndex;

    // Convert pick type
    switch (closest.type) {
        case Gui::Render::PickType::Face:
            result.pickType = View3D::PickType::Face;
            break;
        case Gui::Render::PickType::Edge:
            result.pickType = View3D::PickType::Edge;
            break;
        case Gui::Render::PickType::Vertex:
            result.pickType = View3D::PickType::Vertex;
            break;
        default:
            result.pickType = View3D::PickType::None;
            break;
    }

    return result;
}

void OsgVerseViewerAdapter::setSelectionMode(SelectionMode mode)
{
    _selectionMode = mode;
}

SelectionMode OsgVerseViewerAdapter::getSelectionMode() const
{
    return _selectionMode;
}

void OsgVerseViewerAdapter::startSelection(SelectionMode mode)
{
    _selectionMode = mode;
    _isSelecting = true;
    if (_viewer && (mode == SelectionMode::Rectangle || mode == SelectionMode::Rubberband)) {
        _viewer->setRubberBandEnabled(true);
    }
}

void OsgVerseViewerAdapter::stopSelection()
{
    _isSelecting = false;
    if (_viewer) {
        _viewer->setRubberBandEnabled(false);
    }
}

void OsgVerseViewerAdapter::abortSelection()
{
    _isSelecting = false;
    _selectionMode = SelectionMode::None;
    if (_viewer) {
        _viewer->setRubberBandEnabled(false);
    }
}

bool OsgVerseViewerAdapter::isSelecting() const
{
    return _isSelecting;
}

//===========================================================================
// ViewProvider management
//===========================================================================

void OsgVerseViewerAdapter::addViewProvider(ViewProvider* vp)
{
    if (_viewer) {
        _viewer->addViewProvider(vp);
    }
}

void OsgVerseViewerAdapter::removeViewProvider(ViewProvider* vp)
{
    if (_viewer) {
        _viewer->removeViewProvider(vp);
    }
}

void OsgVerseViewerAdapter::updateViewProvider(ViewProvider* vp)
{
    if (_viewer) {
        _viewer->updateViewProvider(vp);
    }
}

bool OsgVerseViewerAdapter::hasViewProvider(ViewProvider* vp) const
{
    if (_viewer) {
        return _viewer->hasViewProvider(vp);
    }
    return false;
}

std::vector<Gui::ViewProvider*> OsgVerseViewerAdapter::getViewProviders() const
{
    if (_viewer) {
        return _viewer->getViewProviders();
    }
    return {};
}

//===========================================================================
// Editing mode (Phase C - basic framework)
//===========================================================================

void OsgVerseViewerAdapter::setEditingViewProvider(ViewProvider* vp, int mode)
{
    _editingViewProvider = vp;
    _editingMode = mode;

    if (_viewer) {
        _viewer->setEditingViewProvider(vp, mode);
    }
}

Gui::ViewProvider* OsgVerseViewerAdapter::getEditingViewProvider() const
{
    return _editingViewProvider;
}

bool OsgVerseViewerAdapter::isEditingViewProvider() const
{
    return _editingViewProvider != nullptr;
}

void OsgVerseViewerAdapter::resetEditingViewProvider()
{
    _editingViewProvider = nullptr;
    _editingMode = 0;

    if (_viewer) {
        _viewer->resetEditingViewProvider();
    }
}

//===========================================================================
// Phase G: 编辑模式扩展
//===========================================================================

Base::Vector3d OsgVerseViewerAdapter::getPointOnFocalPlane(int x, int y) const
{
    if (_viewer) {
        return _viewer->getPointOnFocalPlane(x, y);
    }
    return Base::Vector3d();
}

void OsgVerseViewerAdapter::setupEditingRoot(void* node, const Base::Matrix4D* mat)
{
    if (_viewer) {
        _viewer->setupEditingRoot(node, mat);
    }
}

void OsgVerseViewerAdapter::resetEditingRoot(bool updateLinks)
{
    if (_viewer) {
        _viewer->resetEditingRoot(updateLinks);
    }
}

void OsgVerseViewerAdapter::setEditingTransform(const Base::Matrix4D& mat)
{
    if (_viewer) {
        _viewer->setEditingTransform(mat);
    }
}

//===========================================================================
// Phase G: Seek
//===========================================================================

bool OsgVerseViewerAdapter::seekToPoint(int screenX, int screenY)
{
    if (_viewer) {
        return _viewer->seekToPoint(screenX, screenY);
    }
    return false;
}

void OsgVerseViewerAdapter::seekToPoint(const Base::Vector3d& worldPos)
{
    if (_viewer) {
        _viewer->seekToPoint(worldPos);
    }
}

//===========================================================================
// Phase G: Pick radius
//===========================================================================

float OsgVerseViewerAdapter::getPickRadius() const
{
    if (_viewer) {
        return _viewer->getPickRadius();
    }
    return 5.0f;
}

void OsgVerseViewerAdapter::setPickRadius(float radius)
{
    if (_viewer) {
        _viewer->setPickRadius(radius);
    }
}

//===========================================================================
// Phase H: Override mode
//===========================================================================

void OsgVerseViewerAdapter::setOverrideMode(const std::string& mode)
{
    if (_viewer) {
        _viewer->setOverrideMode(mode);
    }
}

std::string OsgVerseViewerAdapter::getOverrideMode() const
{
    if (_viewer) {
        return _viewer->getOverrideMode();
    }
    return std::string();
}

//===========================================================================
// Rendering settings (Phase A.4)
//===========================================================================

void OsgVerseViewerAdapter::setRenderMode(RenderMode mode)
{
    if (_viewer) {
        _viewer->setRenderMode(toRenderMode(mode));
    }
}

RenderMode OsgVerseViewerAdapter::getRenderMode() const
{
    if (_viewer) {
        return fromRenderMode(_viewer->getRenderMode());
    }
    return RenderMode::AsIs;
}

void OsgVerseViewerAdapter::setBackgroundColor(const Base::Color& color)
{
    if (_viewer) {
        _viewer->setBackgroundColor(color);
    }
}

Base::Color OsgVerseViewerAdapter::getBackgroundColor() const
{
    if (_viewer) {
        return _viewer->getBackgroundColor();
    }
    return Base::Color();
}

void OsgVerseViewerAdapter::setBackgroundGradient(const BackgroundGradient& gradient)
{
    _backgroundGradient = gradient;

    if (!_viewer) return;

    if (gradient.type == BackgroundGradientType::Linear) {
        _viewer->setGradientBackground(
            gradient.topColor.r, gradient.topColor.g, gradient.topColor.b,
            gradient.bottomColor.r, gradient.bottomColor.g, gradient.bottomColor.b);
    } else {
        _viewer->setBackgroundColor(gradient.bottomColor);
    }
}

BackgroundGradient OsgVerseViewerAdapter::getBackgroundGradient() const
{
    return _backgroundGradient;
}

void OsgVerseViewerAdapter::setBacklightEnabled(bool enabled)
{
    if (_viewer) {
        _viewer->setBacklightEnabled(enabled);
    }
}

bool OsgVerseViewerAdapter::isBacklightEnabled() const
{
    if (_viewer) {
        return _viewer->isBacklightEnabled();
    }
    return false;
}

void OsgVerseViewerAdapter::setAmbientIntensity(float intensity)
{
    if (_viewer) {
        _viewer->setAmbientIntensity(intensity);
    }
}

float OsgVerseViewerAdapter::getAmbientIntensity() const
{
    if (_viewer) {
        return _viewer->getAmbientIntensity();
    }
    return 0.3f;
}

//===========================================================================
// Navigation and interaction (Phase A.5 / E)
//===========================================================================

void OsgVerseViewerAdapter::setNavigationStyle(const std::string& style)
{
    _navigationStyle = style;
    if (_viewer) {
        _viewer->setNavigationStyle(style);
    }
}

std::string OsgVerseViewerAdapter::getNavigationStyle() const
{
    return _navigationStyle;
}

void OsgVerseViewerAdapter::setViewing(bool enable)
{
    _isViewing = enable;
}

bool OsgVerseViewerAdapter::isViewing() const
{
    return _isViewing;
}

//===========================================================================
// Backend info
//===========================================================================

std::string OsgVerseViewerAdapter::getBackendVersion() const
{
    return "1.0";
}

//===========================================================================
// Statistics and debugging
//===========================================================================

Gui::Render::RenderStats OsgVerseViewerAdapter::getStats() const
{
    if (_viewer) {
        return _viewer->getStats();
    }
    return Gui::Render::RenderStats();
}

void OsgVerseViewerAdapter::resetStats()
{
    if (_viewer) {
        _viewer->resetStats();
    }
}

void OsgVerseViewerAdapter::setFPSEnabled(bool enabled)
{
    _fpsEnabled = enabled;
    if (_viewer) {
        _viewer->setStatsEnabled(enabled);
    }
}

bool OsgVerseViewerAdapter::isFPSEnabled() const
{
    return _fpsEnabled;
}

//===========================================================================
// Screenshots
//===========================================================================

QImage OsgVerseViewerAdapter::grabImage(int width, int height)
{
    if (_viewer) {
        return _viewer->grabImage(width, height);
    }
    return QImage();
}

bool OsgVerseViewerAdapter::saveScreenshot(const QString& filename, int width, int height)
{
    if (_viewer) {
        return _viewer->saveScreenshot(filename, width, height);
    }
    return false;
}

void OsgVerseViewerAdapter::addEventCallback(EventType type, EventCallbackFunc cb, void* userData)
{
    if (_viewer) {
        auto osgType = static_cast<Render::OsgVerseViewer::EventType>(static_cast<int>(type));
        Render::OsgVerseViewer::EventCallbackFunc wrapper =
            [cb](Render::OsgVerseViewer::EventType et, void* ev, void* ud) -> bool {
                auto iType = static_cast<EventType>(static_cast<int>(et));
                return cb(iType, ev, ud);
            };
        _viewer->addEventCallback(osgType, wrapper, userData);
    }
}

void OsgVerseViewerAdapter::removeEventCallback(EventType type, EventCallbackFunc /*cb*/, void* userData)
{
    if (_viewer) {
        auto osgType = static_cast<Render::OsgVerseViewer::EventType>(static_cast<int>(type));
        // OsgVerseViewer::removeEventCallback matches by type + userData
        _viewer->removeEventCallback(osgType, nullptr, userData);
    }
}

//===========================================================================
// Selection Polygon
//===========================================================================

std::vector<std::pair<int,int>> OsgVerseViewerAdapter::getSelectionPolygon(bool* isClosed) const
{
    if (_viewer) {
        return _viewer->getSelectionPolygon(isClosed);
    }
    if (isClosed) *isClosed = false;
    return {};
}

std::vector<std::pair<float,float>> OsgVerseViewerAdapter::getSelectionPolygonNormalized(bool* isClosed) const
{
    if (_viewer) {
        return _viewer->getSelectionPolygonNormalized(isClosed);
    }
    if (isClosed) *isClosed = false;
    return {};
}

//===========================================================================
// Ray Picking
//===========================================================================

Base::Vector3d OsgVerseViewerAdapter::getPointOnRay(const QPoint& screenPos, const ViewProvider* vp) const
{
    if (_viewer) {
        return _viewer->getPointOnRay(screenPos, vp);
    }
    return Base::Vector3d();
}

Base::Vector3d OsgVerseViewerAdapter::getPointOnRay(const Base::Vector3d& rayOrigin,
                                                     const Base::Vector3d& rayDir,
                                                     const ViewProvider* vp) const
{
    if (_viewer) {
        return _viewer->getPointOnRay(rayOrigin, rayDir, vp);
    }
    return Base::Vector3d();
}

//===========================================================================
// Viewport on Placement Plane
//===========================================================================

Base::BoundBox2d OsgVerseViewerAdapter::getViewportOnXYPlaneOfPlacement(const Base::Placement& plc) const
{
    if (_viewer) {
        return _viewer->getViewportOnXYPlaneOfPlacement(plc);
    }
    return Base::BoundBox2d(0, 0, 0, 0);
}

//===========================================================================
// Coordinate Projection (forwarded to OsgVerseViewer)
//===========================================================================

Base::Vector3d OsgVerseViewerAdapter::getViewDirection() const
{
    if (_viewer) return _viewer->getViewDirection();
    return Base::Vector3d(0, 0, -1);
}

Base::Vector3d OsgVerseViewerAdapter::getUpDirection() const
{
    if (_viewer) return _viewer->getUpDirection();
    return Base::Vector3d(0, 1, 0);
}

QPoint OsgVerseViewerAdapter::getPointOnViewport(const Base::Vector3d& pt) const
{
    if (_viewer) return _viewer->getPointOnViewport(pt);
    return QPoint(0, 0);
}

Base::Vector3d OsgVerseViewerAdapter::getPointOnLine(const QPoint& screenPos,
                                                      const Base::Vector3d& axisCenter,
                                                      const Base::Vector3d& axis) const
{
    if (_viewer) return _viewer->getPointOnLine(screenPos, axisCenter, axis);
    return Base::Vector3d();
}

Base::Vector3d OsgVerseViewerAdapter::getPointOnXYPlaneOfPlacement(const QPoint& screenPos,
                                                                    const Base::Placement& plc) const
{
    if (_viewer) return _viewer->getPointOnXYPlaneOfPlacement(screenPos, plc);
    return Base::Vector3d();
}

void OsgVerseViewerAdapter::projectPointToLine(const QPoint& screenPos,
                                                Base::Vector3d& pt1,
                                                Base::Vector3d& pt2) const
{
    if (_viewer) {
        _viewer->projectPointToLine(screenPos, pt1, pt2);
    } else {
        pt1 = pt2 = Base::Vector3d();
    }
}

Base::Vector2d OsgVerseViewerAdapter::getNormalizedPosition(const QPoint& screenPos) const
{
    if (_viewer) return _viewer->getNormalizedPosition(screenPos);
    return Base::Vector2d(0.5, 0.5);
}

Base::Vector3d OsgVerseViewerAdapter::projectOnNearPlane(const Base::Vector2d& pt) const
{
    if (_viewer) return _viewer->projectOnNearPlane(pt);
    return Base::Vector3d();
}

Base::Vector3d OsgVerseViewerAdapter::projectOnFarPlane(const Base::Vector2d& pt) const
{
    if (_viewer) return _viewer->projectOnFarPlane(pt);
    return Base::Vector3d();
}

Base::Vector3d OsgVerseViewerAdapter::getCenterPointOnFocalPlane() const
{
    if (_viewer) return _viewer->getCenterPointOnFocalPlane();
    return Base::Vector3d();
}

void OsgVerseViewerAdapter::getNearPlane(Base::Vector3d& pt, Base::Vector3d& normal) const
{
    if (_viewer) {
        _viewer->getNearPlane(pt, normal);
    } else {
        pt = Base::Vector3d();
        normal = Base::Vector3d(0, 0, 1);
    }
}

void OsgVerseViewerAdapter::getFarPlane(Base::Vector3d& pt, Base::Vector3d& normal) const
{
    if (_viewer) {
        _viewer->getFarPlane(pt, normal);
    } else {
        pt = Base::Vector3d();
        normal = Base::Vector3d(0, 0, -1);
    }
}

void OsgVerseViewerAdapter::getDimensions(float& height, float& width) const
{
    if (_viewer) {
        _viewer->getDimensions(height, width);
    } else {
        height = width = 0;
    }
}

float OsgVerseViewerAdapter::getMaxDimension() const
{
    if (_viewer) return _viewer->getMaxDimension();
    return 0;
}

//===========================================================================
// Editing Extensions (forwarded to OsgVerseViewer)
//===========================================================================

void OsgVerseViewerAdapter::setEditing(bool edit)
{
    if (_viewer) _viewer->setEditing(edit);
}

bool OsgVerseViewerAdapter::isEditing() const
{
    if (_viewer) return _viewer->isEditing();
    return false;
}

void OsgVerseViewerAdapter::setEditingCursor(const QCursor& cursor)
{
    if (_viewer) _viewer->setEditingCursor(cursor);
}

void OsgVerseViewerAdapter::setComponentCursor(const QCursor& cursor)
{
    if (_viewer) _viewer->setComponentCursor(cursor);
}

void OsgVerseViewerAdapter::setRedirectToSceneGraph(bool redirect)
{
    if (_viewer) _viewer->setRedirectToSceneGraph(redirect);
}

bool OsgVerseViewerAdapter::isRedirectedToSceneGraph() const
{
    if (_viewer) return _viewer->isRedirectedToSceneGraph();
    return false;
}

void OsgVerseViewerAdapter::setSelectionEnabled(bool enable)
{
    if (_viewer) _viewer->setSelectionEnabled(enable);
}

bool OsgVerseViewerAdapter::isSelectionEnabled() const
{
    if (_viewer) return _viewer->isSelectionEnabled();
    return true;
}

void OsgVerseViewerAdapter::boxZoom(int x1, int y1, int x2, int y2)
{
    if (_viewer) _viewer->boxZoom(x1, y1, x2, y2);
}

void OsgVerseViewerAdapter::savePicture(int width, int height, int samples,
                                         const QColor& bg, QImage& img) const
{
    if (_viewer) {
        _viewer->savePicture(width, height, samples, bg, img);
    }
}

void OsgVerseViewerAdapter::alignToSelection()
{
    if (_viewer) _viewer->alignToSelection();
}

void OsgVerseViewerAdapter::setPopupMenuEnabled(bool on)
{
    if (_viewer) _viewer->setPopupMenuEnabled(on);
}

bool OsgVerseViewerAdapter::isPopupMenuEnabled() const
{
    if (_viewer) return _viewer->isPopupMenuEnabled();
    return true;
}

//===========================================================================
// Graphics Overlay (forwarded to OsgVerseViewer)
//===========================================================================

void OsgVerseViewerAdapter::addGraphicsItem(void* item)
{
    if (_viewer) _viewer->addGraphicsItem(item);
}

void OsgVerseViewerAdapter::removeGraphicsItem(void* item)
{
    if (_viewer) _viewer->removeGraphicsItem(item);
}

void OsgVerseViewerAdapter::clearGraphicsItems()
{
    if (_viewer) _viewer->clearGraphicsItems();
}

#endif // RENDER_HAS_OSGVERSE_BACKEND
