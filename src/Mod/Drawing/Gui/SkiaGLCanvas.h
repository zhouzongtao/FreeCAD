/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                   *
 *                                                                         *
 *   Skia-based 2D Canvas with OpenGL GPU acceleration                    *
 *                                                                         *
 ***************************************************************************/

#ifndef DRAWING_SKIA_GL_CANVAS_H
#define DRAWING_SKIA_GL_CANVAS_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QElapsedTimer>
#include <memory>
#include <vector>

#include "SkiaTypes.h"

// Forward declarations for Skia types
class SkSurface;
class SkCanvas;
class GrDirectContext;

namespace DrawingGui {

/**
 * @brief Skia-based 2D drawing canvas with OpenGL GPU acceleration
 * 
 * This widget provides GPU-accelerated 2D graphics using Skia's
 * OpenGL backend via QOpenGLWidget.
 */
class SkiaGLCanvas : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit SkiaGLCanvas(QWidget* parent = nullptr);
    ~SkiaGLCanvas() override;

    // Drawing mode
    void setDrawMode(DrawMode mode);
    DrawMode drawMode() const { return m_drawMode; }

    // View transformation
    void setZoom(float zoom);
    float zoom() const { return m_zoom; }
    void setPan(float x, float y);
    SkiaPoint pan() const { return m_pan; }
    void resetView();
    void zoomToFit();

    // Geometry management
    void addLine(const SkiaLine& line);
    void addCircle(const SkiaCircle& circle);
    void addRect(const SkiaRect& rect);
    void clearGeometry();
    
    // Grid
    void setShowGrid(bool show);
    bool showGrid() const { return m_showGrid; }
    void setGridSpacing(float spacing);

    // Colors
    void setBackgroundColor(uint32_t color);
    void setGridColor(uint32_t color);
    void setCurrentColor(uint32_t color);
    void setCurrentStrokeWidth(float width);

    // Export
    bool exportToSVG(const QString& filename);
    bool exportToPNG(const QString& filename);

Q_SIGNALS:
    void lineCreated(const SkiaLine& line);
    void circleCreated(const SkiaCircle& circle);
    void rectCreated(const SkiaRect& rect);
    void viewChanged();
    void cursorPositionChanged(float x, float y);

protected:
    // QOpenGLWidget overrides
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    
    // Mouse/keyboard events
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    // Skia GL initialization
    void initSkiaGL(int w, int h);
    void cleanupSkia();

    // Rendering
    void render(SkCanvas* canvas);
    void drawGrid(SkCanvas* canvas);
    void drawGeometry(SkCanvas* canvas);
    void drawTempGeometry(SkCanvas* canvas);
    void drawCursor(SkCanvas* canvas);

    // Coordinate transformation
    SkiaPoint screenToWorld(const QPoint& screenPos) const;
    QPoint worldToScreen(const SkiaPoint& worldPos) const;

    // Drawing helpers
    void finishCurrentDrawing();
    void cancelCurrentDrawing();

private:
    // Skia GL objects
    class SkiaGLImpl;
    std::unique_ptr<SkiaGLImpl> m_impl;

    // View state
    float m_zoom = 1.0f;
    SkiaPoint m_pan{0, 0};
    
    // Drawing state
    DrawMode m_drawMode = DrawMode::None;
    std::vector<SkiaPoint> m_tempPoints;
    bool m_isDrawing = false;
    
    // Mouse state
    QPoint m_lastMousePos;
    bool m_isPanning = false;
    
    // Geometry storage
    std::vector<SkiaLine> m_lines;
    std::vector<SkiaCircle> m_circles;
    std::vector<SkiaRect> m_rects;
    
    // Display settings
    bool m_showGrid = true;
    float m_gridSpacing = 10.0f;
    uint32_t m_backgroundColor = 0xFFFFFFFF; // White
    uint32_t m_gridColor = 0xFFE0E0E0;       // Light gray
    uint32_t m_currentColor = 0xFF000000;    // Black
    float m_currentStrokeWidth = 2.0f;
    
    // Cursor position in world coordinates
    SkiaPoint m_cursorWorld{0, 0};
    
    // Animation timer
    QTimer m_timer;
    QElapsedTimer m_elapsedTimer;
};

} // namespace DrawingGui

#endif // DRAWING_SKIA_GL_CANVAS_H
