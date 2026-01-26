// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef OSGVERSEGUI_WIDGET_H
#define OSGVERSEGUI_WIDGET_H

#include "OsgVerseGuiExport.h"
#include <QOpenGLWidget>
#include <osg/ref_ptr>

// Forward declarations
namespace osgViewer {
    class Viewer;
    class GraphicsWindowEmbedded;
}

namespace OsgVerseGui {

// Forward declaration
class OsgVerseViewer;

/**
 * @brief Qt OpenGL widget that embeds an OSG viewer
 * 
 * This widget provides the Qt/OpenGL integration for the OsgVerse rendering engine.
 * It creates an OSG viewer and manages the OpenGL context, event loop integration,
 * and rendering.
 * 
 * Phase 1: Basic widget creation and rendering
 * Phase 2: Event handling (mouse, keyboard)
 */
class OsgVerseGuiExport OsgVerseWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit OsgVerseWidget(QWidget* parent = nullptr);
    ~OsgVerseWidget() override;
    
    /**
     * @brief Get the OSG viewer instance
     * @return Pointer to the OSG viewer
     */
    osgViewer::Viewer* getViewer() const { return _viewer.get(); }

    /**
     * @brief Navigation style enumeration
     */
    enum class NavigationStyle {
        Trackball,      // Default OSG trackball (similar to Blender)
        CAD,            // CAD-style: middle=rotate, shift+middle=pan
        Touchpad,       // Touchpad-friendly navigation
        Terrain         // Terrain/walkthrough style
    };

    /**
     * @brief Set the navigation style
     * @param style The navigation style to use
     */
    void setNavigationStyle(NavigationStyle style);

    /**
     * @brief Get the current navigation style
     */
    NavigationStyle getNavigationStyle() const { return _currentNavStyle; }

    /**
     * @brief Set the OsgVerseViewer that owns this widget
     *
     * This allows the widget to call selection methods on the viewer
     * when the user performs selection gestures.
     */
    void setViewer(OsgVerseViewer* viewer) { _osgVerseViewer = viewer; }

    /**
     * @brief Get the OsgVerseViewer that owns this widget
     */
    OsgVerseViewer* getOsgVerseViewer() const { return _osgVerseViewer; }

protected:
    // Qt OpenGL lifecycle
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;
    
    // Event handling (Phase 2)
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    // Helper methods for event conversion
    int qtButtonToOsg(Qt::MouseButton button);
    int qtKeyToOsg(int key);
    unsigned int getButtonMask();

    // Handle single-click selection
    void handleSingleClickSelection(const QPoint& pos);

    osg::ref_ptr<osgViewer::Viewer> _viewer;
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> _graphicsWindow;
    NavigationStyle _currentNavStyle = NavigationStyle::Trackball;
    OsgVerseViewer* _osgVerseViewer = nullptr;  ///< Back-reference to viewer for selection

    // For click detection (distinguish click from drag)
    QPoint _mousePressPos;
    bool _mousePressed = false;
};

} // namespace OsgVerseGui

#endif // OSGVERSEGUI_WIDGET_H
