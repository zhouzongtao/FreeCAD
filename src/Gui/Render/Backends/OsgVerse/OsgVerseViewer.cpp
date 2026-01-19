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
#ifndef _PreComp_
#include <QImage>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#endif

#include "OsgVerseViewer.h"
#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>
#include <osgQt/GraphicsWindowQt>
#include <osg/Camera>
#include <osg/Light>
#include <osg/LightSource>
#include <osgDB/WriteFile>

using namespace Gui::Render;

//===========================================================================
// OsgVerseViewer Implementation
//===========================================================================

OsgVerseViewer::OsgVerseViewer()
{
    // Create engine
    _engine = std::make_unique<OsgVerseEngine>();
    _engine->initialize();

    // Initialize viewer
    initializeViewer();
    initializeWidget();
    setupDefaultCamera();
    setupDefaultLighting();
}

OsgVerseViewer::~OsgVerseViewer()
{
    if (_viewer) {
        _viewer->setDone(true);
        delete _viewer;
        _viewer = nullptr;
    }

    if (_widget) {
        delete _widget;
        _widget = nullptr;
    }
}

//-----------------------------------------------------------------------
// Scene Graph Management
//-----------------------------------------------------------------------

void OsgVerseViewer::setSceneRoot(RenderNode::Ptr root)
{
    _sceneRoot = root;
    _engine->setSceneRoot(root);
}

RenderNode::Ptr OsgVerseViewer::getSceneRoot() const
{
    return _sceneRoot;
}

void OsgVerseViewer::updateScene()
{
    _engine->updateScene();
    if (_widget) {
        _widget->update();
    }
}

//-----------------------------------------------------------------------
// Rendering Control
//-----------------------------------------------------------------------

void OsgVerseViewer::render()
{
    if (_viewer) {
        _viewer->frame();
    }
}

void OsgVerseViewer::setRenderMode(RenderMode mode)
{
    _renderMode = mode;
    _engine->setRenderMode(mode);
}

RenderMode OsgVerseViewer::getRenderMode() const
{
    return _renderMode;
}

void OsgVerseViewer::setBackgroundColor(const Color& color)
{
    _backgroundColor = color;
    _engine->setBackgroundColor(color);

    if (_viewer) {
        osg::Camera* camera = _viewer->getCamera();
        if (camera) {
            camera->setClearColor(osg::Vec4(color.r, color.g, color.b, color.a));
        }
    }
}

Color OsgVerseViewer::getBackgroundColor() const
{
    return _backgroundColor;
}

//-----------------------------------------------------------------------
// Camera Control
//-----------------------------------------------------------------------

void OsgVerseViewer::setCamera(const CameraParams& params)
{
    _cameraParams = params;

    if (_viewer) {
        osg::Camera* camera = _viewer->getCamera();
        if (camera) {
            // Set projection matrix
            double fovy = params.fieldOfView;
            double aspectRatio = params.aspectRatio;
            double nearPlane = params.nearPlane;
            double farPlane = params.farPlane;

            camera->setProjectionMatrixAsPerspective(fovy, aspectRatio, nearPlane, farPlane);

            // Set view matrix
            osg::Vec3d eye(params.position.x, params.position.y, params.position.z);
            osg::Vec3d center(params.target.x, params.target.y, params.target.z);
            osg::Vec3d up(params.upVector.x, params.upVector.y, params.upVector.z);

            camera->setViewMatrixAsLookAt(eye, center, up);
        }
    }
}

CameraParams OsgVerseViewer::getCamera() const
{
    return _cameraParams;
}

void OsgVerseViewer::resetCamera()
{
    if (_viewer) {
        osgGA::CameraManipulator* manipulator = _viewer->getCameraManipulator();
        if (manipulator) {
            manipulator->home(0.0);
        }
    }
}

void OsgVerseViewer::fitAll()
{
    if (_viewer) {
        _viewer->home();
    }
}

void OsgVerseViewer::fitSelection()
{
    // TODO: Implement fit to selection
    fitAll();
}

//-----------------------------------------------------------------------
// Light Control
//-----------------------------------------------------------------------

void OsgVerseViewer::setAmbientIntensity(float intensity)
{
    _ambientIntensity = intensity;
    // TODO: Update scene lighting
}

float OsgVerseViewer::getAmbientIntensity() const
{
    return _ambientIntensity;
}

void OsgVerseViewer::setBacklightEnabled(bool enabled)
{
    _backlightEnabled = enabled;
    // TODO: Update backlight
}

bool OsgVerseViewer::isBacklightEnabled() const
{
    return _backlightEnabled;
}

//-----------------------------------------------------------------------
// Window Integration
//-----------------------------------------------------------------------

QWidget* OsgVerseViewer::getWidget() const
{
    return _widget;
}

void OsgVerseViewer::resize(int width, int height)
{
    if (_widget) {
        _widget->resize(width, height);
    }
}

void OsgVerseViewer::onResize(int width, int height)
{
    if (_graphicsWindow) {
        _graphicsWindow->resized(0, 0, width, height);
    }

    if (_viewer) {
        osg::Camera* camera = _viewer->getCamera();
        if (camera) {
            double aspectRatio = static_cast<double>(width) / static_cast<double>(height);
            camera->setProjectionMatrixAsPerspective(
                _cameraParams.fieldOfView,
                aspectRatio,
                _cameraParams.nearPlane,
                _cameraParams.farPlane
            );
        }
    }
}

//-----------------------------------------------------------------------
// Screenshot Functionality
//-----------------------------------------------------------------------

QImage OsgVerseViewer::grabImage(int width, int height)
{
    if (!_widget) {
        return QImage();
    }

    // Render to image
    QImage image = _widget->grabFramebuffer();

    if (width > 0 && height > 0 && (image.width() != width || image.height() != height)) {
        image = image.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    return image;
}

bool OsgVerseViewer::saveScreenshot(const QString& filename, int width, int height)
{
    QImage image = grabImage(width, height);
    if (image.isNull()) {
        return false;
    }

    return image.save(filename);
}

//-----------------------------------------------------------------------
// Statistics
//-----------------------------------------------------------------------

RenderStats OsgVerseViewer::getStats() const
{
    return _engine->getStats();
}

void OsgVerseViewer::resetStats() const
{
    _engine->resetStats();
}

//-----------------------------------------------------------------------
// Compatibility Interface
//-----------------------------------------------------------------------

void* OsgVerseViewer::getNativePointer() const
{
    return _viewer;
}

void OsgVerseViewer::setEventCallback(EventCallback callback)
{
    _eventCallback = std::move(callback);
}

//-----------------------------------------------------------------------
// Viewer Modes
//-----------------------------------------------------------------------

RenderViewer::ViewerMode OsgVerseViewer::getViewerMode() const
{
    return _viewerMode;
}

void OsgVerseViewer::setViewerMode(ViewerMode mode)
{
    _viewerMode = mode;
}

bool OsgVerseViewer::isAnimating() const
{
    return _isAnimating;
}

void OsgVerseViewer::stopAnimation()
{
    _isAnimating = false;
}

//-----------------------------------------------------------------------
// OsgVerse-specific Interface
//-----------------------------------------------------------------------

void OsgVerseViewer::setCameraManipulator(void* manipulator)
{
    if (_viewer && manipulator) {
        _viewer->setCameraManipulator(static_cast<osgGA::CameraManipulator*>(manipulator));
    }
}

void OsgVerseViewer::setStatsEnabled(bool enabled)
{
    _statsEnabled = enabled;
    if (_viewer) {
        _viewer->getViewerStats()->collectStats("frame_rate", enabled);
    }
}

//-----------------------------------------------------------------------
// Private Methods
//-----------------------------------------------------------------------

void OsgVerseViewer::initializeViewer()
{
    _viewer = new osgViewer::Viewer();
    _viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);

    // Set camera manipulator
    _viewer->setCameraManipulator(new osgGA::TrackballManipulator());

    // Connect engine to viewer
    _engine->setViewer(_viewer);
}

void OsgVerseViewer::initializeWidget()
{
    _widget = new ViewerWidget(_viewer);
    _graphicsWindow = _widget->getGraphicsWindow();
}

void OsgVerseViewer::setupDefaultCamera()
{
    if (!_viewer) {
        return;
    }

    osg::Camera* camera = _viewer->getCamera();
    if (!camera) {
        return;
    }

    // Set default camera parameters
    _cameraParams.position = {0.0f, -10.0f, 5.0f};
    _cameraParams.target = {0.0f, 0.0f, 0.0f};
    _cameraParams.upVector = {0.0f, 0.0f, 1.0f};
    _cameraParams.fieldOfView = 45.0f;
    _cameraParams.aspectRatio = 1.333f;
    _cameraParams.nearPlane = 0.1f;
    _cameraParams.farPlane = 1000.0f;

    // Apply camera parameters
    camera->setProjectionMatrixAsPerspective(
        _cameraParams.fieldOfView,
        _cameraParams.aspectRatio,
        _cameraParams.nearPlane,
        _cameraParams.farPlane
    );

    camera->setClearColor(osg::Vec4(_backgroundColor.r, _backgroundColor.g, _backgroundColor.b, _backgroundColor.a));
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OsgVerseViewer::setupDefaultLighting()
{
    // TODO: Setup default scene lighting
}

//===========================================================================
// ViewerWidget Implementation
//===========================================================================

OsgVerseViewer::ViewerWidget::ViewerWidget(osgViewer::Viewer* viewer, QWidget* parent)
    : QOpenGLWidget(parent)
    , _viewer(viewer)
{
    // Create graphics window
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits();
    traits->windowDecoration = false;
    traits->x = 0;
    traits->y = 0;
    traits->width = 800;
    traits->height = 600;
    traits->doubleBuffer = true;
    traits->samples = 4;

    _graphicsWindow = new osgQt::GraphicsWindowQt(traits.get());

    // Set graphics context
    if (_viewer) {
        osg::Camera* camera = _viewer->getCamera();
        if (camera) {
            camera->setGraphicsContext(_graphicsWindow);
            camera->setViewport(0, 0, traits->width, traits->height);
        }
    }

    // Enable mouse tracking
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

OsgVerseViewer::ViewerWidget::~ViewerWidget()
{
}

void OsgVerseViewer::ViewerWidget::paintEvent(QPaintEvent* event)
{
    if (_viewer) {
        _viewer->frame();
    }
}

void OsgVerseViewer::ViewerWidget::resizeEvent(QResizeEvent* event)
{
    QOpenGLWidget::resizeEvent(event);

    if (_graphicsWindow) {
        _graphicsWindow->resized(0, 0, width(), height());
    }
}

void OsgVerseViewer::ViewerWidget::mousePressEvent(QMouseEvent* event)
{
    if (_graphicsWindow) {
        _graphicsWindow->getEventQueue()->mouseButtonPress(event->x(), event->y(), event->button());
    }
}

void OsgVerseViewer::ViewerWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (_graphicsWindow) {
        _graphicsWindow->getEventQueue()->mouseButtonRelease(event->x(), event->y(), event->button());
    }
}

void OsgVerseViewer::ViewerWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (_graphicsWindow) {
        _graphicsWindow->getEventQueue()->mouseMotion(event->x(), event->y());
    }
}

void OsgVerseViewer::ViewerWidget::wheelEvent(QWheelEvent* event)
{
    if (_graphicsWindow) {
        int delta = event->angleDelta().y();
        osgGA::GUIEventAdapter::ScrollingMotion motion = delta > 0 ?
            osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN;
        _graphicsWindow->getEventQueue()->mouseScroll(motion);
    }
}

void OsgVerseViewer::ViewerWidget::keyPressEvent(QKeyEvent* event)
{
    if (_graphicsWindow) {
        _graphicsWindow->getEventQueue()->keyPress(event->key());
    }
}

void OsgVerseViewer::ViewerWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (_graphicsWindow) {
        _graphicsWindow->getEventQueue()->keyRelease(event->key());
    }
}
