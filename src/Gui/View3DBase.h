/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                    *
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

#ifndef GUI_VIEW3DBASE_H
#define GUI_VIEW3DBASE_H

#include "MDIView.h"
#include "View3D/IViewer3D.h"

namespace Gui
{

/** Abstract base class for 3D views
 *  Provides common interface for different rendering backends (Coin3D, OsgVerse)
 */
class GuiExport View3DBase : public MDIView
{
    Q_OBJECT
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    enum class BackendType
    {
        Coin3D,
        OsgVerse
    };

    View3DBase(Gui::Document* pcDocument, QWidget* parent, Qt::WindowFlags wflags = Qt::WindowFlags())
        : MDIView(pcDocument, parent, wflags)
    {}

    ~View3DBase() override = default;

    // Abstract interface - must be implemented by derived classes
    virtual View3D::IViewer3D* getViewerInterface() = 0;
    virtual BackendType getBackendType() const = 0;
};

}  // namespace Gui

#endif  // GUI_VIEW3DBASE_H
