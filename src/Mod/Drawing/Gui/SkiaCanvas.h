/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                   *
 *                                                                         *
 *   Skia-based 2D Canvas for Drawing workbench                           *
 *                                                                         *
 ***************************************************************************/

#ifndef DRAWING_SKIA_CANVAS_H
#define DRAWING_SKIA_CANVAS_H

#include <QWidget>
#include <QTimer>
#include <memory>
#include <vector>
#include <functional>

// Forward declarations for Skia types
class SkSurface;
class SkCanvas;
class SkPath;
class SkPaint;
class GrDirectContext;

namespace DrawingGui {

// Geometry types for drawing
struct SkiaPoint {
    float x, y;
    SkiaPoint(float _x = 0, float _y = 0) : x(_x), y(_y) {}
};

struct SkiaLine {
    SkiaPoint start, end;
    float strokeWidth = 2.0f;
    uint32_t color = 0xFF000000; // ARGB black
};

struct SkiaCircle {
    SkiaPoint center;
    float radius;
    float strokeWidth = 2.0f;
    uint32_t color = 0xFF000000;
    bool filled = false;
};

struct SkiaRect {
    SkiaPoint topLeft;
    float width, height;
    float strokeWidth = 2.0f;
    uint32_t color = 0xFF000000;
    bool filled = false;
};

// Drawing mode enumeration
enum class DrawMode {
    None,
    Line,
    Circle,
    Rectangle,
    Polyline,
    Pan,
    Zoom
};

/**
 * @brief Skia-based 2D drawing canvas widget
 * 
 * This widget provides hardware-accelerated 2D rendering using Skia
 * with Metal backend on macOS.
 */
class SkiaCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit SkiaCanvas(QWidget* parent = nullptr);
    ~SkiaCanvas() override;

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
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    // Skia initialization
    bool initSkia();
    void cleanupSkia();
    void createSurface(int width, int height);

    // Rendering
    void render();
    void drawGrid();
    void drawGeometry();
    void drawTempGeometry();
    void drawCursor();

    // Coordinate transformation
    SkiaPoint screenToWorld(const QPoint& screenPos) const;
    QPoint worldToScreen(const SkiaPoint& worldPos) const;

    // Drawing helpers
    void finishCurrentDrawing();
    void cancelCurrentDrawing();

private:
    // Skia objects (using pimpl to hide Skia types)
    class SkiaImpl;
    std::unique_ptr<SkiaImpl> m_impl;

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
};

} // namespace DrawingGui

#endif // DRAWING_SKIA_CANVAS_H
