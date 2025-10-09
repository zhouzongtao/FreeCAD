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

#ifndef DRAWING_PRECOMPILED_H
#define DRAWING_PRECOMPILED_H

#include <FCConfig.h>

// Importing of App classes
#ifdef FC_OS_WIN32
# define DrawingAppExport __declspec(dllimport)
# define PartExport __declspec(dllimport)
# define PartGuiExport __declspec(dllimport)
#else // for Linux
# define DrawingAppExport
# define PartExport
# define PartGuiExport
#endif

#ifdef _PreComp_

// Python
#include <Python.h>

// Standard library
#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <memory>

// STL
#include <algorithm>
#include <functional>

// OpenCASCADE
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>

// FreeCAD Base
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/Console.h>
#include <Base/Vector3D.h>
#include <Base/Matrix.h>
#include <Base/Placement.h>

// FreeCAD App
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Property.h>
#include <App/PropertyStandard.h>
#include <App/PropertyLinks.h>

#endif // _PreComp_

#endif // DRAWING_PRECOMPILED_H
