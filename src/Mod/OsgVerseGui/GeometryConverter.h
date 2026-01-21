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

#ifndef OSGVERSEGUI_GEOMETRYCONVERTER_H
#define OSGVERSEGUI_GEOMETRYCONVERTER_H

#include "PreCompiled.h"
#include <osg/ref_ptr>
#include <osg/Geometry>
#include <osg/Geode>

// OCCT forward declarations
class TopoDS_Shape;
class TopoDS_Face;

namespace OsgVerseGui {

/**
 * @brief 几何体转换器：将 OCCT TopoShape 转换为 OSG Geometry
 * 
 * 这个类负责将 FreeCAD 的 OCCT 几何体转换为 OSG 可以渲染的格式。
 * 主要步骤：
 * 1. 使用 BRepMesh_IncrementalMesh 进行三角剖分
 * 2. 遍历 Face，提取三角形数据
 * 3. 计算法线
 * 4. 创建 OSG Geometry
 * 
 * Phase 2 实现：基本的三角剖分和转换
 * 未来扩展：LOD、优化、纹理坐标等
 */
class OsgVerseGuiExport GeometryConverter {
public:
    /**
     * @brief 转换选项
     */
    struct OsgVerseGuiExport ConversionOptions {
        double deflection = 0.1;        ///< 三角剖分精度（越小越精细）
        double angle = 0.5;             ///< 角度偏差（弧度）
        bool computeNormals = true;     ///< 是否计算法线
        bool relative = false;          ///< 是否使用相对精度
        
        ConversionOptions() = default;
    };
    
    /**
     * @brief 转换结果统计
     */
    struct ConversionStats {
        int vertexCount = 0;      ///< 顶点数
        int triangleCount = 0;    ///< 三角形数
        int faceCount = 0;        ///< Face 数
        double conversionTime = 0.0; ///< 转换时间（秒）
        
        ConversionStats() = default;
    };
    
    /**
     * @brief 将 TopoDS_Shape 转换为 OSG Geode
     * 
     * 这是主要的转换入口。将 OCCT 形状转换为包含几何体的 OSG Geode。
     * 
     * @param shape OCCT 形状
     * @param options 转换选项
     * @param stats 转换统计（可选，用于性能分析）
     * @return OSG Geode，包含转换后的几何体
     * 
     * @note 如果转换失败，返回 nullptr
     */
    static osg::ref_ptr<osg::Geode> convertShape(
        const TopoDS_Shape& shape,
        const ConversionOptions& options = ConversionOptions(),
        ConversionStats* stats = nullptr
    );
    
    /**
     * @brief 将 TopoDS_Shape 转换为 OSG Geometry
     * 
     * 底层转换方法，直接返回 Geometry 对象。
     * 
     * @param shape OCCT 形状
     * @param options 转换选项
     * @param stats 转换统计（可选）
     * @return OSG Geometry
     */
    static osg::ref_ptr<osg::Geometry> convertToGeometry(
        const TopoDS_Shape& shape,
        const ConversionOptions& options = ConversionOptions(),
        ConversionStats* stats = nullptr
    );

private:
    /**
     * @brief 对形状进行三角剖分
     * 
     * 使用 BRepMesh_IncrementalMesh 对形状进行网格化。
     * 
     * @param shape 要剖分的形状
     * @param deflection 精度
     * @param angle 角度偏差
     * @param relative 是否使用相对精度
     * @return 是否成功
     */
    static bool tessellateShape(
        const TopoDS_Shape& shape,
        double deflection,
        double angle,
        bool relative
    );
    
    /**
     * @brief 从三角剖分的形状中提取几何数据
     * 
     * 遍历所有 Face，提取顶点、法线和索引数据。
     * 
     * @param shape 已剖分的形状
     * @param vertices 输出：顶点数组
     * @param normals 输出：法线数组
     * @param indices 输出：索引数组
     * @param faceCount 输出：Face 数量
     * @return 是否成功
     */
    static bool extractGeometryData(
        const TopoDS_Shape& shape,
        std::vector<osg::Vec3>& vertices,
        std::vector<osg::Vec3>& normals,
        std::vector<unsigned int>& indices,
        int& faceCount
    );
    
    /**
     * @brief 从单个 Face 提取三角形数据
     * 
     * @param face 要处理的 Face
     * @param vertices 输出：顶点数组
     * @param normals 输出：法线数组
     * @param indices 输出：索引数组
     * @param vertexOffset 当前顶点偏移量
     * @return 是否成功
     */
    static bool extractFaceTriangles(
        const TopoDS_Face& face,
        std::vector<osg::Vec3>& vertices,
        std::vector<osg::Vec3>& normals,
        std::vector<unsigned int>& indices,
        unsigned int& vertexOffset
    );
    
    /**
     * @brief 创建 OSG Geometry 对象
     * 
     * 从提取的数据创建 OSG Geometry。
     * 
     * @param vertices 顶点数组
     * @param normals 法线数组
     * @param indices 索引数组
     * @return OSG Geometry
     */
    static osg::ref_ptr<osg::Geometry> createOsgGeometry(
        const std::vector<osg::Vec3>& vertices,
        const std::vector<osg::Vec3>& normals,
        const std::vector<unsigned int>& indices
    );
};

} // namespace OsgVerseGui

#endif // OSGVERSEGUI_GEOMETRYCONVERTER_H
