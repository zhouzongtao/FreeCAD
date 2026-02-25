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

#include "PreCompiled.h"
#include "PointsGeometryConverter.h"

#include <Mod/Points/App/Points.h>
#include <Base/Console.h>

#include <cmath>

FC_LOG_LEVEL_INIT("PointsGeometryConverter", true, true)

namespace PointsGui {

bool PointsGeometryConverterImpl::convert(const Points::PointKernel& points,
                                          Gui::Render::GeometryData& output,
                                          const Gui::Render::ConversionOptions& options)
{
    output.clear();

    // Extract points
    if (!extractPoints(points, output, options)) {
        return false;
    }

    // Compute bounding box
    output.computeBoundingBox();

    return !output.isEmpty();
}

bool PointsGeometryConverterImpl::extractPoints(const Points::PointKernel& points,
                                                Gui::Render::GeometryData& output,
                                                const Gui::Render::ConversionOptions& /*options*/)
{
    if (points.size() == 0) {
        return false;
    }

    // Reserve memory
    size_t pointCount = points.size();
    output.vertices.reserve(pointCount * 3);
    output.pointIndices.reserve(pointCount);

    // Get the raw points data
    const std::vector<Points::PointKernel::value_type>& kernel = points.getBasicPoints();

    // Add points, filtering out NaN values
    int32_t validIndex = 0;
    for (size_t i = 0; i < kernel.size(); ++i) {
        const auto& pt = kernel[i];

        // Check for NaN values (invalid points)
        if (std::isnan(pt.x) || std::isnan(pt.y) || std::isnan(pt.z)) {
            continue;
        }

        // Add vertex
        output.addVertex(pt.x, pt.y, pt.z);
        output.pointIndices.push_back(validIndex++);
    }

    FC_LOG("PointsGeometryConverterImpl: Converted point cloud with "
           << validIndex << " valid points out of " << pointCount << " total");

    return output.hasPoints();
}

} // namespace PointsGui
