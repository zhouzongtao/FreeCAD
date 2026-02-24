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

#ifndef GUI_VIEW3DOSGVERSEPY_H
#define GUI_VIEW3DOSGVERSEPY_H

#ifdef RENDER_HAS_OSGVERSE_BACKEND

#include "MDIView.h"
#include "MDIViewPy.h"

namespace Gui
{

class View3DOsgVerse;

class View3DOsgVersePy: public Py::PythonExtension<View3DOsgVersePy>
{
public:
    using BaseType = Py::PythonExtension<View3DOsgVersePy>;
    static void init_type();

    explicit View3DOsgVersePy(View3DOsgVerse* vi);
    ~View3DOsgVersePy() override;

    View3DOsgVerse* getView3DOsgVersePtr();
    Py::Object repr() override;
    Py::Object getattr(const char*) override;
    int setattr(const char*, const Py::Object&) override;
    Py::Object cast_to_base();

    // Camera methods
    Py::Object getCamera();
    Py::Object setCamera(const Py::Tuple&);
    Py::Object getViewDirection();
    Py::Object getUpDirection();
    Py::Object setViewDirection(const Py::Tuple&);
    Py::Object getCameraOrientation();
    Py::Object setCameraOrientation(const Py::Tuple&);
    Py::Object getCameraType();
    Py::Object setCameraType(const Py::Tuple&);
    Py::Object listCameraTypes();

    // View methods
    Py::Object fitAll(const Py::Tuple&);
    Py::Object viewBottom();
    Py::Object viewFront();
    Py::Object viewLeft();
    Py::Object viewRear();
    Py::Object viewRight();
    Py::Object viewTop();
    Py::Object viewIsometric();
    Py::Object zoomIn();
    Py::Object zoomOut();

    // Interaction methods
    Py::Object getPointOnFocalPlane(const Py::Tuple&);
    Py::Object projectPointToLine(const Py::Tuple&);
    Py::Object getPointOnViewport(const Py::Tuple&);
    Py::Object getObjectInfo(const Py::Tuple&);
    Py::Object getSize();
    Py::Object getCursorPos();

    // Navigation
    Py::Object listNavigationTypes();
    Py::Object getNavigationType();
    Py::Object setNavigationType(const Py::Tuple&);

    // Misc
    Py::Object toggleClippingPlane(const Py::Tuple& args, const Py::Dict&);
    Py::Object hasClippingPlane();
    Py::Object saveImage(const Py::Tuple&);
    Py::Object redraw();
    Py::Object setAxisCross(const Py::Tuple&);
    Py::Object hasAxisCross();

private:
    using method_varargs_handler = PyObject* (*)(PyObject* _self, PyObject* _args);
    static method_varargs_handler pycxx_handler;
    static PyObject* method_varargs_ext_handler(PyObject* _self, PyObject* _args);
    Py::Object getattribute(const char*);

private:
    Gui::MDIViewPy base;
};

}  // namespace Gui

#endif // RENDER_HAS_OSGVERSE_BACKEND

#endif  // GUI_VIEW3DOSGVERSEPY_H
