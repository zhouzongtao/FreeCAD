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

#ifndef GUI_RENDER_UTILS_GEOMETRYCONVERTER_H
#define GUI_RENDER_UTILS_GEOMETRYCONVERTER_H

#include <vector>
#include <memory>

#include <FCGlobal.h>
#include "../Core/RenderTypes.h"
#include "../Core/RenderNode.h"

// 前向声明 FreeCAD 几何类型 / Forward declarations for FreeCAD geometry types
namespace Mesh {
class MeshObject;
}

namespace Part {
class TopoShape;
}

namespace Data {
struct ComplexGeoData;
}

namespace Gui {
namespace Render {
namespace Utils {

/**
 * @brief 几何数据转换工具 / Geometry data conversion utilities
 *
 * 在 FreeCAD 几何数据与渲染节点之间进行转换。
 * Converts between FreeCAD geometry data and render nodes.
 *
 * 支持的几何类型 / Supported Geometry Types:
 * - Mesh（多边形网格）
 * - Part（BREP 几何）
 * - Points（点云）
 * - Lines（线框）
 */
class GuiExport GeometryConverter {
public:
    //-----------------------------------------------------------------------
    // Mesh 转换 / Mesh Conversion
    //-----------------------------------------------------------------------

    /**
     * @brief 从 MeshObject 创建渲染节点 / Create render node from MeshObject
     *
     * @param mesh Mesh 对象 / Mesh object
     * @param smoothing 是否平滑法线 / Whether to smooth normals
     * @return 渲染节点 / Render node
     */
    static RenderNode::Ptr fromMeshObject(
        const Mesh::MeshObject& mesh,
        bool smoothing = true
    );

    /**
     * @brief 从顶点和面创建网格节点 / Create mesh node from vertices and faces
     *
     * @param vertices 顶点坐标 / Vertex coordinates
     * @param faces 面索引（每个面是一组顶点索引）/ Face indices
     * @param normals 可选的法线数组 / Optional normals array
     * @return 渲染节点 / Render node
     */
    static RenderNode::Ptr fromMeshData(
        const std::vector<Vec3f>& vertices,
        const std::vector<std::vector<int32_t>>& faces,
        const std::vector<Vec3f>* normals = nullptr
    );

    /**
     * @brief 创建线框网格 / Create wireframe mesh
     */
    static RenderNode::Ptr createWireframe(
        const std::vector<Vec3f>& vertices,
        const std::vector<std::pair<int32_t, int32_t>>& edges
    );

    //-----------------------------------------------------------------------
    // Part/BREP 转换 / Part/BREP Conversion
    //-----------------------------------------------------------------------

    /**
     * @brief 从 TopoShape 创建渲染节点 / Create render node from TopoShape
     *
     * @param shape TopoShape 对象 / TopoShape object
     * @param quality 细分质量 / Tessellation quality
     * @return 渲染节点 / Render node
     */
    static RenderNode::Ptr fromTopoShape(
        const Part::TopoShape& shape,
        float quality = 0.5f
    );

    /**
     * @brief 从多个 TopoShape 创建组合节点 / Create combined node from multiple TopoShapes
     */
    static RenderGroup::Ptr fromTopoShapes(
        const std::vector<Part::TopoShape>& shapes,
        float quality = 0.5f
    );

    //-----------------------------------------------------------------------
    // 点云转换 / Point Cloud Conversion
    //-----------------------------------------------------------------------

    /**
     * @brief 创建点云节点 / Create point cloud node
     *
     * @param points 点坐标 / Point coordinates
     * @param colors 可选的点颜色 / Optional point colors
     * @return 渲染节点 / Render node
     */
    static RenderNode::Ptr createPointCloud(
        const std::vector<Vec3f>& points,
        const std::vector<Color>* colors = nullptr
    );

    //-----------------------------------------------------------------------
    // 几何体原语 / Geometry Primitives
    //-----------------------------------------------------------------------

    /**
     * @brief 创建盒子 / Create box
     */
    static RenderNode::Ptr createBox(
        const Vec3f& size,
        const Vec3f& center = Vec3f()
    );

    /**
     * @brief 创建球体 / Create sphere
     */
    static RenderNode::Ptr createSphere(
        float radius,
        const Vec3f& center = Vec3f(),
        int segments = 32
    );

    /**
     * @brief 创建圆柱 / Create cylinder
     */
    static RenderNode::Ptr createCylinder(
        float radius,
        float height,
        int segments = 32
    );

    /**
     * @brief 创建圆锥 / Create cone
     */
    static RenderNode::Ptr createCone(
        float radius,
        float height,
        int segments = 32
    );

    /**
     * @brief 创建圆环 / Create torus
     */
    static RenderNode::Ptr createTorus(
        float majorRadius,
        float minorRadius,
        int majorSegments = 32,
        int minorSegments = 16
    );

    //-----------------------------------------------------------------------
    // 法线计算 / Normal Calculation
    //-----------------------------------------------------------------------

    /**
     * @brief 计算面法线 / Calculate face normals
     */
    static std::vector<Vec3f> calculateFaceNormals(
        const std::vector<Vec3f>& vertices,
        const std::vector<std::vector<int32_t>>& faces
    );

    /**
     * @brief 计算顶点法线（平均）/ Calculate vertex normals (averaged)
     */
    static std::vector<Vec3f> calculateVertexNormals(
        const std::vector<Vec3f>& vertices,
        const std::vector<std::vector<int32_t>>& faces
    );

    /**
     * @brief 计算平滑法线 / Calculate smooth normals
     *
     * 根据面之间的角度决定是否平滑。
     * Determines smoothing based on angle between faces.
     *
     * @param maxSmoothAngle 最大平滑角度（度）/ Maximum smoothing angle (degrees)
     */
    static std::vector<Vec3f> calculateSmoothNormals(
        const std::vector<Vec3f>& vertices,
        const std::vector<std::vector<int32_t>>& faces,
        float maxSmoothAngle = 30.0f
    );

    //-----------------------------------------------------------------------
    // 网格简化 / Mesh Simplification
    //-----------------------------------------------------------------------

    /**
     * @brief 简化网格 / Simplify mesh
     *
     * 使用边坍缩算法减少网格顶点数。
     * Uses edge collapse to reduce vertex count.
     *
     * @param vertices 输入顶点 / Input vertices
     * @param faces 输入面 / Input faces
     * @param targetReduction 目标减少比例 (0-1) / Target reduction ratio (0-1)
     * @return 简化后的网格 / Simplified mesh
     */
    static std::pair<std::vector<Vec3f>, std::vector<std::vector<int32_t>>>
    simplifyMesh(
        const std::vector<Vec3f>& vertices,
        const std::vector<std::vector<int32_t>>& faces,
        float targetReduction = 0.5f
    );

    //-----------------------------------------------------------------------
    // 网格修复 / Mesh Repair
    //-----------------------------------------------------------------------

    /**
     * @brief 修复网格法线 / Fix mesh normals
     *
     * 确保所有法线指向外部。
     * Ensure all normals point outward.
     */
    static void fixNormals(
        std::vector<Vec3f>& vertices,
        std::vector<std::vector<int32_t>>& faces,
        std::vector<Vec3f>& normals
    );

    /**
     * @brief 移除重复顶点 / Remove duplicate vertices
     */
    static void removeDuplicateVertices(
        std::vector<Vec3f>& vertices,
        std::vector<std::vector<int32_t>>& faces,
        float tolerance = 1e-6f
    );

    /**
     * @brief 移除退化面 / Remove degenerate faces
     *
     * 移除面积为零或非常小的面。
     * Remove faces with zero or very small area.
     */
    static void removeDegenerateFaces(
        std::vector<std::vector<int32_t>>& faces,
        float minArea = 1e-10f
    );
};

} // namespace Utils
} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_UTILS_GEOMETRYCONVERTER_H
