/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                   *
 *                                                                         *
 *   Skia-based 2D Canvas implementation                                  *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#include "SkiaCanvas.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QImage>
#include <QFile>
#include <cmath>

// Skia headers
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkPath.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkPaint.h"
#include "include/core/SkColor.h"
#include "include/core/SkFont.h"
#include "include/core/SkTypeface.h"
#include "include/core/SkStream.h"
#include "include/core/SkData.h"
#include "include/encode/SkPngEncoder.h"
#include "include/svg/SkSVGCanvas.h"
#include "include/core/SkPictureRecorder.h"

#include <Base/Console.h>

namespace DrawingGui {

// Private implementation class to hide Skia details
class SkiaCanvas::SkiaImpl {
public:
    sk_sp<SkSurface> surface;
    SkCanvas* canvas = nullptr;
    
    bool createRasterSurface(int width, int height) {
        if (width <= 0 || height <= 0) return false;
        
        SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
        surface = SkSurfaces::Raster(info);
        if (!surface) {
            Base::Console().error("SkiaCanvas: Failed to create raster surface\n");
            return false;
        }
        canvas = surface->getCanvas();
        return true;
    }
    
    QImage toQImage() {
        if (!surface) return QImage();
        
        SkPixmap pixmap;
        if (!surface->peekPixels(&pixmap)) {
            return QImage();
        }
        
        // Create QImage from Skia pixel data
        QImage image(
            static_cast<const uchar*>(pixmap.addr()),
            pixmap.width(),
            pixmap.height(),
            pixmap.rowBytes(),
            QImage::Format_RGBA8888_Premultiplied
        );
        
        // Need to copy since pixmap data is owned by Skia
        return image.copy();
    }
};

SkiaCanvas::SkiaCanvas(QWidget* parent)
    : QWidget(parent)
    , m_impl(std::make_unique<SkiaImpl>())
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
    
    // Initialize with default size
    initSkia();
    
    Base::Console().message("SkiaCanvas: Initialized\n");
}

SkiaCanvas::~SkiaCanvas()
{
    cleanupSkia();
}

bool SkiaCanvas::initSkia()
{
    int w = width() > 0 ? width() : 800;
    int h = height() > 0 ? height() : 600;
    return m_impl->createRasterSurface(w, h);
}

void SkiaCanvas::cleanupSkia()
{
    m_impl->surface.reset();
    m_impl->canvas = nullptr;
}

void SkiaCanvas::createSurface(int width, int height)
{
    m_impl->createRasterSurface(width, height);
}

void SkiaCanvas::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    
    render();
    
    // Convert Skia surface to QImage and paint
    QImage image = m_impl->toQImage();
    if (!image.isNull()) {
        QPainter painter(this);
        painter.drawImage(0, 0, image);
    }
}

void SkiaCanvas::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    createSurface(event->size().width(), event->size().height());
    update();
}

void SkiaCanvas::render()
{
    if (!m_impl->canvas) return;
    
    SkCanvas* canvas = m_impl->canvas;
    
    // Clear background
    canvas->clear(m_backgroundColor);
    
    // Apply view transformation
    canvas->save();
    canvas->translate(width() / 2.0f + m_pan.x, height() / 2.0f + m_pan.y);
    canvas->scale(m_zoom, -m_zoom); // Flip Y for standard coordinate system
    
    // Draw grid
    if (m_showGrid) {
        drawGrid();
    }
    
    // Draw all geometry
    drawGeometry();
    
    // Draw temporary geometry (rubber band)
    drawTempGeometry();
    
    canvas->restore();
    
    // Draw cursor info (in screen coordinates)
    drawCursor();
}

void SkiaCanvas::drawGrid()
{
    if (!m_impl->canvas) return;
    
    SkCanvas* canvas = m_impl->canvas;
    SkPaint paint;
    paint.setColor(m_gridColor);
    paint.setStrokeWidth(1.0f / m_zoom);
    paint.setAntiAlias(true);
    
    // Calculate visible area in world coordinates
    float halfW = (width() / 2.0f) / m_zoom;
    float halfH = (height() / 2.0f) / m_zoom;
    float left = -m_pan.x / m_zoom - halfW;
    float right = -m_pan.x / m_zoom + halfW;
    float bottom = m_pan.y / m_zoom - halfH;
    float top = m_pan.y / m_zoom + halfH;
    
    // Snap to grid
    float startX = std::floor(left / m_gridSpacing) * m_gridSpacing;
    float startY = std::floor(bottom / m_gridSpacing) * m_gridSpacing;
    
    // Draw vertical lines
    for (float x = startX; x <= right; x += m_gridSpacing) {
        canvas->drawLine(x, bottom, x, top, paint);
    }
    
    // Draw horizontal lines
    for (float y = startY; y <= top; y += m_gridSpacing) {
        canvas->drawLine(left, y, right, y, paint);
    }
    
    // Draw axes with different color
    paint.setColor(0xFFCCCCCC);
    paint.setStrokeWidth(2.0f / m_zoom);
    canvas->drawLine(left, 0, right, 0, paint);  // X axis
    canvas->drawLine(0, bottom, 0, top, paint);  // Y axis
}

void SkiaCanvas::drawGeometry()
{
    if (!m_impl->canvas) return;
    
    SkCanvas* canvas = m_impl->canvas;
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    
    // Draw lines
    for (const auto& line : m_lines) {
        paint.setColor(line.color);
        paint.setStrokeWidth(line.strokeWidth / m_zoom);
        canvas->drawLine(line.start.x, line.start.y, 
                        line.end.x, line.end.y, paint);
    }
    
    // Draw circles
    for (const auto& circle : m_circles) {
        paint.setColor(circle.color);
        paint.setStrokeWidth(circle.strokeWidth / m_zoom);
        paint.setStyle(circle.filled ? SkPaint::kFill_Style : SkPaint::kStroke_Style);
        canvas->drawCircle(circle.center.x, circle.center.y, circle.radius, paint);
    }
    
    // Draw rectangles
    for (const auto& rect : m_rects) {
        paint.setColor(rect.color);
        paint.setStrokeWidth(rect.strokeWidth / m_zoom);
        paint.setStyle(rect.filled ? SkPaint::kFill_Style : SkPaint::kStroke_Style);
        SkRect skRect = SkRect::MakeXYWH(rect.topLeft.x, rect.topLeft.y, 
                                          rect.width, rect.height);
        canvas->drawRect(skRect, paint);
    }
}

void SkiaCanvas::drawTempGeometry()
{
    if (!m_impl->canvas || m_tempPoints.empty()) return;
    
    SkCanvas* canvas = m_impl->canvas;
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(0xFF0066FF); // Blue for temp geometry
    paint.setStrokeWidth(2.0f / m_zoom);
    
    switch (m_drawMode) {
        case DrawMode::Line:
            if (m_tempPoints.size() >= 1) {
                canvas->drawLine(m_tempPoints[0].x, m_tempPoints[0].y,
                               m_cursorWorld.x, m_cursorWorld.y, paint);
            }
            break;
            
        case DrawMode::Circle:
            if (m_tempPoints.size() >= 1) {
                float dx = m_cursorWorld.x - m_tempPoints[0].x;
                float dy = m_cursorWorld.y - m_tempPoints[0].y;
                float radius = std::sqrt(dx * dx + dy * dy);
                canvas->drawCircle(m_tempPoints[0].x, m_tempPoints[0].y, radius, paint);
            }
            break;
            
        case DrawMode::Rectangle:
            if (m_tempPoints.size() >= 1) {
                float x = std::min(m_tempPoints[0].x, m_cursorWorld.x);
                float y = std::min(m_tempPoints[0].y, m_cursorWorld.y);
                float w = std::abs(m_cursorWorld.x - m_tempPoints[0].x);
                float h = std::abs(m_cursorWorld.y - m_tempPoints[0].y);
                SkRect rect = SkRect::MakeXYWH(x, y, w, h);
                canvas->drawRect(rect, paint);
            }
            break;
            
        case DrawMode::Polyline:
            if (!m_tempPoints.empty()) {
                SkPathBuilder builder;
                builder.moveTo(m_tempPoints[0].x, m_tempPoints[0].y);
                for (size_t i = 1; i < m_tempPoints.size(); ++i) {
                    builder.lineTo(m_tempPoints[i].x, m_tempPoints[i].y);
                }
                builder.lineTo(m_cursorWorld.x, m_cursorWorld.y);
                canvas->drawPath(builder.detach(), paint);
            }
            break;
            
        default:
            break;
    }
    
    // Draw points
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(0xFFFF0000); // Red for points
    for (const auto& pt : m_tempPoints) {
        canvas->drawCircle(pt.x, pt.y, 4.0f / m_zoom, paint);
    }
}

void SkiaCanvas::drawCursor()
{
    if (!m_impl->canvas) return;
    
    SkCanvas* canvas = m_impl->canvas;
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(0xFF333333);
    
    // Draw coordinate text
    SkFont font;
    font.setSize(12);
    
    char coordText[64];
    snprintf(coordText, sizeof(coordText), "X: %.2f  Y: %.2f", 
             m_cursorWorld.x, m_cursorWorld.y);
    
    canvas->drawString(coordText, 10, height() - 10, font, paint);
}

// Coordinate transformation
SkiaPoint SkiaCanvas::screenToWorld(const QPoint& screenPos) const
{
    float x = (screenPos.x() - width() / 2.0f - m_pan.x) / m_zoom;
    float y = -(screenPos.y() - height() / 2.0f - m_pan.y) / m_zoom;
    return SkiaPoint(x, y);
}

QPoint SkiaCanvas::worldToScreen(const SkiaPoint& worldPos) const
{
    int x = static_cast<int>(worldPos.x * m_zoom + width() / 2.0f + m_pan.x);
    int y = static_cast<int>(-worldPos.y * m_zoom + height() / 2.0f + m_pan.y);
    return QPoint(x, y);
}

// Mouse event handlers
void SkiaCanvas::mousePressEvent(QMouseEvent* event)
{
    m_lastMousePos = event->pos();
    m_cursorWorld = screenToWorld(event->pos());
    
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    
    if (event->button() == Qt::LeftButton) {
        switch (m_drawMode) {
            case DrawMode::Line:
            case DrawMode::Circle:
            case DrawMode::Rectangle:
                if (m_tempPoints.empty()) {
                    m_tempPoints.push_back(m_cursorWorld);
                    m_isDrawing = true;
                } else {
                    finishCurrentDrawing();
                }
                break;
                
            case DrawMode::Polyline:
                m_tempPoints.push_back(m_cursorWorld);
                m_isDrawing = true;
                break;
                
            default:
                break;
        }
    }
    
    if (event->button() == Qt::RightButton && m_isDrawing) {
        if (m_drawMode == DrawMode::Polyline && m_tempPoints.size() >= 2) {
            finishCurrentDrawing();
        } else {
            cancelCurrentDrawing();
        }
    }
    
    update();
}

void SkiaCanvas::mouseMoveEvent(QMouseEvent* event)
{
    QPoint delta = event->pos() - m_lastMousePos;
    m_cursorWorld = screenToWorld(event->pos());
    
    if (m_isPanning) {
        m_pan.x += delta.x();
        m_pan.y += delta.y();
        Q_EMIT viewChanged();
    }
    
    m_lastMousePos = event->pos();
    Q_EMIT cursorPositionChanged(m_cursorWorld.x, m_cursorWorld.y);
    update();
}

void SkiaCanvas::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
}

void SkiaCanvas::wheelEvent(QWheelEvent* event)
{
    float zoomFactor = event->angleDelta().y() > 0 ? 1.1f : 0.9f;
    
    // Zoom towards cursor position
    QPoint mousePos = event->position().toPoint();
    SkiaPoint worldBefore = screenToWorld(mousePos);
    
    m_zoom *= zoomFactor;
    m_zoom = std::max(0.01f, std::min(100.0f, m_zoom)); // Clamp zoom
    
    SkiaPoint worldAfter = screenToWorld(mousePos);
    
    // Adjust pan to keep cursor at same world position
    m_pan.x += (worldAfter.x - worldBefore.x) * m_zoom;
    m_pan.y -= (worldAfter.y - worldBefore.y) * m_zoom;
    
    Q_EMIT viewChanged();
    update();
}

void SkiaCanvas::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        cancelCurrentDrawing();
        setDrawMode(DrawMode::None);
    }
    else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (m_drawMode == DrawMode::Polyline && m_tempPoints.size() >= 2) {
            finishCurrentDrawing();
        }
    }
    
    QWidget::keyPressEvent(event);
}

void SkiaCanvas::finishCurrentDrawing()
{
    switch (m_drawMode) {
        case DrawMode::Line:
            if (m_tempPoints.size() >= 1) {
                SkiaLine line;
                line.start = m_tempPoints[0];
                line.end = m_cursorWorld;
                line.color = m_currentColor;
                line.strokeWidth = m_currentStrokeWidth;
                m_lines.push_back(line);
                Q_EMIT lineCreated(line);
                Base::Console().message("SkiaCanvas: Line created (%.2f,%.2f) to (%.2f,%.2f)\n",
                    line.start.x, line.start.y, line.end.x, line.end.y);
            }
            break;
            
        case DrawMode::Circle:
            if (m_tempPoints.size() >= 1) {
                SkiaCircle circle;
                circle.center = m_tempPoints[0];
                float dx = m_cursorWorld.x - m_tempPoints[0].x;
                float dy = m_cursorWorld.y - m_tempPoints[0].y;
                circle.radius = std::sqrt(dx * dx + dy * dy);
                circle.color = m_currentColor;
                circle.strokeWidth = m_currentStrokeWidth;
                m_circles.push_back(circle);
                Q_EMIT circleCreated(circle);
                Base::Console().message("SkiaCanvas: Circle created center(%.2f,%.2f) r=%.2f\n",
                    circle.center.x, circle.center.y, circle.radius);
            }
            break;
            
        case DrawMode::Rectangle:
            if (m_tempPoints.size() >= 1) {
                SkiaRect rect;
                rect.topLeft.x = std::min(m_tempPoints[0].x, m_cursorWorld.x);
                rect.topLeft.y = std::min(m_tempPoints[0].y, m_cursorWorld.y);
                rect.width = std::abs(m_cursorWorld.x - m_tempPoints[0].x);
                rect.height = std::abs(m_cursorWorld.y - m_tempPoints[0].y);
                rect.color = m_currentColor;
                rect.strokeWidth = m_currentStrokeWidth;
                m_rects.push_back(rect);
                Q_EMIT rectCreated(rect);
                Base::Console().message("SkiaCanvas: Rectangle created\n");
            }
            break;
            
        default:
            break;
    }
    
    m_tempPoints.clear();
    m_isDrawing = false;
    update();
}

void SkiaCanvas::cancelCurrentDrawing()
{
    m_tempPoints.clear();
    m_isDrawing = false;
    update();
}

// Public API implementations
void SkiaCanvas::setDrawMode(DrawMode mode)
{
    if (m_isDrawing) {
        cancelCurrentDrawing();
    }
    m_drawMode = mode;
    
    switch (mode) {
        case DrawMode::None:
            setCursor(Qt::ArrowCursor);
            break;
        case DrawMode::Pan:
            setCursor(Qt::OpenHandCursor);
            break;
        default:
            setCursor(Qt::CrossCursor);
            break;
    }
}

void SkiaCanvas::setZoom(float zoom)
{
    m_zoom = std::max(0.01f, std::min(100.0f, zoom));
    Q_EMIT viewChanged();
    update();
}

void SkiaCanvas::setPan(float x, float y)
{
    m_pan.x = x;
    m_pan.y = y;
    Q_EMIT viewChanged();
    update();
}

void SkiaCanvas::resetView()
{
    m_zoom = 1.0f;
    m_pan = SkiaPoint(0, 0);
    Q_EMIT viewChanged();
    update();
}

void SkiaCanvas::zoomToFit()
{
    // Calculate bounding box of all geometry
    if (m_lines.empty() && m_circles.empty() && m_rects.empty()) {
        resetView();
        return;
    }
    
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    
    for (const auto& line : m_lines) {
        minX = std::min({minX, line.start.x, line.end.x});
        maxX = std::max({maxX, line.start.x, line.end.x});
        minY = std::min({minY, line.start.y, line.end.y});
        maxY = std::max({maxY, line.start.y, line.end.y});
    }
    
    for (const auto& circle : m_circles) {
        minX = std::min(minX, circle.center.x - circle.radius);
        maxX = std::max(maxX, circle.center.x + circle.radius);
        minY = std::min(minY, circle.center.y - circle.radius);
        maxY = std::max(maxY, circle.center.y + circle.radius);
    }
    
    for (const auto& rect : m_rects) {
        minX = std::min(minX, rect.topLeft.x);
        maxX = std::max(maxX, rect.topLeft.x + rect.width);
        minY = std::min(minY, rect.topLeft.y);
        maxY = std::max(maxY, rect.topLeft.y + rect.height);
    }
    
    float contentW = maxX - minX;
    float contentH = maxY - minY;
    float centerX = (minX + maxX) / 2.0f;
    float centerY = (minY + maxY) / 2.0f;
    
    if (contentW > 0 && contentH > 0) {
        float zoomX = (width() * 0.8f) / contentW;
        float zoomY = (height() * 0.8f) / contentH;
        m_zoom = std::min(zoomX, zoomY);
        m_pan.x = -centerX * m_zoom;
        m_pan.y = centerY * m_zoom;
    }
    
    Q_EMIT viewChanged();
    update();
}

void SkiaCanvas::addLine(const SkiaLine& line)
{
    m_lines.push_back(line);
    update();
}

void SkiaCanvas::addCircle(const SkiaCircle& circle)
{
    m_circles.push_back(circle);
    update();
}

void SkiaCanvas::addRect(const SkiaRect& rect)
{
    m_rects.push_back(rect);
    update();
}

void SkiaCanvas::clearGeometry()
{
    m_lines.clear();
    m_circles.clear();
    m_rects.clear();
    update();
}

void SkiaCanvas::setShowGrid(bool show)
{
    m_showGrid = show;
    update();
}

void SkiaCanvas::setGridSpacing(float spacing)
{
    m_gridSpacing = spacing;
    update();
}

void SkiaCanvas::setBackgroundColor(uint32_t color)
{
    m_backgroundColor = color;
    update();
}

void SkiaCanvas::setGridColor(uint32_t color)
{
    m_gridColor = color;
    update();
}

void SkiaCanvas::setCurrentColor(uint32_t color)
{
    m_currentColor = color;
}

void SkiaCanvas::setCurrentStrokeWidth(float width)
{
    m_currentStrokeWidth = width;
}

bool SkiaCanvas::exportToSVG(const QString& filename)
{
    if (!m_impl->surface) return false;
    
    // Calculate bounds
    SkRect bounds = SkRect::MakeWH(width(), height());
    
    // Create SVG stream
    SkFILEWStream stream(filename.toStdString().c_str());
    if (!stream.isValid()) {
        Base::Console().error("SkiaCanvas: Failed to open SVG file for writing\n");
        return false;
    }
    
    // Create SVG canvas
    std::unique_ptr<SkCanvas> svgCanvas = SkSVGCanvas::Make(bounds, &stream);
    if (!svgCanvas) {
        Base::Console().error("SkiaCanvas: Failed to create SVG canvas\n");
        return false;
    }
    
    // Render to SVG
    svgCanvas->clear(m_backgroundColor);
    svgCanvas->translate(width() / 2.0f + m_pan.x, height() / 2.0f + m_pan.y);
    svgCanvas->scale(m_zoom, -m_zoom);
    
    // Draw geometry to SVG canvas
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    
    for (const auto& line : m_lines) {
        paint.setColor(line.color);
        paint.setStrokeWidth(line.strokeWidth / m_zoom);
        svgCanvas->drawLine(line.start.x, line.start.y, line.end.x, line.end.y, paint);
    }
    
    for (const auto& circle : m_circles) {
        paint.setColor(circle.color);
        paint.setStrokeWidth(circle.strokeWidth / m_zoom);
        svgCanvas->drawCircle(circle.center.x, circle.center.y, circle.radius, paint);
    }
    
    for (const auto& rect : m_rects) {
        paint.setColor(rect.color);
        paint.setStrokeWidth(rect.strokeWidth / m_zoom);
        SkRect skRect = SkRect::MakeXYWH(rect.topLeft.x, rect.topLeft.y, rect.width, rect.height);
        svgCanvas->drawRect(skRect, paint);
    }
    
    Base::Console().message("SkiaCanvas: Exported to SVG: %s\n", filename.toStdString().c_str());
    return true;
}

bool SkiaCanvas::exportToPNG(const QString& filename)
{
    if (!m_impl->surface) return false;
    
    sk_sp<SkImage> image = m_impl->surface->makeImageSnapshot();
    if (!image) {
        Base::Console().error("SkiaCanvas: Failed to create image snapshot\n");
        return false;
    }
    
    sk_sp<SkData> pngData = SkPngEncoder::Encode(nullptr, image.get(), {});
    if (!pngData) {
        Base::Console().error("SkiaCanvas: Failed to encode PNG\n");
        return false;
    }
    
    SkFILEWStream stream(filename.toStdString().c_str());
    if (!stream.isValid()) {
        Base::Console().error("SkiaCanvas: Failed to open PNG file for writing\n");
        return false;
    }
    
    stream.write(pngData->data(), pngData->size());
    Base::Console().message("SkiaCanvas: Exported to PNG: %s\n", filename.toStdString().c_str());
    return true;
}

} // namespace DrawingGui
