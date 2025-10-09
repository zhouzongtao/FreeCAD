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

#ifndef DRAWINGGUI_PRECOMPILED_H
#define DRAWINGGUI_PRECOMPILED_H

#include <FCConfig.h>

// Importing of App classes
#ifdef FC_OS_WIN32
# define DrawingGuiExport __declspec(dllexport)
# define DrawingAppExport __declspec(dllimport)
# define PartExport __declspec(dllimport)
# define PartGuiExport __declspec(dllimport)
#else // for Linux
# define DrawingGuiExport
# define DrawingAppExport
# define PartExport
# define PartGuiExport
#endif

#ifdef _PreComp_

// Python
#include <Python.h>

// Qt
#include <QApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QPixmap>
#include <QToolBar>
#include <QToolButton>
#include <QInputDialog>
#include <QMessageBox>

// Open Inventor
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMarkerSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTransform.h>

// Standard library
#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <memory>

// FreeCAD Base
#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/Vector3D.h>

// FreeCAD App
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

// FreeCAD Gui
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#endif // _PreComp_

#endif // DRAWINGGUI_PRECOMPILED_H
