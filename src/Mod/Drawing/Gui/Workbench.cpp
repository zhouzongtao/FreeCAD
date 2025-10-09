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

#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>
#include <Base/Console.h>

#include "Workbench.h"
// #include "Command.h"  // Temporarily disabled

using namespace DrawingGui;

TYPESYSTEM_SOURCE(DrawingGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench()
{
}

Workbench::~Workbench() = default;

void Workbench::activated()
{
    Base::Console().log("Drawing workbench activated\n");
}

void Workbench::deactivated()
{
    Base::Console().log("Drawing workbench deactivated\n");
}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    
    // Create Drawing menu
    Gui::MenuItem* drawing = new Gui::MenuItem;
    drawing->setCommand("&Drawing");
    
    // Create submenu
    Gui::MenuItem* create = new Gui::MenuItem;
    create->setCommand("&Create");
    *create << "Drawing_Line"
            << "Drawing_Circle" 
            << "Drawing_Rectangle"
            << "Drawing_Polygon"
            << "Separator"
            << "Drawing_Text"
            << "Drawing_Dimension";
    
    Gui::MenuItem* modify = new Gui::MenuItem;
    modify->setCommand("&Modify");
    *modify << "Drawing_Move"
            << "Drawing_Copy"
            << "Drawing_Rotate"
            << "Drawing_Scale"
            << "Separator"
            << "Drawing_Trim"
            << "Drawing_Extend";
    
    *drawing << create << modify;
    
    // Insert Drawing menu before Help
    root->insertItem(drawing, root->findItem("&Help"));
    
    return root;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    
    // Create Drawing toolbar
    Gui::ToolBarItem* drawing = new Gui::ToolBarItem(root);
    drawing->setCommand("Drawing Creation");
    *drawing << "Drawing_Line"
             << "Drawing_Circle"
             << "Drawing_Rectangle" 
             << "Drawing_Polygon"
             << "Separator"
             << "Drawing_Text"
             << "Drawing_Dimension";
    
    // Modify toolbar
    Gui::ToolBarItem* modify = new Gui::ToolBarItem(root);
    modify->setCommand("Drawing Modification");
    *modify << "Drawing_Move"
            << "Drawing_Copy"
            << "Drawing_Rotate"
            << "Drawing_Scale"
            << "Separator"
            << "Drawing_Trim"
            << "Drawing_Extend";
    
    return root;
}

Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    return StdWorkbench::setupCommandBars();
}

Gui::DockWindowItems* Workbench::setupDockWindows() const
{
    Gui::DockWindowItems* root = StdWorkbench::setupDockWindows();
    
    // Add Drawing-specific dock windows here if needed
    // For example: Drawing properties panel, layer manager, etc.
    
    return root;
}
