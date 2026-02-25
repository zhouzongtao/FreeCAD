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
#include "ShapeGeometryConverter.h"

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Polygon3D.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_TangentialDeflection.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <TopLoc_Location.hxx>
#include <Precision.hxx>

#include <Base/Console.h>

FC_LOG_LEVEL_INIT("ShapeGeometryConverter", true, true)

namespace PartGui {

bool ShapeGeometryConverterImpl::ensureTessellatedImpl(const TopoDS_Shape& shape, double deflection)
{
    if (shape.IsNull()) {
        return false;
    }

    // Check if already tessellated
    bool needTessellation = false;
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        const TopoDS_Face& face = TopoDS::Face(exp.Current());
        TopLoc_Location loc;
        Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, loc);
        if (triangulation.IsNull()) {
            needTessellation = true;
            break;
        }
    }

    if (needTessellation) {
        try {
            BRepMesh_IncrementalMesh mesh(shape, deflection, Standard_False, 0.5);
            if (!mesh.IsDone()) {
                FC_WARN("ShapeGeometryConverterImpl: Tessellation failed");
                return false;
            }
        }
        catch (const Standard_Failure& e) {
            FC_WARN("ShapeGeometryConverterImpl: Tessellation exception: " << e.GetMessageString());
            return false;
        }
    }

    return true;
}

bool ShapeGeometryConverterImpl::convert(const TopoDS_Shape& shape,
                                         Gui::Render::GeometryData& output,
                                         const Gui::Render::ConversionOptions& options)
{
    output.clear();

    if (shape.IsNull()) {
        return false;
    }

    // Ensure tessellation
    if (!ensureTessellatedImpl(shape, options.deflection)) {
        return false;
    }

    // Extract face data
    if (!extractFaces(shape, output, options)) {
        // Continue to try extracting edges and vertices
    }

    // Extract edge data
    if (options.extractEdges) {
        extractEdges(shape, output, options);
    }

    // Extract vertex data
    if (options.extractVertices) {
        extractVertices(shape, output, options);
    }

    // Compute bounding box
    output.computeBoundingBox();

    return !output.isEmpty();
}

bool ShapeGeometryConverterImpl::extractFaces(const TopoDS_Shape& shape,
                                              Gui::Render::GeometryData& output,
                                              const Gui::Render::ConversionOptions& options)
{
    if (shape.IsNull()) {
        return false;
    }

    // Ensure tessellation
    if (!ensureTessellatedImpl(shape, options.deflection)) {
        return false;
    }

    // Estimate vertex count for pre-allocation
    size_t estimatedVertices = 0;
    size_t estimatedFaces = 0;
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        const TopoDS_Face& face = TopoDS::Face(exp.Current());
        TopLoc_Location loc;
        Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, loc);
        if (!triangulation.IsNull()) {
            estimatedVertices += triangulation->NbNodes();
            estimatedFaces += triangulation->NbTriangles();
        }
    }

    output.reserve(estimatedVertices, estimatedFaces);

    // Iterate over all faces
    for (TopExp_Explorer faceExp(shape, TopAbs_FACE); faceExp.More(); faceExp.Next()) {
        const TopoDS_Face& face = TopoDS::Face(faceExp.Current());
        TopLoc_Location loc;
        Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, loc);

        if (triangulation.IsNull()) {
            continue;
        }

        // Get transformation matrix
        gp_Trsf transform;
        bool hasTransform = false;
        if (!loc.IsIdentity()) {
            transform = loc.Transformation();
            hasTransform = true;
        }

        // Determine face orientation
        bool reversed = (face.Orientation() == TopAbs_REVERSED);

        // Starting vertex index for current face
        int32_t vertexOffset = static_cast<int32_t>(output.getVertexCount());

        // Add vertices - using OCCT 7.x API
        int nbNodes = triangulation->NbNodes();
        for (int i = 1; i <= nbNodes; ++i) {
            gp_Pnt p = triangulation->Node(i);
            if (hasTransform) {
                p.Transform(transform);
            }
            output.addVertex(static_cast<float>(p.X()),
                            static_cast<float>(p.Y()),
                            static_cast<float>(p.Z()));
        }

        // Add normals (if requested) - using OCCT 7.x API
        if (options.computeVertexNormals && triangulation->HasNormals()) {
            for (int i = 1; i <= nbNodes; ++i) {
                gp_Dir normal = triangulation->Normal(i);
                float nx = static_cast<float>(normal.X());
                float ny = static_cast<float>(normal.Y());
                float nz = static_cast<float>(normal.Z());
                if (hasTransform) {
                    gp_Vec v(nx, ny, nz);
                    v.Transform(transform);
                    if (v.Magnitude() > Precision::Confusion()) {
                        v.Normalize();
                    }
                    nx = static_cast<float>(v.X());
                    ny = static_cast<float>(v.Y());
                    nz = static_cast<float>(v.Z());
                }
                if (reversed) {
                    nx = -nx;
                    ny = -ny;
                    nz = -nz;
                }
                output.addNormal(nx, ny, nz);
            }
            output.normalBinding = Gui::Render::GeometryData::NormalBinding::PerVertex;
        }

        // Add triangles
        const Poly_Array1OfTriangle& triangles = triangulation->Triangles();
        for (int i = triangles.Lower(); i <= triangles.Upper(); ++i) {
            const Poly_Triangle& tri = triangles(i);
            int n1, n2, n3;
            tri.Get(n1, n2, n3);

            // Adjust indices (OCC is 1-based)
            n1 = vertexOffset + n1 - 1;
            n2 = vertexOffset + n2 - 1;
            n3 = vertexOffset + n3 - 1;

            // Adjust triangle winding based on face orientation
            if (reversed) {
                output.addTriangle(n1, n3, n2);
            } else {
                output.addTriangle(n1, n2, n3);
            }
        }
    }

    return output.hasFaces();
}

bool ShapeGeometryConverterImpl::extractEdges(const TopoDS_Shape& shape,
                                              Gui::Render::GeometryData& output,
                                              const Gui::Render::ConversionOptions& options)
{
    if (shape.IsNull()) {
        return false;
    }

    // Iterate over all edges
    for (TopExp_Explorer edgeExp(shape, TopAbs_EDGE); edgeExp.More(); edgeExp.Next()) {
        const TopoDS_Edge& edge = TopoDS::Edge(edgeExp.Current());

        if (BRep_Tool::Degenerated(edge)) {
            continue;
        }

        // Try to get polygon from triangulation
        TopLoc_Location loc;
        Handle(Poly_Polygon3D) polygon = BRep_Tool::Polygon3D(edge, loc);

        if (!polygon.IsNull()) {
            // Use existing polygon - using OCCT 7.x API
            gp_Trsf transform;
            bool hasTransform = !loc.IsIdentity();
            if (hasTransform) {
                transform = loc.Transformation();
            }

            int nbNodes = polygon->NbNodes();
            int32_t prevIndex = -1;

            for (int i = 1; i <= nbNodes; ++i) {
                gp_Pnt p = polygon->Nodes()(i);
                if (hasTransform) {
                    p.Transform(transform);
                }

                int32_t index = output.addVertex(
                    static_cast<float>(p.X()),
                    static_cast<float>(p.Y()),
                    static_cast<float>(p.Z()));

                if (prevIndex >= 0) {
                    output.addLine(prevIndex, index);
                }
                prevIndex = index;
            }
        } else {
            // Discretize curve
            try {
                BRepAdaptor_Curve curve(edge);
                GCPnts_TangentialDeflection discretizer(
                    curve, options.deflection, 0.1);

                if (discretizer.NbPoints() >= 2) {
                    int32_t prevIndex = -1;
                    for (int i = 1; i <= discretizer.NbPoints(); ++i) {
                        gp_Pnt p = discretizer.Value(i);
                        int32_t index = output.addVertex(
                            static_cast<float>(p.X()),
                            static_cast<float>(p.Y()),
                            static_cast<float>(p.Z()));

                        if (prevIndex >= 0) {
                            output.addLine(prevIndex, index);
                        }
                        prevIndex = index;
                    }
                }
            }
            catch (const Standard_Failure&) {
                // Skip edges that can't be discretized
                continue;
            }
        }
    }

    return output.hasLines();
}

bool ShapeGeometryConverterImpl::extractVertices(const TopoDS_Shape& shape,
                                                 Gui::Render::GeometryData& output,
                                                 const Gui::Render::ConversionOptions& /*options*/)
{
    if (shape.IsNull()) {
        return false;
    }

    // Iterate over all vertices
    for (TopExp_Explorer vertExp(shape, TopAbs_VERTEX); vertExp.More(); vertExp.Next()) {
        const TopoDS_Vertex& vertex = TopoDS::Vertex(vertExp.Current());
        gp_Pnt p = BRep_Tool::Pnt(vertex);

        int32_t index = output.addVertex(
            static_cast<float>(p.X()),
            static_cast<float>(p.Y()),
            static_cast<float>(p.Z()));

        output.pointIndices.push_back(index);
    }

    return output.hasPoints();
}

} // namespace PartGui
