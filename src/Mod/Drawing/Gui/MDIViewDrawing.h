/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                   *
 *                                                                         *
 *   MDI View for Skia-based Drawing canvas                               *
 *                                                                         *
 ***************************************************************************/

#ifndef DRAWING_MDIVIEW_DRAWING_H
#define DRAWING_MDIVIEW_DRAWING_H

#include <Gui/MDIView.h>
#include <QPointer>

namespace DrawingGui {

class SkiaCanvas;
class SkiaOpenGLCanvas;
#ifdef __APPLE__
class SkiaMetalCanvas;
#endif

// Forward declare geometry types
struct SkiaLine;
struct SkiaCircle;

/**
 * @brief GPU backend type for rendering
 */
enum class GpuBackend {
    CPU,        // Software raster (cross-platform)
    OpenGL,     // OpenGL GPU (cross-platform) - requires Skia with skia_use_gl=true
    Metal       // Metal GPU (macOS only, default on macOS)
};

/**
 * @brief MDI View window for 2D Drawing using Skia
 * 
 * Supports multiple rendering backends:
 * - CPU: Software raster (all platforms)
 * - OpenGL: GPU accelerated (Windows, Linux, macOS)
 * - Metal: GPU accelerated (macOS only, best performance)
 */
class MDIViewDrawing : public Gui::MDIView
{
    Q_OBJECT
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MDIViewDrawing(Gui::Document* doc, QWidget* parent = nullptr, 
#ifdef __APPLE__
                   GpuBackend backend = GpuBackend::Metal);  // Default to Metal on macOS
#else
                   GpuBackend backend = GpuBackend::OpenGL); // Default to OpenGL on other platforms
#endif
    ~MDIViewDrawing() override;

    // Check rendering backend
    GpuBackend backend() const { return m_backend; }
    bool isGpuRendering() const { return m_backend != GpuBackend::CPU; }
    QString backendName() const;

    // MDIView interface
    const char* getName() const override { return "MDIViewDrawing"; }
    bool onMsg(const char* pMsg, const char** ppReturn) override;
    bool onHasMsg(const char* pMsg) const override;
    void onUpdate() override;

    // Drawing commands
    void cmdDrawLine();
    void cmdDrawCircle();
    void cmdDrawRectangle();
    void cmdZoomFit();
    void cmdZoomIn();
    void cmdZoomOut();
    void cmdExportSVG();
    void cmdExportPNG();
    void cmdClear();

protected:
    void closeEvent(QCloseEvent* event) override;

private Q_SLOTS:
    void onLineCreated(const SkiaLine& line);
    void onCircleCreated(const SkiaCircle& circle);
    void onCursorPositionChanged(float x, float y);

private:
    void setupToolbar();
    void createActions();

    GpuBackend m_backend;
    
    // Canvas widgets (only one is used at a time)
    SkiaCanvas* m_cpuCanvas = nullptr;
    SkiaOpenGLCanvas* m_glCanvas = nullptr;
#ifdef __APPLE__
    SkiaMetalCanvas* m_metalCanvas = nullptr;
#endif
    
    // Actions
    QAction* m_actLine = nullptr;
    QAction* m_actCircle = nullptr;
    QAction* m_actRectangle = nullptr;
    QAction* m_actZoomFit = nullptr;
    QAction* m_actZoomIn = nullptr;
    QAction* m_actZoomOut = nullptr;
    QAction* m_actExportSVG = nullptr;
    QAction* m_actExportPNG = nullptr;
    QAction* m_actClear = nullptr;
};

} // namespace DrawingGui

#endif // DRAWING_MDIVIEW_DRAWING_H
