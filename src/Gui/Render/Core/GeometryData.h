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

#ifndef GUI_RENDER_GEOMETRYDATA_H
#define GUI_RENDER_GEOMETRYDATA_H

#include <vector>
#include <cstdint>
#include <memory>

#include <FCGlobal.h>
#include "RenderTypes.h"

namespace Gui {
namespace Render {

/**
 * @brief 抽象几何数据结构 / Abstract geometry data structure
 *
 * 后端无关的几何数据表示，用于在 FreeCAD 几何对象和渲染节点之间传递数据。
 * Backend-agnostic geometry data representation for transferring data
 * between FreeCAD geometry objects and render nodes.
 *
 * 支持的几何类型：
 * Supported geometry types:
 * - 三角形网格（面）/ Triangle meshes (faces)
 * - 线段集合（边）/ Line segments (edges)
 * - 点集（顶点）/ Point sets (vertices)
 *
 * 数据布局：
 * Data layout:
 * - vertices: [x0, y0, z0, x1, y1, z1, ...] (每3个float一个顶点)
 * - normals: [nx0, ny0, nz0, nx1, ny1, nz1, ...] (每3个float一个法线)
 * - colors: [r0, g0, b0, a0, r1, g1, b1, a1, ...] (每4个float一个颜色)
 * - texCoords: [u0, v0, u1, v1, ...] (每2个float一个纹理坐标)
 * - faceIndices: [i0, i1, i2, -1, i3, i4, i5, -1, ...] (-1作为面分隔符)
 * - lineIndices: [i0, i1, -1, i2, i3, -1, ...] (-1作为线分隔符)
 */
struct GuiExport GeometryData {
    //=========================================================================
    // 顶点数据 / Vertex Data
    //=========================================================================

    /// 顶点坐标 [x, y, z, x, y, z, ...] / Vertex coordinates
    std::vector<float> vertices;

    /// 法线向量 [nx, ny, nz, nx, ny, nz, ...] / Normal vectors
    std::vector<float> normals;

    /// 顶点颜色 [r, g, b, a, r, g, b, a, ...] / Vertex colors
    std::vector<float> colors;

    /// 纹理坐标 [u, v, u, v, ...] / Texture coordinates
    std::vector<float> texCoords;

    //=========================================================================
    // 索引数据 / Index Data
    //=========================================================================

    /// 面索引，-1 作为面分隔符 / Face indices, -1 as face separator
    /// 例如：[0, 1, 2, -1, 2, 3, 0, -1] 表示两个三角形
    /// Example: [0, 1, 2, -1, 2, 3, 0, -1] represents two triangles
    std::vector<int32_t> faceIndices;

    /// 线索引，-1 作为线分隔符 / Line indices, -1 as line separator
    /// 例如：[0, 1, -1, 2, 3, -1] 表示两条线段
    /// Example: [0, 1, -1, 2, 3, -1] represents two line segments
    std::vector<int32_t> lineIndices;

    /// 点索引（可选，如果为空则使用所有顶点）/ Point indices (optional)
    std::vector<int32_t> pointIndices;

    //=========================================================================
    // 元数据 / Metadata
    //=========================================================================

    /// 法线绑定方式 / Normal binding mode
    enum class NormalBinding {
        None,           ///< 无法线 / No normals
        PerVertex,      ///< 每顶点法线 / Per-vertex normals
        PerFace,        ///< 每面法线 / Per-face normals
        PerFaceIndexed  ///< 每面索引法线 / Per-face indexed normals
    };
    NormalBinding normalBinding{NormalBinding::None};

    /// 颜色绑定方式 / Color binding mode
    enum class ColorBinding {
        None,           ///< 无颜色 / No colors
        Overall,        ///< 整体颜色 / Overall color
        PerVertex,      ///< 每顶点颜色 / Per-vertex colors
        PerFace,        ///< 每面颜色 / Per-face colors
        PerFaceIndexed  ///< 每面索引颜色 / Per-face indexed colors
    };
    ColorBinding colorBinding{ColorBinding::None};

    /// 是否有纹理坐标 / Has texture coordinates
    bool hasTexCoords{false};

    /// 边界框 / Bounding box
    BoundingBox boundingBox;

    //=========================================================================
    // 辅助方法 / Helper Methods
    //=========================================================================

    /**
     * @brief 获取顶点数量 / Get vertex count
     */
    size_t getVertexCount() const {
        return vertices.size() / 3;
    }

    /**
     * @brief 获取面数量 / Get face count
     *
     * 通过统计 -1 分隔符来计算面数
     * Counts faces by counting -1 separators
     */
    size_t getFaceCount() const {
        size_t count = 0;
        for (int32_t idx : faceIndices) {
            if (idx == -1) ++count;
        }
        return count;
    }

    /**
     * @brief 获取线段数量 / Get line count
     */
    size_t getLineCount() const {
        size_t count = 0;
        for (int32_t idx : lineIndices) {
            if (idx == -1) ++count;
        }
        return count;
    }

    /**
     * @brief 清空所有数据 / Clear all data
     */
    void clear() {
        vertices.clear();
        normals.clear();
        colors.clear();
        texCoords.clear();
        faceIndices.clear();
        lineIndices.clear();
        pointIndices.clear();
        normalBinding = NormalBinding::None;
        colorBinding = ColorBinding::None;
        hasTexCoords = false;
        boundingBox = BoundingBox();
    }

    /**
     * @brief 检查是否为空 / Check if empty
     */
    bool isEmpty() const {
        return vertices.empty();
    }

    /**
     * @brief 检查是否有面数据 / Check if has face data
     */
    bool hasFaces() const {
        return !faceIndices.empty();
    }

    /**
     * @brief 检查是否有线数据 / Check if has line data
     */
    bool hasLines() const {
        return !lineIndices.empty();
    }

    /**
     * @brief 检查是否有点数据 / Check if has point data
     */
    bool hasPoints() const {
        return !vertices.empty();
    }

    /**
     * @brief 预分配内存 / Reserve memory
     *
     * @param vertexCount 预期顶点数 / Expected vertex count
     * @param faceCount 预期面数 / Expected face count
     * @param lineCount 预期线数 / Expected line count
     */
    void reserve(size_t vertexCount, size_t faceCount = 0, size_t lineCount = 0) {
        vertices.reserve(vertexCount * 3);
        normals.reserve(vertexCount * 3);
        if (faceCount > 0) {
            // 假设平均每个面3个顶点 + 1个分隔符
            // Assume average 3 vertices per face + 1 separator
            faceIndices.reserve(faceCount * 4);
        }
        if (lineCount > 0) {
            // 假设平均每条线2个顶点 + 1个分隔符
            // Assume average 2 vertices per line + 1 separator
            lineIndices.reserve(lineCount * 3);
        }
    }

    /**
     * @brief 添加顶点 / Add vertex
     *
     * @return 新顶点的索引 / Index of new vertex
     */
    int32_t addVertex(float x, float y, float z) {
        int32_t index = static_cast<int32_t>(vertices.size() / 3);
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
        return index;
    }

    /**
     * @brief 添加法线 / Add normal
     */
    void addNormal(float nx, float ny, float nz) {
        normals.push_back(nx);
        normals.push_back(ny);
        normals.push_back(nz);
    }

    /**
     * @brief 添加颜色 / Add color
     */
    void addColor(float r, float g, float b, float a = 1.0f) {
        colors.push_back(r);
        colors.push_back(g);
        colors.push_back(b);
        colors.push_back(a);
    }

    /**
     * @brief 添加三角形面 / Add triangle face
     */
    void addTriangle(int32_t i0, int32_t i1, int32_t i2) {
        faceIndices.push_back(i0);
        faceIndices.push_back(i1);
        faceIndices.push_back(i2);
        faceIndices.push_back(-1);  // 面分隔符 / Face separator
    }

    /**
     * @brief 添加线段 / Add line segment
     */
    void addLine(int32_t i0, int32_t i1) {
        lineIndices.push_back(i0);
        lineIndices.push_back(i1);
        lineIndices.push_back(-1);  // 线分隔符 / Line separator
    }

    /**
     * @brief 计算边界框 / Compute bounding box
     */
    void computeBoundingBox() {
        if (vertices.empty()) {
            boundingBox = BoundingBox();
            return;
        }

        float minX = vertices[0], maxX = vertices[0];
        float minY = vertices[1], maxY = vertices[1];
        float minZ = vertices[2], maxZ = vertices[2];

        for (size_t i = 0; i < vertices.size(); i += 3) {
            float x = vertices[i];
            float y = vertices[i + 1];
            float z = vertices[i + 2];

            if (x < minX) minX = x;
            if (x > maxX) maxX = x;
            if (y < minY) minY = y;
            if (y > maxY) maxY = y;
            if (z < minZ) minZ = z;
            if (z > maxZ) maxZ = z;
        }

        // Base::BoundBox3d uses MinX/MaxX/MinY/MaxY/MinZ/MaxZ members
        boundingBox.MinX = static_cast<double>(minX);
        boundingBox.MinY = static_cast<double>(minY);
        boundingBox.MinZ = static_cast<double>(minZ);
        boundingBox.MaxX = static_cast<double>(maxX);
        boundingBox.MaxY = static_cast<double>(maxY);
        boundingBox.MaxZ = static_cast<double>(maxZ);
    }
};

/**
 * @brief 几何数据智能指针 / Geometry data smart pointer
 */
using GeometryDataPtr = std::shared_ptr<GeometryData>;

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_GEOMETRYDATA_H
