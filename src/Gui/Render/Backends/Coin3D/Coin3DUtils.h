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

#ifndef GUI_RENDER_BACKENDS_COIN3D_COIN3DUTILS_H
#define GUI_RENDER_BACKENDS_COIN3D_COIN3DUTILS_H

#include <vector>
#include <memory>

#include <Inventor/SbColor.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbVec4f.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SbRotation.h>
#include <Inventor/nodes/SoNode.h>
#include <FCGlobal.h>

#include "../../Core/RenderTypes.h"
#include "../../Core/RenderNode.h"

namespace Gui {
namespace Render {

// 前向声明 / Forward declarations
class Coin3DNode;
class Coin3DGroup;

/**
 * @brief Coin3D 类型转换工具 / Coin3D type conversion utilities
 *
 * 提供 FreeCAD 类型与 Coin3D 类型之间的双向转换。
 * Provides bidirectional conversion between FreeCAD and Coin3D types.
 */
namespace Coin3DUtils {

//-------------------------------------------------------------------------
// 基础类型转换 / Basic Type Conversions
//-------------------------------------------------------------------------

/**
 * @brief Color 转 SbColor / Color to SbColor
 */
inline SbColor colorToSbColor(const Color& color)
{
    return SbColor(color.r, color.g, color.b);
}

/**
 * @brief SbColor 转 Color / SbColor to Color
 */
inline Color sbColorToColor(const SbColor& sbColor)
{
    return Color(sbColor[0], sbColor[1], sbColor[2], 1.0f);
}

/**
 * @brief Vec3f 转 SbVec3f / Vec3f to SbVec3f
 */
inline SbVec3f vec3fToSbVec3f(const Vec3f& vec)
{
    return SbVec3f(vec.x, vec.y, vec.z);
}

/**
 * @brief SbVec3f 转 Vec3f / SbVec3f to Vec3f
 */
inline Vec3f sbVec3fToVec3f(const SbVec3f& sbVec)
{
    return Vec3f(sbVec[0], sbVec[1], sbVec[2]);
}

/**
 * @brief Vec3d 转 SbVec3f (double to float) / Vec3d to SbVec3f
 */
inline SbVec3f vec3dToSbVec3f(const Vec3d& vec)
{
    return SbVec3f(static_cast<float>(vec.x),
                   static_cast<float>(vec.y),
                   static_cast<float>(vec.z));
}

/**
 * @brief SbVec3f 转 Vec3d (float to double) / SbVec3f to Vec3d
 */
inline Vec3d sbVec3fToVec3d(const SbVec3f& sbVec)
{
    return Vec3d(sbVec[0], sbVec[1], sbVec[2]);
}

/**
 * @brief Quaternion 转 SbRotation / Quaternion to SbRotation
 */
SbRotation quaternionToSbRotation(const Quaternion& quat);

/**
 * @brief SbRotation 转 Quaternion / SbRotation to Quaternion
 */
Quaternion sbRotationToQuaternion(const SbRotation& rot);

/**
 * @brief Matrix4 转 SbMatrix / Matrix4 to SbMatrix
 */
SbMatrix matrix4ToSbMatrix(const Matrix4& mat);

/**
 * @brief SbMatrix 转 Matrix4 / SbMatrix to Matrix4
 */
Matrix4 sbMatrixToMatrix4(const SbMatrix& sbMat);

//-------------------------------------------------------------------------
// 渲染模式转换 / Render Mode Conversions
//-------------------------------------------------------------------------

/**
 * @brief RenderMode 转 Coin3D 绘制样式 / RenderMode to Coin3D draw style
 *
 * 返回 SoDrawStyle::Style 枚举值
 * Returns SoDrawStyle::Style enum value
 */
int renderModeToDrawStyle(RenderMode mode);

/**
 * @brief Coin3D 绘制样式转 RenderMode / Coin3D draw style to RenderMode
 */
RenderMode drawStyleToRenderMode(int style);

/**
 * @brief RenderMode 转 SoShapeHintsType / RenderMode to SoShapeHintsType
 */
int renderModeToShapeHints(RenderMode mode);

//-------------------------------------------------------------------------
// 节点转换 / Node Conversions
//-------------------------------------------------------------------------

/**
 * @brief 包装 SoNode 为 RenderNode / Wrap SoNode as RenderNode
 *
 * 将现有的 Coin3D 节点包装为渲染抽象层节点。
 * Wraps an existing Coin3D node into a render abstraction layer node.
 *
 * @param coinNode Coin3D 节点 / Coin3D node
 * @param takeOwnership 是否获取所有权 / Whether to take ownership
 * @return 包装后的 RenderNode / Wrapped RenderNode
 */
RenderNode::Ptr wrapSoNode(SoNode* coinNode, bool takeOwnership = false);

/**
 * @brief 从 RenderNode 提取 SoNode / Extract SoNode from RenderNode
 *
 * @param renderNode 抽象渲染节点 / Abstract render node
 * @return Coin3D 节点指针，失败返回 nullptr / Coin3D node pointer, nullptr on failure
 */
SoNode* unwrapToSoNode(RenderNode* renderNode);

/**
 * @brief 递归包装 SoNode 子树 / Recursively wrap SoNode subtree
 *
 * @param coinNode Coin3D 根节点 / Coin3D root node
 * @param takeOwnership 是否获取所有权 / Whether to take ownership
 * @return 包装后的 RenderNode 根节点 / Wrapped RenderNode root node
 */
RenderNode::Ptr wrapSoNodeTree(SoNode* coinNode, bool takeOwnership = false);

/**
 * @brief 递归提取 SoNode 子树 / Recursively extract SoNode subtree
 *
 * @param renderNode 抽象渲染根节点 / Abstract render root node
 * @return Coin3D 根节点 / Coin3D root node
 */
SoNode* unwrapToSoNodeTree(RenderNode* renderNode);

//-------------------------------------------------------------------------
// 几何转换 / Geometry Conversions
//-------------------------------------------------------------------------

/**
 * @brief 将顶点数组转换为 SbVec3f 数组 / Convert vertex array to SbVec3f array
 */
std::vector<SbVec3f> verticesToSbVec3f(const std::vector<Vec3f>& vertices);

/**
 * @brief 将 SbVec3f 数组转换为顶点数组 / Convert SbVec3f array to vertex array
 */
std::vector<Vec3f> sbVec3fToVertices(const std::vector<SbVec3f>& sbVecs);

/**
 * @brief 将颜色数组转换为 SbColor 数组 / Convert color array to SbColor array
 */
std::vector<SbColor> colorsToSbColor(const std::vector<Color>& colors);

/**
 * @brief 将 SbColor 数组转换为颜色数组 / Convert SbColor array to color array
 */
std::vector<Color> sbColorToColors(const std::vector<SbColor>& sbColors);

//-------------------------------------------------------------------------
// 场景图操作 / Scene Graph Operations
//-------------------------------------------------------------------------

/**
 * @brief 查找场景中的节点类型 / Find node types in scene
 *
 * @param root 根节点 / Root node
 * @param type 要查找的节点类型 / Node type to find
 * @return 匹配的节点列表 / List of matching nodes
 */
std::vector<SoNode*> findNodesByType(SoNode* root, int soType);

/**
 * @brief 计算场景图边界框 / Calculate scene graph bounding box
 *
 * @param root 根节点 / Root node
 * @return 边界框 / Bounding box
 */
BoundingBox calculateSceneBoundingBox(SoNode* root);

/**
 * @brief 克隆 SoNode 子树 / Clone SoNode subtree
 *
 * @param node 要克隆的节点 / Node to clone
 * @return 克隆后的节点 / Cloned node
 */
SoNode* cloneSoNode(SoNode* node);

/**
 * @brief 替换场景中的节点 / Replace node in scene
 *
 * @param parent 父节点 / Parent node
 * @param oldNode 要替换的节点 / Node to replace
 * @param newNode 新节点 / New node
 * @return 是否成功 / Whether successful
 */
bool replaceNode(SoNode* parent, SoNode* oldNode, SoNode* newNode);

//-------------------------------------------------------------------------
// 材质转换 / Material Conversions
//-------------------------------------------------------------------------

/**
 * @brief 从 FreeCAD App::Material 转换 / Convert from FreeCAD App::Material
 *
 * @param appMaterial FreeCAD App 层材质 / FreeCAD App layer material
 * @param ambientColor 输出环境光颜色 / Output ambient color
 * @param diffuseColor 输出漫反射颜色 / Output diffuse color
 * @param specularColor 输出镜面反射颜色 / Output specular color
 * @param emissiveColor 输出发光颜色 / Output emissive color
 * @param shininess 输出光泽度 / Output shininess
 * @param transparency 输出透明度 / Output transparency
 */
void convertAppMaterial(
    const class App::Material& appMaterial,
    SbColor& ambientColor,
    SbColor& diffuseColor,
    SbColor& specularColor,
    SbColor& emissiveColor,
    float& shininess,
    float& transparency
);

/**
 * @brief 转换为 FreeCAD App::Material / Convert to FreeCAD App::Material
 *
 * @param ambientColor 环境光颜色 / Ambient color
 * @param diffuseColor 漫反射颜色 / Diffuse color
 * @param specularColor 镜面反射颜色 / Specular color
 * @param emissiveColor 发光颜色 / Emissive color
 * @param shininess 光泽度 / Shininess
 * @param transparency 透明度 / Transparency
 * @param appMaterial 输出 FreeCAD App 层材质 / Output FreeCAD App layer material
 */
void convertToAppMaterial(
    const SbColor& ambientColor,
    const SbColor& diffuseColor,
    const SbColor& specularColor,
    const SbColor& emissiveColor,
    float shininess,
    float transparency,
    class App::Material& appMaterial
);

//-------------------------------------------------------------------------
// 调试和诊断 / Debug and Diagnostics
//-------------------------------------------------------------------------

/**
 * @brief 打印节点树 / Print node tree
 *
 * @param root 根节点 / Root node
 * @param indent 缩进级别 / Indent level
 */
void printNodeTree(SoNode* root, int indent = 0);

/**
 * @brief 获取节点类型名称 / Get node type name
 */
std::string getNodeTypeName(SoNode* node);

/**
 * @brief 验证节点树 / Validate node tree
 *
 * 检查节点树的完整性（循环引用、空指针等）。
 * Check node tree integrity (circular references, null pointers, etc.).
 *
 * @param root 根节点 / Root node
 * @return 是否有效 / Whether valid
 */
bool validateNodeTree(SoNode* root);

//-------------------------------------------------------------------------
// 性能工具 / Performance Utilities
//-------------------------------------------------------------------------

/**
 * @brief 优化场景图 / Optimize scene graph
 *
 * 执行常见的场景图优化操作：
 * - 合并相同材质的节点
 * - 移除冗余的 Separator
 * - 共享相同几何体
 *
 * Performs common scene graph optimizations:
 * - Merge nodes with same material
 * - Remove redundant Separators
 * - Share identical geometry
 *
 * @param root 根节点 / Root node
 * @return 优化后的根节点 / Optimized root node
 */
SoNode* optimizeSceneGraph(SoNode* root);

/**
 * @brief 计算场景统计信息 / Calculate scene statistics
 *
 * 统计节点数量、三角形数量、内存使用等。
 * Count nodes, triangles, memory usage, etc.
 */
struct SceneStats {
    int nodeCount{0};
    int triangleCount{0};
    int lineCount{0};
    int pointCount{0};
    size_t estimatedMemoryBytes{0};
};

SceneStats calculateSceneStats(SoNode* root);

/**
 * @brief 简化场景图 / Simplify scene graph
 *
 * 通过降采样等简化几何体，减少渲染开销。
 * Simplify geometry through downsampling to reduce rendering cost.
 *
 * @param root 根节点 / Root node
 * @param targetReduction 目标减少比例 (0-1) / Target reduction ratio (0-1)
 * @return 简化后的根节点 / Simplified root node
 */
SoNode* simplifySceneGraph(SoNode* root, float targetReduction = 0.5f);

} // namespace Coin3DUtils

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_COIN3D_COIN3DUTILS_H
