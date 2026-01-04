/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                   *
 *                                                                         *
 *   Skia Metal GPU Canvas implementation (macOS)                         *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#include "SkiaMetalCanvas.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QWindow>
#include <cmath>

// Metal and Cocoa headers
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Cocoa/Cocoa.h>

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

// Skia GPU headers
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/mtl/GrMtlDirectContext.h"
#include "include/gpu/ganesh/mtl/GrMtlBackendContext.h"
#include "include/gpu/ganesh/mtl/GrMtlBackendSurface.h"
#include "include/gpu/ganesh/mtl/GrMtlTypes.h"

#include <Base/Console.h>

namespace DrawingGui {

// Metal implementation class
class SkiaMetalCanvas::MetalImpl {
public:
    id<MTLDevice> device = nil;
    id<MTLCommandQueue> commandQueue = nil;
    CAMetalLayer* metalLayer = nil;
    sk_sp<GrDirectContext> grContext;
    sk_sp<SkSurface> surface;
    SkCanvas* canvas = nullptr;
    
    int width = 0;
    int height = 0;
    
    bool init(QWidget* widget) {
        // Get default Metal device
        device = MTLCreateSystemDefaultDevice();
        if (!device) {
            Base::Console().error("SkiaMetalCanvas: No Metal device available\n");
            return false;
        }
        
        Base::Console().message("SkiaMetalCanvas: Using Metal device: %s\n", 
            [[device name] UTF8String]);
        
        // Create command queue
        commandQueue = [device newCommandQueue];
        if (!commandQueue) {
            Base::Console().error("SkiaMetalCanvas: Failed to create command queue\n");
            return false;
        }
        
        // Create Metal layer
        metalLayer = [CAMetalLayer layer];
        metalLayer.device = device;
        metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        metalLayer.framebufferOnly = NO;  // Allow reading back for export
        metalLayer.contentsScale = widget->devicePixelRatio();
        
        // Attach Metal layer to widget's native view
        NSView* view = reinterpret_cast<NSView*>(widget->winId());
        if (!view) {
            Base::Console().error("SkiaMetalCanvas: Failed to get native view\n");
            return false;
        }
        
        [view setWantsLayer:YES];
        [view setLayer:metalLayer];
        
        // Create Skia GPU context
        GrMtlBackendContext backendContext = {};
        backendContext.fDevice.retain((__bridge void*)device);
        backendContext.fQueue.retain((__bridge void*)commandQueue);
        
        grContext = GrDirectContexts::MakeMetal(backendContext);
        if (!grContext) {
            Base::Console().error("SkiaMetalCanvas: Failed to create Skia GPU context\n");
            return false;
        }
        
        Base::Console().message("SkiaMetalCanvas: GPU context created successfully\n");
        return true;
    }
    
    bool createSurface(int w, int h, qreal devicePixelRatio) {
        if (!grContext || !metalLayer) return false;
        
        width = w * devicePixelRatio;
        height = h * devicePixelRatio;
        
        if (width <= 0 || height <= 0) return false;
        
        // Update Metal layer size
        metalLayer.drawableSize = CGSizeMake(width, height);
        
        // Surface will be created per-frame from drawable
        return true;
    }
    
    bool beginFrame() {
        if (!grContext || !metalLayer) return false;
        
        @autoreleasepool {
            id<CAMetalDrawable> drawable = [metalLayer nextDrawable];
            if (!drawable) {
                return false;
            }
            
            // Create Skia surface from Metal texture
            GrMtlTextureInfo textureInfo;
            textureInfo.fTexture.retain((__bridge void*)drawable.texture);
            
            GrBackendRenderTarget backendRT = GrBackendRenderTargets::MakeMtl(
                width, height, textureInfo);
            
            surface = SkSurfaces::WrapBackendRenderTarget(
                grContext.get(),
                backendRT,
                kTopLeft_GrSurfaceOrigin,
                kBGRA_8888_SkColorType,
                nullptr,  // colorSpace
                nullptr   // surfaceProps
            );
            
            if (!surface) {
                return false;
            }
            
            canvas = surface->getCanvas();
            
            // Store drawable for endFrame
            currentDrawable = drawable;
            return true;
        }
    }
    
    void endFrame() {
        if (!grContext || !currentDrawable) return;
        
        @autoreleasepool {
            // Flush Skia commands
            grContext->flushAndSubmit(surface.get());
            
            // Present drawable
            id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
            [commandBuffer presentDrawable:currentDrawable];
            [commandBuffer commit];
            
            // Clear references
            surface.reset();
            canvas = nullptr;
            currentDrawable = nil;
        }
    }
    
    void cleanup() {
        surface.reset();
        canvas = nullptr;
        grContext.reset();
        currentDrawable = nil;
        metalLayer = nil;
        commandQueue = nil;
        device = nil;
    }
    
private:
    id<CAMetalDrawable> currentDrawable = nil;
};

// SkiaMetalCanvas implementation
SkiaMetalCanvas::SkiaMetalCanvas(QWidget* parent)
    : QWidget(parent)
    , m_metal(std::make_unique<MetalImpl>())
{
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_NativeWindow);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    
    // Initialize Metal
    m_gpuAvailable = m_metal->init(this);
    
    if (m_gpuAvailable) {
        Base::Console().message("SkiaMetalCanvas: GPU rendering enabled\n");
    } else {
        Base::Console().warning("SkiaMetalCanvas: GPU not available, falling back to CPU\n");
    }
}

SkiaMetalCanvas::~SkiaMetalCanvas()
{
    cleanup();
}

bool SkiaMetalCanvas::initMetal()
{
    return m_metal->init(this);
}

bool SkiaMetalCanvas::initSkiaGpu()
{
    return m_metal->createSurface(width(), height(), devicePixelRatio());
}

void SkiaMetalCanvas::cleanup()
{
    m_metal->cleanup();
}

void SkiaMetalCanvas::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    
    if (!m_gpuAvailable) return;
    
    m_metal->createSurface(width(), height(), devicePixelRatio());
    
    if (!m_metal->beginFrame()) {
        return;
    }
    
    render();
    
    m_metal->endFrame();
}

void SkiaMetalCanvas::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_gpuAvailable) {
        m_metal->createSurface(event->size().width(), event->size().height(), devicePixelRatio());
    }
    update();
}

void SkiaMetalCanvas::render()
{
    SkCanvas* canvas = m_metal->canvas;
    if (!canvas) return;
    
    float scale = devicePixelRatio();
    
    // Clear background
    canvas->clear(m_backgroundColor);
    
    // Apply view transformation
    canvas->save();
    canvas->scale(scale, scale);  // Handle high DPI
    canvas->translate(width() / 2.0f + m_pan.x, height() / 2.0f + m_pan.y);
    canvas->scale(m_zoom, -m_zoom);
    
    // Draw grid
    if (m_showGrid) {
        drawGrid();
    }
    
    // Draw all geometry
    drawGeometry();
    
    // Draw temporary geometry
    drawTempGeometry();
    
    canvas->restore();
    
    // Draw cursor info
    canvas->save();
    canvas->scale(scale, scale);
    drawCursor();
    canvas->restore();
}

void SkiaMetalCanvas::drawGrid()
{
    SkCanvas* canvas = m_metal->canvas;
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
    
    for (float x = startX; x <= right; x += m_gridSpacing) {
        canvas->drawLine(x, bottom, x, top, paint);
    }
    
    for (float y = startY; y <= top; y += m_gridSpacing) {
        canvas->drawLine(left, y, right, y, paint);
    }
    
    // Axes
    paint.setColor(0xFFCCCCCC);
    paint.setStrokeWidth(2.0f / m_zoom);
    canvas->drawLine(left, 0, right, 0, paint);
    canvas->drawLine(0, bottom, 0, top, paint);
}

void SkiaMetalCanvas::drawGeometry()
{
    SkCanvas* canvas = m_metal->canvas;
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
        SkRect skRect = SkRect::MakeXYWH(rect.topLeft.x, rect.topLeft.y, rect.width, rect.height);
        canvas->drawRect(skRect, paint);
    }
}

void SkiaMetalCanvas::drawTempGeometry()
{
    SkCanvas* canvas = m_metal->canvas;
    if (!canvas || m_tempPoints.empty()) return;
    
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
    
    // Draw points
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(0xFFFF0000);
    for (const auto& pt : m_tempPoints) {
        canvas->drawCircle(pt.x, pt.y, 4.0f / m_zoom, paint);
    }
}

void SkiaMetalCanvas::drawCursor()
{
    SkCanvas* canvas = m_metal->canvas;
    if (!canvas) return;
    
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(0xFF333333);
    
    SkFont font;
    font.setSize(12);
    
    char coordText[64];
    snprintf(coordText, sizeof(coordText), "X: %.2f  Y: %.2f  [GPU: Metal]", 
             m_cursorWorld.x, m_cursorWorld.y);
    
    canvas->drawString(coordText, 10, height() - 10, font, paint);
}

SkiaPoint SkiaMetalCanvas::screenToWorld(const QPoint& screenPos) const
{
    float x = (screenPos.x() - width() / 2.0f - m_pan.x) / m_zoom;
    float y = -(screenPos.y() - height() / 2.0f - m_pan.y) / m_zoom;
    return SkiaPoint(x, y);
}

QPoint SkiaMetalCanvas::worldToScreen(const SkiaPoint& worldPos) const
{
    int x = static_cast<int>(worldPos.x * m_zoom + width() / 2.0f + m_pan.x);
    int y = static_cast<int>(-worldPos.y * m_zoom + height() / 2.0f + m_pan.y);
    return QPoint(x, y);
}

// Mouse event handlers
void SkiaMetalCanvas::mousePressEvent(QMouseEvent* event)
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

void SkiaMetalCanvas::mouseMoveEvent(QMouseEvent* event)
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

void SkiaMetalCanvas::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
}

void SkiaMetalCanvas::wheelEvent(QWheelEvent* event)
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

void SkiaMetalCanvas::keyPressEvent(QKeyEvent* event)
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

void SkiaMetalCanvas::finishCurrentDrawing()
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
                Base::Console().message("SkiaMetalCanvas: Line created\n");
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
                Base::Console().message("SkiaMetalCanvas: Circle created\n");
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
                Base::Console().message("SkiaMetalCanvas: Rectangle created\n");
            }
            break;
            
        default:
            break;
    }
    
    m_tempPoints.clear();
    m_isDrawing = false;
    update();
}

void SkiaMetalCanvas::cancelCurrentDrawing()
{
    m_tempPoints.clear();
    m_isDrawing = false;
    update();
}

// Public API
void SkiaMetalCanvas::setDrawMode(DrawMode mode)
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

void SkiaMetalCanvas::setZoom(float zoom)
{
    m_zoom = std::max(0.01f, std::min(100.0f, zoom));
    Q_EMIT viewChanged();
    update();
}

void SkiaMetalCanvas::setPan(float x, float y)
{
    m_pan.x = x;
    m_pan.y = y;
    Q_EMIT viewChanged();
    update();
}

void SkiaMetalCanvas::resetView()
{
    m_zoom = 1.0f;
    m_pan = SkiaPoint(0, 0);
    Q_EMIT viewChanged();
    update();
}

void SkiaMetalCanvas::zoomToFit()
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

void SkiaMetalCanvas::addLine(const SkiaLine& line)
{
    m_lines.push_back(line);
    update();
}

void SkiaMetalCanvas::addCircle(const SkiaCircle& circle)
{
    m_circles.push_back(circle);
    update();
}

void SkiaMetalCanvas::addRect(const SkiaRect& rect)
{
    m_rects.push_back(rect);
    update();
}

void SkiaMetalCanvas::clearGeometry()
{
    m_lines.clear();
    m_circles.clear();
    m_rects.clear();
    update();
}

void SkiaMetalCanvas::setShowGrid(bool show)
{
    m_showGrid = show;
    update();
}

void SkiaMetalCanvas::setGridSpacing(float spacing)
{
    m_gridSpacing = spacing;
    update();
}

void SkiaMetalCanvas::setBackgroundColor(uint32_t color)
{
    m_backgroundColor = color;
    update();
}

void SkiaMetalCanvas::setGridColor(uint32_t color)
{
    m_gridColor = color;
    update();
}

void SkiaMetalCanvas::setCurrentColor(uint32_t color)
{
    m_currentColor = color;
}

void SkiaMetalCanvas::setCurrentStrokeWidth(float w)
{
    m_currentStrokeWidth = w;
}

bool SkiaMetalCanvas::exportToSVG(const QString& filename)
{
    // For SVG export, we render to a CPU surface
    SkRect bounds = SkRect::MakeWH(width(), height());
    
    SkFILEWStream stream(filename.toStdString().c_str());
    if (!stream.isValid()) {
        Base::Console().error("SkiaMetalCanvas: Failed to open SVG file\n");
        return false;
    }
    
    std::unique_ptr<SkCanvas> svgCanvas = SkSVGCanvas::Make(bounds, &stream);
    if (!svgCanvas) {
        Base::Console().error("SkiaMetalCanvas: Failed to create SVG canvas\n");
        return false;
    }
    
    // Render to SVG
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
    
    Base::Console().message("SkiaMetalCanvas: Exported to SVG\n");
    return true;
}

bool SkiaMetalCanvas::exportToPNG(const QString& filename)
{
    // Create a raster surface for PNG export
    SkImageInfo info = SkImageInfo::MakeN32Premul(width(), height());
    sk_sp<SkSurface> rasterSurface = SkSurfaces::Raster(info);
    if (!rasterSurface) {
        Base::Console().error("SkiaMetalCanvas: Failed to create raster surface for PNG\n");
        return false;
    }
    
    SkCanvas* canvas = rasterSurface->getCanvas();
    
    // Render to raster surface
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
    if (!image) {
        Base::Console().error("SkiaMetalCanvas: Failed to create image snapshot\n");
        return false;
    }
    
    sk_sp<SkData> pngData = SkPngEncoder::Encode(nullptr, image.get(), {});
    if (!pngData) {
        Base::Console().error("SkiaMetalCanvas: Failed to encode PNG\n");
        return false;
    }
    
    SkFILEWStream stream(filename.toStdString().c_str());
    if (!stream.isValid()) {
        Base::Console().error("SkiaMetalCanvas: Failed to open PNG file\n");
        return false;
    }
    
    stream.write(pngData->data(), pngData->size());
    Base::Console().message("SkiaMetalCanvas: Exported to PNG\n");
    return true;
}

} // namespace DrawingGui
