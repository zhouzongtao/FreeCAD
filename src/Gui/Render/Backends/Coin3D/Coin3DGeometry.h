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

#ifndef GUI_RENDER_BACKENDS_COIN3D_COIN3DGEOMETRY_H
#define GUI_RENDER_BACKENDS_COIN3D_COIN3DGEOMETRY_H

#include <vector>
#include <memory>

#include <FCGlobal.h>
#include "../../Core/RenderTypes.h"
#include "Coin3DNode.h"

// 前向声明 Coin3D 几何节点 / Forward declarations for Coin3D geometry nodes
class SoCoordinate3;
class SoIndexedFaceSet;
class SoFaceSet;
class SoLineSet;
class SoPointSet;
class SoIndexedTriangleStripSet;
class SoTriangleStripSet;
class SoVertexProperty;
class SoShape;
class SoCallbackAction;

namespace Gui {
namespace Render {

/**
 * @brief Coin3D 几何节点包装器基类 / Coin3D geometry node wrapper base
 *
 * 所有几何体包装器的基类，提供通用几何操作。
 * Base class for all geometry wrappers, providing common geometry operations.
 */
class GuiExport Coin3DGeometry : public Coin3DNode {
public:
    RENDER_NODE_STATIC_TYPE(NodeType::Geometry)

    /**
     * @brief 构造函数 / Constructor
     */
    Coin3DGeometry(SoShape* shape, bool ownsNode);

    /**
     * @brief 析构函数 / Destructor
     */
    ~Coin3DGeometry() override;

    /**
     * @brief 获取 SoShape 指针 / Get SoShape pointer
     */
    SoShape* getSoShape() const;

    /**
     * @brief 获取边界框 / Get bounding box
     */
    BoundingBox getBoundingBox() const override;

    /**
     * @brief 设置线宽 / Set line width
     */
    void setLineWidth(float width);

    /**
     * @brief 获取线宽 / Get line width
     */
    float getLineWidth() const;

    /**
     * @brief 设置点大小 / Set point size
     */
    void setPointSize(float size);

    /**
     * @brief 获取点大小 / Get point size
     */
    float getPointSize() const;

protected:
    /// 绘制样式节点 / Draw style node
    class SoDrawStyle* _drawStyle{nullptr};
};

//-------------------------------------------------------------------------
// 索引面集包装器 / Indexed Face Set Wrapper
//-------------------------------------------------------------------------

/**
 * @brief SoIndexedFaceSet 包装器 / SoIndexedFaceSet wrapper
 *
 * 用于渲染任意多边形网格。
 * For rendering arbitrary polygon meshes.
 */
class GuiExport Coin3DIndexedFaceSet : public Coin3DGeometry {
public:
    /**
     * @brief 构造函数 - 创建新节点 / Constructor - Create new node
     */
    Coin3DIndexedFaceSet();

    /**
     * @brief 从现有节点创建 / Create from existing node
     */
    explicit Coin3DIndexedFaceSet(SoIndexedFaceSet* ifs, bool ownsNode = false);

    /**
     * @brief 析构函数 / Destructor
     */
    ~Coin3DIndexedFaceSet() override;

    /**
     * @brief 获取 SoIndexedFaceSet 指针 / Get SoIndexedFaceSet pointer
     */
    SoIndexedFaceSet* getIndexedFaceSet() const;

    //-----------------------------------------------------------------------
    // 顶点操作 / Vertex Operations
    //-----------------------------------------------------------------------

    /**
     * @brief 设置顶点坐标 / Set vertex coordinates
     */
    void setCoordinates(const std::vector<Vec3f>& coords);

    /**
     * @brief 获取顶点坐标 / Get vertex coordinates
     */
    std::vector<Vec3f> getCoordinates() const;

    /**
     * @brief 添加顶点 / Add vertex
     */
    void addCoordinate(const Vec3f& coord);

    /**
     * @brief 设置顶点数量（预留空间）/ Set vertex count (reserve space)
     */
    void setNumCoordinates(int num);

    //-----------------------------------------------------------------------
    // 索引操作 / Index Operations
    //-----------------------------------------------------------------------

    /**
     * @brief 设置面索引 / Set face indices
     *
     * 每个面以 -1 结束。
     * Each face ends with -1.
     */
    void setCoordIndex(const std::vector<int32_t>& indices);

    /**
     * @brief 获取面索引 / Get face indices
     */
    std::vector<int32_t> getCoordIndex() const;

    /**
     * @brief 添加面 / Add face
     *
     * @param vertices 面的顶点索引 / Face vertex indices
     */
    void addFace(const std::vector<int32_t>& vertices);

    //-----------------------------------------------------------------------
    // 法线操作 / Normal Operations
    //-----------------------------------------------------------------------

    /**
     * @brief 设置顶点法线 / Set vertex normals
     */
    void setNormals(const std::vector<Vec3f>& normals);

    /**
     * @brief 获取顶点法线 / Get vertex normals
     */
    std::vector<Vec3f> getNormals() const;

    /**
     * @brief 设置法线索引 / Set normal indices
     */
    void setNormalIndex(const std::vector<int32_t>& indices);

    /**
     * @brief 设置法线绑定 / Set normal binding
     */
    enum class NormalBinding {
        PerVertex,
        PerFace,
        Overall
    };
    void setNormalBinding(NormalBinding binding);

    //-----------------------------------------------------------------------
    // 纹理坐标操作 / Texture Coordinate Operations
    //-----------------------------------------------------------------------

    /**
     * @brief 设置纹理坐标 / Set texture coordinates
     */
    void setTextureCoords(const std::vector<Vec2f>& coords);

    /**
     * @brief 获取纹理坐标 / Get texture coordinates
     */
    std::vector<Vec2f> getTextureCoords() const;

    //-----------------------------------------------------------------------
    // 材质索引操作 / Material Index Operations
    //-----------------------------------------------------------------------

    /**
     * @brief 设置材质索引 / Set material indices
     */
    void setMaterialIndex(const std::vector<int32_t>& indices);

    /**
     * @brief 获取材质索引 / Get material indices
     */
    std::vector<int32_t> getMaterialIndex() const;

    //-----------------------------------------------------------------------
    // 工具方法 / Utility Methods
    //-----------------------------------------------------------------------

    /**
     * @brief 计算法线 / Calculate normals
     *
     * 自动计算顶点或面法线。
     * Automatically calculate vertex or face normals.
     */
    void calculateNormals(bool perVertex = true);

    /**
     * @brief 获取面数 / Get face count
     */
    int getFaceCount() const;

    /**
     * @brief 获取顶点数 / Get vertex count
     */
    int getVertexCount() const;
};

//-------------------------------------------------------------------------
// 面集包装器 / Face Set Wrapper
//-------------------------------------------------------------------------

/**
 * @brief SoFaceSet 包装器 / SoFaceSet wrapper
 *
 * 用于渲染简单的面集（不需要面分隔符）。
 * For rendering simple face sets (no face separators needed).
 */
class GuiExport Coin3DFaceSet : public Coin3DGeometry {
public:
    /**
     * @brief 构造函数 - 创建新节点 / Constructor - Create new node
     */
    Coin3DFaceSet();

    /**
     * @brief 从现有节点创建 / Create from existing node
     */
    explicit Coin3DFaceSet(SoFaceSet* fs, bool ownsNode = false);

    /**
     * @brief 析构函数 / Destructor
     */
    ~Coin3DFaceSet() override;

    /**
     * @brief 获取 SoFaceSet 指针 / Get SoFaceSet pointer
     */
    SoFaceSet* getFaceSet() const;

    /**
     * @brief 设置顶点数量 / Set number of vertices per face
     */
    void setNumVertices(const std::vector<int32_t>& numVertices);

    /**
     * @brief 设置顶点坐标 / Set vertex coordinates
     */
    void setCoordinates(const std::vector<Vec3f>& coords);
};

//-------------------------------------------------------------------------
// 线集包装器 / Line Set Wrapper
//-------------------------------------------------------------------------

/**
 * @brief SoLineSet 包装器 / SoLineSet wrapper
 */
class GuiExport Coin3DLineSet : public Coin3DGeometry {
public:
    /**
     * @brief 构造函数 / Constructor
     */
    Coin3DLineSet();

    /**
     * @brief 从现有节点创建 / Create from existing node
     */
    explicit Coin3DLineSet(SoLineSet* ls, bool ownsNode = false);

    /**
     * @brief 析构函数 / Destructor
     */
    ~Coin3DLineSet() override;

    /**
     * @brief 设置顶点坐标 / Set vertex coordinates
     */
    void setCoordinates(const std::vector<Vec3f>& coords);

    /**
     * @brief 设置每条线的顶点数 / Set vertices per line
     */
    void setNumVertices(const std::vector<int32_t>& numVertices);
};

//-------------------------------------------------------------------------
// 点集包装器 / Point Set Wrapper
//-------------------------------------------------------------------------

/**
 * @brief SoPointSet 包装器 / SoPointSet wrapper
 */
class GuiExport Coin3DPointSet : public Coin3DGeometry {
public:
    /**
     * @brief 构造函数 / Constructor
     */
    Coin3DPointSet();

    /**
     * @brief 从现有节点创建 / Create from existing node
     */
    explicit Coin3DPointSet(SoPointSet* ps, bool ownsNode = false);

    /**
     * @brief 析构函数 / Destructor
     */
    ~Coin3DPointSet() override;

    /**
     * @brief 设置顶点坐标 / Set vertex coordinates
     */
    void setCoordinates(const std::vector<Vec3f>& coords);

    /**
     * @brief 获取起点索引 / Get starting point index
     */
    int getstartIndex() const;
};

//-------------------------------------------------------------------------
// 几何构建器 / Geometry Builder
//-------------------------------------------------------------------------

/**
 * @brief 几何构建器 / Geometry builder
 *
 * 简化几何体创建的工具类。
 * Utility class to simplify geometry creation.
 */
class GuiExport GeometryBuilder {
public:
    /**
     * @brief 创建立方体 / Create cube
     *
     * @param size 尺寸 / Size
     * @param center 中心位置 / Center position
     */
    static RenderNode::Ptr createCube(float size = 1.0f, const Vec3f& center = Vec3f());

    /**
     * @brief 创建球体 / Create sphere
     *
     * @param radius 半径 / Radius
     * @param center 中心位置 / Center position
     */
    static RenderNode::Ptr createSphere(float radius = 1.0f, const Vec3f& center = Vec3f());

    /**
     * @brief 创建圆柱 / Create cylinder
     *
     * @param radius 半径 / Radius
     * @param height 高度 / Height
     */
    static RenderNode::Ptr createCylinder(float radius = 1.0f, float height = 2.0f);

    /**
     * @brief 创建圆锥 / Create cone
     *
     * @param radius 底部半径 / Bottom radius
     * @param height 高度 / Height
     */
    static RenderNode::Ptr createCone(float radius = 1.0f, float height = 2.0f);

    /**
     * @brief 从顶点创建网格 / Create mesh from vertices
     *
     * @param vertices 顶点坐标 / Vertex coordinates
     * @param faces 面索引 / Face indices
     * @param normals 顶点法线 / Vertex normals
     */
    static RenderNode::Ptr createMesh(
        const std::vector<Vec3f>& vertices,
        const std::vector<std::vector<int32_t>>& faces,
        const std::vector<Vec3f>* normals = nullptr
    );

    /**
     * @brief 创建线框几何 / Create wireframe geometry
     *
     * @param vertices 顶点坐标 / Vertex coordinates
     * @param edges 边索引对 / Edge index pairs
     */
    static RenderNode::Ptr createWireframe(
        const std::vector<Vec3f>& vertices,
        const std::vector<std::pair<int32_t, int32_t>>& edges
    );

    /**
     * @brief 创建点云 / Create point cloud
     *
     * @param points 点坐标 / Point coordinates
     */
    static RenderNode::Ptr createPointCloud(const std::vector<Vec3f>& points);
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_COIN3D_COIN3DGEOMETRY_H
