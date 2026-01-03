/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                   *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <QMessageBox>
#endif

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>

#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoEvent.h>
#include <Inventor/nodes/SoEventCallback.h>

using namespace Gui;

namespace DrawingGui {

// =============================================================================
// Helper class for interactive point picking
// =============================================================================

class DrawLineHandler {
public:
    DrawLineHandler() : view(nullptr) {}
    
    std::vector<Base::Vector3d> points;
    Gui::View3DInventorViewer* view;
    
    void finish() {
        if (view) {
            view->setEditing(false);
            view->setRedirectToSceneGraph(false);
            view->removeEventCallback(SoEvent::getClassTypeId(), 
                                     DrawLineHandler::eventCallback, this);
        }
        points.clear();
        Gui::getMainWindow()->showMessage(QString());
    }
    
    static void eventCallback(void* userData, SoEventCallback* eventCB) {
        DrawLineHandler* handler = static_cast<DrawLineHandler*>(userData);
        const SoEvent* event = eventCB->getEvent();
        
        // Handle ESC key
        if (event->isOfType(SoKeyboardEvent::getClassTypeId())) {
            const SoKeyboardEvent* ke = static_cast<const SoKeyboardEvent*>(event);
            if (ke->getState() == SoButtonEvent::DOWN && 
                ke->getKey() == SoKeyboardEvent::ESCAPE) {
                handler->finish();
                eventCB->setHandled();
                return;
            }
        }
        
        // Handle mouse click
        if (event->isOfType(SoMouseButtonEvent::getClassTypeId())) {
            const SoMouseButtonEvent* mbe = static_cast<const SoMouseButtonEvent*>(event);
            
            if (mbe->getButton() == SoMouseButtonEvent::BUTTON1 &&
                mbe->getState() == SoButtonEvent::DOWN) {
                
                SbVec2s pos = mbe->getPosition();
                SbVec3f point3d = handler->view->getPointOnFocalPlane(pos);
                Base::Vector3d pt(point3d[0], point3d[1], 0.0);
                
                handler->points.push_back(pt);
                
                if (handler->points.size() == 1) {
                    Base::Console().log("First point: (%.2f, %.2f)\n", pt.x, pt.y);
                    Gui::getMainWindow()->showMessage(
                        QObject::tr("Click second point..."));
                }
                else if (handler->points.size() == 2) {
                    Base::Console().log("Second point: (%.2f, %.2f)\n", pt.x, pt.y);
                    handler->createLine();
                    handler->finish();
                }
                
                eventCB->setHandled();
            }
        }
    }
    
    void createLine() {
        if (points.size() != 2) return;
        
        try {
            Gui::Command::openCommand("Create Line");
            Gui::Command::doCommand(Gui::Command::Doc, "import Part");
            Gui::Command::doCommand(Gui::Command::Doc, 
                "line = Part.makeLine(FreeCAD.Vector(%f, %f, %f), FreeCAD.Vector(%f, %f, %f))",
                points[0].x, points[0].y, points[0].z,
                points[1].x, points[1].y, points[1].z);
            Gui::Command::doCommand(Gui::Command::Doc, 
                "obj = FreeCAD.ActiveDocument.addObject('Part::Feature', 'Line')");
            Gui::Command::doCommand(Gui::Command::Doc, "obj.Shape = line");
            Gui::Command::doCommand(Gui::Command::Doc, 
                "obj.ViewObject.LineColor = (0.0, 0.0, 0.0)");
            Gui::Command::doCommand(Gui::Command::Doc, "obj.ViewObject.LineWidth = 2.0");
            Gui::Command::doCommand(Gui::Command::Doc, "FreeCAD.ActiveDocument.recompute()");
            Gui::Command::commitCommand();
            
            double length = (points[1] - points[0]).Length();
            Base::Console().log("Line created: length = %.2f mm\n", length);
            
        } catch (const Base::Exception& e) {
            Gui::Command::abortCommand();
            Base::Console().warning("Failed to create line: %s\n", e.what());
        }
    }
};

// Global handler instance
static DrawLineHandler* lineHandler = nullptr;

// =============================================================================
// CmdDrawingLine
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
    
    if (!Gui::Application::Instance->activeDocument()) {
        QMessageBox::warning(Gui::getMainWindow(), 
            QObject::tr("No Document"),
            QObject::tr("Please create or open a document first."));
        return;
    }
    
    // Get active 3D view
    Gui::MDIView* mdi = Gui::getMainWindow()->activeWindow();
    Gui::View3DInventor* view3d = qobject_cast<Gui::View3DInventor*>(mdi);
    if (!view3d) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("No 3D View"),
            QObject::tr("Please activate a 3D view first."));
        return;
    }
    
    // Clean up previous handler if exists
    if (lineHandler) {
        lineHandler->finish();
        delete lineHandler;
    }
    
    // Create new handler
    lineHandler = new DrawLineHandler();
    lineHandler->view = view3d->getViewer();
    lineHandler->view->setEditing(true);
    lineHandler->view->setRedirectToSceneGraph(true);
    lineHandler->view->addEventCallback(
        SoEvent::getClassTypeId(),
        DrawLineHandler::eventCallback,
        lineHandler);
    
    Base::Console().log("=== Draw Line ===\n");
    Gui::getMainWindow()->showMessage(QObject::tr("Click first point (ESC to cancel)..."));
}

bool CmdDrawingLine::isActive()
{
    return Gui::Application::Instance->activeDocument() != nullptr;
}

// =============================================================================
// CmdDrawingCircle
// =============================================================================

class DrawCircleHandler {
public:
    DrawCircleHandler() : view(nullptr) {}
    
    std::vector<Base::Vector3d> points;
    Gui::View3DInventorViewer* view;
    
    void finish() {
        if (view) {
            view->setEditing(false);
            view->setRedirectToSceneGraph(false);
            view->removeEventCallback(SoEvent::getClassTypeId(),
                                     DrawCircleHandler::eventCallback, this);
        }
        points.clear();
        Gui::getMainWindow()->showMessage(QString());
    }
    
    static void eventCallback(void* userData, SoEventCallback* eventCB) {
        DrawCircleHandler* handler = static_cast<DrawCircleHandler*>(userData);
        const SoEvent* event = eventCB->getEvent();
        
        if (event->isOfType(SoKeyboardEvent::getClassTypeId())) {
            const SoKeyboardEvent* ke = static_cast<const SoKeyboardEvent*>(event);
            if (ke->getState() == SoButtonEvent::DOWN &&
                ke->getKey() == SoKeyboardEvent::ESCAPE) {
                handler->finish();
                eventCB->setHandled();
                return;
            }
        }
        
        if (event->isOfType(SoMouseButtonEvent::getClassTypeId())) {
            const SoMouseButtonEvent* mbe = static_cast<const SoMouseButtonEvent*>(event);
            
            if (mbe->getButton() == SoMouseButtonEvent::BUTTON1 &&
                mbe->getState() == SoButtonEvent::DOWN) {
                
                SbVec2s pos = mbe->getPosition();
                SbVec3f point3d = handler->view->getPointOnFocalPlane(pos);
                Base::Vector3d pt(point3d[0], point3d[1], 0.0);
                
                handler->points.push_back(pt);
                
                if (handler->points.size() == 1) {
                    Base::Console().log("Center: (%.2f, %.2f)\n", pt.x, pt.y);
                    Gui::getMainWindow()->showMessage(
                        QObject::tr("Click point on circle..."));
                }
                else if (handler->points.size() == 2) {
                    handler->createCircle();
                    handler->finish();
                }
                
                eventCB->setHandled();
            }
        }
    }
    
    void createCircle() {
        if (points.size() != 2) return;
        
        double radius = (points[1] - points[0]).Length();
        
        try {
            Gui::Command::openCommand("Create Circle");
            Gui::Command::doCommand(Gui::Command::Doc, "import Part");
            Gui::Command::doCommand(Gui::Command::Doc,
                "circle = Part.makeCircle(%f, FreeCAD.Vector(%f, %f, %f), FreeCAD.Vector(0, 0, 1))",
                radius, points[0].x, points[0].y, points[0].z);
            Gui::Command::doCommand(Gui::Command::Doc,
                "obj = FreeCAD.ActiveDocument.addObject('Part::Feature', 'Circle')");
            Gui::Command::doCommand(Gui::Command::Doc, "obj.Shape = circle");
            Gui::Command::doCommand(Gui::Command::Doc,
                "obj.ViewObject.LineColor = (0.0, 0.0, 0.0)");
            Gui::Command::doCommand(Gui::Command::Doc, "obj.ViewObject.LineWidth = 2.0");
            Gui::Command::doCommand(Gui::Command::Doc, "FreeCAD.ActiveDocument.recompute()");
            Gui::Command::commitCommand();
            
            Base::Console().log("Circle created: radius = %.2f mm\n", radius);
            
        } catch (const Base::Exception& e) {
            Gui::Command::abortCommand();
            Base::Console().warning("Failed to create circle: %s\n", e.what());
        }
    }
};

static DrawCircleHandler* circleHandler = nullptr;

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
    
    if (!Gui::Application::Instance->activeDocument()) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("No Document"),
            QObject::tr("Please create or open a document first."));
        return;
    }
    
    Gui::MDIView* mdi = Gui::getMainWindow()->activeWindow();
    Gui::View3DInventor* view3d = qobject_cast<Gui::View3DInventor*>(mdi);
    if (!view3d) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("No 3D View"),
            QObject::tr("Please activate a 3D view first."));
        return;
    }
    
    if (circleHandler) {
        circleHandler->finish();
        delete circleHandler;
    }
    
    circleHandler = new DrawCircleHandler();
    circleHandler->view = view3d->getViewer();
    circleHandler->view->setEditing(true);
    circleHandler->view->setRedirectToSceneGraph(true);
    circleHandler->view->addEventCallback(
        SoEvent::getClassTypeId(),
        DrawCircleHandler::eventCallback,
        circleHandler);
    
    Base::Console().log("=== Draw Circle ===\n");
    Gui::getMainWindow()->showMessage(QObject::tr("Click center point (ESC to cancel)..."));
}

bool CmdDrawingCircle::isActive()
{
    return Gui::Application::Instance->activeDocument() != nullptr;
}

// =============================================================================
// Command registration
// =============================================================================

void CreateDrawingCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    
    rcCmdMgr.addCommand(new CmdDrawingLine());
    rcCmdMgr.addCommand(new CmdDrawingCircle());
}

} // namespace DrawingGui
