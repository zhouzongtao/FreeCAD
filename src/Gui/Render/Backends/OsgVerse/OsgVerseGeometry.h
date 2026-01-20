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
 *   but WITHOUT ANY WARRANhout even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEGEOMETRY_H
#define GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEGEOMETRY_H

#include <memory>
#include <vector>

#include <osg/Geometry>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <osg/Geode>

#include <FCGlobal.h>
#include "../../Core/RenderTypes.h"
#include "OsgVerseNode.h"

namespace Gui {
namespace Render {

/**
 * @brief OsgVerse 几何体系统 / OsgVerse geometry system
 *
 * 提供几何体数据管理，包括顶点、法线、纹理坐标、颜色和索引。
 * Provides geometry data management including vertices, normals, texcoords, colors and indices.
 *
 * 设计原则 / Design Principles:
 * 1. 高效的顶点数据管理
 * 2. 支持多种图元类型
 * 3. 兼容 Coin3D 几何体
 * 4. 支持几何体优化
 */
class GuiExport OsgVerseGeometry : public OsgVerseNode {
public:
    /**
     * @brief 图元类型 / Primitive type
     */
    enum class PrimitiveType {
        Points,          ///< 点 / Points
        Lines,           ///< 线段 / Lines
        LineStrip,       ///< 线条 / Line strip
        LineLoop,        ///< 闭合线条 / Line loop
        Triangles,       ///< 三角形 / Triangles
        TriangleStrip,   ///< 三角形带 / Triangle strip
        TriangleFan,     ///< 三角形扇 / Triangle fan
        Quads,           ///< 四边形 / Quads
        QuadStrip,       ///< 四边形带 / Quad strip
        Polygon          ///< 多边形 / Polygon
    };

    /**
     * @brief 构造函数 / Constructor
     */
    OsgVerseGeometry();

    /**
     * @brief 从现有 osg::Geometry 创建 / Create from existing osg::Geometry
     */
    explicit OsgVerseGeometry(osg::Geometry* geometry, bool ownsNode = false);

    /**
     * @brief 析构函数 / Destructor
     */
    ~OsgVerseGeometry() override;

    /**
     * @brief 获取 osg::Geometry 指针 / Get osg::Geometry pointer
     */
    osg::Geometry* getOsgGeometry() const;

    //-----------------------------------------------------------------------
    // 顶点数据 / Vertex Data
    //-----------------------------------------------------------------------

    /**
     * @brief 设置顶点位置 / Set vertex positions
     */
    void setVertices(const std::vector<Vec3f>& vertices);
    void setVertices(const float* data, size_t count);

    /**
     * @brief 获取顶点位置 / Get vertex positions
     */
    std::vector<Vec3f> getVertices() const;
    size_t getVertexCount() const;

    /**
     * @brief 设置法线 / Set normals
     */
    void setNormals(const std::vector<Vec3f>& normals);
    void setNormals(const float* data, size_t count);

    /**
     * @brief 获取法线 / Get normals
     */
    std::vector<Vec3f> getNormals() const;
    bool hasNormals() const;

    /**
     * @brief 设置纹理坐标 / Set texture coordinates
     *
     * @param unit 纹理单元 / Texture unit (0-7)
     */
    void setTexCoords(const std::vector<Vec2f>& texCoords, unsigned int unit = 0);
    void setTexCoords(const float* data, size_t count, unsigned int unit = 0);

    /**
     * @brief 获取纹理坐标 / Get texture coordinates
     */
    std::vector<Vec2f> getTexCoords(unsigned int unit = 0) const;
    bool hasTexCoords(unsigned int unit = 0) const;

    /**
     * @brief 设置顶点颜色 / Set vertex colors
     */
    void setColors(const std::vector<Color>& colors);
    void setColors(const float* data, size_t count);

    /**
     * @brief 获取顶点颜色 / Get vertex colors
     */
    std::vector<Color> getColors() const;
    bool hasColors() const;

    /**
     * @brief 设置切线 / Set tangents
     */
    void setTangents(const std::vector<Vec3f>& tangents);
    std::vector<Vec3f> getTangents() const;
    bool hasTangents() const;

    /**
     * @brief 设置副切线 / Set bitangents
     */
    void setBitangents(const std::vector<Vec3f>& bitangents);
    std::vector<Vec3f> getBitangents() const;
    bool hasBitangents() const;

    //-----------------------------------------------------------------------
    // 索引数据 / Index Data
    //-----------------------------------------------------------------------

    /**
     * @brief 设置索引 / Set indices
     */
    void setIndices(const std::vector<uint32_t>& indices);
    void setIndices(const uint32_t* data, size_t count);

    /**
     * @brief 获取索引 / Get indices
     */
    std::vector<uint32_t> getIndices() const;
    size_t getIndexCount() const;
    bool hasIndices() const;

    //-----------------------------------------------------------------------
    // 图元类型 / Primitive Type
    //-----------------------------------------------------------------------

    /**
     * @brief 设置图元类型 / Set primitive type
     */
    void setPrimitiveType(PrimitiveType type);
    PrimitiveType getPrimitiveType() const;

    /**
     * @brief 添加图元集 / Add primitive set
     *
     * 用于复杂几何体，支持多个图元集。
     * For complex geometry, supports multiple primitive sets.
     */
    void addPrimitiveSet(PrimitiveType type, unsigned int first, unsigned int count);

    /**
     * @brief 清除所有图元集 / Clear all primitive sets
     */
    void clearPrimitiveSets();

    //-----------------------------------------------------------------------
    // 几何体操作 / Geometry Operations
    //-----------------------------------------------------------------------

    /**
     * @brief 计算法线 / Compute normals
     *
     * 自动计算顶点法线（如果未提供）。
     * Automatically compute vertex normals (if not provided).
     */
    void computeNormals();

    /**
     * @brief 计算切线和副切线 / Compute tangents and bitangents
     *
     * 用于法线贴图。
     * For normal mapping.
     */
    void computeTangents();

    /**
     * @brief 计算边界框 / Compute bounding box
     */
    BoundingBox computeBoundingBox() const;

    /**
     * @brief 优化几何体 / Optimize geometry
     *
     * 移除重复顶点，优化索引。
     * Remove duplicate vertices, optimize indices.
     */
    void optimize();

    /**
     * @brief 反转法线 / Reverse normals
     */
    void reverseNormals();

    /**
     * @brief 反转面朝向 / Reverse winding order
     */
    void reverseWinding();

    //-----------------------------------------------------------------------
    // Coin3D 兼容性 / Coin3D Compatibility
    //-----------------------------------------------------------------------

    /**
     * @brief 从 Coin3D 几何体转换 / Convert from Coin3D geometry
     */
    void setFromCoin3DGeometry(const void* coinGeometry);

    //-----------------------------------------------------------------------
    // 工具方法 / Utility Methods
    //-----------------------------------------------------------------------

    /**
     * @brief 创建 Geode 节点 / Create Geode node
     *
     * 将几何体包装到 Geode 中以便添加到场景图。
     * Wrap geometry in Geode for adding to scene graph.
     */
    osg::Geode* createGeode() const;

    /**
     * @brief 克隆几何体 / Clone geometry
     */
    RenderNode::Ptr clone() const override;

    /**
     * @brief 清除所有数据 / Clear all data
     */
    void clear();

    /**
     * @brief 获取内存使用量 / Get memory usage
     */
    size_t getMemoryUsage() const;

private:
    /**
     * @brief 转换图元类型 / Convert primitive type
     */
    GLenum primitiveTypeToGL(PrimitiveType type) const;
    PrimitiveType glToPrimitiveType(GLenum mode) const;

    /**
     * @brief 更新几何体 / Update geometry
     */
    void updateGeometry();

    PrimitiveType _primitiveType{PrimitiveType::Triangles};
};

/**
 * @brief 几何体构建器 / Geometry builder
 *
 * 提供便捷的几何体构建方法。
 * Provides convenient geometry building methods.
 */
class GuiExport OsgVerseGeometryBuilder {
public:
    /**
     * @brief 创建立方体 / Create cube
     */
    static std::unique_ptr<OsgVerseGeometry> createCube(float size = 1.0f);

    /**
     * @brief 创建球体 / Create sphere
     */
    static std::unique_ptr<OsgVerseGeometry> createSphere(float radius = 1.0f,
                                                          unsigned int segments = 32);

    /**
     * @brief 创建圆柱体 / Create cylinder
     */
    static std::unique_ptr<OsgVerseGeometry> createCylinder(float radius = 1.0f,
                                                            float height = 2.0f,
                                                            unsigned int segments = 32);

    /**
     * @brief 创建圆锥体 / Create cone
     */
    static std::unique_ptr<OsgVerseGeometry> createCone(float radius = 1.0f,
                                                        float height = 2.0f,
                                                        unsigned int segments = 32);

    /**
     * @brief 创建平面 / Create plane
     */
    static std::unique_ptr<OsgVerseGeometry> createPlane(float width = 1.0f,
                                                         float height = 1.0f,
                                                         unsigned int widthSegments = 1,
                                                         unsigned int heightSegments = 1);

    /**
     * @brief 创建圆环 / Create torus
     */
    static std::unique_ptr<OsgVerseGeometry> createTorus(float majorRadius = 1.0f,
                                                         float minorRadius = 0.3f,
                                                         unsigned int majorSegments = 32,
                                                         unsigned int minorSegments = 16);

    /**
     * @brief 从点云创建 / Create from point cloud
     */
    static std::unique_ptr<OsgVerseGeometry> createFromPointCloud(
        const std::vector<Vec3f>& points,
        const std::vector<Color>* colors = nullptr);

    /**
     * @brief 从三角网格创建 / Create from triangle mesh
     */
    static std::unique_ptr<OsgVerseGeometry> createFromTriangleMesh(
        const std::vector<Vec3f>& vertices,
        const std::vector<uint32_t>& indices,
        const std::vector<Vec3f>* normals = nullptr,
        const std::vector<Vec2f>* texCoords = nullptr);
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEGEOMETRY_H
