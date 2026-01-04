/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                   *
 *                                                                         *
 *   MDI View for Skia-based Drawing canvas                               *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#include "MDIViewDrawing.h"
#include "SkiaCanvas.h"

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

MDIViewDrawing::MDIViewDrawing(Gui::Document* doc, QWidget* parent, bool useGpu)
    : Gui::MDIView(doc, parent)
    , m_useGpu(useGpu)
{
#ifdef __APPLE__
    if (m_useGpu) {
        // Try GPU rendering first
        m_gpuCanvas = new SkiaMetalCanvas(this);
        if (m_gpuCanvas->isGpuAvailable()) {
            setCentralWidget(m_gpuCanvas);
            
            connect(m_gpuCanvas, &SkiaMetalCanvas::lineCreated, 
                    this, &MDIViewDrawing::onLineCreated);
            connect(m_gpuCanvas, &SkiaMetalCanvas::circleCreated,
                    this, &MDIViewDrawing::onCircleCreated);
            connect(m_gpuCanvas, &SkiaMetalCanvas::cursorPositionChanged,
                    this, &MDIViewDrawing::onCursorPositionChanged);
            
            Base::Console().message("MDIViewDrawing: Using GPU (Metal) rendering\n");
        } else {
            // Fall back to CPU
            delete m_gpuCanvas;
            m_gpuCanvas = nullptr;
            m_useGpu = false;
        }
    }
#else
    m_useGpu = false;
#endif

    if (!m_useGpu) {
        // Use CPU rendering
        m_cpuCanvas = new SkiaCanvas(this);
        setCentralWidget(m_cpuCanvas);
        
        connect(m_cpuCanvas, &SkiaCanvas::lineCreated, 
                this, &MDIViewDrawing::onLineCreated);
        connect(m_cpuCanvas, &SkiaCanvas::circleCreated,
                this, &MDIViewDrawing::onCircleCreated);
        connect(m_cpuCanvas, &SkiaCanvas::cursorPositionChanged,
                this, &MDIViewDrawing::onCursorPositionChanged);
        
        Base::Console().message("MDIViewDrawing: Using CPU (Raster) rendering\n");
    }
    
    // Setup UI
    createActions();
    setupToolbar();
    
    QString title = m_useGpu ? tr("Drawing Canvas [GPU: Metal]") 
                             : tr("Drawing Canvas [CPU: Raster]");
    setWindowTitle(title);
    resize(800, 600);
    
    Base::Console().message("MDIViewDrawing: Created\n");
}

MDIViewDrawing::~MDIViewDrawing()
{
    Base::Console().message("MDIViewDrawing: Destroyed\n");
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
    
#ifdef __APPLE__
    if (m_useGpu && m_gpuCanvas) {
        layout->addWidget(m_gpuCanvas);
    } else
#endif
    {
        layout->addWidget(m_cpuCanvas);
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
#ifdef __APPLE__
    if (m_useGpu && m_gpuCanvas) {
        m_gpuCanvas->update();
    } else
#endif
    if (m_cpuCanvas) {
        m_cpuCanvas->update();
    }
}

void MDIViewDrawing::closeEvent(QCloseEvent* event)
{
    Gui::MDIView::closeEvent(event);
}

// Command implementations
void MDIViewDrawing::cmdDrawLine()
{
#ifdef __APPLE__
    if (m_useGpu && m_gpuCanvas) {
        m_gpuCanvas->setDrawMode(DrawMode::Line);
    } else
#endif
    if (m_cpuCanvas) {
        m_cpuCanvas->setDrawMode(DrawMode::Line);
    }
    Gui::getMainWindow()->showMessage(tr("Click first point for line..."));
}

void MDIViewDrawing::cmdDrawCircle()
{
#ifdef __APPLE__
    if (m_useGpu && m_gpuCanvas) {
        m_gpuCanvas->setDrawMode(DrawMode::Circle);
    } else
#endif
    if (m_cpuCanvas) {
        m_cpuCanvas->setDrawMode(DrawMode::Circle);
    }
    Gui::getMainWindow()->showMessage(tr("Click center point for circle..."));
}

void MDIViewDrawing::cmdDrawRectangle()
{
#ifdef __APPLE__
    if (m_useGpu && m_gpuCanvas) {
        m_gpuCanvas->setDrawMode(DrawMode::Rectangle);
    } else
#endif
    if (m_cpuCanvas) {
        m_cpuCanvas->setDrawMode(DrawMode::Rectangle);
    }
    Gui::getMainWindow()->showMessage(tr("Click first corner for rectangle..."));
}

void MDIViewDrawing::cmdZoomFit()
{
#ifdef __APPLE__
    if (m_useGpu && m_gpuCanvas) {
        m_gpuCanvas->zoomToFit();
    } else
#endif
    if (m_cpuCanvas) {
        m_cpuCanvas->zoomToFit();
    }
}

void MDIViewDrawing::cmdZoomIn()
{
#ifdef __APPLE__
    if (m_useGpu && m_gpuCanvas) {
        m_gpuCanvas->setZoom(m_gpuCanvas->zoom() * 1.2f);
    } else
#endif
    if (m_cpuCanvas) {
        m_cpuCanvas->setZoom(m_cpuCanvas->zoom() * 1.2f);
    }
}

void MDIViewDrawing::cmdZoomOut()
{
#ifdef __APPLE__
    if (m_useGpu && m_gpuCanvas) {
        m_gpuCanvas->setZoom(m_gpuCanvas->zoom() / 1.2f);
    } else
#endif
    if (m_cpuCanvas) {
        m_cpuCanvas->setZoom(m_cpuCanvas->zoom() / 1.2f);
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
#ifdef __APPLE__
        if (m_useGpu && m_gpuCanvas) {
            m_gpuCanvas->exportToSVG(filename);
        } else
#endif
        if (m_cpuCanvas) {
            m_cpuCanvas->exportToSVG(filename);
        }
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
#ifdef __APPLE__
        if (m_useGpu && m_gpuCanvas) {
            m_gpuCanvas->exportToPNG(filename);
        } else
#endif
        if (m_cpuCanvas) {
            m_cpuCanvas->exportToPNG(filename);
        }
    }
}

void MDIViewDrawing::cmdClear()
{
#ifdef __APPLE__
    if (m_useGpu && m_gpuCanvas) {
        m_gpuCanvas->clearGeometry();
    } else
#endif
    if (m_cpuCanvas) {
        m_cpuCanvas->clearGeometry();
    }
}

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
