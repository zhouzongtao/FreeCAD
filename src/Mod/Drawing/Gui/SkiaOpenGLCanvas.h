/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                   *
 *                                                                         *
 *   Skia OpenGL GPU Canvas for Drawing workbench (Cross-platform)        *
 *                                                                         *
 ***************************************************************************/

#ifndef DRAWING_SKIA_OPENGL_CANVAS_H
#define DRAWING_SKIA_OPENGL_CANVAS_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <memory>
#include <vector>

#include "SkiaTypes.h"

// Skia smart pointer template
#include "include/core/SkRefCnt.h"

// Forward declarations
class GrDirectContext;
class SkSurface;
class SkCanvas;

namespace DrawingGui {

/**
 * @brief Skia OpenGL GPU-accelerated 2D drawing canvas
 * 
 * This widget uses Skia with OpenGL backend for hardware-accelerated
 * 2D rendering. Works on Windows, Linux, and macOS.
 */
class SkiaOpenGLCanvas : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit SkiaOpenGLCanvas(QWidget* parent = nullptr);
    ~SkiaOpenGLCanvas() override;

    // Check if GPU rendering is available
    bool isGpuAvailable() const { return m_gpuAvailable; }
    
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
    // Skia/OpenGL initialization
    bool initSkiaGpu();
    void cleanup();
    bool createSurface(int width, int height);
    
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
    // Skia GPU context
    sk_sp<GrDirectContext> m_grContext;
    sk_sp<SkSurface> m_surface;
    
    bool m_gpuAvailable = false;
    int m_surfaceWidth = 0;
    int m_surfaceHeight = 0;

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
    uint32_t m_backgroundColor = 0xFFFFFFFF;
    uint32_t m_gridColor = 0xFFE0E0E0;
    uint32_t m_currentColor = 0xFF000000;
    float m_currentStrokeWidth = 2.0f;
    
    // Cursor position
    SkiaPoint m_cursorWorld{0, 0};
};

} // namespace DrawingGui

#endif // DRAWING_SKIA_OPENGL_CANVAS_H
