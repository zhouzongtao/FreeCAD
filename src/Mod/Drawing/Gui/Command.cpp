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

#include <QInputDialog>
#include <QMessageBox>
#include <QApplication>

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
// #include <Gui/Selection.h> // Temporarily disabled
#include <Gui/CommandT.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <App/Application.h>
#include <App/Document.h>

#include <Inventor/nodes/SoCamera.h>
#include <Inventor/SbLinear.h>

#include "Command.h"
#include "../App/DrawingFeature.h"

using namespace DrawingGui;
using namespace Gui;

// =============================================================================
// DrawingCommand base class
// =============================================================================

DrawingCommand::DrawingCommand(const char* sMenu, const char* sToolTip, 
                              const char* sWhat, const char* sStatus, 
                              const char* sPixmap, const char* sAccel)
    : Gui::Command(sMenu, sToolTip, sWhat, sStatus, sPixmap, sAccel)
    , maxPoints(0)
    , isPickingPoints(false)
    , previewObject(nullptr)
{
}

DrawingCommand::~DrawingCommand() = default;

Base::Vector3d DrawingCommand::getCurrentPoint(const SbVec2s& pos, Gui::View3DInventorViewer* viewer)
{
    // Get the current working plane (simplified - assume XY plane)
    SbVec3f point, normal;
    viewer->getNearPlane(pos, point, normal);
    
    // Project to XY plane (Z=0)
    Base::Vector3d result(point[0], point[1], 0.0);
    
    return snapToGrid(result);
}

Base::Vector3d DrawingCommand::snapToGrid(const Base::Vector3d& point)
{
    // Simple grid snapping (1mm grid)
    double gridSize = 1.0;
    
    double x = round(point.x / gridSize) * gridSize;
    double y = round(point.y / gridSize) * gridSize;
    double z = point.z; // Keep Z unchanged
    
    return Base::Vector3d(x, y, z);
}

bool DrawingCommand::getCoordinateInput(const QString& prompt, Base::Vector3d& point)
{
    bool ok;
    QString input = QInputDialog::getText(nullptr, tr("Coordinate Input"), 
                                         prompt, QLineEdit::Normal, QString(), &ok);
    
    if (!ok || input.isEmpty()) {
        return false;
    }
    
    // Parse coordinate input (format: "x,y" or "x,y,z")
    QStringList coords = input.split(',');
    if (coords.size() >= 2) {
        point.x = coords[0].trimmed().toDouble();
        point.y = coords[1].trimmed().toDouble();
        point.z = (coords.size() >= 3) ? coords[2].trimmed().toDouble() : 0.0;
        return true;
    }
    
    QMessageBox::warning(nullptr, tr("Invalid Input"), 
                        tr("Please enter coordinates in format: x,y or x,y,z"));
    return false;
}

void DrawingCommand::startPointPicking(const QString& prompt)
{
    isPickingPoints = true;
    pickedPoints.clear();
    
    // Show status message
    getMainWindow()->showMessage(prompt);
    
    // Enable mouse tracking in 3D view
    Gui::View3DInventor* view = qobject_cast<Gui::View3DInventor*>(getMainWindow()->activeWindow());
    if (view) {
        view->getViewer()->setEditing(true);
        view->getViewer()->addEventCallback(SoMouseButtonEvent::getClassTypeId(), 
                                           handleMouseEventCallback, this);
    }
}

bool DrawingCommand::handleMouseEvent(const SbVec2s& pos, int button, bool pressed)
{
    // Base implementation - to be overridden by derived classes
    return false;
}

void DrawingCommand::finishCommand()
{
    isPickingPoints = false;
    
    // Disable mouse tracking
    Gui::View3DInventor* view = qobject_cast<Gui::View3DInventor*>(getMainWindow()->activeWindow());
    if (view) {
        view->getViewer()->setEditing(false);
        view->getViewer()->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), 
                                              handleMouseEventCallback, this);
    }
    
    // Clean up preview object
    if (previewObject) {
        getActiveGuiDocument()->getDocument()->removeObject(previewObject->getNameInDocument());
        previewObject = nullptr;
    }
    
    getMainWindow()->showMessage(QString());
}

void DrawingCommand::cancelCommand()
{
    finishCommand();
    getMainWindow()->showMessage(tr("Command cancelled"));
}

// =============================================================================
// CmdDrawingLine implementation
// =============================================================================

CmdDrawingLine::CmdDrawingLine()
    : DrawingCommand("Drawing_Line", 
                    QT_TR_NOOP("Create line"),
                    QT_TR_NOOP("Create a line by picking two points"),
                    QT_TR_NOOP("Create a line by picking two points"),
                    "Drawing_Line",
                    "L")
{
    maxPoints = 2;
}

CmdDrawingLine::~CmdDrawingLine() = default;

void CmdDrawingLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    
    if (!getActiveGuiDocument()) {
        QMessageBox::warning(nullptr, tr("No Document"), 
                            tr("Please create or open a document first."));
        return;
    }
    
    startPointPicking(tr("Pick first point for line:"));
}

bool CmdDrawingLine::isActive()
{
    return getActiveGuiDocument() != nullptr;
}

bool CmdDrawingLine::handleMouseEvent(const SbVec2s& pos, int button, bool pressed)
{
    if (!pressed || button != 1) // Only handle left mouse button press
        return false;
    
    Gui::View3DInventor* view = qobject_cast<Gui::View3DInventor*>(getMainWindow()->activeWindow());
    if (!view)
        return false;
    
    Base::Vector3d point = getCurrentPoint(pos, view->getViewer());
    pickedPoints.push_back(point);
    
    if (pickedPoints.size() == 1) {
        getMainWindow()->showMessage(tr("Pick second point for line:"));
        updatePreview();
    } else if (pickedPoints.size() == 2) {
        createLine();
        finishCommand();
    }
    
    return true;
}

void CmdDrawingLine::updatePreview()
{
    if (pickedPoints.size() == 1 && !previewObject) {
        // Create preview line
        App::Document* doc = getActiveGuiDocument()->getDocument();
        previewObject = doc->addObject("Drawing::Line", "PreviewLine");
        
        Drawing::Line* line = static_cast<Drawing::Line*>(previewObject);
        line->StartPoint.setValue(pickedPoints[0]);
        line->Construction.setValue(true); // Mark as construction for preview
        doc->recompute();
    }
}

void CmdDrawingLine::createLine()
{
    if (pickedPoints.size() != 2)
        return;
    
    try {
        openCommand(QT_TRANSLATE_NOOP("Command", "Create Line"));
        
        App::Document* doc = getActiveGuiDocument()->getDocument();
        Drawing::Line* line = static_cast<Drawing::Line*>(
            doc->addObject("Drawing::Line", "Line"));
        
        line->StartPoint.setValue(pickedPoints[0]);
        line->EndPoint.setValue(pickedPoints[1]);
        
        // Calculate length and angle
        Base::Vector3d diff = pickedPoints[1] - pickedPoints[0];
        line->Length.setValue(diff.Length());
        line->Angle.setValue(atan2(diff.y, diff.x) * 180.0 / M_PI);
        
        doc->recompute();
        commitCommand();
        
        getMainWindow()->showMessage(tr("Line created successfully"));
        
    } catch (const Base::Exception& e) {
        abortCommand();
        QMessageBox::critical(nullptr, tr("Error"), 
                             tr("Failed to create line: %1").arg(QString::fromUtf8(e.what())));
    }
}

// =============================================================================
// CmdDrawingCircle implementation
// =============================================================================

CmdDrawingCircle::CmdDrawingCircle()
    : DrawingCommand("Drawing_Circle",
                    QT_TR_NOOP("Create circle"),
                    QT_TR_NOOP("Create a circle by picking center and radius"),
                    QT_TR_NOOP("Create a circle by picking center and radius"),
                    "Drawing_Circle",
                    "C")
    , currentState(PickingCenter)
{
    maxPoints = 2;
}

CmdDrawingCircle::~CmdDrawingCircle() = default;

void CmdDrawingCircle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    
    if (!getActiveGuiDocument()) {
        QMessageBox::warning(nullptr, tr("No Document"), 
                            tr("Please create or open a document first."));
        return;
    }
    
    currentState = PickingCenter;
    startPointPicking(tr("Pick center point for circle:"));
}

bool CmdDrawingCircle::isActive()
{
    return getActiveGuiDocument() != nullptr;
}

bool CmdDrawingCircle::handleMouseEvent(const SbVec2s& pos, int button, bool pressed)
{
    if (!pressed || button != 1)
        return false;
    
    Gui::View3DInventor* view = qobject_cast<Gui::View3DInventor*>(getMainWindow()->activeWindow());
    if (!view)
        return false;
    
    Base::Vector3d point = getCurrentPoint(pos, view->getViewer());
    
    if (currentState == PickingCenter) {
        pickedPoints.clear();
        pickedPoints.push_back(point);
        currentState = PickingRadius;
        getMainWindow()->showMessage(tr("Pick point on circle:"));
        updatePreview();
    } else if (currentState == PickingRadius) {
        pickedPoints.push_back(point);
        createCircle();
        finishCommand();
    }
    
    return true;
}

void CmdDrawingCircle::updatePreview()
{
    if (pickedPoints.size() == 1 && !previewObject) {
        App::Document* doc = getActiveGuiDocument()->getDocument();
        previewObject = doc->addObject("Drawing::Circle", "PreviewCircle");
        
        Drawing::Circle* circle = static_cast<Drawing::Circle*>(previewObject);
        circle->Center.setValue(pickedPoints[0]);
        circle->Radius.setValue(5.0); // Default preview radius
        circle->Construction.setValue(true);
        doc->recompute();
    }
}

void CmdDrawingCircle::createCircle()
{
    if (pickedPoints.size() != 2)
        return;
    
    try {
        openCommand(QT_TRANSLATE_NOOP("Command", "Create Circle"));
        
        App::Document* doc = getActiveGuiDocument()->getDocument();
        Drawing::Circle* circle = static_cast<Drawing::Circle*>(
            doc->addObject("Drawing::Circle", "Circle"));
        
        Base::Vector3d center = pickedPoints[0];
        Base::Vector3d radiusPoint = pickedPoints[1];
        double radius = (radiusPoint - center).Length();
        
        circle->Center.setValue(center);
        circle->Radius.setValue(radius);
        circle->FirstAngle.setValue(0.0);
        circle->LastAngle.setValue(360.0);
        
        doc->recompute();
        commitCommand();
        
        getMainWindow()->showMessage(tr("Circle created successfully"));
        
    } catch (const Base::Exception& e) {
        abortCommand();
        QMessageBox::critical(nullptr, tr("Error"), 
                             tr("Failed to create circle: %1").arg(QString::fromUtf8(e.what())));
    }
}

// =============================================================================
// Command registration
// =============================================================================

void DrawingGui::CreateDrawingCommands()
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    
    rcCmdMgr.addCommand(new CmdDrawingLine());
    rcCmdMgr.addCommand(new CmdDrawingCircle());
    // Add more commands as they are implemented
}
