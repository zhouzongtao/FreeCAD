/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                   *
 *                                                                         *
 *   MDI View for Skia-based Drawing canvas                               *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#include "MDIViewDrawing.h"
#include "SkiaCanvas.h"

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

MDIViewDrawing::MDIViewDrawing(Gui::Document* doc, QWidget* parent)
    : Gui::MDIView(doc, parent)
    , m_canvas(nullptr)
{
    // Create canvas
    m_canvas = new SkiaCanvas(this);
    setCentralWidget(m_canvas);
    
    // Setup UI
    createActions();
    setupToolbar();
    
    // Connect signals
    connect(m_canvas, &SkiaCanvas::lineCreated, 
            this, &MDIViewDrawing::onLineCreated);
    connect(m_canvas, &SkiaCanvas::circleCreated,
            this, &MDIViewDrawing::onCircleCreated);
    connect(m_canvas, &SkiaCanvas::cursorPositionChanged,
            this, &MDIViewDrawing::onCursorPositionChanged);
    
    setWindowTitle(tr("Drawing Canvas [Skia]"));
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
    layout->addWidget(m_canvas);
    
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
    if (m_canvas) {
        m_canvas->update();
    }
}

void MDIViewDrawing::closeEvent(QCloseEvent* event)
{
    Gui::MDIView::closeEvent(event);
}

// Command implementations
void MDIViewDrawing::cmdDrawLine()
{
    m_canvas->setDrawMode(DrawMode::Line);
    Gui::getMainWindow()->showMessage(tr("Click first point for line..."));
}

void MDIViewDrawing::cmdDrawCircle()
{
    m_canvas->setDrawMode(DrawMode::Circle);
    Gui::getMainWindow()->showMessage(tr("Click center point for circle..."));
}

void MDIViewDrawing::cmdDrawRectangle()
{
    m_canvas->setDrawMode(DrawMode::Rectangle);
    Gui::getMainWindow()->showMessage(tr("Click first corner for rectangle..."));
}

void MDIViewDrawing::cmdZoomFit()
{
    m_canvas->zoomToFit();
}

void MDIViewDrawing::cmdZoomIn()
{
    m_canvas->setZoom(m_canvas->zoom() * 1.2f);
}

void MDIViewDrawing::cmdZoomOut()
{
    m_canvas->setZoom(m_canvas->zoom() / 1.2f);
}

void MDIViewDrawing::cmdExportSVG()
{
    QString filename = QFileDialog::getSaveFileName(
        this, tr("Export SVG"), QString(), tr("SVG Files (*.svg)"));
    
    if (!filename.isEmpty()) {
        if (!filename.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive)) {
            filename += QStringLiteral(".svg");
        }
        m_canvas->exportToSVG(filename);
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
        m_canvas->exportToPNG(filename);
    }
}

void MDIViewDrawing::cmdClear()
{
    m_canvas->clearGeometry();
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
    QString msg = QStringLiteral("X: %1  Y: %2").arg(x, 0, 'f', 2).arg(y, 0, 'f', 2);
    Q_UNUSED(msg);
    // Could update status bar here
}

} // namespace DrawingGui
