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

#include "Coin3DGeometry.h"
#include <Inventor/nodes/SoShape.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoVertexProperty.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/SbVec3f.h>
#include <Base/Console.h>

namespace Gui {
namespace Render {

//===========================================================================
// Coin3DGeometry - 基础几何包装器类 / Base Geometry Wrapper
//===========================================================================

Coin3DGeometry::Coin3DGeometry(SoShape* shape, bool ownsNode)
    : Coin3DNode(shape, ownsNode, NodeType::Geometry)
{
    // 创建绘制样式节点 / Create draw style node
    _drawStyle = new SoDrawStyle();
    _drawStyle->ref();
}

Coin3DGeometry::~Coin3DGeometry()
{
    if (_drawStyle) {
        _drawStyle->unref();
    }
}

SoShape* Coin3DGeometry::getSoShape() const
{
    return static_cast<SoShape*>(_coinNode);
}

BoundingBox Coin3DGeometry::getBoundingBox() const
{
    // TODO: 实现边界框计算 / Implement bounding box calculation
    // 需要使用 SoGetBoundingBoxAction
    return BoundingBox();
}

void Coin3DGeometry::setLineWidth(float width)
{
    if (_drawStyle) {
        _drawStyle->lineWidth.setValue(width);
        touch();
    }
}

float Coin3DGeometry::getLineWidth() const
{
    if (_drawStyle) {
        return _drawStyle->lineWidth.getValue();
    }
    return 1.0f;
}

void Coin3DGeometry::setPointSize(float size)
{
    if (_drawStyle) {
        _drawStyle->pointSize.setValue(size);
        touch();
    }
}

float Coin3DGeometry::getPointSize() const
{
    if (_drawStyle) {
        return _drawStyle->pointSize.getValue();
    }
    return 1.0f;
}

//===========================================================================
// Coin3DIndexedFaceSet - 索引面集包装器 / Indexed Face Set Wrapper
//===========================================================================

Coin3DIndexedFaceSet::Coin3DIndexedFaceSet()
    : Coin3DGeometry(new SoIndexedFaceSet(), true)
{
    _coinNode->ref();
}

Coin3DIndexedFaceSet::Coin3DIndexedFaceSet(SoIndexedFaceSet* ifs, bool ownsNode)
    : Coin3DGeometry(ifs, ownsNode)
{
}

Coin3DIndexedFaceSet::~Coin3DIndexedFaceSet() = default;

SoIndexedFaceSet* Coin3DIndexedFaceSet::getIndexedFaceSet() const
{
    return static_cast<SoIndexedFaceSet*>(_coinNode);
}

void Coin3DIndexedFaceSet::setCoordinates(const std::vector<Vec3f>& coords)
{
    auto* ifs = getIndexedFaceSet();
    if (!ifs) {
        return;
    }

    // 获取或创建顶点属性 / Get or create vertex property
    SoVertexProperty* vp = nullptr;
    if (ifs->vertexProperty.getValue()) {
        vp = static_cast<SoVertexProperty*>(ifs->vertexProperty.getValue());
    } else {
        vp = new SoVertexProperty();
        ifs->vertexProperty.setValue(vp);
    }

    vp->vertex.setNum(static_cast<int>(coords.size()));
    for (size_t i = 0; i < coords.size(); ++i) {
        vp->vertex.set1Value(static_cast<int>(i), coords[i].x, coords[i].y, coords[i].z);
    }

    touch();
}

std::vector<Vec3f> Coin3DIndexedFaceSet::getCoordinates() const
{
    std::vector<Vec3f> result;
    auto* ifs = getIndexedFaceSet();
    if (!ifs || !ifs->vertexProperty.getValue()) {
        return result;
    }

    auto* vp = static_cast<SoVertexProperty*>(ifs->vertexProperty.getValue());
    int num = vp->vertex.getNum();
    result.reserve(static_cast<size_t>(num));

    for (int i = 0; i < num; ++i) {
        const SbVec3f& v = vp->vertex[i];
        result.push_back(Vec3f(v[0], v[1], v[2]));
    }

    return result;
}

void Coin3DIndexedFaceSet::addCoordinate(const Vec3f& coord)
{
    auto* ifs = getIndexedFaceSet();
    if (!ifs) {
        return;
    }

    SoVertexProperty* vp = nullptr;
    if (ifs->vertexProperty.getValue()) {
        vp = static_cast<SoVertexProperty*>(ifs->vertexProperty.getValue());
    } else {
        vp = new SoVertexProperty();
        ifs->vertexProperty.setValue(vp);
    }

    int idx = vp->vertex.getNum();
    vp->vertex.set1Value(idx, coord.x, coord.y, coord.z);
    touch();
}

void Coin3DIndexedFaceSet::setNumCoordinates(int num)
{
    auto* ifs = getIndexedFaceSet();
    if (!ifs) {
        return;
    }

    SoVertexProperty* vp = nullptr;
    if (ifs->vertexProperty.getValue()) {
        vp = static_cast<SoVertexProperty*>(ifs->vertexProperty.getValue());
    } else {
        vp = new SoVertexProperty();
        ifs->vertexProperty.setValue(vp);
    }

    vp->vertex.setNum(num);
    touch();
}

void Coin3DIndexedFaceSet::setCoordIndex(const std::vector<int32_t>& indices)
{
    auto* ifs = getIndexedFaceSet();
    if (!ifs) {
        return;
    }

    ifs->coordIndex.setNum(static_cast<int>(indices.size()));
    for (size_t i = 0; i < indices.size(); ++i) {
        ifs->coordIndex.set1Value(static_cast<int>(i), indices[i]);
    }

    touch();
}

std::vector<int32_t> Coin3DIndexedFaceSet::getCoordIndex() const
{
    std::vector<int32_t> result;
    auto* ifs = getIndexedFaceSet();
    if (!ifs) {
        return result;
    }

    int num = ifs->coordIndex.getNum();
    result.reserve(static_cast<size_t>(num));

    for (int i = 0; i < num; ++i) {
        result.push_back(ifs->coordIndex[i]);
    }

    return result;
}

void Coin3DIndexedFaceSet::addFace(const std::vector<int32_t>& vertices)
{
    auto* ifs = getIndexedFaceSet();
    if (!ifs) {
        return;
    }

    int startIdx = ifs->coordIndex.getNum();

    // 添加顶点索引 / Add vertex indices
    for (const auto& v : vertices) {
        ifs->coordIndex.set1Value(startIdx++, v);
    }
    // 面结束标记 / Face end marker
    ifs->coordIndex.set1Value(startIdx, -1);

    touch();
}

void Coin3DIndexedFaceSet::setNormals(const std::vector<Vec3f>& normals)
{
    auto* ifs = getIndexedFaceSet();
    if (!ifs) {
        return;
    }

    SoVertexProperty* vp = nullptr;
    if (ifs->vertexProperty.getValue()) {
        vp = static_cast<SoVertexProperty*>(ifs->vertexProperty.getValue());
    } else {
        vp = new SoVertexProperty();
        ifs->vertexProperty.setValue(vp);
    }

    vp->normal.setNum(static_cast<int>(normals.size()));
    for (size_t i = 0; i < normals.size(); ++i) {
        vp->normal.set1Value(static_cast<int>(i), normals[i].x, normals[i].y, normals[i].z);
    }

    touch();
}

std::vector<Vec3f> Coin3DIndexedFaceSet::getNormals() const
{
    std::vector<Vec3f> result;
    auto* ifs = getIndexedFaceSet();
    if (!ifs || !ifs->vertexProperty.getValue()) {
        return result;
    }

    auto* vp = static_cast<SoVertexProperty*>(ifs->vertexProperty.getValue());
    int num = vp->normal.getNum();
    result.reserve(static_cast<size_t>(num));

    for (int i = 0; i < num; ++i) {
        const SbVec3f& v = vp->normal[i];
        result.push_back(Vec3f(v[0], v[1], v[2]));
    }

    return result;
}

void Coin3DIndexedFaceSet::setNormalIndex(const std::vector<int32_t>& indices)
{
    auto* ifs = getIndexedFaceSet();
    if (!ifs) {
        return;
    }

    ifs->normalIndex.setNum(static_cast<int>(indices.size()));
    for (size_t i = 0; i < indices.size(); ++i) {
        ifs->normalIndex.set1Value(static_cast<int>(i), indices[i]);
    }

    touch();
}

void Coin3DIndexedFaceSet::setNormalBinding(NormalBinding binding)
{
    auto* ifs = getIndexedFaceSet();
    if (!ifs || !ifs->vertexProperty.getValue()) {
        return;
    }

    auto* vp = static_cast<SoVertexProperty*>(ifs->vertexProperty.getValue());

    switch (binding) {
        case NormalBinding::PerVertex:
            vp->normalBinding.setValue(SoVertexProperty::PER_VERTEX);
            break;
        case NormalBinding::PerFace:
            vp->normalBinding.setValue(SoVertexProperty::PER_FACE);
            break;
        case NormalBinding::Overall:
            vp->normalBinding.setValue(SoVertexProperty::OVERALL);
            break;
    }

    touch();
}

void Coin3DIndexedFaceSet::setTextureCoords(const std::vector<Vec2f>& coords)
{
    auto* ifs = getIndexedFaceSet();
    if (!ifs) {
        return;
    }

    SoVertexProperty* vp = nullptr;
    if (ifs->vertexProperty.getValue()) {
        vp = static_cast<SoVertexProperty*>(ifs->vertexProperty.getValue());
    } else {
        vp = new SoVertexProperty();
        ifs->vertexProperty.setValue(vp);
    }

    vp->texCoord.setNum(static_cast<int>(coords.size()));
    for (size_t i = 0; i < coords.size(); ++i) {
        vp->texCoord.set1Value(static_cast<int>(i), coords[i].x, coords[i].y);
    }

    touch();
}

std::vector<Vec2f> Coin3DIndexedFaceSet::getTextureCoords() const
{
    std::vector<Vec2f> result;
    auto* ifs = getIndexedFaceSet();
    if (!ifs || !ifs->vertexProperty.getValue()) {
        return result;
    }

    auto* vp = static_cast<SoVertexProperty*>(ifs->vertexProperty.getValue());
    int num = vp->texCoord.getNum();
    result.reserve(static_cast<size_t>(num));

    for (int i = 0; i < num; ++i) {
        const SbVec2f& v = vp->texCoord[i];
        result.push_back(Vec2f(v[0], v[1]));
    }

    return result;
}

void Coin3DIndexedFaceSet::setMaterialIndex(const std::vector<int32_t>& indices)
{
    auto* ifs = getIndexedFaceSet();
    if (!ifs) {
        return;
    }

    ifs->materialIndex.setNum(static_cast<int>(indices.size()));
    for (size_t i = 0; i < indices.size(); ++i) {
        ifs->materialIndex.set1Value(static_cast<int>(i), indices[i]);
    }

    touch();
}

std::vector<int32_t> Coin3DIndexedFaceSet::getMaterialIndex() const
{
    std::vector<int32_t> result;
    auto* ifs = getIndexedFaceSet();
    if (!ifs) {
        return result;
    }

    int num = ifs->materialIndex.getNum();
    result.reserve(static_cast<size_t>(num));

    for (int i = 0; i < num; ++i) {
        result.push_back(ifs->materialIndex[i]);
    }

    return result;
}

void Coin3DIndexedFaceSet::calculateNormals(bool perVertex)
{
    // TODO: 实现法线计算 / Implement normal calculation
    // 可以使用 SoNormalCalculator 或手动计算
    (void)perVertex;
    Base::Console().warning("Coin3DIndexedFaceSet::calculateNormals: Not yet implemented\n");
}

int Coin3DIndexedFaceSet::getFaceCount() const
{
    auto* ifs = getIndexedFaceSet();
    if (!ifs) {
        return 0;
    }

    // 计算面数（由 -1 分隔）/ Count faces (separated by -1)
    int count = 0;
    int num = ifs->coordIndex.getNum();
    for (int i = 0; i < num; ++i) {
        if (ifs->coordIndex[i] == -1) {
            ++count;
        }
    }

    return count;
}

int Coin3DIndexedFaceSet::getVertexCount() const
{
    auto* ifs = getIndexedFaceSet();
    if (!ifs || !ifs->vertexProperty.getValue()) {
        return 0;
    }

    auto* vp = static_cast<SoVertexProperty*>(ifs->vertexProperty.getValue());
    return vp->vertex.getNum();
}

//===========================================================================
// Coin3DFaceSet - 面集包装器 / Face Set Wrapper
//===========================================================================

Coin3DFaceSet::Coin3DFaceSet()
    : Coin3DGeometry(new SoFaceSet(), true)
{
    _coinNode->ref();
}

Coin3DFaceSet::Coin3DFaceSet(SoFaceSet* fs, bool ownsNode)
    : Coin3DGeometry(fs, ownsNode)
{
}

Coin3DFaceSet::~Coin3DFaceSet() = default;

SoFaceSet* Coin3DFaceSet::getFaceSet() const
{
    return static_cast<SoFaceSet*>(_coinNode);
}

void Coin3DFaceSet::setNumVertices(const std::vector<int32_t>& numVertices)
{
    auto* fs = getFaceSet();
    if (!fs) {
        return;
    }

    fs->numVertices.setNum(static_cast<int>(numVertices.size()));
    for (size_t i = 0; i < numVertices.size(); ++i) {
        fs->numVertices.set1Value(static_cast<int>(i), numVertices[i]);
    }

    touch();
}

void Coin3DFaceSet::setCoordinates(const std::vector<Vec3f>& coords)
{
    auto* fs = getFaceSet();
    if (!fs) {
        return;
    }

    SoVertexProperty* vp = nullptr;
    if (fs->vertexProperty.getValue()) {
        vp = static_cast<SoVertexProperty*>(fs->vertexProperty.getValue());
    } else {
        vp = new SoVertexProperty();
        fs->vertexProperty.setValue(vp);
    }

    vp->vertex.setNum(static_cast<int>(coords.size()));
    for (size_t i = 0; i < coords.size(); ++i) {
        vp->vertex.set1Value(static_cast<int>(i), coords[i].x, coords[i].y, coords[i].z);
    }

    touch();
}

//===========================================================================
// Coin3DLineSet - 线集包装器 / Line Set Wrapper
//===========================================================================

Coin3DLineSet::Coin3DLineSet()
    : Coin3DGeometry(new SoLineSet(), true)
{
    _coinNode->ref();
}

Coin3DLineSet::Coin3DLineSet(SoLineSet* ls, bool ownsNode)
    : Coin3DGeometry(ls, ownsNode)
{
}

Coin3DLineSet::~Coin3DLineSet() = default;

void Coin3DLineSet::setCoordinates(const std::vector<Vec3f>& coords)
{
    auto* ls = static_cast<SoLineSet*>(_coinNode);
    if (!ls) {
        return;
    }

    SoVertexProperty* vp = nullptr;
    if (ls->vertexProperty.getValue()) {
        vp = static_cast<SoVertexProperty*>(ls->vertexProperty.getValue());
    } else {
        vp = new SoVertexProperty();
        ls->vertexProperty.setValue(vp);
    }

    vp->vertex.setNum(static_cast<int>(coords.size()));
    for (size_t i = 0; i < coords.size(); ++i) {
        vp->vertex.set1Value(static_cast<int>(i), coords[i].x, coords[i].y, coords[i].z);
    }

    touch();
}

void Coin3DLineSet::setNumVertices(const std::vector<int32_t>& numVertices)
{
    auto* ls = static_cast<SoLineSet*>(_coinNode);
    if (!ls) {
        return;
    }

    ls->numVertices.setNum(static_cast<int>(numVertices.size()));
    for (size_t i = 0; i < numVertices.size(); ++i) {
        ls->numVertices.set1Value(static_cast<int>(i), numVertices[i]);
    }

    touch();
}

//===========================================================================
// Coin3DPointSet - 点集包装器 / Point Set Wrapper
//===========================================================================

Coin3DPointSet::Coin3DPointSet()
    : Coin3DGeometry(new SoPointSet(), true)
{
    _coinNode->ref();
}

Coin3DPointSet::Coin3DPointSet(SoPointSet* ps, bool ownsNode)
    : Coin3DGeometry(ps, ownsNode)
{
}

Coin3DPointSet::~Coin3DPointSet() = default;

void Coin3DPointSet::setCoordinates(const std::vector<Vec3f>& coords)
{
    auto* ps = static_cast<SoPointSet*>(_coinNode);
    if (!ps) {
        return;
    }

    SoVertexProperty* vp = nullptr;
    if (ps->vertexProperty.getValue()) {
        vp = static_cast<SoVertexProperty*>(ps->vertexProperty.getValue());
    } else {
        vp = new SoVertexProperty();
        ps->vertexProperty.setValue(vp);
    }

    vp->vertex.setNum(static_cast<int>(coords.size()));
    for (size_t i = 0; i < coords.size(); ++i) {
        vp->vertex.set1Value(static_cast<int>(i), coords[i].x, coords[i].y, coords[i].z);
    }

    ps->numPoints = static_cast<int>(coords.size());
    touch();
}

int Coin3DPointSet::getstartIndex() const
{
    auto* ps = static_cast<SoPointSet*>(_coinNode);
    if (!ps) {
        return 0;
    }
    return ps->startIndex.getValue();
}

//===========================================================================
// GeometryBuilder - 几何构建器 / Geometry Builder
//===========================================================================

RenderNode::Ptr GeometryBuilder::createCube(float size, const Vec3f& center)
{
    auto* cube = new SoCube();
    cube->width = size;
    cube->height = size;
    cube->depth = size;

    auto sep = std::make_shared<Coin3DSeparator>();

    // 添加平移到中心 / Add translation to center
    if (center.x != 0.0f || center.y != 0.0f || center.z != 0.0f) {
        auto transform = std::make_shared<Coin3DTransform>();
        transform->setTranslation(center);
        sep->addChild(transform);
    }

    sep->addCoinChild(cube);
    return sep;
}

RenderNode::Ptr GeometryBuilder::createSphere(float radius, const Vec3f& center)
{
    auto* sphere = new SoSphere();
    sphere->radius = radius;

    auto sep = std::make_shared<Coin3DSeparator>();

    if (center.x != 0.0f || center.y != 0.0f || center.z != 0.0f) {
        auto transform = std::make_shared<Coin3DTransform>();
        transform->setTranslation(center);
        sep->addChild(transform);
    }

    sep->addCoinChild(sphere);
    return sep;
}

RenderNode::Ptr GeometryBuilder::createCylinder(float radius, float height)
{
    auto* cylinder = new SoCylinder();
    cylinder->radius = radius;
    cylinder->height = height;

    auto geom = std::make_shared<Coin3DGeometry>(cylinder, true);
    return geom;
}

RenderNode::Ptr GeometryBuilder::createCone(float radius, float height)
{
    auto* cone = new SoCone();
    cone->bottomRadius = radius;
    cone->height = height;

    auto geom = std::make_shared<Coin3DGeometry>(cone, true);
    return geom;
}

RenderNode::Ptr GeometryBuilder::createMesh(
    const std::vector<Vec3f>& vertices,
    const std::vector<std::vector<int32_t>>& faces,
    const std::vector<Vec3f>* normals)
{
    auto ifs = std::make_shared<Coin3DIndexedFaceSet>();

    // 设置顶点 / Set vertices
    ifs->setCoordinates(vertices);

    // 设置法线（如果有）/ Set normals if provided
    if (normals && !normals->empty()) {
        ifs->setNormals(*normals);
        ifs->setNormalBinding(Coin3DIndexedFaceSet::NormalBinding::PerVertex);
    }

    // 设置面索引 / Set face indices
    std::vector<int32_t> indices;
    for (const auto& face : faces) {
        for (const auto& v : face) {
            indices.push_back(v);
        }
        indices.push_back(-1);  // 面结束标记 / Face end marker
    }
    ifs->setCoordIndex(indices);

    return ifs;
}

RenderNode::Ptr GeometryBuilder::createWireframe(
    const std::vector<Vec3f>& vertices,
    const std::vector<std::pair<int32_t, int32_t>>& edges)
{
    auto sep = std::make_shared<Coin3DSeparator>();

    // 设置顶点 / Set vertices
    auto coords = new SoVertexProperty();
    coords->vertex.setNum(static_cast<int>(vertices.size()));
    for (size_t i = 0; i < vertices.size(); ++i) {
        coords->vertex.set1Value(static_cast<int>(i), vertices[i].x, vertices[i].y, vertices[i].z);
    }

    // 创建线集 / Create line set
    auto* lineSet = new SoLineSet();
    lineSet->vertexProperty.setValue(coords);

    // 设置每条线的顶点数（都是2）/ Set vertices per line (all are 2)
    std::vector<int32_t> numVertices(edges.size(), 2);
    lineSet->numVertices.setNum(static_cast<int>(edges.size()));
    for (size_t i = 0; i < edges.size(); ++i) {
        lineSet->numVertices.set1Value(static_cast<int>(i), numVertices[i]);
    }

    // TODO: 设置线索引 / Set line indices
    // Coin3D SoLineSet 需要更复杂的索引设置

    sep->addCoinChild(lineSet);
    return sep;
}

RenderNode::Ptr GeometryBuilder::createPointCloud(const std::vector<Vec3f>& points)
{
    auto pointSet = std::make_shared<Coin3DPointSet>();
    pointSet->setCoordinates(points);
    return pointSet;
}

} // namespace Render
} // namespace Gui
