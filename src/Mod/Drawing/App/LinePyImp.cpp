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

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

#include "DrawingFeature.h"

// Include auto-generated binding files
#include <Drawing/LinePy.h>
#include <Drawing/LinePy.cpp>

using namespace Drawing;

// returns a string which represents the object e.g. when printed in python
std::string LinePy::representation() const
{
    Line* line = getLinePtr();
    std::stringstream str;
    str << "<Drawing::Line object>";
    str << " Length=" << line->Length.getValue();
    str << " Angle=" << line->Angle.getValue();
    return str.str();
}

PyObject* LinePy::calculateGeometry(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        getLinePtr()->calculateGeometry();
        Py_INCREF(Py_None);
        return Py_None;
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

Py::Object LinePy::getDirection() const
{
    try {
        Base::Vector3d dir = getLinePtr()->getDirection();
        return Py::asObject(new Base::VectorPy(dir));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

PyObject* LinePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int LinePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}

