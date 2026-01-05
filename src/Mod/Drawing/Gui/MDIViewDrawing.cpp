/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                   *
 *                                                                         *
 *   MDI View for Skia-based Drawing canvas                               *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#include "MDIViewDrawing.h"
#include "SkiaCanvas.h"
#include "SkiaGLCanvas.h"

#ifdef __APPLE__
#include "SkiaMetalCanvas.h"
#endif

#include <QToolBar>
#include <QAction>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QStatusBar>
#include <QLabel>
#include <QOpenGLContext>

#include <Gui/MainWindow.h>
#include <Gui/Document.h>
#include <Base/Console.h>

namespace DrawingGui {

TYPESYSTEM_SOURCE_ABSTRACT(DrawingGui::MDIViewDrawing, Gui::MDIView)

MDIViewDrawing::MDIViewDrawing(Gui::Document* doc, QWidget* parent, RenderBackend backend)
    : Gui::MDIView(doc, parent)
{
    initCanvas(backend);
    createActions();
    setupToolbar();
    
    // Set window title based on backend
    QString backendName;
    switch (m_backend) {
        case RenderBackend::OpenGL:
            backendName = QStringLiteral("GPU: OpenGL");
            break;
        case RenderBackend::Metal:
            backendName = QStringLiteral("GPU: Metal");
            break;
        case RenderBackend::CPU:
        default:
            backendName = QStringLiteral("CPU: Raster");
            break;
    }
    setWindowTitle(tr("Drawing Canvas [%1]").arg(backendName));
    resize(800, 600);
    
    Base::Console().message("MDIViewDrawing: Created with %s backend\n", 
                           backendName.toStdString().c_str());
}

MDIViewDrawing::~MDIViewDrawing()
{
    Base::Console().message("MDIViewDrawing: Destroyed\n");
}

void MDIViewDrawing::initCanvas(RenderBackend backend)
{
    // Auto-select best backend
    if (backend == RenderBackend::Auto) {
#ifdef __APPLE__
        // On macOS, prefer Metal, then OpenGL, then CPU
        backend = RenderBackend::Metal;
#else
        // On Windows/Linux, prefer OpenGL, then CPU
        backend = RenderBackend::OpenGL;
#endif
    }
    
    // Try to create the requested backend
#ifdef __APPLE__
    if (backend == RenderBackend::Metal) {
        m_metalCanvas = new SkiaMetalCanvas(this);
        if (m_metalCanvas->isGpuAvailable()) {
            m_backend = RenderBackend::Metal;
            
            connect(m_metalCanvas, &SkiaMetalCanvas::lineCreated, 
                    this, &MDIViewDrawing::onLineCreated);
            connect(m_metalCanvas, &SkiaMetalCanvas::circleCreated,
                    this, &MDIViewDrawing::onCircleCreated);
            connect(m_metalCanvas, &SkiaMetalCanvas::cursorPositionChanged,
                    this, &MDIViewDrawing::onCursorPositionChanged);
            
            Base::Console().message("MDIViewDrawing: Using Metal GPU rendering\n");
            return;
        } else {
            delete m_metalCanvas;
            m_metalCanvas = nullptr;
            backend = RenderBackend::OpenGL; // Fall back to OpenGL
        }
    }
#endif

    if (backend == RenderBackend::OpenGL) {
        // Check if OpenGL is available
        QOpenGLContext testContext;
        if (testContext.create()) {
            m_glCanvas = new SkiaGLCanvas(this);
            m_backend = RenderBackend::OpenGL;
            
            connect(m_glCanvas, &SkiaGLCanvas::lineCreated, 
                    this, &MDIViewDrawing::onLineCreated);
            connect(m_glCanvas, &SkiaGLCanvas::circleCreated,
                    this, &MDIViewDrawing::onCircleCreated);
            connect(m_glCanvas, &SkiaGLCanvas::cursorPositionChanged,
                    this, &MDIViewDrawing::onCursorPositionChanged);
            
            Base::Console().message("MDIViewDrawing: Using OpenGL GPU rendering\n");
            return;
        } else {
            Base::Console().warning("MDIViewDrawing: OpenGL not available, falling back to CPU\n");
            backend = RenderBackend::CPU;
        }
    }
    
    // CPU fallback
    m_cpuCanvas = new SkiaCanvas(this);
    m_backend = RenderBackend::CPU;
    
    connect(m_cpuCanvas, &SkiaCanvas::lineCreated, 
            this, &MDIViewDrawing::onLineCreated);
    connect(m_cpuCanvas, &SkiaCanvas::circleCreated,
            this, &MDIViewDrawing::onCircleCreated);
    connect(m_cpuCanvas, &SkiaCanvas::cursorPositionChanged,
            this, &MDIViewDrawing::onCursorPositionChanged);
    
    Base::Console().message("MDIViewDrawing: Using CPU (Raster) rendering\n");
}

void MDIViewDrawing::createActions()
{
    m_actLine = new QAction(tr("Line"), this);
    m_actLine->setShortcut(QKeySequence(tr("L")));
    m_actLine->setToolTip(tr("Draw a line"));
    connect(m_actLine, &QAction::triggered, this, &MDIViewDrawing::cmdDrawLine);
    
    m_actCircle = new QAction(tr("Circle"), this);
    m_actCircle->setShortcut(QKeySequence(tr("C")));
    m_actCircle->setToolTip(tr("Draw a circle"));
    connect(m_actCircle, &QAction::triggered, this, &MDIViewDrawing::cmdDrawCircle);
    
    m_actRectangle = new QAction(tr("Rectangle"), this);
    m_actRectangle->setShortcut(QKeySequence(tr("R")));
    m_actRectangle->setToolTip(tr("Draw a rectangle"));
    connect(m_actRectangle, &QAction::triggered, this, &MDIViewDrawing::cmdDrawRectangle);
    
    m_actZoomFit = new QAction(tr("Zoom Fit"), this);
    m_actZoomFit->setShortcut(QKeySequence(tr("F")));
    connect(m_actZoomFit, &QAction::triggered, this, &MDIViewDrawing::cmdZoomFit);
    
    m_actZoomIn = new QAction(tr("Zoom In"), this);
    m_actZoomIn->setShortcut(QKeySequence::ZoomIn);
    connect(m_actZoomIn, &QAction::triggered, this, &MDIViewDrawing::cmdZoomIn);
    
    m_actZoomOut = new QAction(tr("Zoom Out"), this);
    m_actZoomOut->setShortcut(QKeySequence::ZoomOut);
    connect(m_actZoomOut, &QAction::triggered, this, &MDIViewDrawing::cmdZoomOut);
    
    m_actExportSVG = new QAction(tr("Export SVG"), this);
    connect(m_actExportSVG, &QAction::triggered, this, &MDIViewDrawing::cmdExportSVG);
    
    m_actExportPNG = new QAction(tr("Export PNG"), this);
    connect(m_actExportPNG, &QAction::triggered, this, &MDIViewDrawing::cmdExportPNG);
    
    m_actClear = new QAction(tr("Clear"), this);
    connect(m_actClear, &QAction::triggered, this, &MDIViewDrawing::cmdClear);
}


void MDIViewDrawing::setupToolbar()
{
    QToolBar* toolbar = new QToolBar(tr("Drawing Tools"), this);
    toolbar->setMovable(false);
    
    toolbar->addAction(m_actLine);
    toolbar->addAction(m_actCircle);
    toolbar->addAction(m_actRectangle);
    toolbar->addSeparator();
    toolbar->addAction(m_actZoomFit);
    toolbar->addAction(m_actZoomIn);
    toolbar->addAction(m_actZoomOut);
    toolbar->addSeparator();
    toolbar->addAction(m_actExportSVG);
    toolbar->addAction(m_actExportPNG);
    toolbar->addSeparator();
    toolbar->addAction(m_actClear);
    
    // Add toolbar at top
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(toolbar);
    
    // Add the active canvas
    switch (m_backend) {
#ifdef __APPLE__
        case RenderBackend::Metal:
            if (m_metalCanvas) layout->addWidget(m_metalCanvas);
            break;
#endif
        case RenderBackend::OpenGL:
            if (m_glCanvas) layout->addWidget(m_glCanvas);
            break;
        case RenderBackend::CPU:
        default:
            if (m_cpuCanvas) layout->addWidget(m_cpuCanvas);
            break;
    }
    
    QWidget* container = new QWidget(this);
    container->setLayout(layout);
    setCentralWidget(container);
}

bool MDIViewDrawing::onMsg(const char* pMsg, const char** ppReturn)
{
    Q_UNUSED(ppReturn);
    
    if (strcmp(pMsg, "ViewFit") == 0) {
        cmdZoomFit();
        return true;
    }
    else if (strcmp(pMsg, "ZoomIn") == 0) {
        cmdZoomIn();
        return true;
    }
    else if (strcmp(pMsg, "ZoomOut") == 0) {
        cmdZoomOut();
        return true;
    }
    
    return false;
}

bool MDIViewDrawing::onHasMsg(const char* pMsg) const
{
    if (strcmp(pMsg, "ViewFit") == 0) return true;
    if (strcmp(pMsg, "ZoomIn") == 0) return true;
    if (strcmp(pMsg, "ZoomOut") == 0) return true;
    return false;
}

void MDIViewDrawing::onUpdate()
{
    switch (m_backend) {
#ifdef __APPLE__
        case RenderBackend::Metal:
            if (m_metalCanvas) m_metalCanvas->update();
            break;
#endif
        case RenderBackend::OpenGL:
            if (m_glCanvas) m_glCanvas->update();
            break;
        case RenderBackend::CPU:
        default:
            if (m_cpuCanvas) m_cpuCanvas->update();
            break;
    }
}

void MDIViewDrawing::closeEvent(QCloseEvent* event)
{
    Gui::MDIView::closeEvent(event);
}

// Command implementations - use macros to reduce repetition
#define CANVAS_CALL(method) \
    switch (m_backend) { \
        case RenderBackend::OpenGL: \
            if (m_glCanvas) m_glCanvas->method; \
            break; \
        case RenderBackend::CPU: \
        default: \
            if (m_cpuCanvas) m_cpuCanvas->method; \
            break; \
    }

#ifdef __APPLE__
#undef CANVAS_CALL
#define CANVAS_CALL(method) \
    switch (m_backend) { \
        case RenderBackend::Metal: \
            if (m_metalCanvas) m_metalCanvas->method; \
            break; \
        case RenderBackend::OpenGL: \
            if (m_glCanvas) m_glCanvas->method; \
            break; \
        case RenderBackend::CPU: \
        default: \
            if (m_cpuCanvas) m_cpuCanvas->method; \
            break; \
    }
#endif

void MDIViewDrawing::cmdDrawLine()
{
    CANVAS_CALL(setDrawMode(DrawMode::Line));
    Gui::getMainWindow()->showMessage(tr("Click first point for line..."));
}

void MDIViewDrawing::cmdDrawCircle()
{
    CANVAS_CALL(setDrawMode(DrawMode::Circle));
    Gui::getMainWindow()->showMessage(tr("Click center point for circle..."));
}

void MDIViewDrawing::cmdDrawRectangle()
{
    CANVAS_CALL(setDrawMode(DrawMode::Rectangle));
    Gui::getMainWindow()->showMessage(tr("Click first corner for rectangle..."));
}

void MDIViewDrawing::cmdZoomFit()
{
    CANVAS_CALL(zoomToFit());
}

void MDIViewDrawing::cmdZoomIn()
{
    switch (m_backend) {
#ifdef __APPLE__
        case RenderBackend::Metal:
            if (m_metalCanvas) m_metalCanvas->setZoom(m_metalCanvas->zoom() * 1.2f);
            break;
#endif
        case RenderBackend::OpenGL:
            if (m_glCanvas) m_glCanvas->setZoom(m_glCanvas->zoom() * 1.2f);
            break;
        case RenderBackend::CPU:
        default:
            if (m_cpuCanvas) m_cpuCanvas->setZoom(m_cpuCanvas->zoom() * 1.2f);
            break;
    }
}

void MDIViewDrawing::cmdZoomOut()
{
    switch (m_backend) {
#ifdef __APPLE__
        case RenderBackend::Metal:
            if (m_metalCanvas) m_metalCanvas->setZoom(m_metalCanvas->zoom() / 1.2f);
            break;
#endif
        case RenderBackend::OpenGL:
            if (m_glCanvas) m_glCanvas->setZoom(m_glCanvas->zoom() / 1.2f);
            break;
        case RenderBackend::CPU:
        default:
            if (m_cpuCanvas) m_cpuCanvas->setZoom(m_cpuCanvas->zoom() / 1.2f);
            break;
    }
}

void MDIViewDrawing::cmdExportSVG()
{
    QString filename = QFileDialog::getSaveFileName(
        this, tr("Export SVG"), QString(), tr("SVG Files (*.svg)"));
    
    if (!filename.isEmpty()) {
        if (!filename.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive)) {
            filename += QStringLiteral(".svg");
        }
        CANVAS_CALL(exportToSVG(filename));
    }
}

void MDIViewDrawing::cmdExportPNG()
{
    QString filename = QFileDialog::getSaveFileName(
        this, tr("Export PNG"), QString(), tr("PNG Files (*.png)"));
    
    if (!filename.isEmpty()) {
        if (!filename.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive)) {
            filename += QStringLiteral(".png");
        }
        CANVAS_CALL(exportToPNG(filename));
    }
}

void MDIViewDrawing::cmdClear()
{
    CANVAS_CALL(clearGeometry());
}

#undef CANVAS_CALL

// Slots
void MDIViewDrawing::onLineCreated(const SkiaLine& line)
{
    Q_UNUSED(line);
    Gui::getMainWindow()->showMessage(tr("Line created"));
}

void MDIViewDrawing::onCircleCreated(const SkiaCircle& circle)
{
    Q_UNUSED(circle);
    Gui::getMainWindow()->showMessage(tr("Circle created"));
}

void MDIViewDrawing::onCursorPositionChanged(float x, float y)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
}

} // namespace DrawingGui
