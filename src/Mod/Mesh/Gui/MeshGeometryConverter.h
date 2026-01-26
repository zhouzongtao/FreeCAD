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

#ifndef MESHGUI_MESHGEOMETRYCONVERTER_H
#define MESHGUI_MESHGEOMETRYCONVERTER_H

#include <Gui/Render/Core/GeometryConverter.h>
#include <Mod/Mesh/MeshGlobal.h>

namespace Mesh {
    class MeshObject;
}

namespace MeshGui {

/**
 * @brief Full implementation of MeshGeometryConverter for MeshGui
 *
 * This class provides the complete implementation for converting
 * Mesh::MeshObject to GeometryData.
 */
class MeshGuiExport MeshGeometryConverterImpl {
public:
    MeshGeometryConverterImpl() = default;
    ~MeshGeometryConverterImpl() = default;

    /**
     * @brief Convert MeshObject to GeometryData
     *
     * Full implementation with mesh data extraction.
     */
    bool convert(const Mesh::MeshObject& mesh,
                 Gui::Render::GeometryData& output,
                 const Gui::Render::ConversionOptions& options = Gui::Render::ConversionOptions());

    /**
     * @brief Extract faces only
     */
    bool extractFaces(const Mesh::MeshObject& mesh,
                      Gui::Render::GeometryData& output,
                      const Gui::Render::ConversionOptions& options = Gui::Render::ConversionOptions());

    /**
     * @brief Extract edges only (boundary edges)
     */
    bool extractEdges(const Mesh::MeshObject& mesh,
                      Gui::Render::GeometryData& output,
                      const Gui::Render::ConversionOptions& options = Gui::Render::ConversionOptions());
};

} // namespace MeshGui

#endif // MESHGUI_MESHGEOMETRYCONVERTER_H
