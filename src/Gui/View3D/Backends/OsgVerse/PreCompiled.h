/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Development Team                             *
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

#ifndef GUI_VIEW3D_OSGVERSE_PRECOMPILED_H
#define GUI_VIEW3D_OSGVERSE_PRECOMPILED_H

#include <FCConfig.h>

// Don't redefine GuiExport - it's already defined in FCGlobal.h
#include <FCGlobal.h>

#ifdef _PreComp_

// standard
#include <cassert>
#include <cmath>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

// Qt
#include <QWidget>
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QImage>

// OSG
#include <osg/ref_ptr>
#include <osg/Node>
#include <osg/Group>
#include <osg/Camera>
#include <osg/Light>
#include <osg/LightSource>
#include <osg/Version>
#include <osg/ComputeBoundsVisitor>
#include <osg/MatrixTransform>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/Material>
#include <osgViewer/Viewer>
#include <osgViewer/GraphicsWindow>
#include <osgGA/TrackballManipulator>

// FreeCAD Base
#include <Base/Console.h>
#include <Base/Vector3D.h>
#include <Base/Matrix.h>

// FreeCAD Gui (forward declarations to avoid circular dependencies)
namespace Gui {
    class ViewProvider;
    class ViewProviderDocumentObject;
}

#endif //_PreComp_

#endif // GUI_VIEW3D_OSGVERSE_PRECOMPILED_H
