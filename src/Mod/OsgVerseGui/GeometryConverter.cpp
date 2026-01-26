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

#include "PreCompiled.h"

#include "GeometryConverter.h"
#include <Base/Console.h>

// OCCT includes
#include <BRepMesh_IncrementalMesh.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopLoc_Location.hxx>
#include <Poly_Triangulation.hxx>
#include <BRep_Tool.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <Poly_Triangle.hxx>
#include <BRepGProp_Face.hxx>
#include <BRepLProp_SLProps.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <Geom_Surface.hxx>
#include <ShapeAnalysis.hxx>

// OSG includes
#include <osg/Array>
#include <osg/PrimitiveSet>

using namespace OsgVerseGui;

//===========================================================================
// 日志宏
//===========================================================================

#define GEOM_LOG_INFO(msg, ...) \
    Base::Console().log("[GeometryConverter] " msg "\n", ##__VA_ARGS__)

#define GEOM_LOG_ERROR(msg, ...) \
    Base::Console().error("[GeometryConverter] " msg "\n", ##__VA_ARGS__)

#ifdef _DEBUG
    #define GEOM_LOG_DEBUG(msg, ...) \
        Base::Console().log("[GeometryConverter] " msg "\n", ##__VA_ARGS__)
#else
    #define GEOM_LOG_DEBUG(msg, ...)
#endif

//===========================================================================
// GeometryConverter Implementation
//===========================================================================

osg::ref_ptr<osg::Geode> GeometryConverter::convertShape(
    const TopoDS_Shape& shape,
    const ConversionOptions& options,
    ConversionStats* stats
)
{
    if (shape.IsNull()) {
        GEOM_LOG_ERROR("Cannot convert null shape");
        return nullptr;
    }
    
    GEOM_LOG_DEBUG("Converting shape (deflection=%.3f, angle=%.3f)", 
                   options.deflection, options.angle);
    
    // 转换为 Geometry
    osg::ref_ptr<osg::Geometry> geometry = convertToGeometry(shape, options, stats);
    
    if (!geometry) {
        GEOM_LOG_ERROR("Failed to convert shape to geometry");
        return nullptr;
    }
    
    // 创建 Geode 并添加 Geometry
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable(geometry.get());
    
    GEOM_LOG_DEBUG("Shape converted successfully");
    
    return geode;
}

osg::ref_ptr<osg::Geometry> GeometryConverter::convertToGeometry(
    const TopoDS_Shape& shape,
    const ConversionOptions& options,
    ConversionStats* stats
)
{
    // Step 1: 三角剖分
    if (!tessellateShape(shape, options.deflection, options.angle, options.relative)) {
        GEOM_LOG_ERROR("Tessellation failed");
        return nullptr;
    }

    // Step 2: 提取几何数据（使用选项）
    std::vector<osg::Vec3> vertices;
    std::vector<osg::Vec3> normals;
    std::vector<unsigned int> indices;
    int faceCount = 0;

    if (!extractGeometryDataWithOptions(shape, vertices, normals, indices, faceCount, options)) {
        GEOM_LOG_ERROR("Failed to extract geometry data");
        return nullptr;
    }
    
    if (vertices.empty() || indices.empty()) {
        GEOM_LOG_ERROR("No geometry data extracted");
        return nullptr;
    }
    
    // Step 3: 创建 OSG Geometry
    osg::ref_ptr<osg::Geometry> geometry = createOsgGeometry(vertices, normals, indices);
    
    if (!geometry) {
        GEOM_LOG_ERROR("Failed to create OSG geometry");
        return nullptr;
    }
    
    // 统计信息
    if (stats) {
        stats->vertexCount = vertices.size();
        stats->triangleCount = indices.size() / 3;
        stats->faceCount = faceCount;
        stats->conversionTime = 0.0;  // TODO: Add timing if needed
    }
    
    GEOM_LOG_INFO("Converted: %d vertices, %d triangles, %d faces",
                  static_cast<int>(vertices.size()),
                  static_cast<int>(indices.size() / 3),
                  faceCount);
    
    return geometry;
}

bool GeometryConverter::tessellateShape(
    const TopoDS_Shape& shape,
    double deflection,
    double angle,
    bool relative
)
{
    try {
        // 使用 BRepMesh_IncrementalMesh 进行三角剖分
        BRepMesh_IncrementalMesh mesh(shape, deflection, relative, angle);
        
        if (!mesh.IsDone()) {
            GEOM_LOG_ERROR("Mesh generation failed");
            return false;
        }
        
        GEOM_LOG_DEBUG("Tessellation completed");
        return true;
    }
    catch (const Standard_Failure& e) {
        GEOM_LOG_ERROR("Tessellation exception: %s", e.GetMessageString());
        return false;
    }
    catch (...) {
        GEOM_LOG_ERROR("Unknown tessellation exception");
        return false;
    }
}

/**
 * @brief 计算表面上某点的法线
 *
 * @param face 面
 * @param u U 参数
 * @param v V 参数
 * @param transform 变换矩阵
 * @param reversed 是否反转
 * @return 法线向量
 */
static osg::Vec3 computeSurfaceNormalAtUV(
    const TopoDS_Face& face,
    double u, double v,
    const gp_Trsf& transform,
    bool reversed
)
{
    try {
        BRepAdaptor_Surface surface(face);
        BRepLProp_SLProps props(surface, u, v, 1, 1e-7);

        if (props.IsNormalDefined()) {
            gp_Dir dir = props.Normal();
            gp_Vec normal(dir);

            // 应用变换（只变换方向，不变换位置）
            normal.Transform(transform);

            // 根据面方向调整法线
            if (reversed) {
                normal.Reverse();
            }

            if (normal.Magnitude() > 1e-7) {
                normal.Normalize();
                return osg::Vec3(normal.X(), normal.Y(), normal.Z());
            }
        }
    }
    catch (...) {
        // 计算失败，返回默认法线
    }

    return osg::Vec3(0.0f, 0.0f, 1.0f);  // 默认法线
}

bool GeometryConverter::extractGeometryData(
    const TopoDS_Shape& shape,
    std::vector<osg::Vec3>& vertices,
    std::vector<osg::Vec3>& normals,
    std::vector<unsigned int>& indices,
    int& faceCount
)
{
    // 使用默认选项调用
    ConversionOptions defaultOpts;
    return extractGeometryDataWithOptions(shape, vertices, normals, indices, faceCount, defaultOpts);
}

bool GeometryConverter::extractGeometryDataWithOptions(
    const TopoDS_Shape& shape,
    std::vector<osg::Vec3>& vertices,
    std::vector<osg::Vec3>& normals,
    std::vector<unsigned int>& indices,
    int& faceCount,
    const ConversionOptions& options
)
{
    faceCount = 0;
    unsigned int vertexOffset = 0;

    try {
        // 遍历所有 Face
        TopExp_Explorer explorer(shape, TopAbs_FACE);

        while (explorer.More()) {
            const TopoDS_Face& face = TopoDS::Face(explorer.Current());

            if (!extractFaceTrianglesWithOptions(face, vertices, normals, indices, vertexOffset, options)) {
                GEOM_LOG_ERROR("Failed to extract face triangles");
                // 继续处理其他 Face
            }
            else {
                faceCount++;
            }

            explorer.Next();
        }

        if (faceCount == 0) {
            GEOM_LOG_ERROR("No faces found in shape");
            return false;
        }

        GEOM_LOG_DEBUG("Extracted %d faces", faceCount);
        return true;
    }
    catch (const Standard_Failure& e) {
        GEOM_LOG_ERROR("Extraction exception: %s", e.GetMessageString());
        return false;
    }
    catch (...) {
        GEOM_LOG_ERROR("Unknown extraction exception");
        return false;
    }
}

bool GeometryConverter::extractFaceTriangles(
    const TopoDS_Face& face,
    std::vector<osg::Vec3>& vertices,
    std::vector<osg::Vec3>& normals,
    std::vector<unsigned int>& indices,
    unsigned int& vertexOffset
)
{
    // 使用默认选项调用
    ConversionOptions defaultOpts;
    return extractFaceTrianglesWithOptions(face, vertices, normals, indices, vertexOffset, defaultOpts);
}

bool GeometryConverter::extractFaceTrianglesWithOptions(
    const TopoDS_Face& face,
    std::vector<osg::Vec3>& vertices,
    std::vector<osg::Vec3>& normals,
    std::vector<unsigned int>& indices,
    unsigned int& vertexOffset,
    const ConversionOptions& options
)
{
    // 获取 Face 的位置和三角剖分
    TopLoc_Location location;
    Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);

    if (triangulation.IsNull()) {
        GEOM_LOG_DEBUG("Face has no triangulation");
        return false;
    }

    // 获取变换矩阵
    gp_Trsf transform = location.Transformation();

    // 获取 Face 方向（用于法线）
    TopAbs_Orientation orientation = face.Orientation();
    bool reversed = (orientation == TopAbs_REVERSED);

    // 提取顶点 - OCCT 7.x API
    int nodeCount = triangulation->NbNodes();

    // 为这个 Face 的顶点预留空间
    size_t startVertex = vertices.size();
    vertices.reserve(startVertex + nodeCount);
    normals.reserve(startVertex + nodeCount);

    // 检查是否有 UV 参数用于精确法线计算
    bool hasUVParams = triangulation->HasUVNodes();

    // 添加顶点
    for (int i = 1; i <= nodeCount; i++) {
        gp_Pnt point = triangulation->Node(i).Transformed(transform);
        vertices.push_back(osg::Vec3(point.X(), point.Y(), point.Z()));
    }

    // 提取三角形 - OCCT 7.x API
    int triangleCount = triangulation->NbTriangles();

    indices.reserve(indices.size() + triangleCount * 3);

    for (int i = 1; i <= triangleCount; i++) {
        const Poly_Triangle& triangle = triangulation->Triangle(i);

        int n1, n2, n3;
        triangle.Get(n1, n2, n3);

        // 根据 Face 方向调整顶点顺序
        if (reversed) {
            indices.push_back(vertexOffset + n1 - 1);
            indices.push_back(vertexOffset + n3 - 1);
            indices.push_back(vertexOffset + n2 - 1);
        }
        else {
            indices.push_back(vertexOffset + n1 - 1);
            indices.push_back(vertexOffset + n2 - 1);
            indices.push_back(vertexOffset + n3 - 1);
        }
    }

    // 计算法线
    if (options.computeNormals) {
        if (options.perVertexNormals && hasUVParams) {
            // 每顶点精确法线：使用 UV 参数在曲面上计算法线
            for (int i = 1; i <= nodeCount; i++) {
                gp_Pnt2d uv = triangulation->UVNode(i);
                osg::Vec3 normal = computeSurfaceNormalAtUV(face, uv.X(), uv.Y(), transform, reversed);
                normals.push_back(normal);
            }
            GEOM_LOG_DEBUG("Computed %d per-vertex normals using UV parameters", nodeCount);
        }
        else {
            // 简单方法：使用 Face 的中心法线
            try {
                BRepGProp_Face faceProp(face);
                gp_Pnt center;
                gp_Vec normal;

                // 在 Face 中心计算法线
                faceProp.Normal(0.5, 0.5, center, normal);

                if (normal.Magnitude() > 1e-7) {
                    normal.Normalize();

                    // 应用变换
                    normal.Transform(transform);

                    // 根据方向调整法线
                    if (reversed) {
                        normal.Reverse();
                    }

                    osg::Vec3 osgNormal(normal.X(), normal.Y(), normal.Z());

                    // 为这个 Face 的所有顶点使用相同的法线
                    for (int i = 0; i < nodeCount; i++) {
                        normals.push_back(osgNormal);
                    }
                }
                else {
                    // 法线无效，使用默认法线
                    osg::Vec3 defaultNormal(0.0f, 0.0f, 1.0f);
                    for (int i = 0; i < nodeCount; i++) {
                        normals.push_back(defaultNormal);
                    }
                }
            }
            catch (...) {
                // 法线计算失败，使用默认法线
                osg::Vec3 defaultNormal(0.0f, 0.0f, 1.0f);
                for (int i = 0; i < nodeCount; i++) {
                    normals.push_back(defaultNormal);
                }
            }
        }
    }

    // 更新顶点偏移
    vertexOffset += nodeCount;

    return true;
}

osg::ref_ptr<osg::Geometry> GeometryConverter::createOsgGeometry(
    const std::vector<osg::Vec3>& vertices,
    const std::vector<osg::Vec3>& normals,
    const std::vector<unsigned int>& indices
)
{
    if (vertices.empty() || indices.empty()) {
        GEOM_LOG_ERROR("Cannot create geometry from empty data");
        return nullptr;
    }
    
    // 创建 Geometry
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    
    // 设置顶点数组
    osg::ref_ptr<osg::Vec3Array> vertexArray = new osg::Vec3Array(vertices.size());
    for (size_t i = 0; i < vertices.size(); i++) {
        (*vertexArray)[i] = vertices[i];
    }
    geometry->setVertexArray(vertexArray.get());
    
    // 设置法线数组
    if (!normals.empty() && normals.size() == vertices.size()) {
        osg::ref_ptr<osg::Vec3Array> normalArray = new osg::Vec3Array(normals.size());
        for (size_t i = 0; i < normals.size(); i++) {
            (*normalArray)[i] = normals[i];
        }
        geometry->setNormalArray(normalArray.get());
        geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    }
    
    // 设置索引数组
    osg::ref_ptr<osg::DrawElementsUInt> indexArray = 
        new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, indices.size());
    for (size_t i = 0; i < indices.size(); i++) {
        (*indexArray)[i] = indices[i];
    }
    geometry->addPrimitiveSet(indexArray.get());
    
    GEOM_LOG_DEBUG("Created OSG geometry: %d vertices, %d indices",
                   static_cast<int>(vertices.size()),
                   static_cast<int>(indices.size()));
    
    return geometry;
}
