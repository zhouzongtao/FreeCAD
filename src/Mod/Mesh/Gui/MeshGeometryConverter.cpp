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
#include "MeshGeometryConverter.h"

#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Base/Console.h>

FC_LOG_LEVEL_INIT("MeshGeometryConverter", true, true)

namespace MeshGui {

bool MeshGeometryConverterImpl::convert(const Mesh::MeshObject& mesh,
                                        Gui::Render::GeometryData& output,
                                        const Gui::Render::ConversionOptions& options)
{
    output.clear();

    // Extract faces (triangles)
    if (!extractFaces(mesh, output, options)) {
        return false;
    }

    // Extract boundary edges if requested
    if (options.extractEdges) {
        extractEdges(mesh, output, options);
    }

    // Compute bounding box
    output.computeBoundingBox();

    return !output.isEmpty();
}

bool MeshGeometryConverterImpl::extractFaces(const Mesh::MeshObject& mesh,
                                             Gui::Render::GeometryData& output,
                                             const Gui::Render::ConversionOptions& options)
{
    const MeshCore::MeshKernel& kernel = mesh.getKernel();

    if (kernel.CountPoints() == 0 || kernel.CountFacets() == 0) {
        return false;
    }

    // Get mesh data
    const MeshCore::MeshPointArray& points = kernel.GetPoints();
    const MeshCore::MeshFacetArray& facets = kernel.GetFacets();

    // Reserve space
    output.reserve(kernel.CountPoints(), kernel.CountFacets());

    // Add vertices
    for (const auto& point : points) {
        output.addVertex(static_cast<float>(point.x),
                        static_cast<float>(point.y),
                        static_cast<float>(point.z));
    }

    // Add normals if requested
    if (options.computeVertexNormals) {
        // Compute per-vertex normals by averaging adjacent face normals
        std::vector<Base::Vector3f> normals(kernel.CountPoints(), Base::Vector3f(0, 0, 0));
        std::vector<int> normalCounts(kernel.CountPoints(), 0);

        for (const auto& facet : facets) {
            // Get facet vertices
            const Base::Vector3f& p0 = points[facet._aulPoints[0]];
            const Base::Vector3f& p1 = points[facet._aulPoints[1]];
            const Base::Vector3f& p2 = points[facet._aulPoints[2]];

            // Compute face normal
            Base::Vector3f v1 = p1 - p0;
            Base::Vector3f v2 = p2 - p0;
            Base::Vector3f faceNormal = v1 % v2;  // Cross product
            faceNormal.Normalize();

            // Add to vertex normals
            for (int i = 0; i < 3; ++i) {
                normals[facet._aulPoints[i]] += faceNormal;
                normalCounts[facet._aulPoints[i]]++;
            }
        }

        // Normalize and add to output
        for (size_t i = 0; i < normals.size(); ++i) {
            if (normalCounts[i] > 0) {
                normals[i].Normalize();
            }
            output.addNormal(normals[i].x, normals[i].y, normals[i].z);
        }

        output.normalBinding = Gui::Render::GeometryData::NormalBinding::PerVertex;
    }

    // Add triangles
    for (const auto& facet : facets) {
        output.addTriangle(
            static_cast<int32_t>(facet._aulPoints[0]),
            static_cast<int32_t>(facet._aulPoints[1]),
            static_cast<int32_t>(facet._aulPoints[2])
        );
    }

    FC_LOG("MeshGeometryConverterImpl: Converted mesh with "
           << kernel.CountPoints() << " vertices and "
           << kernel.CountFacets() << " faces");

    return output.hasFaces();
}

bool MeshGeometryConverterImpl::extractEdges(const Mesh::MeshObject& mesh,
                                             Gui::Render::GeometryData& output,
                                             const Gui::Render::ConversionOptions& /*options*/)
{
    const MeshCore::MeshKernel& kernel = mesh.getKernel();

    if (kernel.CountPoints() == 0) {
        return false;
    }

    // Get boundary edges
    // Note: For now, we extract all edges. A more sophisticated implementation
    // would only extract boundary edges or feature edges.
    const MeshCore::MeshFacetArray& facets = kernel.GetFacets();

    // Track which edges have been added (to avoid duplicates)
    std::set<std::pair<unsigned long, unsigned long>> addedEdges;

    for (const auto& facet : facets) {
        // Add the three edges of each triangle
        for (int i = 0; i < 3; ++i) {
            unsigned long idx0 = facet._aulPoints[i];
            unsigned long idx1 = facet._aulPoints[(i + 1) % 3];

            // Normalize edge direction for comparison
            auto edge = (idx0 < idx1) ? std::make_pair(idx0, idx1)
                                     : std::make_pair(idx1, idx0);

            if (addedEdges.find(edge) == addedEdges.end()) {
                addedEdges.insert(edge);
                output.addLine(static_cast<int32_t>(idx0),
                              static_cast<int32_t>(idx1));
            }
        }
    }

    FC_LOG("MeshGeometryConverterImpl: Extracted " << addedEdges.size() << " edges");

    return output.hasLines();
}

} // namespace MeshGui
