/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                   *
 *                                                                         *
 *   Skia OpenGL GPU Canvas implementation (Cross-platform)               *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#include "SkiaOpenGLCanvas.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <cmath>

// Skia headers
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkPath.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkPaint.h"
#include "include/core/SkColor.h"
#include "include/core/SkFont.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkStream.h"
#include "include/core/SkData.h"
#include "include/encode/SkPngEncoder.h"
#include "include/svg/SkSVGCanvas.h"

// Skia GPU/OpenGL headers
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/ganesh/gl/GrGLTypes.h"
#include "include/gpu/GpuTypes.h"

#include <Base/Console.h>

namespace DrawingGui {

SkiaOpenGLCanvas::SkiaOpenGLCanvas(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    
    // Request OpenGL format
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    setFormat(format);
    
    Base::Console().message("SkiaOpenGLCanvas: Created, waiting for GL init\n");
}

SkiaOpenGLCanvas::~SkiaOpenGLCanvas()
{
    cleanup();
}

void SkiaOpenGLCanvas::initializeGL()
{
    initializeOpenGLFunctions();
    
    Base::Console().message("SkiaOpenGLCanvas: OpenGL initialized\n");
    Base::Console().message("  Vendor: %s\n", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    Base::Console().message("  Renderer: %s\n", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    Base::Console().message("  Version: %s\n", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    
    m_gpuAvailable = initSkiaGpu();
    
    if (m_gpuAvailable) {
        Base::Console().message("SkiaOpenGLCanvas: GPU rendering ready\n");
    } else {
        Base::Console().warning("SkiaOpenGLCanvas: GPU init failed\n");
    }
}

bool SkiaOpenGLCanvas::initSkiaGpu()
{
    // Create Skia OpenGL interface
    sk_sp<const GrGLInterface> interface = GrGLMakeNativeInterface();
    if (!interface) {
        Base::Console().error("SkiaOpenGLCanvas: Failed to create GL interface\n");
        return false;
    }
    
    // Create Skia GPU context
    m_grContext = GrDirectContexts::MakeGL(interface);
    if (!m_grContext) {
        Base::Console().error("SkiaOpenGLCanvas: Failed to create GPU context\n");
        return false;
    }
    
    return true;
}

void SkiaOpenGLCanvas::resizeGL(int w, int h)
{
    if (!m_gpuAvailable) return;
    
    m_surfaceWidth = w * devicePixelRatio();
    m_surfaceHeight = h * devicePixelRatio();
    
    // Surface will be recreated in paintGL
    m_surface.reset();
}

bool SkiaOpenGLCanvas::createSurface(int width, int height)
{
    if (!m_grContext || width <= 0 || height <= 0) return false;
    
    // Reset GPU context state
    m_grContext->resetContext();
    
    // Get current framebuffer info
    GLint fbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
    
    GrGLFramebufferInfo fbInfo;
    fbInfo.fFBOID = static_cast<GrGLuint>(fbo);
    fbInfo.fFormat = GL_RGBA8;
    
    int sampleCount = format().samples();
    if (sampleCount <= 0) sampleCount = 1;
    
    GrBackendRenderTarget backendRT = GrBackendRenderTargets::MakeGL(
        width, height, 
        sampleCount,
        8,  // stencil bits
        fbInfo
    );
    
    m_surface = SkSurfaces::WrapBackendRenderTarget(
        m_grContext.get(),
        backendRT,
        kBottomLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType,
        nullptr,
        nullptr
    );
    
    if (!m_surface) {
        Base::Console().error("SkiaOpenGLCanvas: WrapBackendRenderTarget failed\n");
    }
    
    return m_surface != nullptr;
}

void SkiaOpenGLCanvas::paintGL()
{
    if (!m_gpuAvailable || !m_grContext) {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }
    
    // Reset context state before rendering
    m_grContext->resetContext();
    
    // Recreate surface if needed
    int w = width() * devicePixelRatio();
    int h = height() * devicePixelRatio();
    
    if (!m_surface || m_surfaceWidth != w || m_surfaceHeight != h) {
        m_surfaceWidth = w;
        m_surfaceHeight = h;
        if (!createSurface(w, h)) {
            Base::Console().warning("SkiaOpenGLCanvas: Failed to create surface\n");
            glClearColor(1.0f, 0.0f, 0.0f, 1.0f);  // Red = error
            glClear(GL_COLOR_BUFFER_BIT);
            return;
        }
    }
    
    SkCanvas* canvas = m_surface->getCanvas();
    if (!canvas) {
        Base::Console().warning("SkiaOpenGLCanvas: No canvas\n");
        return;
    }
    
    render(canvas);
    
    // Flush Skia commands to OpenGL
    m_grContext->flushAndSubmit(m_surface.get());
}

void SkiaOpenGLCanvas::cleanup()
{
    makeCurrent();
    m_surface.reset();
    m_grContext.reset();
    doneCurrent();
}

void SkiaOpenGLCanvas::render(SkCanvas* canvas)
{
    float scale = devicePixelRatio();
    
    // Clear background
    canvas->clear(m_backgroundColor);
    
    // Apply view transformation
    canvas->save();
    canvas->scale(scale, scale);
    canvas->translate(width() / 2.0f + m_pan.x, height() / 2.0f + m_pan.y);
    canvas->scale(m_zoom, -m_zoom);
    
    if (m_showGrid) {
        drawGrid(canvas);
    }
    
    drawGeometry(canvas);
    drawTempGeometry(canvas);
    
    canvas->restore();
    
    // Draw cursor info (in screen space)
    canvas->save();
    canvas->scale(scale, scale);
    drawCursor(canvas);
    canvas->restore();
}

void SkiaOpenGLCanvas::drawGrid(SkCanvas* canvas)
{
    SkPaint paint;
    paint.setColor(m_gridColor);
    paint.setStrokeWidth(1.0f / m_zoom);
    paint.setAntiAlias(true);
    
    float halfW = (width() / 2.0f) / m_zoom;
    float halfH = (height() / 2.0f) / m_zoom;
    float left = -m_pan.x / m_zoom - halfW;
    float right = -m_pan.x / m_zoom + halfW;
    float bottom = m_pan.y / m_zoom - halfH;
    float top = m_pan.y / m_zoom + halfH;
    
    float startX = std::floor(left / m_gridSpacing) * m_gridSpacing;
    float startY = std::floor(bottom / m_gridSpacing) * m_gridSpacing;
    
    for (float x = startX; x <= right; x += m_gridSpacing) {
        canvas->drawLine(x, bottom, x, top, paint);
    }
    
    for (float y = startY; y <= top; y += m_gridSpacing) {
        canvas->drawLine(left, y, right, y, paint);
    }
    
    // Draw axes
    paint.setColor(0xFFCCCCCC);
    paint.setStrokeWidth(2.0f / m_zoom);
    canvas->drawLine(left, 0, right, 0, paint);
    canvas->drawLine(0, bottom, 0, top, paint);
}

void SkiaOpenGLCanvas::drawGeometry(SkCanvas* canvas)
{
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    
    for (const auto& line : m_lines) {
        paint.setColor(line.color);
        paint.setStrokeWidth(line.strokeWidth / m_zoom);
        canvas->drawLine(line.start.x, line.start.y, line.end.x, line.end.y, paint);
    }
    
    for (const auto& circle : m_circles) {
        paint.setColor(circle.color);
        paint.setStrokeWidth(circle.strokeWidth / m_zoom);
        paint.setStyle(circle.filled ? SkPaint::kFill_Style : SkPaint::kStroke_Style);
        canvas->drawCircle(circle.center.x, circle.center.y, circle.radius, paint);
    }
    
    for (const auto& rect : m_rects) {
        paint.setColor(rect.color);
        paint.setStrokeWidth(rect.strokeWidth / m_zoom);
        paint.setStyle(rect.filled ? SkPaint::kFill_Style : SkPaint::kStroke_Style);
        SkRect skRect = SkRect::MakeXYWH(rect.topLeft.x, rect.topLeft.y, rect.width, rect.height);
        canvas->drawRect(skRect, paint);
    }
}

void SkiaOpenGLCanvas::drawTempGeometry(SkCanvas* canvas)
{
    if (m_tempPoints.empty()) return;
    
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(0xFF0066FF);
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
    
    // Draw temp points
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(0xFFFF0000);
    for (const auto& pt : m_tempPoints) {
        canvas->drawCircle(pt.x, pt.y, 4.0f / m_zoom, paint);
    }
}

void SkiaOpenGLCanvas::drawCursor(SkCanvas* canvas)
{
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(0xFF333333);
    
    SkFont font;
    font.setSize(12);
    
    char coordText[64];
    snprintf(coordText, sizeof(coordText), "X: %.2f  Y: %.2f  [GPU: OpenGL]", 
             m_cursorWorld.x, m_cursorWorld.y);
    
    canvas->drawString(coordText, 10, height() - 10, font, paint);
}

SkiaPoint SkiaOpenGLCanvas::screenToWorld(const QPoint& screenPos) const
{
    float x = (screenPos.x() - width() / 2.0f - m_pan.x) / m_zoom;
    float y = -(screenPos.y() - height() / 2.0f - m_pan.y) / m_zoom;
    return SkiaPoint(x, y);
}

QPoint SkiaOpenGLCanvas::worldToScreen(const SkiaPoint& worldPos) const
{
    int x = static_cast<int>(worldPos.x * m_zoom + width() / 2.0f + m_pan.x);
    int y = static_cast<int>(-worldPos.y * m_zoom + height() / 2.0f + m_pan.y);
    return QPoint(x, y);
}

// Mouse event handlers
void SkiaOpenGLCanvas::mousePressEvent(QMouseEvent* event)
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

void SkiaOpenGLCanvas::mouseMoveEvent(QMouseEvent* event)
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

void SkiaOpenGLCanvas::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
}

void SkiaOpenGLCanvas::wheelEvent(QWheelEvent* event)
{
    float zoomFactor = event->angleDelta().y() > 0 ? 1.1f : 0.9f;
    
    QPoint mousePos = event->position().toPoint();
    SkiaPoint worldBefore = screenToWorld(mousePos);
    
    m_zoom *= zoomFactor;
    m_zoom = std::max(0.01f, std::min(100.0f, m_zoom));
    
    SkiaPoint worldAfter = screenToWorld(mousePos);
    
    m_pan.x += (worldAfter.x - worldBefore.x) * m_zoom;
    m_pan.y -= (worldAfter.y - worldBefore.y) * m_zoom;
    
    Q_EMIT viewChanged();
    update();
}

void SkiaOpenGLCanvas::keyPressEvent(QKeyEvent* event)
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
    
    QOpenGLWidget::keyPressEvent(event);
}

void SkiaOpenGLCanvas::finishCurrentDrawing()
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
            }
            break;
            
        default:
            break;
    }
    
    m_tempPoints.clear();
    m_isDrawing = false;
    update();
}

void SkiaOpenGLCanvas::cancelCurrentDrawing()
{
    m_tempPoints.clear();
    m_isDrawing = false;
    update();
}

void SkiaOpenGLCanvas::setDrawMode(DrawMode mode)
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

void SkiaOpenGLCanvas::setZoom(float zoom)
{
    m_zoom = std::max(0.01f, std::min(100.0f, zoom));
    Q_EMIT viewChanged();
    update();
}

void SkiaOpenGLCanvas::setPan(float x, float y)
{
    m_pan.x = x;
    m_pan.y = y;
    Q_EMIT viewChanged();
    update();
}

void SkiaOpenGLCanvas::resetView()
{
    m_zoom = 1.0f;
    m_pan = SkiaPoint(0, 0);
    Q_EMIT viewChanged();
    update();
}

void SkiaOpenGLCanvas::zoomToFit()
{
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

void SkiaOpenGLCanvas::addLine(const SkiaLine& line) { m_lines.push_back(line); update(); }
void SkiaOpenGLCanvas::addCircle(const SkiaCircle& circle) { m_circles.push_back(circle); update(); }
void SkiaOpenGLCanvas::addRect(const SkiaRect& rect) { m_rects.push_back(rect); update(); }
void SkiaOpenGLCanvas::clearGeometry() { m_lines.clear(); m_circles.clear(); m_rects.clear(); update(); }
void SkiaOpenGLCanvas::setShowGrid(bool show) { m_showGrid = show; update(); }
void SkiaOpenGLCanvas::setGridSpacing(float spacing) { m_gridSpacing = spacing; update(); }
void SkiaOpenGLCanvas::setBackgroundColor(uint32_t color) { m_backgroundColor = color; update(); }
void SkiaOpenGLCanvas::setGridColor(uint32_t color) { m_gridColor = color; update(); }
void SkiaOpenGLCanvas::setCurrentColor(uint32_t color) { m_currentColor = color; }
void SkiaOpenGLCanvas::setCurrentStrokeWidth(float w) { m_currentStrokeWidth = w; }

bool SkiaOpenGLCanvas::exportToSVG(const QString& filename)
{
    SkRect bounds = SkRect::MakeWH(width(), height());
    SkFILEWStream stream(filename.toStdString().c_str());
    if (!stream.isValid()) return false;
    
    std::unique_ptr<SkCanvas> svgCanvas = SkSVGCanvas::Make(bounds, &stream);
    if (!svgCanvas) return false;
    
    svgCanvas->clear(m_backgroundColor);
    svgCanvas->translate(width() / 2.0f + m_pan.x, height() / 2.0f + m_pan.y);
    svgCanvas->scale(m_zoom, -m_zoom);
    
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
    
    return true;
}

bool SkiaOpenGLCanvas::exportToPNG(const QString& filename)
{
    SkImageInfo info = SkImageInfo::MakeN32Premul(width(), height());
    sk_sp<SkSurface> rasterSurface = SkSurfaces::Raster(info);
    if (!rasterSurface) return false;
    
    SkCanvas* canvas = rasterSurface->getCanvas();
    canvas->clear(m_backgroundColor);
    canvas->translate(width() / 2.0f + m_pan.x, height() / 2.0f + m_pan.y);
    canvas->scale(m_zoom, -m_zoom);
    
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    
    for (const auto& line : m_lines) {
        paint.setColor(line.color);
        paint.setStrokeWidth(line.strokeWidth / m_zoom);
        canvas->drawLine(line.start.x, line.start.y, line.end.x, line.end.y, paint);
    }
    
    for (const auto& circle : m_circles) {
        paint.setColor(circle.color);
        paint.setStrokeWidth(circle.strokeWidth / m_zoom);
        canvas->drawCircle(circle.center.x, circle.center.y, circle.radius, paint);
    }
    
    for (const auto& rect : m_rects) {
        paint.setColor(rect.color);
        paint.setStrokeWidth(rect.strokeWidth / m_zoom);
        SkRect skRect = SkRect::MakeXYWH(rect.topLeft.x, rect.topLeft.y, rect.width, rect.height);
        canvas->drawRect(skRect, paint);
    }
    
    sk_sp<SkImage> image = rasterSurface->makeImageSnapshot();
    if (!image) return false;
    
    sk_sp<SkData> pngData = SkPngEncoder::Encode(nullptr, image.get(), {});
    if (!pngData) return false;
    
    SkFILEWStream pngStream(filename.toStdString().c_str());
    if (!pngStream.isValid()) return false;
    
    pngStream.write(pngData->data(), pngData->size());
    return true;
}

} // namespace DrawingGui
