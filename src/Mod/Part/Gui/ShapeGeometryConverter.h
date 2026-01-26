/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Development Team                           *
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

#ifndef PARTGUI_SHAPEGEOMETRYCONVERTER_H
#define PARTGUI_SHAPEGEOMETRYCONVERTER_H

#include <Gui/Render/Core/GeometryConverter.h>
#include <Mod/Part/PartGlobal.h>

class TopoDS_Shape;

namespace PartGui {

/**
 * @brief Full implementation of ShapeGeometryConverter for PartGui
 *
 * This class provides the complete implementation for converting
 * TopoDS_Shape objects to GeometryData, with full OpenCASCADE support.
 *
 * This is a standalone class that does not inherit from the stub
 * ShapeGeometryConverter in the Gui core. Use this class directly
 * in PartGui for shape conversion.
 */
class PartGuiExport ShapeGeometryConverterImpl {
public:
    ShapeGeometryConverterImpl() = default;
    ~ShapeGeometryConverterImpl() = default;

    /**
     * @brief Convert TopoDS_Shape to GeometryData
     *
     * Full implementation with OpenCASCADE tessellation support.
     */
    bool convert(const TopoDS_Shape& shape,
                 Gui::Render::GeometryData& output,
                 const Gui::Render::ConversionOptions& options = Gui::Render::ConversionOptions());

    /**
     * @brief Extract faces only (for shaded mode)
     */
    bool extractFaces(const TopoDS_Shape& shape,
                      Gui::Render::GeometryData& output,
                      const Gui::Render::ConversionOptions& options = Gui::Render::ConversionOptions());

    /**
     * @brief Extract edges only (for wireframe mode)
     */
    bool extractEdges(const TopoDS_Shape& shape,
                      Gui::Render::GeometryData& output,
                      const Gui::Render::ConversionOptions& options = Gui::Render::ConversionOptions());

    /**
     * @brief Extract vertices only (for point mode)
     */
    bool extractVertices(const TopoDS_Shape& shape,
                         Gui::Render::GeometryData& output,
                         const Gui::Render::ConversionOptions& options = Gui::Render::ConversionOptions());

private:
    /**
     * @brief Ensure shape is tessellated
     */
    bool ensureTessellatedImpl(const TopoDS_Shape& shape, double deflection);
};

} // namespace PartGui

#endif // PARTGUI_SHAPEGEOMETRYCONVERTER_H
