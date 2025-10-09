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
# include <Python.h>
# include <Base/Console.h>
# include <Base/Interpreter.h>
# include <Base/PyObjectBase.h>
# include <Gui/Application.h>
# include <Gui/Language/Translator.h>
#endif

#include "Workbench.h"
#include "ViewProviderDrawing.h"
// #include "Command.h"  // Temporarily disabled

// use a different name to CreateCommand()
// void CreateDrawingCommands(void);  // Temporarily disabled

void loadDrawingResource()
{
    // add resources and reloads the translators
    // Q_INIT_RESOURCE(Drawing);  // Temporarily disabled - resource file may not exist
    Gui::Translator::instance()->refresh();
}

namespace DrawingGui {
extern PyObject* initModule();
}

/* Python entry */
PyMOD_INIT_FUNC(DrawingGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(nullptr);
    }

    // Load dependent modules
    try {
        Base::Interpreter().loadModule("Drawing");
        Base::Interpreter().loadModule("PartGui");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }

    PyObject* mod = DrawingGui::initModule();
    Base::Console().log("Loading DrawingGui module... done\n");

    // Register workbench
    (void)new DrawingGui::Workbench();

    // Register view providers - temporarily disabled as init() methods don't exist
    // DrawingGui::ViewProviderDrawing::init();
    // DrawingGui::ViewProviderLine::init();
    // DrawingGui::ViewProviderCircle::init();
    // DrawingGui::ViewProviderRectangle::init();
    // DrawingGui::ViewProviderPolygon::init();
    // DrawingGui::ViewProviderText::init();
    // DrawingGui::ViewProviderDimension::init();

    // Register commands - temporarily disabled
    // CreateDrawingCommands();

    // Load resources - temporarily disabled
    // loadDrawingResource();

    PyMOD_Return(mod);
}
