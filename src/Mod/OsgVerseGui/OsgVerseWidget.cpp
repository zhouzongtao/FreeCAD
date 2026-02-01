// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#ifndef _PreComp_
#include <App/Document.h>
#endif

#include "OsgVerseWidget.h"
#include "OsgVerseViewer.h"
#include "OsgVerseNaviCube.h"
#include <Base/Console.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProviderDocumentObject.h>

// OSG includes
#include <osgViewer/Viewer>
#include <osgViewer/GraphicsWindow>
#include <osg/Camera>
#include <osgGA/TrackballManipulator>
#include <osgGA/OrbitManipulator>
#include <osgGA/TerrainManipulator>
#include <osgGA/GUIEventAdapter>

// Qt includes
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QGuiApplication>

using namespace OsgVerseGui;

OsgVerseWidget::OsgVerseWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    try {
        // Set OpenGL format (降级到 OpenGL 2.1 兼容性模式以支持 OSG)
        QSurfaceFormat format;
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
        format.setSamples(4); // Anti-aliasing
        format.setVersion(2, 1);  // OpenGL 2.1 (OSG 需要)
        format.setProfile(QSurfaceFormat::CompatibilityProfile);  // 兼容性模式
        format.setRenderableType(QSurfaceFormat::OpenGL);
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
        
        // Setup camera manipulator (Phase 2)
        osg::ref_ptr<osgGA::TrackballManipulator> manipulator = new osgGA::TrackballManipulator();
        manipulator->setAllowThrow(false);  // Disable momentum
        // Don't fix vertical axis to avoid gimbal lock when viewing from top/bottom
        // manipulator->setVerticalAxisFixed(true);
        _viewer->setCameraManipulator(manipulator.get());
        
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
    Base::Console().warning("OsgVerseWidget::initializeGL called, widget size: %d x %d\n", 
                           width(), height());
    
    // Viewer is already created in constructor
    // Now realize it (needs OpenGL context)
    if (_viewer.valid() && !_viewer->isRealized()) {
        try {
            _viewer->realize();
            Base::Console().warning("OsgVerseWidget: Viewer realized successfully\n");
        }
        catch (const std::exception& e) {
            Base::Console().error("OsgVerseWidget: Failed to realize viewer: %s\n", e.what());
        }
    }
    
    // Get actual pixel size (considering high DPI)
    qreal dpr = devicePixelRatio();
    int pixelWidth = width() * dpr;
    int pixelHeight = height() * dpr;
    
    Base::Console().warning("OsgVerseWidget: Device pixel ratio: %.2f, pixel size: %d x %d\n",
                           dpr, pixelWidth, pixelHeight);
    
    // Update the viewport to match actual widget size
    if (_graphicsWindow.valid()) {
        _graphicsWindow->getEventQueue()->windowResize(0, 0, pixelWidth, pixelHeight);
        _graphicsWindow->resized(0, 0, pixelWidth, pixelHeight);
        Base::Console().warning("OsgVerseWidget: GraphicsWindow resized to %d x %d\n",
                               pixelWidth, pixelHeight);
    }
    
    if (_viewer.valid()) {
        osg::Camera* camera = _viewer->getCamera();
        camera->setViewport(0, 0, pixelWidth, pixelHeight);
        
        // Update projection matrix with actual aspect ratio
        double aspectRatio = static_cast<double>(pixelWidth) / static_cast<double>(pixelHeight);
        camera->setProjectionMatrixAsPerspective(30.0, aspectRatio, 1.0, 1000.0);
        
        Base::Console().warning("OsgVerseWidget: Camera viewport set to %d x %d, aspect: %.2f\n",
                               pixelWidth, pixelHeight, aspectRatio);
    }
}

void OsgVerseWidget::resizeGL(int width, int height)
{
    Base::Console().warning("OsgVerseWidget::resizeGL called: %d x %d\n", width, height);
    
    // Get actual pixel size (considering high DPI)
    qreal dpr = devicePixelRatio();
    int pixelWidth = width * dpr;
    int pixelHeight = height * dpr;
    
    // Notify graphics window of resize with window resize event
    if (_graphicsWindow.valid()) {
        _graphicsWindow->getEventQueue()->windowResize(0, 0, pixelWidth, pixelHeight);
        _graphicsWindow->resized(0, 0, pixelWidth, pixelHeight);
    }
    
    // Update camera viewport and projection
    if (_viewer.valid()) {
        osg::Camera* camera = _viewer->getCamera();
        camera->setViewport(0, 0, pixelWidth, pixelHeight);
        
        // Update projection matrix to maintain aspect ratio
        double aspectRatio = static_cast<double>(pixelWidth) / static_cast<double>(pixelHeight);
        camera->setProjectionMatrixAsPerspective(30.0, aspectRatio, 1.0, 1000.0);
        
        Base::Console().warning("OsgVerseWidget: Viewport updated to %d x %d (DPR: %.2f)\n", 
                               pixelWidth, pixelHeight, dpr);
    }
    
    // Force update
    update();
}

void OsgVerseWidget::paintGL()
{
    // Render one frame (main scene)
    if (_viewer.valid()) {
        _viewer->frame();
    }

    // Draw NaviCube AFTER main scene (so it appears on top)
    if (_osgVerseViewer) {
        OsgVerseNaviCube* naviCube = _osgVerseViewer->getNaviCube();
        if (naviCube && naviCube->isEnabled()) {
            naviCube->draw();
        }
    }
}

//===========================================================================
// Helper methods for event conversion (Phase 2)
//===========================================================================

int OsgVerseWidget::qtButtonToOsg(Qt::MouseButton button)
{
    switch (button) {
        case Qt::LeftButton:
            return 1;
        case Qt::MiddleButton:
            return 2;
        case Qt::RightButton:
            return 3;
        default:
            return 0;
    }
}

int OsgVerseWidget::qtKeyToOsg(int key)
{
    // Most Qt keys map directly to OSG keys
    // Special keys may need conversion, but for now direct mapping works
    return key;
}

unsigned int OsgVerseWidget::getButtonMask()
{
    unsigned int mask = 0;
    Qt::MouseButtons buttons = QGuiApplication::mouseButtons();
    
    if (buttons & Qt::LeftButton) {
        mask |= 1 << 0;  // Left button
    }
    if (buttons & Qt::MiddleButton) {
        mask |= 1 << 1;  // Middle button
    }
    if (buttons & Qt::RightButton) {
        mask |= 1 << 2;  // Right button
    }
    
    return mask;
}

//===========================================================================
// Event handlers (Phase 2)
//===========================================================================

void OsgVerseWidget::mousePressEvent(QMouseEvent* event)
{
    // Check NaviCube first - it has priority for mouse events
    if (_osgVerseViewer) {
        OsgVerseNaviCube* naviCube = _osgVerseViewer->getNaviCube();
        if (naviCube && naviCube->isEnabled() && naviCube->handleMouseEvent(event)) {
            event->accept();
            update();
            return;
        }
    }

    // Check if we're in selection mode
    if (_osgVerseViewer && _osgVerseViewer->isSelecting() && event->button() == Qt::LeftButton) {
        // Start selection
        _osgVerseViewer->setSelectionStart(event->pos());
        event->accept();
        return;
    }

    // Track mouse press for click detection
    if (event->button() == Qt::LeftButton) {
        _mousePressPos = event->pos();
        _mousePressed = true;
    }

    if (_graphicsWindow.valid()) {
        int button = qtButtonToOsg(event->button());
        _graphicsWindow->getEventQueue()->mouseButtonPress(
            event->x(), event->y(), button
        );
    }

    // Trigger repaint
    update();
}

void OsgVerseWidget::mouseMoveEvent(QMouseEvent* event)
{
    // Check NaviCube first for hover effects
    if (_osgVerseViewer) {
        OsgVerseNaviCube* naviCube = _osgVerseViewer->getNaviCube();
        if (naviCube && naviCube->isEnabled()) {
            // Always let NaviCube process move events for hover highlighting
            naviCube->handleMouseEvent(event);
        }
    }

    // Check if we're in selection mode and left button is pressed
    if (_osgVerseViewer && _osgVerseViewer->isSelecting() && (event->buttons() & Qt::LeftButton)) {
        // Update selection
        _osgVerseViewer->updateSelectionEnd(event->pos());
        event->accept();
        return;
    }

    if (_graphicsWindow.valid()) {
        _graphicsWindow->getEventQueue()->mouseMotion(
            event->x(), event->y()
        );
    }

    // Trigger repaint
    update();
}

void OsgVerseWidget::mouseReleaseEvent(QMouseEvent* event)
{
    // Check NaviCube first - it has priority for mouse events
    if (_osgVerseViewer) {
        OsgVerseNaviCube* naviCube = _osgVerseViewer->getNaviCube();
        if (naviCube && naviCube->isEnabled() && naviCube->handleMouseEvent(event)) {
            event->accept();
            _mousePressed = false;  // Reset press state
            update();
            return;
        }
    }

    // Check if we're in selection mode
    if (_osgVerseViewer && _osgVerseViewer->isSelecting() && event->button() == Qt::LeftButton) {
        // Finish selection
        _osgVerseViewer->finishSelection();
        event->accept();
        update();
        return;
    }

    // Check if this was a click (not a drag) on left button
    if (event->button() == Qt::LeftButton && _mousePressed) {
        _mousePressed = false;
        // Calculate distance from press to release
        QPoint delta = event->pos() - _mousePressPos;
        int distance = delta.manhattanLength();
        // If the mouse moved less than 5 pixels, treat it as a click
        if (distance < 5) {
            handleSingleClickSelection(event->pos());
        }
    }

    if (_graphicsWindow.valid()) {
        int button = qtButtonToOsg(event->button());
        _graphicsWindow->getEventQueue()->mouseButtonRelease(
            event->x(), event->y(), button
        );
    }

    // Trigger repaint
    update();
}

void OsgVerseWidget::wheelEvent(QWheelEvent* event)
{
    if (_graphicsWindow.valid()) {
        // Determine scroll direction
        osgGA::GUIEventAdapter::ScrollingMotion motion =
            event->angleDelta().y() > 0 ?
            osgGA::GUIEventAdapter::SCROLL_UP :
            osgGA::GUIEventAdapter::SCROLL_DOWN;
        
        _graphicsWindow->getEventQueue()->mouseScroll(motion);
    }
    
    // Accept the event
    event->accept();
    
    // Trigger repaint
    update();
}

void OsgVerseWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    // Check NaviCube first - ignore double clicks on NaviCube
    if (_osgVerseViewer) {
        OsgVerseNaviCube* naviCube = _osgVerseViewer->getNaviCube();
        if (naviCube && naviCube->isEnabled()) {
            // Check if double click is on NaviCube area - if so, ignore it
            // This prevents mouse stickiness issues
            if (naviCube->handleMouseEvent(event)) {
                event->accept();
                update();
                return;
            }
        }
    }

    if (_graphicsWindow.valid()) {
        int button = qtButtonToOsg(event->button());
        _graphicsWindow->getEventQueue()->mouseDoubleButtonPress(
            event->x(), event->y(), button
        );
    }

    // Trigger repaint
    update();
}

void OsgVerseWidget::keyPressEvent(QKeyEvent* event)
{
    // Handle special keys locally (will be fully implemented in Phase 3)
    if (event->key() == Qt::Key_V) {
        // viewAll - placeholder for Phase 3
        Base::Console().log("OsgVerseWidget: 'V' key pressed (viewAll - Phase 3)\n");
        event->accept();
        update();
        return;
    }
    
    if (event->key() == Qt::Key_Home) {
        // Reset camera - placeholder for Phase 3
        Base::Console().log("OsgVerseWidget: 'Home' key pressed (reset camera - Phase 3)\n");
        event->accept();
        update();
        return;
    }
    
    // Forward to OSG event queue
    if (_graphicsWindow.valid()) {
        int key = qtKeyToOsg(event->key());
        _graphicsWindow->getEventQueue()->keyPress(key);
    }
    
    // Trigger repaint
    update();
}

void OsgVerseWidget::keyReleaseEvent(QKeyEvent* event)
{
    // Forward to OSG event queue
    if (_graphicsWindow.valid()) {
        int key = qtKeyToOsg(event->key());
        _graphicsWindow->getEventQueue()->keyRelease(key);
    }
    
    // Trigger repaint
    update();
}

void OsgVerseWidget::focusInEvent(QFocusEvent* event)
{
    QOpenGLWidget::focusInEvent(event);
    
    // Trigger repaint when gaining focus
    update();
}

void OsgVerseWidget::focusOutEvent(QFocusEvent* event)
{
    QOpenGLWidget::focusOutEvent(event);
}

//===========================================================================
// Navigation style (Phase 5)
//===========================================================================

void OsgVerseWidget::setNavigationStyle(NavigationStyle style)
{
    if (!_viewer.valid()) {
        return;
    }

    _currentNavStyle = style;

    osg::ref_ptr<osgGA::CameraManipulator> manipulator;

    switch (style) {
        case NavigationStyle::CAD: {
            // OrbitManipulator is closer to CAD navigation
            // Middle button rotates around the center, scroll zooms
            osg::ref_ptr<osgGA::OrbitManipulator> orbit = new osgGA::OrbitManipulator();
            orbit->setAllowThrow(false);
            // Don't fix vertical axis to avoid gimbal lock
            // orbit->setVerticalAxisFixed(true);
            manipulator = orbit;
            break;
        }

        case NavigationStyle::Terrain: {
            // TerrainManipulator for walkthrough/terrain navigation
            osg::ref_ptr<osgGA::TerrainManipulator> terrain = new osgGA::TerrainManipulator();
            terrain->setAllowThrow(false);
            manipulator = terrain;
            break;
        }

        case NavigationStyle::Touchpad: {
            // Use TrackballManipulator with adjusted settings for touchpad
            osg::ref_ptr<osgGA::TrackballManipulator> trackball = new osgGA::TrackballManipulator();
            trackball->setAllowThrow(false);
            // Don't fix vertical axis to avoid gimbal lock
            // trackball->setVerticalAxisFixed(true);
            // Trackball works well with touchpad gestures
            manipulator = trackball;
            break;
        }

        case NavigationStyle::Trackball:
        default: {
            // Default trackball - good for general 3D navigation
            osg::ref_ptr<osgGA::TrackballManipulator> trackball = new osgGA::TrackballManipulator();
            trackball->setAllowThrow(false);
            // Don't fix vertical axis to avoid gimbal lock
            // trackball->setVerticalAxisFixed(true);
            manipulator = trackball;
            break;
        }
    }

    // Preserve current camera position
    osg::Vec3d eye, center, up;
    osgGA::CameraManipulator* currentManip = _viewer->getCameraManipulator();
    if (currentManip) {
        currentManip->getHomePosition(eye, center, up);
        manipulator->setHomePosition(eye, center, up);
    }

    _viewer->setCameraManipulator(manipulator.get());

    // Go to home position
    _viewer->home();

    update();
}

//===========================================================================
// Single click selection
//===========================================================================

void OsgVerseWidget::handleSingleClickSelection(const QPoint& pos)
{
    if (!_osgVerseViewer) {
        return;
    }

    // Perform pick operation
    Gui::View3D::PickResult result = _osgVerseViewer->pick(pos);

    if (result.valid && result.viewProvider) {
        // Get the ViewProvider and its DocumentObject
        Gui::ViewProviderDocumentObject* vpDoc =
            dynamic_cast<Gui::ViewProviderDocumentObject*>(result.viewProvider);

        if (vpDoc && vpDoc->getObject()) {
            App::DocumentObject* obj = vpDoc->getObject();
            App::Document* doc = obj->getDocument();

            if (doc) {
                const char* docName = doc->getName();
                const char* objName = obj->getNameInDocument();

                Base::Console().log("OsgVerseWidget: Click selection - doc=%s, obj=%s\n",
                    docName, objName);

                // Check if Ctrl is pressed for additive selection
                Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
                bool additive = modifiers & Qt::ControlModifier;

                if (!additive) {
                    // Clear current selection if not additive
                    Gui::Selection().clearSelection();
                }

                // Add to selection
                Gui::Selection().addSelection(docName, objName);
            }
        }
    } else {
        // Clicked on empty space - clear selection
        Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
        if (!(modifiers & Qt::ControlModifier)) {
            Gui::Selection().clearSelection();
            Base::Console().log("OsgVerseWidget: Click on empty space - selection cleared\n");
        }
    }
}
