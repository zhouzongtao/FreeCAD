// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#include "OsgVerseWidget.h"
#include <Base/Console.h>

// OSG includes
#include <osgViewer/Viewer>
#include <osgViewer/GraphicsWindow>
#include <osg/Camera>

// Qt includes
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>

using namespace OsgVerseGui;

OsgVerseWidget::OsgVerseWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    try {
        // Set OpenGL format
        QSurfaceFormat format;
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
        format.setSamples(4); // Anti-aliasing
        format.setVersion(3, 3);
        format.setProfile(QSurfaceFormat::CoreProfile);
        setFormat(format);
        
        // Enable mouse tracking for hover events
        setMouseTracking(true);
        
        // Enable keyboard focus
        setFocusPolicy(Qt::StrongFocus);
        
        // Create OSG viewer immediately (not in initializeGL)
        // This allows getViewer() to work before the widget is shown
        _viewer = new osgViewer::Viewer();
        
        // Create graphics window traits
        osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
        traits->x = 0;
        traits->y = 0;
        traits->width = 640;  // Default size
        traits->height = 480;
        traits->windowDecoration = false;
        traits->doubleBuffer = true;
        traits->sharedContext = nullptr;
        
        // Create embedded graphics window
        _graphicsWindow = new osgViewer::GraphicsWindowEmbedded(traits.get());
        
        // Set up camera
        osg::Camera* camera = _viewer->getCamera();
        camera->setGraphicsContext(_graphicsWindow.get());
        camera->setViewport(0, 0, 640, 480);
        camera->setProjectionMatrixAsPerspective(30.0, 640.0/480.0, 1.0, 1000.0);
        camera->setClearColor(osg::Vec4(0.2, 0.2, 0.3, 1.0));
        
        // Set threading model
        _viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
        
        // DON'T call realize() here - it needs OpenGL context
        // Will be called in initializeGL()
    }
    catch (const std::exception& e) {
        Base::Console().error("OsgVerseWidget: Exception in constructor: %s\n", e.what());
        _viewer = nullptr;
        _graphicsWindow = nullptr;
        throw;
    }
    catch (...) {
        Base::Console().error("OsgVerseWidget: Unknown exception in constructor\n");
        _viewer = nullptr;
        _graphicsWindow = nullptr;
        throw;
    }
}

OsgVerseWidget::~OsgVerseWidget()
{
    // Cleanup is handled by osg::ref_ptr
    _viewer = nullptr;
    _graphicsWindow = nullptr;
}

void OsgVerseWidget::initializeGL()
{
    // Viewer is already created in constructor
    // Now realize it (needs OpenGL context)
    if (_viewer.valid() && !_viewer->isRealized()) {
        try {
            _viewer->realize();
        }
        catch (const std::exception& e) {
            Base::Console().error("OsgVerseWidget: Failed to realize viewer: %s\n", e.what());
        }
    }
    
    // Update the viewport to match actual widget size
    if (_graphicsWindow.valid()) {
        _graphicsWindow->resized(0, 0, width(), height());
    }
    
    if (_viewer.valid()) {
        osg::Camera* camera = _viewer->getCamera();
        camera->setViewport(0, 0, width(), height());
        
        // Update projection matrix with actual aspect ratio
        double aspectRatio = static_cast<double>(width()) / static_cast<double>(height());
        camera->setProjectionMatrixAsPerspective(30.0, aspectRatio, 1.0, 1000.0);
    }
}

void OsgVerseWidget::resizeGL(int width, int height)
{
    // Notify graphics window of resize
    if (_graphicsWindow.valid()) {
        _graphicsWindow->resized(0, 0, width, height);
    }
    
    // Update camera viewport and projection
    if (_viewer.valid()) {
        osg::Camera* camera = _viewer->getCamera();
        camera->setViewport(0, 0, width, height);
        
        // Update projection matrix to maintain aspect ratio
        double aspectRatio = static_cast<double>(width) / static_cast<double>(height);
        camera->setProjectionMatrixAsPerspective(30.0, aspectRatio, 1.0, 1000.0);
    }
}

void OsgVerseWidget::paintGL()
{
    // Render one frame
    if (_viewer.valid()) {
        _viewer->frame();
    }
}

//===========================================================================
// Event handlers (Phase 2 - stub implementations)
//===========================================================================

void OsgVerseWidget::mousePressEvent(QMouseEvent* event)
{
    // TODO: Phase 2 - Implement mouse press handling
    // For now, just pass to base class
    QOpenGLWidget::mousePressEvent(event);
    
    // Trigger repaint
    update();
}

void OsgVerseWidget::mouseMoveEvent(QMouseEvent* event)
{
    // TODO: Phase 2 - Implement mouse move handling
    QOpenGLWidget::mouseMoveEvent(event);
    
    // Trigger repaint
    update();
}

void OsgVerseWidget::mouseReleaseEvent(QMouseEvent* event)
{
    // TODO: Phase 2 - Implement mouse release handling
    QOpenGLWidget::mouseReleaseEvent(event);
    
    // Trigger repaint
    update();
}

void OsgVerseWidget::wheelEvent(QWheelEvent* event)
{
    // TODO: Phase 2 - Implement mouse wheel handling (zoom)
    QOpenGLWidget::wheelEvent(event);
    
    // Trigger repaint
    update();
}

void OsgVerseWidget::keyPressEvent(QKeyEvent* event)
{
    // TODO: Phase 2 - Implement keyboard handling
    QOpenGLWidget::keyPressEvent(event);
    
    // Trigger repaint
    update();
}

void OsgVerseWidget::keyReleaseEvent(QKeyEvent* event)
{
    // TODO: Phase 2 - Implement keyboard handling
    QOpenGLWidget::keyReleaseEvent(event);
    
    // Trigger repaint
    update();
}
