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

/**
 * @brief MDI View window for 2D Drawing using Skia
 */
class MDIViewDrawing : public Gui::MDIView
{
    Q_OBJECT
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MDIViewDrawing(Gui::Document* doc, QWidget* parent = nullptr);
    ~MDIViewDrawing() override;

    // Access to canvas
    SkiaCanvas* getCanvas() { return m_canvas; }

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
    void onLineCreated(const struct SkiaLine& line);
    void onCircleCreated(const struct SkiaCircle& circle);
    void onCursorPositionChanged(float x, float y);

private:
    void setupToolbar();
    void createActions();

    SkiaCanvas* m_canvas;
    
    // Actions
    QAction* m_actLine;
    QAction* m_actCircle;
    QAction* m_actRectangle;
    QAction* m_actZoomFit;
    QAction* m_actZoomIn;
    QAction* m_actZoomOut;
    QAction* m_actExportSVG;
    QAction* m_actExportPNG;
    QAction* m_actClear;
};

} // namespace DrawingGui

#endif // DRAWING_MDIVIEW_DRAWING_H
