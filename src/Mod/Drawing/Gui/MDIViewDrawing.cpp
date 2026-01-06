/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                   *
 *                                                                         *
 *   MDI View for Skia-based Drawing canvas                               *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#include "MDIViewDrawing.h"
#include "SkiaCanvas.h"
#include "SkiaOpenGLCanvas.h"

#ifdef __APPLE__
#include "SkiaMetalCanvas.h"
#endif

#include <QToolBar>
#include <QAction>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QStatusBar>
#include <QLabel>

#include <Gui/MainWindow.h>
#include <Gui/Document.h>
#include <Base/Console.h>

namespace DrawingGui {

TYPESYSTEM_SOURCE_ABSTRACT(DrawingGui::MDIViewDrawing, Gui::MDIView)

MDIViewDrawing::MDIViewDrawing(Gui::Document* doc, QWidget* parent, GpuBackend backend)
    : Gui::MDIView(doc, parent)
    , m_backend(backend)
{
    // Set opaque background to prevent any transparency/garbage issues
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);
    
    // Remove any existing central widget from base class
    QWidget* oldCentral = centralWidget();
    if (oldCentral) {
        oldCentral->hide();
        oldCentral->setParent(nullptr);
        oldCentral->deleteLater();
    }
    
    QWidget* canvas = nullptr;
    
#ifdef __APPLE__
    if (backend == GpuBackend::Metal) {
        m_metalCanvas = new SkiaMetalCanvas(this);
        if (m_metalCanvas->isGpuAvailable()) {
            canvas = m_metalCanvas;
            connect(m_metalCanvas, &SkiaMetalCanvas::lineCreated, 
                    this, &MDIViewDrawing::onLineCreated);
            connect(m_metalCanvas, &SkiaMetalCanvas::circleCreated,
                    this, &MDIViewDrawing::onCircleCreated);
            connect(m_metalCanvas, &SkiaMetalCanvas::cursorPositionChanged,
                    this, &MDIViewDrawing::onCursorPositionChanged);
        } else {
            delete m_metalCanvas;
            m_metalCanvas = nullptr;
            backend = GpuBackend::OpenGL;
            m_backend = backend;
        }
    }
#else
    if (backend == GpuBackend::Metal) {
        backend = GpuBackend::OpenGL;
        m_backend = backend;
    }
#endif

    if (!canvas && backend == GpuBackend::OpenGL) {
        m_glCanvas = new SkiaOpenGLCanvas(this);
        canvas = m_glCanvas;
        connect(m_glCanvas, &SkiaOpenGLCanvas::lineCreated, 
                this, &MDIViewDrawing::onLineCreated);
        connect(m_glCanvas, &SkiaOpenGLCanvas::circleCreated,
                this, &MDIViewDrawing::onCircleCreated);
        connect(m_glCanvas, &SkiaOpenGLCanvas::cursorPositionChanged,
                this, &MDIViewDrawing::onCursorPositionChanged);
    }

    if (!canvas || backend == GpuBackend::CPU) {
        if (m_glCanvas) {
            delete m_glCanvas;
            m_glCanvas = nullptr;
        }
        m_cpuCanvas = new SkiaCanvas(this);
        canvas = m_cpuCanvas;
        m_backend = GpuBackend::CPU;
        connect(m_cpuCanvas, &SkiaCanvas::lineCreated, 
                this, &MDIViewDrawing::onLineCreated);
        connect(m_cpuCanvas, &SkiaCanvas::circleCreated,
                this, &MDIViewDrawing::onCircleCreated);
        connect(m_cpuCanvas, &SkiaCanvas::cursorPositionChanged,
                this, &MDIViewDrawing::onCursorPositionChanged);
    }
    
    createActions();
    setupToolbar();
    
    QString title = tr("Drawing Canvas [%1]").arg(backendName());
    setWindowTitle(title);
    resize(800, 600);
}

MDIViewDrawing::~MDIViewDrawing() = default;

QString MDIViewDrawing::backendName() const
{
    switch (m_backend) {
        case GpuBackend::CPU:    return QStringLiteral("CPU: Raster");
        case GpuBackend::OpenGL: return QStringLiteral("GPU: OpenGL");
        case GpuBackend::Metal:  return QStringLiteral("GPU: Metal");
        default:                 return QStringLiteral("Unknown");
    }
}

void MDIViewDrawing::createActions()
{
    m_actLine = new QAction(tr("Line"), this);
    m_actLine->setShortcut(QKeySequence(tr("L")));
    connect(m_actLine, &QAction::triggered, this, &MDIViewDrawing::cmdDrawLine);
    
    m_actCircle = new QAction(tr("Circle"), this);
    m_actCircle->setShortcut(QKeySequence(tr("C")));
    connect(m_actCircle, &QAction::triggered, this, &MDIViewDrawing::cmdDrawCircle);
    
    m_actRectangle = new QAction(tr("Rectangle"), this);
    m_actRectangle->setShortcut(QKeySequence(tr("R")));
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
    
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(toolbar);
    
    QWidget* canvas = nullptr;
#ifdef __APPLE__
    if (m_metalCanvas) canvas = m_metalCanvas;
    else
#endif
    if (m_glCanvas) canvas = m_glCanvas;
    else canvas = m_cpuCanvas;
    
    layout->addWidget(canvas, 1);  // Give canvas stretch factor
    
    QWidget* container = new QWidget(this);
    container->setLayout(layout);
    
    // Set opaque background on container
    container->setAutoFillBackground(true);
    QPalette pal = container->palette();
    pal.setColor(QPalette::Window, Qt::white);
    container->setPalette(pal);
    
    setCentralWidget(container);
}

bool MDIViewDrawing::onMsg(const char* pMsg, const char** ppReturn)
{
    Q_UNUSED(ppReturn);
    if (strcmp(pMsg, "ViewFit") == 0) { cmdZoomFit(); return true; }
    if (strcmp(pMsg, "ZoomIn") == 0) { cmdZoomIn(); return true; }
    if (strcmp(pMsg, "ZoomOut") == 0) { cmdZoomOut(); return true; }
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
#ifdef __APPLE__
    if (m_metalCanvas) { m_metalCanvas->update(); return; }
#endif
    if (m_glCanvas) { m_glCanvas->update(); return; }
    if (m_cpuCanvas) { m_cpuCanvas->update(); }
}

void MDIViewDrawing::closeEvent(QCloseEvent* event)
{
    Gui::MDIView::closeEvent(event);
}

void MDIViewDrawing::cmdDrawLine()
{
#ifdef __APPLE__
    if (m_metalCanvas) { m_metalCanvas->setDrawMode(DrawMode::Line); }
    else
#endif
    if (m_glCanvas) { m_glCanvas->setDrawMode(DrawMode::Line); }
    else if (m_cpuCanvas) { m_cpuCanvas->setDrawMode(DrawMode::Line); }
    Gui::getMainWindow()->showMessage(tr("Click first point for line..."));
}

void MDIViewDrawing::cmdDrawCircle()
{
#ifdef __APPLE__
    if (m_metalCanvas) { m_metalCanvas->setDrawMode(DrawMode::Circle); }
    else
#endif
    if (m_glCanvas) { m_glCanvas->setDrawMode(DrawMode::Circle); }
    else if (m_cpuCanvas) { m_cpuCanvas->setDrawMode(DrawMode::Circle); }
    Gui::getMainWindow()->showMessage(tr("Click center point for circle..."));
}

void MDIViewDrawing::cmdDrawRectangle()
{
#ifdef __APPLE__
    if (m_metalCanvas) { m_metalCanvas->setDrawMode(DrawMode::Rectangle); }
    else
#endif
    if (m_glCanvas) { m_glCanvas->setDrawMode(DrawMode::Rectangle); }
    else if (m_cpuCanvas) { m_cpuCanvas->setDrawMode(DrawMode::Rectangle); }
    Gui::getMainWindow()->showMessage(tr("Click first corner for rectangle..."));
}

void MDIViewDrawing::cmdZoomFit()
{
#ifdef __APPLE__
    if (m_metalCanvas) { m_metalCanvas->zoomToFit(); return; }
#endif
    if (m_glCanvas) { m_glCanvas->zoomToFit(); return; }
    if (m_cpuCanvas) { m_cpuCanvas->zoomToFit(); }
}

void MDIViewDrawing::cmdZoomIn()
{
#ifdef __APPLE__
    if (m_metalCanvas) { m_metalCanvas->setZoom(m_metalCanvas->zoom() * 1.2f); return; }
#endif
    if (m_glCanvas) { m_glCanvas->setZoom(m_glCanvas->zoom() * 1.2f); return; }
    if (m_cpuCanvas) { m_cpuCanvas->setZoom(m_cpuCanvas->zoom() * 1.2f); }
}

void MDIViewDrawing::cmdZoomOut()
{
#ifdef __APPLE__
    if (m_metalCanvas) { m_metalCanvas->setZoom(m_metalCanvas->zoom() / 1.2f); return; }
#endif
    if (m_glCanvas) { m_glCanvas->setZoom(m_glCanvas->zoom() / 1.2f); return; }
    if (m_cpuCanvas) { m_cpuCanvas->setZoom(m_cpuCanvas->zoom() / 1.2f); }
}

void MDIViewDrawing::cmdExportSVG()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Export SVG"), QString(), tr("SVG Files (*.svg)"));
    if (!filename.isEmpty()) {
        if (!filename.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive))
            filename += QStringLiteral(".svg");
#ifdef __APPLE__
        if (m_metalCanvas) { m_metalCanvas->exportToSVG(filename); return; }
#endif
        if (m_glCanvas) { m_glCanvas->exportToSVG(filename); return; }
        if (m_cpuCanvas) { m_cpuCanvas->exportToSVG(filename); }
    }
}

void MDIViewDrawing::cmdExportPNG()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Export PNG"), QString(), tr("PNG Files (*.png)"));
    if (!filename.isEmpty()) {
        if (!filename.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive))
            filename += QStringLiteral(".png");
#ifdef __APPLE__
        if (m_metalCanvas) { m_metalCanvas->exportToPNG(filename); return; }
#endif
        if (m_glCanvas) { m_glCanvas->exportToPNG(filename); return; }
        if (m_cpuCanvas) { m_cpuCanvas->exportToPNG(filename); }
    }
}

void MDIViewDrawing::cmdClear()
{
#ifdef __APPLE__
    if (m_metalCanvas) { m_metalCanvas->clearGeometry(); return; }
#endif
    if (m_glCanvas) { m_glCanvas->clearGeometry(); return; }
    if (m_cpuCanvas) { m_cpuCanvas->clearGeometry(); }
}

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
