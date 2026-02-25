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

#include "Coin3DUtils.h"
#include "Coin3DNode.h"
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/SoFullPath.h>
#include <Inventor/SoType.h>
#include <App/Material.h>
#include <Base/Console.h>

namespace Gui {
namespace Render {
namespace Coin3DUtils {

//===========================================================================
// 四元数和矩阵转换 / Quaternion and Matrix Conversions
//===========================================================================

SbRotation quaternionToSbRotation(const Quaternion& quat)
{
    // Coin3D SbRotation 使用 (x, y, z, w) 格式的四元数
    // Coin3D SbRotation uses quaternion in (x, y, z, w) format
    return SbRotation(quat.x, quat.y, quat.z, quat.w);
}

Quaternion sbRotationToQuaternion(const SbRotation& rot)
{
    float q1, q2, q3, q4;
    rot.getValue(q1, q2, q3, q4);
    return Quaternion(q1, q2, q3, q4);
}

SbMatrix matrix4ToSbMatrix(const Matrix4& mat)
{
    SbMatrix sbMat;

    // Base::Matrix4D 使用行主序 / Base::Matrix4D uses row-major order
    // SbMatrix 使用行主序 / SbMatrix uses row-major order
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            sbMat[i][j] = static_cast<float>(mat[i][j]);
        }
    }

    return sbMat;
}

Matrix4 sbMatrixToMatrix4(const SbMatrix& sbMat)
{
    Matrix4 mat;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            mat[i][j] = sbMat[i][j];
        }
    }

    return mat;
}

//===========================================================================
// 渲染模式转换 / Render Mode Conversions
//===========================================================================

int renderModeToDrawStyle(RenderMode mode)
{
    switch (mode) {
        case RenderMode::Wireframe:
            return SoDrawStyle::LINES;
        case RenderMode::Points:
            return SoDrawStyle::POINTS;
        case RenderMode::Shaded:
        case RenderMode::Flat:
        case RenderMode::Gouraud:
        case RenderMode::Phong:
        case RenderMode::Default:
        default:
            return SoDrawStyle::FILLED;
        case RenderMode::BoundingBox:
            return SoDrawStyle::INVISIBLE;
    }
}

RenderMode drawStyleToRenderMode(int style)
{
    switch (style) {
        case SoDrawStyle::LINES:
            return RenderMode::Wireframe;
        case SoDrawStyle::POINTS:
            return RenderMode::Points;
        case SoDrawStyle::FILLED:
            return RenderMode::Shaded;
        case SoDrawStyle::INVISIBLE:
            return RenderMode::BoundingBox;
        default:
            return RenderMode::Default;
    }
}

int renderModeToShapeHints(RenderMode mode)
{
    // SoShapeHints 设置 / SoShapeHints settings
    switch (mode) {
        case RenderMode::Flat:
            // 平面着色需要关闭顶点排序 / Flat shading needs vertex ordering off
            return 0;  // SoShapeHints::UNKNOWN_ORDERING
        case RenderMode::Gouraud:
        case RenderMode::Shaded:
        default:
            return 1;  // SoShapeHints::COUNTERCLOCKWISE
    }
}

//===========================================================================
// 节点转换 / Node Conversions
//===========================================================================

RenderNode::Ptr wrapSoNode(SoNode* coinNode, bool takeOwnership)
{
    if (!coinNode) {
        return nullptr;
    }

    SoType type = coinNode->getTypeId();

    // 根据具体类型创建相应的包装器 / Create appropriate wrapper based on type
    if (type.isDerivedFrom(SoSeparator::getClassTypeId())) {
        return std::make_shared<Coin3DSeparator>(
            static_cast<SoSeparator*>(coinNode), takeOwnership);
    }
    else if (type.isDerivedFrom(SoGroup::getClassTypeId())) {
        return std::make_shared<Coin3DGroup>(
            static_cast<SoGroup*>(coinNode), takeOwnership);
    }
    else if (type.isDerivedFrom(SoTransform::getClassTypeId())) {
        return std::make_shared<Coin3DTransform>(
            static_cast<SoTransform*>(coinNode), takeOwnership);
    }
    else if (type.isDerivedFrom(SoSwitch::getClassTypeId())) {
        return std::make_shared<Coin3DSwitch>(
            static_cast<SoSwitch*>(coinNode), takeOwnership);
    }
    else {
        // 通用节点包装器 / Generic node wrapper
        return std::make_shared<Coin3DNode>(coinNode, takeOwnership, NodeType::Node);
    }
}

SoNode* unwrapToSoNode(RenderNode* renderNode)
{
    if (!renderNode) {
        return nullptr;
    }

    if (auto* coin3DNode = dynamic_cast<Coin3DNode*>(renderNode)) {
        return coin3DNode->getCoinNode();
    }

    return nullptr;
}

RenderNode::Ptr wrapSoNodeTree(SoNode* coinNode, bool takeOwnership)
{
    // TODO: 递归包装整个子树 / Recursively wrap entire subtree
    // 当前只包装根节点 / Currently only wraps root node
    return wrapSoNode(coinNode, takeOwnership);
}

SoNode* unwrapToSoNodeTree(RenderNode* renderNode)
{
    if (!renderNode) {
        return nullptr;
    }

    if (auto* coin3DNode = dynamic_cast<Coin3DNode*>(renderNode)) {
        return coin3DNode->getCoinNode();
    }

    return nullptr;
}

//===========================================================================
// 几何转换 / Geometry Conversions
//===========================================================================

std::vector<SbVec3f> verticesToSbVec3f(const std::vector<Vec3f>& vertices)
{
    std::vector<SbVec3f> result;
    result.reserve(vertices.size());

    for (const auto& v : vertices) {
        result.push_back(SbVec3f(v.x, v.y, v.z));
    }

    return result;
}

std::vector<Vec3f> sbVec3fToVertices(const std::vector<SbVec3f>& sbVecs)
{
    std::vector<Vec3f> result;
    result.reserve(sbVecs.size());

    for (const auto& v : sbVecs) {
        result.push_back(Vec3f(v[0], v[1], v[2]));
    }

    return result;
}

std::vector<SbColor> colorsToSbColor(const std::vector<Color>& colors)
{
    std::vector<SbColor> result;
    result.reserve(colors.size());

    for (const auto& c : colors) {
        result.push_back(SbColor(c.r, c.g, c.b));
    }

    return result;
}

std::vector<Color> sbColorToColors(const std::vector<SbColor>& sbColors)
{
    std::vector<Color> result;
    result.reserve(sbColors.size());

    for (const auto& c : sbColors) {
        result.push_back(Color(c[0], c[1], c[2], 1.0f));
    }

    return result;
}

//===========================================================================
// 场景图操作 / Scene Graph Operations
//===========================================================================

std::vector<SoNode*> findNodesByType(SoNode* root, int soType)
{
    std::vector<SoNode*> result;

    // TODO: 实现 SoSearchAction 遍历
    // Implement SoSearchAction traversal
    (void)root;
    (void)soType;

    Base::Console().warning("Coin3DUtils::findNodesByType: Not yet fully implemented\n");
    return result;
}

BoundingBox calculateSceneBoundingBox(SoNode* root)
{
    if (!root) {
        return BoundingBox();
    }

    // 使用 SoGetBoundingBoxAction 计算边界框
    // Use SoGetBoundingBoxAction to calculate bounding box
    static SbViewportRegion vpRegion;
    SoGetBoundingBoxAction bboxAction(vpRegion);
    bboxAction.apply(root);

    SbBox3f box = bboxAction.getBoundingBox();

    if (box.isEmpty()) {
        return BoundingBox();
    }

    float minX, minY, minZ, maxX, maxY, maxZ;
    box.getBounds(minX, minY, minZ, maxX, maxY, maxZ);

    return BoundingBox(minX, minY, minZ, maxX, maxY, maxZ);
}

SoNode* cloneSoNode(SoNode* node)
{
    if (!node) {
        return nullptr;
    }

    // Coin3D 提供了 clone() 方法 / Coin3D provides clone() method
    return node->copy();
}

bool replaceNode(SoNode* parent, SoNode* oldNode, SoNode* newNode)
{
    if (!parent || !oldNode || !newNode) {
        return false;
    }

    if (!parent->getTypeId().isDerivedFrom(SoGroup::getClassTypeId())) {
        return false;
    }

    auto* group = static_cast<SoGroup*>(parent);
    int numChildren = group->getNumChildren();

    for (int i = 0; i < numChildren; ++i) {
        if (group->getChild(i) == oldNode) {
            group->replaceChild(i, newNode);
            return true;
        }
    }

    return false;
}

//===========================================================================
// 材质转换 / Material Conversions
//===========================================================================

void convertAppMaterial(
    const App::Material& appMaterial,
    SbColor& ambientColor,
    SbColor& diffuseColor,
    SbColor& specularColor,
    SbColor& emissiveColor,
    float& shininess,
    float& transparency)
{
    // 从 App::Material 转换 / Convert from App::Material
    ambientColor.setValue(appMaterial.ambientColor.r,
                          appMaterial.ambientColor.g,
                          appMaterial.ambientColor.b);

    diffuseColor.setValue(appMaterial.diffuseColor.r,
                          appMaterial.diffuseColor.g,
                          appMaterial.diffuseColor.b);

    specularColor.setValue(appMaterial.specularColor.r,
                           appMaterial.specularColor.g,
                           appMaterial.specularColor.b);

    emissiveColor.setValue(appMaterial.emissiveColor.r,
                           appMaterial.emissiveColor.g,
                           appMaterial.emissiveColor.b);

    shininess = appMaterial.shininess;
    transparency = appMaterial.transparency;
}

void convertToAppMaterial(
    const SbColor& ambientColor,
    const SbColor& diffuseColor,
    const SbColor& specularColor,
    const SbColor& emissiveColor,
    float shininess,
    float transparency,
    App::Material& appMaterial)
{
    appMaterial.ambientColor.set(ambientColor[0], ambientColor[1], ambientColor[2]);
    appMaterial.diffuseColor.set(diffuseColor[0], diffuseColor[1], diffuseColor[2]);
    appMaterial.specularColor.set(specularColor[0], specularColor[1], specularColor[2]);
    appMaterial.emissiveColor.set(emissiveColor[0], emissiveColor[1], emissiveColor[2]);
    appMaterial.shininess = shininess;
    appMaterial.transparency = transparency;
}

//===========================================================================
// 调试和诊断 / Debug and Diagnostics
//===========================================================================

void printNodeTree(SoNode* root, int indent)
{
    if (!root) {
        return;
    }

    // 打印缩进 / Print indentation
    for (int i = 0; i < indent; ++i) {
        Base::Console().log("  ");
    }

    // 打印节点名称和类型 / Print node name and type
    SbName name = root->getName();
    Base::Console().log("%s (%s)\n",
                        root->getTypeId().getName().getString(),
                        name.getString());

    // 递归打印子节点 / Recursively print children
    if (root->getTypeId().isDerivedFrom(SoGroup::getClassTypeId())) {
        auto* group = static_cast<SoGroup*>(root);
        int numChildren = group->getNumChildren();
        for (int i = 0; i < numChildren; ++i) {
            printNodeTree(group->getChild(i), indent + 1);
        }
    }
}

std::string getNodeTypeName(SoNode* node)
{
    if (!node) {
        return "nullptr";
    }
    return std::string(node->getTypeId().getName().getString());
}

bool validateNodeTree(SoNode* root)
{
    if (!root) {
        return false;
    }

    // TODO: 实现完整的验证逻辑 / Implement full validation logic
    // - 检查循环引用 / Check circular references
    // - 检查空指针 / Check null pointers
    // - 验证节点类型 / Validate node types

    (void)root;
    return true;
}

//===========================================================================
// 性能工具 / Performance Utilities
//===========================================================================

SoNode* optimizeSceneGraph(SoNode* root)
{
    if (!root) {
        return nullptr;
    }

    // TODO: 实现场景图优化 / Implement scene graph optimization
    // - 合并相同材质的节点 / Merge nodes with same material
    // - 移除冗余的 Separator / Remove redundant Separators
    // - 共享相同几何体 / Share identical geometry

    Base::Console().warning("Coin3DUtils::optimizeSceneGraph: Not yet implemented\n");
    return root;
}

SceneStats calculateSceneStats(SoNode* root)
{
    SceneStats stats;

    if (!root) {
        return stats;
    }

    // TODO: 实现场景统计 / Implement scene statistics
    // - 统计节点数量 / Count nodes
    // - 统计三角形数量 / Count triangles
    // - 估算内存使用 / Estimate memory usage

    (void)root;
    Base::Console().warning("Coin3DUtils::calculateSceneStats: Not yet implemented\n");
    return stats;
}

SoNode* simplifySceneGraph(SoNode* root, float targetReduction)
{
    if (!root) {
        return nullptr;
    }

    // TODO: 实现场景图简化 / Implement scene graph simplification
    (void)targetReduction;
    Base::Console().warning("Coin3DUtils::simplifySceneGraph: Not yet implemented\n");
    return root;
}

} // namespace Coin3DUtils
} // namespace Render
} // namespace Gui
