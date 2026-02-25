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

#ifndef GUI_RENDER_GEOMETRYCONVERTER_H
#define GUI_RENDER_GEOMETRYCONVERTER_H

#include <memory>

#include <FCGlobal.h>
#include "GeometryData.h"

// 前向声明 / Forward declarations
class TopoDS_Shape;

namespace Mesh {
    class MeshObject;
}

namespace Points {
    class PointKernel;
}

namespace Gui {
namespace Render {

/**
 * @brief 几何转换选项 / Geometry conversion options
 */
struct GuiExport ConversionOptions {
    /// 三角化偏差（用于 BREP 形状）/ Tessellation deflection (for BREP shapes)
    double deflection{0.1};

    /// 是否计算顶点法线 / Whether to compute vertex normals
    bool computeVertexNormals{true};

    /// 是否提取边线（用于线框模式）/ Whether to extract edges (for wireframe mode)
    bool extractEdges{true};

    /// 是否提取顶点（用于点模式）/ Whether to extract vertices (for point mode)
    bool extractVertices{true};

    /// 是否合并共享顶点 / Whether to merge shared vertices
    bool mergeVertices{false};

    /// 法线平滑角度阈值（弧度）/ Normal smoothing angle threshold (radians)
    /// 超过此角度的边将使用面法线
    /// Edges exceeding this angle will use face normals
    double smoothAngle{0.5};  // ~28.6 degrees

    /// 最小边长（用于过滤短边）/ Minimum edge length (for filtering short edges)
    double minEdgeLength{0.0};
};

/**
 * @brief 几何转换器接口 / Geometry converter interface
 *
 * 定义将 FreeCAD 几何对象转换为 GeometryData 的接口。
 * Defines the interface for converting FreeCAD geometry objects to GeometryData.
 *
 * 实现类：
 * Implementations:
 * - ShapeGeometryConverter - 转换 TopoDS_Shape
 * - MeshGeometryConverter - 转换 Mesh::MeshObject
 * - PointsGeometryConverter - 转换 Points::PointKernel
 */
class GuiExport IGeometryConverter {
public:
    virtual ~IGeometryConverter() = default;

    /**
     * @brief 获取转换器名称 / Get converter name
     */
    virtual std::string getName() const = 0;

    /**
     * @brief 检查是否可以转换给定类型 / Check if can convert given type
     */
    virtual bool canConvert(const std::string& typeName) const = 0;
};

/**
 * @brief TopoDS_Shape 几何转换器 / TopoDS_Shape geometry converter
 *
 * 将 OpenCASCADE TopoDS_Shape 转换为 GeometryData。
 * Converts OpenCASCADE TopoDS_Shape to GeometryData.
 */
class GuiExport ShapeGeometryConverter : public IGeometryConverter {
public:
    ShapeGeometryConverter() = default;
    ~ShapeGeometryConverter() override = default;

    std::string getName() const override { return "ShapeGeometryConverter"; }
    bool canConvert(const std::string& typeName) const override;

    /**
     * @brief 转换 TopoDS_Shape 为 GeometryData / Convert TopoDS_Shape to GeometryData
     *
     * @param shape 输入形状 / Input shape
     * @param output 输出几何数据 / Output geometry data
     * @param options 转换选项 / Conversion options
     * @return true 如果转换成功 / true if conversion succeeded
     */
    bool convert(const TopoDS_Shape& shape,
                 GeometryData& output,
                 const ConversionOptions& options = ConversionOptions());

    /**
     * @brief 仅提取面数据（用于着色模式）/ Extract faces only (for shaded mode)
     */
    bool extractFaces(const TopoDS_Shape& shape,
                      GeometryData& output,
                      const ConversionOptions& options = ConversionOptions());

    /**
     * @brief 仅提取边线数据（用于线框模式）/ Extract edges only (for wireframe mode)
     */
    bool extractEdges(const TopoDS_Shape& shape,
                      GeometryData& output,
                      const ConversionOptions& options = ConversionOptions());

    /**
     * @brief 仅提取顶点数据（用于点模式）/ Extract vertices only (for point mode)
     */
    bool extractVertices(const TopoDS_Shape& shape,
                         GeometryData& output,
                         const ConversionOptions& options = ConversionOptions());

private:
    /**
     * @brief 确保形状已三角化 / Ensure shape is tessellated
     */
    bool ensureTessellated(const TopoDS_Shape& shape, double deflection);
};

/**
 * @brief Mesh 几何转换器 / Mesh geometry converter
 *
 * 将 Mesh::MeshObject 转换为 GeometryData。
 * Converts Mesh::MeshObject to GeometryData.
 */
class GuiExport MeshGeometryConverter : public IGeometryConverter {
public:
    MeshGeometryConverter() = default;
    ~MeshGeometryConverter() override = default;

    std::string getName() const override { return "MeshGeometryConverter"; }
    bool canConvert(const std::string& typeName) const override;

    /**
     * @brief 转换 MeshObject 为 GeometryData / Convert MeshObject to GeometryData
     *
     * @param mesh 输入网格 / Input mesh
     * @param output 输出几何数据 / Output geometry data
     * @param options 转换选项 / Conversion options
     * @return true 如果转换成功 / true if conversion succeeded
     */
    bool convert(const Mesh::MeshObject& mesh,
                 GeometryData& output,
                 const ConversionOptions& options = ConversionOptions());

    /**
     * @brief 仅提取面数据 / Extract faces only
     */
    bool extractFaces(const Mesh::MeshObject& mesh,
                      GeometryData& output,
                      const ConversionOptions& options = ConversionOptions());

    /**
     * @brief 仅提取边线数据（网格边界边）/ Extract edges only (mesh boundary edges)
     */
    bool extractEdges(const Mesh::MeshObject& mesh,
                      GeometryData& output,
                      const ConversionOptions& options = ConversionOptions());
};

/**
 * @brief 点云几何转换器 / Point cloud geometry converter
 *
 * 将 Points::PointKernel 转换为 GeometryData。
 * Converts Points::PointKernel to GeometryData.
 */
class GuiExport PointsGeometryConverter : public IGeometryConverter {
public:
    PointsGeometryConverter() = default;
    ~PointsGeometryConverter() override = default;

    std::string getName() const override { return "PointsGeometryConverter"; }
    bool canConvert(const std::string& typeName) const override;

    /**
     * @brief 转换 PointKernel 为 GeometryData / Convert PointKernel to GeometryData
     *
     * @param points 输入点云 / Input point cloud
     * @param output 输出几何数据 / Output geometry data
     * @param options 转换选项 / Conversion options
     * @return true 如果转换成功 / true if conversion succeeded
     */
    bool convert(const Points::PointKernel& points,
                 GeometryData& output,
                 const ConversionOptions& options = ConversionOptions());
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_GEOMETRYCONVERTER_H
