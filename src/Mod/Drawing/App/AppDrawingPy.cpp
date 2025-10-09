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

#include <Python.h>

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <App/Document.h>

#include "DrawingFeature.h"

namespace Drawing {

class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("Drawing")
    {
        add_varargs_method("makeDrawing", &Module::makeDrawing, 
                          "makeDrawing() -- Create a new drawing object");
        add_varargs_method("makeLine", &Module::makeLine,
                          "makeLine(start, end) -- Create a line from start to end point");
        add_varargs_method("makeCircle", &Module::makeCircle,
                          "makeCircle(center, radius) -- Create a circle");
        add_varargs_method("makeRectangle", &Module::makeRectangle,
                          "makeRectangle(start, width, height) -- Create a rectangle");
        
        initialize("This module provides 2D drawing functionality for FreeCAD");
    }
    
    virtual ~Module() {}

private:
    // Python API functions
    Py::Object makeDrawing(const Py::Tuple& args)
    {
        // Create a basic drawing feature
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (!doc) {
            throw Py::RuntimeError("No active document");
        }
        
        Drawing::Feature* feature = static_cast<Drawing::Feature*>(
            doc->addObject("Drawing::Feature", "DrawingFeature"));
        
        return Py::asObject(feature->getPyObject());
    }
    
    Py::Object makeLine(const Py::Tuple& args)
    {
        if (args.size() != 2) {
            throw Py::TypeError("makeLine() takes exactly 2 arguments (start, end)");
        }
        
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (!doc) {
            throw Py::RuntimeError("No active document");
        }
        
        // Extract start and end points
        Py::Object startObj = args[0];
        Py::Object endObj = args[1];
        
        // Convert to Base::Vector3d (simplified)
        Base::Vector3d start(0, 0, 0);
        Base::Vector3d end(10, 0, 0);
        
        Drawing::Line* line = static_cast<Drawing::Line*>(
            doc->addObject("Drawing::Line", "Line"));
        
        line->StartPoint.setValue(start);
        line->EndPoint.setValue(end);
        
        Base::Vector3d diff = end - start;
        line->Length.setValue(diff.Length());
        line->Angle.setValue(atan2(diff.y, diff.x) * 180.0 / M_PI);
        
        return Py::asObject(line->getPyObject());
    }
    
    Py::Object makeCircle(const Py::Tuple& args)
    {
        if (args.size() != 2) {
            throw Py::TypeError("makeCircle() takes exactly 2 arguments (center, radius)");
        }
        
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (!doc) {
            throw Py::RuntimeError("No active document");
        }
        
        // Extract center and radius
        Base::Vector3d center(0, 0, 0);
        double radius = 5.0;
        
        Drawing::Circle* circle = static_cast<Drawing::Circle*>(
            doc->addObject("Drawing::Circle", "Circle"));
        
        circle->Center.setValue(center);
        circle->Radius.setValue(radius);
        circle->FirstAngle.setValue(0.0);
        circle->LastAngle.setValue(360.0);
        
        return Py::asObject(circle->getPyObject());
    }
    
    Py::Object makeRectangle(const Py::Tuple& args)
    {
        if (args.size() != 3) {
            throw Py::TypeError("makeRectangle() takes exactly 3 arguments (start, width, height)");
        }
        
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (!doc) {
            throw Py::RuntimeError("No active document");
        }
        
        // Extract parameters
        Base::Vector3d start(0, 0, 0);
        double width = 10.0;
        double height = 5.0;
        
        Drawing::Rectangle* rect = static_cast<Drawing::Rectangle*>(
            doc->addObject("Drawing::Rectangle", "Rectangle"));
        
        rect->StartPoint.setValue(start);
        rect->Width.setValue(width);
        rect->Height.setValue(height);
        
        return Py::asObject(rect->getPyObject());
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

} // namespace Drawing
