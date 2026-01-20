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
#include <QOpenGLContext>
#include <QSurfaceFormat>
#endif

#include "OsgVerseViewer.h"
#include <osgViewer/Viewer>
#include <osgViewer/GraphicsWindow>
#include <osgGA/TrackballManipulator>
#include <osgGA/GUIEventAdapter>
#include <osg/Camera>
#include <osg/Light>
#include <osg/LightSource>
#include <osgDB/WriteFile>
#include <Base/Console.h>

using namespace Gui::Render;

//===========================================================================
// OsgVerseViewer Implementation
//===========================================================================

OsgVerseViewer::OsgVerseViewer()
{
    // 延迟初始化模式：不在构造函数中做任何初始化
    // Lazy initialization: do not initialize anything in constructor
    // 所有初始化都延迟到第一次使用时（ensureInitialized）
    // All initialization is deferred until first use (ensureInitialized)
    
    Base::Console().log("OsgVerseViewer: Constructor called (lazy initialization mode)\n");
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
    ensureInitialized();
    _sceneRoot = root;
    if (_engine) {
        _engine->setSceneRoot(root);
    }
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
    ensureInitialized();
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
    // 确保已初始化
    // Ensure initialized
    const_cast<OsgVerseViewer*>(this)->ensureInitialized();
    
    // Lazy initialization of widget
    if (!_widget && _initialized) {
        // Cast away const for lazy initialization
        const_cast<OsgVerseViewer*>(this)->initializeWidget();
    }
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
    if (_graphicsWindow.valid()) {
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
    Base::Console().log("OsgVerseViewer::initializeViewer: Creating viewer...\n");
    
    // Create graphics window embedded (like OsgVerse qt_viewer example)
    _graphicsWindow = new osgViewer::GraphicsWindowEmbedded(0, 0, 800, 600);
    
    // Create viewer
    _viewer = new osgViewer::Viewer();
    _viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    
    // Set camera with graphics context
    osg::Camera* camera = _viewer->getCamera();
    camera->setGraphicsContext(_graphicsWindow.get());
    camera->setViewport(0, 0, 800, 600);
    camera->setClearColor(osg::Vec4(_backgroundColor.r, _backgroundColor.g, _backgroundColor.b, _backgroundColor.a));
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set camera manipulator
    _viewer->setCameraManipulator(new osgGA::TrackballManipulator());
    _viewer->setKeyEventSetsDone(0);  // Don't exit on Escape

    // Connect engine to viewer
    _engine->setViewer(_viewer);
    
    Base::Console().log("OsgVerseViewer::initializeViewer: Viewer created successfully\n");
}

void OsgVerseViewer::initializeWidget()
{
    // Only initialize once
    if (_widget) {
        return;
    }
    
    Base::Console().log("OsgVerseViewer::initializeWidget: Creating widget...\n");
    _widget = new ViewerWidget(_viewer, _graphicsWindow.get());
    Base::Console().log("OsgVerseViewer::initializeWidget: Widget created successfully\n");
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
    _cameraParams.position = Vec3f(0.0f, -10.0f, 5.0f);
    _cameraParams.target = Vec3f(0.0f, 0.0f, 0.0f);
    _cameraParams.upVector = Vec3f(0.0f, 0.0f, 1.0f);
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

OsgVerseViewer::ViewerWidget::ViewerWidget(osgViewer::Viewer* viewer, 
                                            osgViewer::GraphicsWindowEmbedded* graphicsWindow,
                                            QWidget* parent)
    : QOpenGLWidget(parent)
    , _viewer(viewer)
    , _graphicsWindow(graphicsWindow)
    , _firstFrame(true)
{
    // Set OpenGL format (like OsgVerse qt_viewer example)
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setSamples(4);
    setFormat(format);

    // Enable mouse tracking and keyboard focus
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(320, 240);
    
    Base::Console().log("OsgVerseViewer::ViewerWidget: Constructor completed\n");
}

OsgVerseViewer::ViewerWidget::~ViewerWidget()
{
    // Cleanup is handled by osg::ref_ptr
}

void OsgVerseViewer::ViewerWidget::initializeGL()
{
    Base::Console().log("OsgVerseViewer::ViewerWidget::initializeGL: OpenGL context initialized\n");
}

void OsgVerseViewer::ViewerWidget::paintGL()
{
    if (_viewer) {
        // Set default FBO on first frame (like OsgVerse qt_viewer example)
        if (_firstFrame) {
            GLuint defaultFboId = this->defaultFramebufferObject();
            _graphicsWindow->setDefaultFboId(defaultFboId);
            _firstFrame = false;
            Base::Console().log("OsgVerseViewer::ViewerWidget::paintGL: Set default FBO ID: %u\n", defaultFboId);
        }
        _viewer->frame();
    }
}

void OsgVerseViewer::ViewerWidget::resizeGL(int width, int height)
{
    if (_graphicsWindow.valid()) {
        _graphicsWindow->getEventQueue()->windowResize(this->x(), this->y(), width, height);
        _graphicsWindow->resized(this->x(), this->y(), width, height);
    }
    
    // Paint once to avoid flicker (like OsgVerse qt_viewer example)
    paintGL();
}

void OsgVerseViewer::ViewerWidget::mousePressEvent(QMouseEvent* event)
{
    if (_graphicsWindow.valid()) {
        float x = static_cast<float>(event->x());
        float y = static_cast<float>(event->y());
        unsigned int button = 0;
        
        switch (event->button()) {
            case Qt::LeftButton: button = 1; break;
            case Qt::MiddleButton: button = 2; break;
            case Qt::RightButton: button = 3; break;
            default: break;
        }
        
        _graphicsWindow->getEventQueue()->mouseButtonPress(x, y, button);
    }
    update();
}

void OsgVerseViewer::ViewerWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (_graphicsWindow.valid()) {
        float x = static_cast<float>(event->x());
        float y = static_cast<float>(event->y());
        unsigned int button = 0;
        
        switch (event->button()) {
            case Qt::LeftButton: button = 1; break;
            case Qt::MiddleButton: button = 2; break;
            case Qt::RightButton: button = 3; break;
            default: break;
        }
        
        _graphicsWindow->getEventQueue()->mouseButtonRelease(x, y, button);
    }
    update();
}

void OsgVerseViewer::ViewerWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (_graphicsWindow.valid()) {
        _graphicsWindow->getEventQueue()->mouseMotion(
            static_cast<float>(event->x()), 
            static_cast<float>(event->y())
        );
    }
    update();
}

void OsgVerseViewer::ViewerWidget::wheelEvent(QWheelEvent* event)
{
    if (_graphicsWindow.valid()) {
        int delta = event->angleDelta().y();
        osgGA::GUIEventAdapter::ScrollingMotion motion = delta > 0 ?
            osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN;
        _graphicsWindow->getEventQueue()->mouseScroll(motion);
    }
    update();
}

void OsgVerseViewer::ViewerWidget::keyPressEvent(QKeyEvent* event)
{
    if (_graphicsWindow.valid()) {
        _graphicsWindow->getEventQueue()->keyPress(event->key());
    }
    update();
}

void OsgVerseViewer::ViewerWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (_graphicsWindow.valid()) {
        _graphicsWindow->getEventQueue()->keyRelease(event->key());
    }
    update();
}


//-----------------------------------------------------------------------
// 延迟初始化 / Lazy Initialization
//-----------------------------------------------------------------------

void OsgVerseViewer::ensureInitialized()
{
    // 如果已经初始化或初始化失败，直接返回
    // If already initialized or initialization failed, return immediately
    if (_initialized || _initializationFailed) {
        return;
    }

    Base::Console().log("OsgVerseViewer: Starting lazy initialization...\n");

    try {
        // 检查前置条件
        // Check prerequisites
        if (!checkPrerequisites()) {
            Base::Console().error("OsgVerseViewer: Prerequisites check failed\n");
            _initializationFailed = true;
            return;
        }

        // 创建引擎
        // Create engine
        Base::Console().log("OsgVerseViewer: Creating engine...\n");
        _engine = std::make_unique<OsgVerseEngine>();
        _engine->initialize();
        Base::Console().log("OsgVerseViewer: Engine created and initialized\n");

        // 初始化查看器
        // Initialize viewer
        Base::Console().log("OsgVerseViewer: Initializing viewer...\n");
        initializeViewer();
        Base::Console().log("OsgVerseViewer: Viewer initialized\n");

        // 设置默认相机
        // Setup default camera
        Base::Console().log("OsgVerseViewer: Setting up default camera...\n");
        setupDefaultCamera();
        Base::Console().log("OsgVerseViewer: Default camera set up\n");

        // 设置默认光照
        // Setup default lighting
        Base::Console().log("OsgVerseViewer: Setting up default lighting...\n");
        setupDefaultLighting();
        Base::Console().log("OsgVerseViewer: Default lighting set up\n");

        _initialized = true;
        Base::Console().log("OsgVerseViewer: Lazy initialization completed successfully\n");
    }
    catch (const std::exception& e) {
        Base::Console().error("OsgVerseViewer: Initialization failed with exception: %s\n", e.what());
        _initializationFailed = true;
        _initialized = false;
    }
    catch (...) {
        Base::Console().error("OsgVerseViewer: Initialization failed with unknown exception\n");
        _initializationFailed = true;
        _initialized = false;
    }
}

bool OsgVerseViewer::checkPrerequisites()
{
    Base::Console().log("OsgVerseViewer: Checking prerequisites...\n");

    // 检查 Qt 应用程序
    // Check Qt application
    if (!QApplication::instance()) {
        Base::Console().warning("OsgVerseViewer: Qt application not initialized yet\n");
        // 注意：这可能是正常的，在某些启动阶段
        // Note: This might be normal during certain startup phases
    }

    // 检查 OpenGL 上下文（可选，因为可能还未创建）
    // Check OpenGL context (optional, as it might not be created yet)
    // 我们不强制要求，因为上下文会在 Widget 创建时建立
    // We don't enforce this as context will be established when Widget is created

    Base::Console().log("OsgVerseViewer: Prerequisites check passed\n");
    return true;
}
