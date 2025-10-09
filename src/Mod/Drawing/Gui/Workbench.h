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

#ifndef DRAWINGUI_WORKBENCH_H
#define DRAWINGUI_WORKBENCH_H

#include <Gui/Workbench.h>
#include <Mod/Drawing/DrawingGlobal.h>

namespace DrawingGui {

/** Drawing workbench
 *  @author FreeCAD Project
 */
class DrawingGuiExport Workbench : public Gui::StdWorkbench
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    Workbench();
    ~Workbench() override;

    /** Run some actions when the workbench gets activated */
    void activated() override;
    
    /** Run some actions when the workbench gets deactivated */
    void deactivated() override;

protected:
    /// Setup the menus for this workbench
    Gui::MenuItem* setupMenuBar() const override;
    
    /// Setup the toolbars for this workbench  
    Gui::ToolBarItem* setupToolBars() const override;
    
    /// Setup the command bars for this workbench
    Gui::ToolBarItem* setupCommandBars() const override;
    
    /// Setup the dockable windows for this workbench
    Gui::DockWindowItems* setupDockWindows() const override;
};

} // namespace DrawingGui

#endif // DRAWINGUI_WORKBENCH_H
