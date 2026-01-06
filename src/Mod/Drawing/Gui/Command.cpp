/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                   *
 *                                                                         *
 *   Drawing workbench commands                                           *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <QMessageBox>
#endif

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>

#include "MDIViewDrawing.h"
#include "SkiaCanvas.h"

using namespace Gui;

namespace DrawingGui {

// =============================================================================
// CmdDrawingNewCanvas - Open new canvas with platform-optimal GPU backend
// =============================================================================

DEF_STD_CMD_A(CmdDrawingNewCanvas)

CmdDrawingNewCanvas::CmdDrawingNewCanvas()
    : Command("Drawing_NewCanvas")
{
    sAppModule    = "Drawing";
    sGroup        = "Drawing";
#ifdef __APPLE__
    sMenuText     = QT_TR_NOOP("New Canvas (Metal)");
    sToolTipText  = QT_TR_NOOP("Open a new 2D drawing canvas with Metal GPU rendering");
#else
    sMenuText     = QT_TR_NOOP("New Canvas (OpenGL)");
    sToolTipText  = QT_TR_NOOP("Open a new 2D drawing canvas with OpenGL GPU rendering");
#endif
    sWhatsThis    = "Drawing_NewCanvas";
    sStatusTip    = sToolTipText;
    sAccel        = "N";
}

void CmdDrawingNewCanvas::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (!doc) {
        App::GetApplication().newDocument("Drawing");
        doc = Gui::Application::Instance->activeDocument();
    }
    
    // Use platform-optimal backend
#ifdef __APPLE__
    MDIViewDrawing* view = new MDIViewDrawing(doc, Gui::getMainWindow(), GpuBackend::Metal);
#else
    MDIViewDrawing* view = new MDIViewDrawing(doc, Gui::getMainWindow(), GpuBackend::OpenGL);
#endif
    Gui::getMainWindow()->addWindow(view);
}

bool CmdDrawingNewCanvas::isActive()
{
    return true;
}

// =============================================================================
// CmdDrawingNewCanvasCPU - Open new canvas with CPU rendering
// =============================================================================

DEF_STD_CMD_A(CmdDrawingNewCanvasCPU)

CmdDrawingNewCanvasCPU::CmdDrawingNewCanvasCPU()
    : Command("Drawing_NewCanvasCPU")
{
    sAppModule    = "Drawing";
    sGroup        = "Drawing";
    sMenuText     = QT_TR_NOOP("New Canvas (CPU)");
    sToolTipText  = QT_TR_NOOP("Open a new 2D drawing canvas with CPU software rendering");
    sWhatsThis    = "Drawing_NewCanvasCPU";
    sStatusTip    = sToolTipText;
}

void CmdDrawingNewCanvasCPU::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (!doc) {
        App::GetApplication().newDocument("Drawing");
        doc = Gui::Application::Instance->activeDocument();
    }
    
    MDIViewDrawing* view = new MDIViewDrawing(doc, Gui::getMainWindow(), GpuBackend::CPU);
    Gui::getMainWindow()->addWindow(view);
}

bool CmdDrawingNewCanvasCPU::isActive()
{
    return true;
}

// =============================================================================
// CmdDrawingLine - Draw line in active canvas
// =============================================================================

DEF_STD_CMD_A(CmdDrawingLine)

CmdDrawingLine::CmdDrawingLine()
    : Command("Drawing_Line")
{
    sAppModule    = "Drawing";
    sGroup        = "Drawing";
    sMenuText     = QT_TR_NOOP("Line");
    sToolTipText  = QT_TR_NOOP("Draw a line by clicking two points");
    sWhatsThis    = "Drawing_Line";
    sStatusTip    = sToolTipText;
    sAccel        = "L";
}

void CmdDrawingLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    
    Gui::MDIView* mdi = Gui::getMainWindow()->activeWindow();
    MDIViewDrawing* drawView = qobject_cast<MDIViewDrawing*>(mdi);
    
    if (!drawView) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("No Drawing Canvas"),
            QObject::tr("Please open a Drawing Canvas first (Drawing > New Canvas)."));
        return;
    }
    
    drawView->cmdDrawLine();
}

bool CmdDrawingLine::isActive()
{
    Gui::MDIView* mdi = Gui::getMainWindow()->activeWindow();
    return qobject_cast<MDIViewDrawing*>(mdi) != nullptr;
}

// =============================================================================
// CmdDrawingCircle - Draw circle in active canvas
// =============================================================================

DEF_STD_CMD_A(CmdDrawingCircle)

CmdDrawingCircle::CmdDrawingCircle()
    : Command("Drawing_Circle")
{
    sAppModule    = "Drawing";
    sGroup        = "Drawing";
    sMenuText     = QT_TR_NOOP("Circle");
    sToolTipText  = QT_TR_NOOP("Draw a circle by clicking center and radius point");
    sWhatsThis    = "Drawing_Circle";
    sStatusTip    = sToolTipText;
    sAccel        = "C";
}

void CmdDrawingCircle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    
    Gui::MDIView* mdi = Gui::getMainWindow()->activeWindow();
    MDIViewDrawing* drawView = qobject_cast<MDIViewDrawing*>(mdi);
    
    if (!drawView) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("No Drawing Canvas"),
            QObject::tr("Please open a Drawing Canvas first."));
        return;
    }
    
    drawView->cmdDrawCircle();
}

bool CmdDrawingCircle::isActive()
{
    Gui::MDIView* mdi = Gui::getMainWindow()->activeWindow();
    return qobject_cast<MDIViewDrawing*>(mdi) != nullptr;
}

// =============================================================================
// CmdDrawingRectangle - Draw rectangle in active canvas
// =============================================================================

DEF_STD_CMD_A(CmdDrawingRectangle)

CmdDrawingRectangle::CmdDrawingRectangle()
    : Command("Drawing_Rectangle")
{
    sAppModule    = "Drawing";
    sGroup        = "Drawing";
    sMenuText     = QT_TR_NOOP("Rectangle");
    sToolTipText  = QT_TR_NOOP("Draw a rectangle by clicking two corners");
    sWhatsThis    = "Drawing_Rectangle";
    sStatusTip    = sToolTipText;
    sAccel        = "R";
}

void CmdDrawingRectangle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    
    Gui::MDIView* mdi = Gui::getMainWindow()->activeWindow();
    MDIViewDrawing* drawView = qobject_cast<MDIViewDrawing*>(mdi);
    
    if (!drawView) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("No Drawing Canvas"),
            QObject::tr("Please open a Drawing Canvas first."));
        return;
    }
    
    drawView->cmdDrawRectangle();
}

bool CmdDrawingRectangle::isActive()
{
    Gui::MDIView* mdi = Gui::getMainWindow()->activeWindow();
    return qobject_cast<MDIViewDrawing*>(mdi) != nullptr;
}

// =============================================================================
// CmdDrawingExportSVG - Export canvas to SVG
// =============================================================================

DEF_STD_CMD_A(CmdDrawingExportSVG)

CmdDrawingExportSVG::CmdDrawingExportSVG()
    : Command("Drawing_ExportSVG")
{
    sAppModule    = "Drawing";
    sGroup        = "Drawing";
    sMenuText     = QT_TR_NOOP("Export SVG");
    sToolTipText  = QT_TR_NOOP("Export the drawing to SVG format");
    sWhatsThis    = "Drawing_ExportSVG";
    sStatusTip    = sToolTipText;
}

void CmdDrawingExportSVG::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    
    Gui::MDIView* mdi = Gui::getMainWindow()->activeWindow();
    MDIViewDrawing* drawView = qobject_cast<MDIViewDrawing*>(mdi);
    
    if (drawView) {
        drawView->cmdExportSVG();
    }
}

bool CmdDrawingExportSVG::isActive()
{
    Gui::MDIView* mdi = Gui::getMainWindow()->activeWindow();
    return qobject_cast<MDIViewDrawing*>(mdi) != nullptr;
}

// =============================================================================
// Command registration
// =============================================================================

void CreateDrawingCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    
    rcCmdMgr.addCommand(new CmdDrawingNewCanvas());
    rcCmdMgr.addCommand(new CmdDrawingNewCanvasCPU());
    rcCmdMgr.addCommand(new CmdDrawingLine());
    rcCmdMgr.addCommand(new CmdDrawingCircle());
    rcCmdMgr.addCommand(new CmdDrawingRectangle());
    rcCmdMgr.addCommand(new CmdDrawingExportSVG());
}

} // namespace DrawingGui
