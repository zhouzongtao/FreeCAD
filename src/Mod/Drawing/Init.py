# ***************************************************************************
# *   Copyright (c) 2024 FreeCAD Project                                   *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""
Drawing workbench initialization - provides 2D drawing functionality

This workbench is implemented in C++ with full Python bindings to demonstrate
the use of AST/YAPTU tools for creating FreeCAD extensions.
"""

import FreeCAD as App

# Add import/export types for drawing formats
App.addImportType("Drawing Exchange Format (*.dxf *.DXF)", "Drawing")
App.addExportType("Drawing Exchange Format (*.dxf)", "Drawing")
App.addImportType("Scalable Vector Graphics (*.svg *.SVG)", "Drawing")
App.addExportType("Scalable Vector Graphics (*.svg)", "Drawing")

# Add to unit tests
App.__unit_test__ += ["TestDrawing"]
