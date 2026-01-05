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
class SkiaGLCanvas;
#ifdef __APPLE__
class SkiaMetalCanvas;
#endif

// Forward declare geometry types
struct SkiaLine;
struct SkiaCircle;

/**
 * @brief MDI View window for 2D Drawing using Skia
 * 
 * Supports multiple rendering backends:
 * - OpenGL (cross-platform GPU acceleration)
 * - Metal (macOS-specific GPU acceleration)
 * - CPU Raster (fallback software rendering)
 */
class MDIViewDrawing : public Gui::MDIView
{
    Q_OBJECT
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    enum class RenderBackend {
        Auto,       // Automatically select best backend
        OpenGL,     // Cross-platform GPU (Windows, Linux, macOS)
        Metal,      // macOS-only GPU
        CPU         // Software raster (fallback)
    };

    MDIViewDrawing(Gui::Document* doc, QWidget* parent = nullptr, 
                   RenderBackend backend = RenderBackend::Auto);
    ~MDIViewDrawing() override;

    // Check rendering backend
    RenderBackend renderBackend() const { return m_backend; }
    bool isGpuRendering() const { return m_backend != RenderBackend::CPU; }

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
    void initCanvas(RenderBackend backend);

    RenderBackend m_backend = RenderBackend::CPU;
    
    // Canvas widgets (only one is used at a time)
    SkiaCanvas* m_cpuCanvas = nullptr;
    SkiaGLCanvas* m_glCanvas = nullptr;
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
