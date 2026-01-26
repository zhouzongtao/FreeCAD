// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef OSGVERSEGUI_PRECOMPILED_H
#define OSGVERSEGUI_PRECOMPILED_H

#include <FCConfig.h>

// Importing of App classes
#ifdef FC_OS_WIN32
# define OsgVerseGuiExport __declspec(dllexport)
# define PartExport __declspec(dllimport)
# define GuiExport __declspec(dllimport)
#else // for Linux
# define OsgVerseGuiExport
# define PartExport
# define GuiExport
#endif

#ifdef _PreComp_

// Standard library
#include <string>
#include <vector>
#include <map>
#include <memory>

// Qt
#include <QWidget>
#include <QColor>

// OSG
#include <osg/ref_ptr>
#include <osg/Node>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <osg/StateSet>
#include <osg/Material>
#include <osgViewer/Viewer>

// OCCT
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopExp_Explorer.hxx>
#include <BRepMesh_IncrementalMesh.hxx>

// FreeCAD Base
#include <Base/Console.h>
#include <Base/Parameter.h>

// FreeCAD App
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Property.h>

// FreeCAD Gui
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Application.h>
#include <Gui/Document.h>

// Part module
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PropertyPartShape.h>

#endif // _PreComp_

#endif // OSGVERSEGUI_PRECOMPILED_H
