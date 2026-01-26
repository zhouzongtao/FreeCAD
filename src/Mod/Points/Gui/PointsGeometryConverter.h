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

#ifndef POINTSGUI_POINTSGEOMETRYCONVERTER_H
#define POINTSGUI_POINTSGEOMETRYCONVERTER_H

#include <Gui/Render/Core/GeometryConverter.h>
#include <Mod/Points/PointsGlobal.h>

namespace Points {
    class PointKernel;
}

namespace PointsGui {

/**
 * @brief Full implementation of PointsGeometryConverter for PointsGui
 *
 * This class provides the complete implementation for converting
 * Points::PointKernel to GeometryData.
 */
class PointsGuiExport PointsGeometryConverterImpl {
public:
    PointsGeometryConverterImpl() = default;
    ~PointsGeometryConverterImpl() = default;

    /**
     * @brief Convert PointKernel to GeometryData
     *
     * Full implementation with point cloud data extraction.
     *
     * @param points The point cloud to convert
     * @param output The geometry data output
     * @param options Conversion options
     * @return true if conversion succeeded
     */
    bool convert(const Points::PointKernel& points,
                 Gui::Render::GeometryData& output,
                 const Gui::Render::ConversionOptions& options = Gui::Render::ConversionOptions());

    /**
     * @brief Extract points with optional normals
     *
     * @param points The point cloud to extract
     * @param output The geometry data output
     * @param options Conversion options
     * @return true if extraction succeeded
     */
    bool extractPoints(const Points::PointKernel& points,
                       Gui::Render::GeometryData& output,
                       const Gui::Render::ConversionOptions& options = Gui::Render::ConversionOptions());
};

} // namespace PointsGui

#endif // POINTSGUI_POINTSGEOMETRYCONVERTER_H
