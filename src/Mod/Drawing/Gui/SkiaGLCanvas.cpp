/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                   *
 *                                                                         *
 *   Skia-based 2D Canvas with OpenGL GPU acceleration                    *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#include "SkiaGLCanvas.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QPainter>
#include <cmath>

// Skia headers
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkPath.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkPaint.h"
#include "include/core/SkColor.h"
#include "include/core/SkFont.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkStream.h"
#include "include/core/SkData.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkImage.h"
#include "include/encode/SkPngEncoder.h"
#include "include/svg/SkSVGCanvas.h"

// Skia GPU headers
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/GpuTypes.h"

#include <Base/Console.h>

namespace DrawingGui {

// Private implementation with dedicated OpenGL context for Skia
class SkiaGLCanvas::SkiaGLImpl {
public:
    // Dedicated OpenGL context for Skia (separate from Qt's context)
    QOpenGLContext* skiaContext = nullptr;
    QOffscreenSurface* offscreenSurface = nullptr;
    
    // Skia objects
    sk_sp<GrDirectContext> grContext;
    sk_sp<SkSurface> surface;
    SkImageInfo imageInfo;
    
    // Cached image for display
    QImage cachedImage;
    
    bool initialized = false;
    bool gpuOk = false;
    int surfaceWidth = 0;
    int surfaceHeight = 0;
    
    ~SkiaGLImpl() {
        cleanup();
    }
    
    void cleanup() {
        // Must make Skia context current before cleanup
        if (skiaContext && offscreenSurface) {
            skiaContext->makeCurrent(offscreenSurface);
        }
        
        surface.reset();
        if (grContext) {
            grContext->abandonContext();
            grContext.reset();
        }
        
        if (skiaContext) {
            skiaContext->doneCurrent();
            delete skiaContext;
            skiaContext = nullptr;
        }
        
        if (offscreenSurface) {
            delete offscreenSurface;
            offscreenSurface = nullptr;
        }
        
        initialized = false;
        gpuOk = false;
    }
    
    bool initContext(QOpenGLContext* shareContext) {
        // Create dedicated OpenGL context for Skia
        skiaContext = new QOpenGLContext();
        
        // Share with Qt's context for potential resource sharing
        if (shareContext) {
            skiaContext->setShareContext(shareContext);
        }
        
        // Use same format as share context
        QSurfaceFormat format;
        format.setVersion(3, 3);
        format.setProfile(QSurfaceFormat::CoreProfile);
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
        skiaContext->setFormat(format);
        
        if (!skiaContext->create()) {
            Base::Console().error("SkiaGLCanvas: Failed to create dedicated GL context\n");
            delete skiaContext;
            skiaContext = nullptr;
            return false;
        }
        
        // Create offscreen surface for the context
        offscreenSurface = new QOffscreenSurface();
        offscreenSurface->setFormat(skiaContext->format());
        offscreenSurface->create();
        
        if (!offscreenSurface->isValid()) {
            Base::Console().error("SkiaGLCanvas: Failed to create offscreen surface\n");
            delete offscreenSurface;
            offscreenSurface = nullptr;
            delete skiaContext;
            skiaContext = nullptr;
            return false;
        }
        
        // Make Skia context current and create GrDirectContext
        if (!skiaContext->makeCurrent(offscreenSurface)) {
            Base::Console().error("SkiaGLCanvas: Failed to make Skia context current\n");
            cleanup();
            return false;
        }
        
        // Create Skia GPU context
        grContext = GrDirectContexts::MakeGL();
        if (!grContext) {
            Base::Console().warning("SkiaGLCanvas: GPU context failed, will use CPU\n");
            gpuOk = false;
        } else {
            gpuOk = true;
            Base::Console().message("SkiaGLCanvas: Dedicated GPU context created\n");
        }
        
        skiaContext->doneCurrent();
        initialized = true;
        return true;
    }
    
    bool createSurface(int width, int height) {
        if (width <= 0 || height <= 0) return false;
        if (!initialized) return false;
        
        surfaceWidth = width;
        surfaceHeight = height;
        imageInfo = SkImageInfo::MakeN32Premul(width, height);
        
        // Make Skia context current
        if (skiaContext && offscreenSurface) {
            skiaContext->makeCurrent(offscreenSurface);
        }
        
        surface.reset();
        
        if (gpuOk && grContext) {
            // Create GPU-backed offscreen surface
            surface = SkSurfaces::RenderTarget(
                grContext.get(),
                skgpu::Budgeted::kNo,
                imageInfo
            );
            if (surface) {
                if (skiaContext) skiaContext->doneCurrent();
                return true;
            }
            Base::Console().warning("SkiaGLCanvas: GPU surface failed, using CPU\n");
            gpuOk = false;
        }
        
        // CPU fallback
        surface = SkSurfaces::Raster(imageInfo);
        if (skiaContext) skiaContext->doneCurrent();
        return surface != nullptr;
    }
    
    SkCanvas* beginRender() {
        if (!surface) return nullptr;
        
        // Make Skia context current for rendering
        if (gpuOk && skiaContext && offscreenSurface) {
            skiaContext->makeCurrent(offscreenSurface);
        }
        
        return surface->getCanvas();
    }
    
    void endRender() {
        if (!surface) return;
        
        if (gpuOk && grContext) {
            grContext->flushAndSubmit();
            
            // Read pixels back to cached image
            SkBitmap bitmap;
            if (bitmap.tryAllocPixels(imageInfo)) {
                if (surface->readPixels(bitmap.pixmap(), 0, 0)) {
                    cachedImage = QImage(
                        bitmap.width(), bitmap.height(),
                        QImage::Format_RGBA8888_Premultiplied
                    );
                    for (int y = 0; y < bitmap.height(); ++y) {
                        memcpy(cachedImage.scanLine(y), 
                               bitmap.getAddr(0, y), 
                               bitmap.width() * 4);
                    }
                }
            }
        } else {
            // CPU surface - peek pixels directly
            SkPixmap pixmap;
            if (surface->peekPixels(&pixmap)) {
                QImage img(
                    static_cast<const uchar*>(pixmap.addr()),
                    pixmap.width(), pixmap.height(),
                    static_cast<qsizetype>(pixmap.rowBytes()),
                    QImage::Format_RGBA8888_Premultiplied
                );
                cachedImage = img.copy();
            }
        }
        
        // Release Skia context
        if (gpuOk && skiaContext) {
            skiaContext->doneCurrent();
        }
    }
    
    const QImage& getCachedImage() const {
        return cachedImage;
    }
};

SkiaGLCanvas::SkiaGLCanvas(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_impl(std::make_unique<SkiaGLImpl>())
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    Base::Console().message("SkiaGLCanvas: Created\n");
}

SkiaGLCanvas::~SkiaGLCanvas()
{
    m_timer.stop();
    // No need to makeCurrent here - impl cleanup handles its own context
    m_impl->cleanup();
}

void SkiaGLCanvas::initializeGL()
{
    initializeOpenGLFunctions();
    
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    Base::Console().message("SkiaGLCanvas: Qt GL Renderer: %s\n", renderer ? renderer : "?");
    
    // Initialize Skia with its own dedicated context, sharing with Qt's context
    QOpenGLContext* qtContext = QOpenGLContext::currentContext();
    if (!m_impl->initContext(qtContext)) {
        Base::Console().error("SkiaGLCanvas: Failed to init Skia context\n");
        return;
    }
    
    // Create initial surface
    int w = width() > 0 ? width() : 800;
    int h = height() > 0 ? height() : 600;
    if (!m_impl->createSurface(w, h)) {
        Base::Console().error("SkiaGLCanvas: Failed to create initial surface\n");
    }
    
    m_elapsedTimer.start();
    Base::Console().message("SkiaGLCanvas: Init done (GPU=%s)\n", m_impl->gpuOk ? "yes" : "no");
}

void SkiaGLCanvas::resizeGL(int w, int h)
{
    if (w <= 0 || h <= 0) return;
    if (!m_impl || !m_impl->initialized) return;
    
    glViewport(0, 0, w, h);
    m_impl->createSurface(w, h);
}

void SkiaGLCanvas::initSkiaGL(int w, int h)
{
    m_impl->createSurface(w, h);
}

void SkiaGLCanvas::cleanupSkia()
{
    m_impl->cleanup();
}

void SkiaGLCanvas::paintGL()
{
    if (!isVisible() || !m_impl || !m_impl->initialized) return;
    
    // Ensure we have a valid surface
    if (!m_impl->surface) {
        int w = width() > 0 ? width() : 100;
        int h = height() > 0 ? height() : 100;
        if (!m_impl->createSurface(w, h)) return;
    }
    
    // Begin rendering with Skia's dedicated context
    SkCanvas* canvas = m_impl->beginRender();
    if (!canvas) return;
    
    // Render to Skia surface
    render(canvas);
    
    // End rendering and read back pixels
    m_impl->endRender();
    
    // Now we're back in Qt's context - blit the cached image
    const QImage& img = m_impl->getCachedImage();
    if (!img.isNull()) {
        QPainter painter(this);
        painter.drawImage(0, 0, img);
    }
}


void SkiaGLCanvas::render(SkCanvas* canvas)
{
    if (!canvas) return;
    
    canvas->clear(m_backgroundColor);
    canvas->save();
    canvas->translate(width() / 2.0f + m_pan.x, height() / 2.0f + m_pan.y);
    canvas->scale(m_zoom, -m_zoom);
    
    if (m_showGrid) drawGrid(canvas);
    drawGeometry(canvas);
    drawTempGeometry(canvas);
    
    canvas->restore();
    drawCursor(canvas);
}

void SkiaGLCanvas::drawGrid(SkCanvas* canvas)
{
    if (!canvas) return;
    
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
    
    int maxLines = 200, lineCount = 0;
    for (float x = startX; x <= right && lineCount < maxLines; x += m_gridSpacing, lineCount++)
        canvas->drawLine(x, bottom, x, top, paint);
    for (float y = startY; y <= top && lineCount < maxLines; y += m_gridSpacing, lineCount++)
        canvas->drawLine(left, y, right, y, paint);
    
    paint.setColor(0xFFCCCCCC);
    paint.setStrokeWidth(2.0f / m_zoom);
    canvas->drawLine(left, 0, right, 0, paint);
    canvas->drawLine(0, bottom, 0, top, paint);
}

void SkiaGLCanvas::drawGeometry(SkCanvas* canvas)
{
    if (!canvas) return;
    
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
        canvas->drawRect(SkRect::MakeXYWH(rect.topLeft.x, rect.topLeft.y, rect.width, rect.height), paint);
    }
}

void SkiaGLCanvas::drawTempGeometry(SkCanvas* canvas)
{
    if (!canvas || m_tempPoints.empty()) return;
    
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(0xFF0066FF);
    paint.setStrokeWidth(2.0f / m_zoom);
    
    switch (m_drawMode) {
        case DrawMode::Line:
            if (!m_tempPoints.empty())
                canvas->drawLine(m_tempPoints[0].x, m_tempPoints[0].y, m_cursorWorld.x, m_cursorWorld.y, paint);
            break;
        case DrawMode::Circle:
            if (!m_tempPoints.empty()) {
                float r = std::sqrt(std::pow(m_cursorWorld.x - m_tempPoints[0].x, 2) + 
                                   std::pow(m_cursorWorld.y - m_tempPoints[0].y, 2));
                if (r > 0.001f) canvas->drawCircle(m_tempPoints[0].x, m_tempPoints[0].y, r, paint);
            }
            break;
        case DrawMode::Rectangle:
            if (!m_tempPoints.empty()) {
                float x = std::min(m_tempPoints[0].x, m_cursorWorld.x);
                float y = std::min(m_tempPoints[0].y, m_cursorWorld.y);
                float w = std::abs(m_cursorWorld.x - m_tempPoints[0].x);
                float h = std::abs(m_cursorWorld.y - m_tempPoints[0].y);
                if (w > 0.001f && h > 0.001f) canvas->drawRect(SkRect::MakeXYWH(x, y, w, h), paint);
            }
            break;
        default: break;
    }
    
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(0xFFFF0000);
    for (const auto& pt : m_tempPoints)
        canvas->drawCircle(pt.x, pt.y, 4.0f / m_zoom, paint);
}

void SkiaGLCanvas::drawCursor(SkCanvas* canvas)
{
    if (!canvas) return;
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(0xFF333333);
    SkFont font;
    font.setSize(12);
    char text[64];
    snprintf(text, sizeof(text), "X: %.2f  Y: %.2f  [%s]", 
             m_cursorWorld.x, m_cursorWorld.y, m_impl->gpuOk ? "GPU" : "CPU");
    canvas->drawString(text, 10, height() - 10, font, paint);
}

SkiaPoint SkiaGLCanvas::screenToWorld(const QPoint& p) const
{
    return SkiaPoint(
        (p.x() - width() / 2.0f - m_pan.x) / m_zoom,
        -(p.y() - height() / 2.0f - m_pan.y) / m_zoom
    );
}

QPoint SkiaGLCanvas::worldToScreen(const SkiaPoint& p) const
{
    return QPoint(
        static_cast<int>(p.x * m_zoom + width() / 2.0f + m_pan.x),
        static_cast<int>(-p.y * m_zoom + height() / 2.0f + m_pan.y)
    );
}

void SkiaGLCanvas::mousePressEvent(QMouseEvent* event)
{
    if (!event) return;
    
    m_lastMousePos = event->pos();
    m_cursorWorld = screenToWorld(event->pos());
    
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    
    if (event->button() == Qt::LeftButton) {
        if (m_drawMode == DrawMode::Line || m_drawMode == DrawMode::Circle || m_drawMode == DrawMode::Rectangle) {
            if (m_tempPoints.empty()) {
                m_tempPoints.push_back(m_cursorWorld);
                m_isDrawing = true;
            } else {
                finishCurrentDrawing();
            }
        } else if (m_drawMode == DrawMode::Polyline) {
            m_tempPoints.push_back(m_cursorWorld);
            m_isDrawing = true;
        }
    }
    
    if (event->button() == Qt::RightButton && m_isDrawing) {
        cancelCurrentDrawing();
    }
    
    update();
}

void SkiaGLCanvas::mouseMoveEvent(QMouseEvent* event)
{
    if (!event) return;
    
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

void SkiaGLCanvas::mouseReleaseEvent(QMouseEvent* event)
{
    if (!event) return;
    
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
}

void SkiaGLCanvas::wheelEvent(QWheelEvent* event)
{
    if (!event) return;
    
    float factor = event->angleDelta().y() > 0 ? 1.1f : 0.9f;
    QPoint pos = event->position().toPoint();
    SkiaPoint before = screenToWorld(pos);
    m_zoom = std::clamp(m_zoom * factor, 0.01f, 100.0f);
    SkiaPoint after = screenToWorld(pos);
    m_pan.x += (after.x - before.x) * m_zoom;
    m_pan.y -= (after.y - before.y) * m_zoom;
    Q_EMIT viewChanged();
    update();
}

void SkiaGLCanvas::keyPressEvent(QKeyEvent* event)
{
    if (!event) return;
    
    if (event->key() == Qt::Key_Escape) {
        cancelCurrentDrawing();
        setDrawMode(DrawMode::None);
    }
    QOpenGLWidget::keyPressEvent(event);
}

void SkiaGLCanvas::finishCurrentDrawing()
{
    if (m_drawMode == DrawMode::Line && !m_tempPoints.empty()) {
        SkiaLine line{m_tempPoints[0], m_cursorWorld, m_currentStrokeWidth, m_currentColor};
        m_lines.push_back(line);
        Q_EMIT lineCreated(line);
    } else if (m_drawMode == DrawMode::Circle && !m_tempPoints.empty()) {
        float r = std::sqrt(std::pow(m_cursorWorld.x - m_tempPoints[0].x, 2) + 
                           std::pow(m_cursorWorld.y - m_tempPoints[0].y, 2));
        SkiaCircle circle{m_tempPoints[0], r, m_currentStrokeWidth, m_currentColor, false};
        m_circles.push_back(circle);
        Q_EMIT circleCreated(circle);
    } else if (m_drawMode == DrawMode::Rectangle && !m_tempPoints.empty()) {
        SkiaRect rect;
        rect.topLeft = SkiaPoint(std::min(m_tempPoints[0].x, m_cursorWorld.x),
                                  std::min(m_tempPoints[0].y, m_cursorWorld.y));
        rect.width = std::abs(m_cursorWorld.x - m_tempPoints[0].x);
        rect.height = std::abs(m_cursorWorld.y - m_tempPoints[0].y);
        rect.strokeWidth = m_currentStrokeWidth;
        rect.color = m_currentColor;
        rect.filled = false;
        m_rects.push_back(rect);
        Q_EMIT rectCreated(rect);
    }
    m_tempPoints.clear();
    m_isDrawing = false;
    update();
}

void SkiaGLCanvas::cancelCurrentDrawing()
{
    m_tempPoints.clear();
    m_isDrawing = false;
    update();
}


// Public API implementations
void SkiaGLCanvas::setDrawMode(DrawMode mode)
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
    Base::Console().message("SkiaGLCanvas: setDrawMode %d\n", static_cast<int>(mode));
}

void SkiaGLCanvas::setZoom(float zoom)
{
    m_zoom = std::clamp(zoom, 0.01f, 100.0f);
    Q_EMIT viewChanged();
    update();
}

void SkiaGLCanvas::setPan(float x, float y)
{
    m_pan.x = x;
    m_pan.y = y;
    Q_EMIT viewChanged();
    update();
}

void SkiaGLCanvas::resetView()
{
    m_zoom = 1.0f;
    m_pan = SkiaPoint(0, 0);
    Q_EMIT viewChanged();
    update();
}

void SkiaGLCanvas::zoomToFit()
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

void SkiaGLCanvas::addLine(const SkiaLine& line)
{
    m_lines.push_back(line);
    update();
}

void SkiaGLCanvas::addCircle(const SkiaCircle& circle)
{
    m_circles.push_back(circle);
    update();
}

void SkiaGLCanvas::addRect(const SkiaRect& rect)
{
    m_rects.push_back(rect);
    update();
}

void SkiaGLCanvas::clearGeometry()
{
    m_lines.clear();
    m_circles.clear();
    m_rects.clear();
    update();
}

void SkiaGLCanvas::setShowGrid(bool show)
{
    m_showGrid = show;
    update();
}

void SkiaGLCanvas::setGridSpacing(float spacing)
{
    m_gridSpacing = spacing;
    update();
}

void SkiaGLCanvas::setBackgroundColor(uint32_t color)
{
    m_backgroundColor = color;
    update();
}

void SkiaGLCanvas::setGridColor(uint32_t color)
{
    m_gridColor = color;
    update();
}

void SkiaGLCanvas::setCurrentColor(uint32_t color)
{
    m_currentColor = color;
}

void SkiaGLCanvas::setCurrentStrokeWidth(float width)
{
    m_currentStrokeWidth = width;
}

bool SkiaGLCanvas::exportToSVG(const QString& filename)
{
    if (!m_impl->surface) return false;
    
    SkRect bounds = SkRect::MakeWH(width(), height());
    SkFILEWStream stream(filename.toStdString().c_str());
    if (!stream.isValid()) {
        Base::Console().error("SkiaGLCanvas: Failed to open SVG file\n");
        return false;
    }
    
    std::unique_ptr<SkCanvas> svgCanvas = SkSVGCanvas::Make(bounds, &stream);
    if (!svgCanvas) {
        Base::Console().error("SkiaGLCanvas: Failed to create SVG canvas\n");
        return false;
    }
    
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
        svgCanvas->drawRect(SkRect::MakeXYWH(rect.topLeft.x, rect.topLeft.y, rect.width, rect.height), paint);
    }
    
    Base::Console().message("SkiaGLCanvas: Exported to SVG: %s\n", filename.toStdString().c_str());
    return true;
}

bool SkiaGLCanvas::exportToPNG(const QString& filename)
{
    if (!m_impl->surface) return false;
    
    sk_sp<SkImage> image = m_impl->surface->makeImageSnapshot();
    if (!image) {
        Base::Console().error("SkiaGLCanvas: Failed to create image snapshot\n");
        return false;
    }
    
    sk_sp<SkData> pngData = SkPngEncoder::Encode(nullptr, image.get(), {});
    if (!pngData) {
        Base::Console().error("SkiaGLCanvas: Failed to encode PNG\n");
        return false;
    }
    
    SkFILEWStream stream(filename.toStdString().c_str());
    if (!stream.isValid()) {
        Base::Console().error("SkiaGLCanvas: Failed to open PNG file\n");
        return false;
    }
    
    stream.write(pngData->data(), pngData->size());
    Base::Console().message("SkiaGLCanvas: Exported to PNG: %s\n", filename.toStdString().c_str());
    return true;
}

} // namespace DrawingGui
