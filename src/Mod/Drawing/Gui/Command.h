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

#ifndef DRAWINGUI_COMMAND_H
#define DRAWINGUI_COMMAND_H

#include <Gui/Command.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Base/Vector3D.h>
#include <Inventor/SbVec.h>

namespace DrawingGui {

/** Base class for interactive drawing commands
 */
class DrawingGuiExport DrawingCommand : public Gui::Command
{
public:
    DrawingCommand(const char* sMenu, const char* sToolTip=nullptr, 
                   const char* sWhat=nullptr, const char* sStatus=nullptr, 
                   const char* sPixmap=nullptr, const char* sAccel=nullptr);
    ~DrawingCommand() override;

    /// Get current mouse position in 3D space
    Base::Vector3d getCurrentPoint(const SbVec2s& pos, Gui::View3DInventorViewer* viewer);
    
    /// Snap to grid if enabled
    Base::Vector3d snapToGrid(const Base::Vector3d& point);
    
    /// Show coordinate input dialog
    bool getCoordinateInput(const QString& prompt, Base::Vector3d& point);
    
    /// Start interactive point picking
    void startPointPicking(const QString& prompt);
    
    /// Handle mouse events during point picking
    virtual bool handleMouseEvent(const SbVec2s& pos, int button, bool pressed);
    
    /// Finish current command
    virtual void finishCommand();
    
    /// Cancel current command
    virtual void cancelCommand();

protected:
    /// Current picked points
    std::vector<Base::Vector3d> pickedPoints;
    
    /// Maximum points needed for this command
    int maxPoints;
    
    /// Current picking state
    bool isPickingPoints;
    
    /// Preview object for dynamic feedback
    App::DocumentObject* previewObject;
};

/** Command to create a line
 */
class CmdDrawingLine : public DrawingCommand
{
public:
    CmdDrawingLine();
    ~CmdDrawingLine() override;

    void activated(int iMsg) override;
    bool isActive() override;
    const char* className() const override {return "CmdDrawingLine";}

protected:
    bool handleMouseEvent(const SbVec2s& pos, int button, bool pressed) override;
    void updatePreview();
    void createLine();
};

/** Command to create a circle
 */
class CmdDrawingCircle : public DrawingCommand  
{
public:
    CmdDrawingCircle();
    ~CmdDrawingCircle() override;

    void activated(int iMsg) override;
    bool isActive() override;
    const char* className() const override {return "CmdDrawingCircle";}

protected:
    bool handleMouseEvent(const SbVec2s& pos, int button, bool pressed) override;
    void updatePreview();
    void createCircle();

private:
    enum PickingState {
        PickingCenter,
        PickingRadius
    };
    PickingState currentState;
};

/** Command to create a rectangle
 */
class CmdDrawingRectangle : public DrawingCommand
{
public:
    CmdDrawingRectangle();
    ~CmdDrawingRectangle() override;

    void activated(int iMsg) override;
    bool isActive() override;
    const char* className() const override {return "CmdDrawingRectangle";}

protected:
    bool handleMouseEvent(const SbVec2s& pos, int button, bool pressed) override;
    void updatePreview();
    void createRectangle();
};

/** Command to create a polygon
 */
class CmdDrawingPolygon : public DrawingCommand
{
public:
    CmdDrawingPolygon();
    ~CmdDrawingPolygon() override;

    void activated(int iMsg) override;
    bool isActive() override;
    const char* className() const override {return "CmdDrawingPolygon";}

protected:
    bool handleMouseEvent(const SbVec2s& pos, int button, bool pressed) override;
    void updatePreview();
    void createPolygon();
    void finishPolygon();

private:
    bool isFirstPoint;
};

/** Command to add text
 */
class CmdDrawingText : public DrawingCommand
{
public:
    CmdDrawingText();
    ~CmdDrawingText() override;

    void activated(int iMsg) override;
    bool isActive() override;
    const char* className() const override {return "CmdDrawingText";}

protected:
    bool handleMouseEvent(const SbVec2s& pos, int button, bool pressed) override;
    void showTextDialog();
    void createText(const QString& text, const Base::Vector3d& position);
};

/** Command to create dimensions
 */
class CmdDrawingDimension : public DrawingCommand
{
public:
    CmdDrawingDimension();
    ~CmdDrawingDimension() override;

    void activated(int iMsg) override;
    bool isActive() override;
    const char* className() const override {return "CmdDrawingDimension";}

protected:
    bool handleMouseEvent(const SbVec2s& pos, int button, bool pressed) override;
    void updatePreview();
    void createDimension();

private:
    enum PickingState {
        PickingFirst,
        PickingSecond,
        PickingDimLine
    };
    PickingState currentState;
};

/// Create all Drawing commands
void CreateDrawingCommands();

} // namespace DrawingGui

#endif // DRAWINGUI_COMMAND_H
