/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                   *
 *                                                                         *
 *   MDI View for Skia-based Drawing canvas                               *
 *                                                                         *
 *   This implementation uses C++20 template features to provide a        *
 *   unified interface for multiple canvas backends, eliminating          *
 *   repetitive conditional code while maintaining type safety.           *
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

// ============================================================================
// Compile-time verification that canvas classes satisfy SkiaCanvasConcept
// 编译期验证画布类满足 SkiaCanvasConcept
// ============================================================================
static_assert(SkiaCanvasConcept<SkiaCanvas>, 
    "SkiaCanvas must satisfy SkiaCanvasConcept");
static_assert(SkiaCanvasConcept<SkiaOpenGLCanvas>, 
    "SkiaOpenGLCanvas must satisfy SkiaCanvasConcept");

TYPESYSTEM_SOURCE_ABSTRACT(DrawingGui::MDIViewDrawing, Gui::MDIView)

// ============================================================================
// Constructor / 构造函数
// ============================================================================
MDIViewDrawing::MDIViewDrawing(Gui::Document* doc, QWidget* parent, GpuBackend backend)
    : Gui::MDIView(doc, parent)
    , m_backend(backend)
{
    // Set opaque background to prevent any transparency/garbage issues
    // 设置不透明背景以防止透明/花屏问题
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);
    
    // Remove any existing central widget from base class
    // 移除基类中任何现有的中央控件
    QWidget* oldCentral = centralWidget();
    if (oldCentral) {
        oldCentral->hide();
        oldCentral->setParent(nullptr);
        oldCentral->deleteLater();
    }
    
    QWidget* canvas = nullptr;
    
    // Canvas creation with fallback mechanism
    // 画布创建与回退机制
#ifdef __APPLE__
    if (backend == GpuBackend::Metal) {
        m_metalCanvas = new SkiaMetalCanvas(this);
        if (m_metalCanvas->isGpuAvailable()) {
            canvas = m_metalCanvas;
            connectCanvasSignals(m_metalCanvas);
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
        connectCanvasSignals(m_glCanvas);
    }

    if (!canvas || backend == GpuBackend::CPU) {
        if (m_glCanvas) {
            delete m_glCanvas;
            m_glCanvas = nullptr;
        }
        m_cpuCanvas = new SkiaCanvas(this);
        canvas = m_cpuCanvas;
        m_backend = GpuBackend::CPU;
        connectCanvasSignals(m_cpuCanvas);
    }
    
    createActions();
    setupToolbar();
    
    QString title = tr("Drawing Canvas [%1]").arg(backendName());
    setWindowTitle(title);
    resize(800, 600);
}

MDIViewDrawing::~MDIViewDrawing() = default;

// ============================================================================
// Backend information / 后端信息
// ============================================================================
QString MDIViewDrawing::backendName() const
{
    switch (m_backend) {
        case GpuBackend::CPU:    return QStringLiteral("CPU: Raster");
        case GpuBackend::OpenGL: return QStringLiteral("GPU: OpenGL");
        case GpuBackend::Metal:  return QStringLiteral("GPU: Metal");
        default:                 return QStringLiteral("Unknown");
    }
}

// ============================================================================
// Action creation / 创建动作
// ============================================================================
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

// ============================================================================
// Toolbar setup / 工具栏设置
// ============================================================================
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
    
    // Get active canvas widget using template helper
    // 使用模板辅助函数获取活跃画布控件
    QWidget* canvas = getActiveCanvasWidget();
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

// ============================================================================
// MDIView interface implementation / MDIView 接口实现
// ============================================================================
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
    // Using C++20 template to call update() on active canvas
    // 使用 C++20 模板在活跃画布上调用 update()
    withActiveCanvas([](auto* canvas) { 
        canvas->update(); 
    });
}

void MDIViewDrawing::closeEvent(QCloseEvent* event)
{
    Gui::MDIView::closeEvent(event);
}

// ============================================================================
// Drawing commands - Using C++20 template for unified interface
// 绘图命令 - 使用 C++20 模板实现统一接口
// ============================================================================

void MDIViewDrawing::cmdDrawLine()
{
    withActiveCanvas([](auto* canvas) { 
        canvas->setDrawMode(DrawMode::Line); 
    });
    Gui::getMainWindow()->showMessage(tr("Click first point for line..."));
}

void MDIViewDrawing::cmdDrawCircle()
{
    withActiveCanvas([](auto* canvas) { 
        canvas->setDrawMode(DrawMode::Circle); 
    });
    Gui::getMainWindow()->showMessage(tr("Click center point for circle..."));
}

void MDIViewDrawing::cmdDrawRectangle()
{
    withActiveCanvas([](auto* canvas) { 
        canvas->setDrawMode(DrawMode::Rectangle); 
    });
    Gui::getMainWindow()->showMessage(tr("Click first corner for rectangle..."));
}

void MDIViewDrawing::cmdZoomFit()
{
    withActiveCanvas([](auto* canvas) { 
        canvas->zoomToFit(); 
    });
}

void MDIViewDrawing::cmdZoomIn()
{
    withActiveCanvas([](auto* canvas) { 
        canvas->setZoom(canvas->zoom() * 1.2f); 
    });
}

void MDIViewDrawing::cmdZoomOut()
{
    withActiveCanvas([](auto* canvas) { 
        canvas->setZoom(canvas->zoom() / 1.2f); 
    });
}

void MDIViewDrawing::cmdExportSVG()
{
    QString filename = QFileDialog::getSaveFileName(
        this, tr("Export SVG"), QString(), tr("SVG Files (*.svg)"));
    
    if (!filename.isEmpty()) {
        if (!filename.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive)) {
            filename += QStringLiteral(".svg");
        }
        withActiveCanvas([&filename](auto* canvas) { 
            canvas->exportToSVG(filename); 
        });
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
        withActiveCanvas([&filename](auto* canvas) { 
            canvas->exportToPNG(filename); 
        });
    }
}

void MDIViewDrawing::cmdClear()
{
    withActiveCanvas([](auto* canvas) { 
        canvas->clearGeometry(); 
    });
}

// ============================================================================
// Signal handlers / 信号处理
// ============================================================================
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

// ============================================================================
// Helper: Get active canvas as QWidget*
// 辅助函数：获取活跃画布的 QWidget 指针
// ============================================================================
QWidget* MDIViewDrawing::getActiveCanvasWidget()
{
#ifdef __APPLE__
    if (m_metalCanvas) return m_metalCanvas;
#endif
    if (m_glCanvas) return m_glCanvas;
    return m_cpuCanvas;
}

} // namespace DrawingGui
