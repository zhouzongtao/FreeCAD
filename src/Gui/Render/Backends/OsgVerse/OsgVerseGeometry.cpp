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
#ifndef _PreComp_
#endif

#include "OsgVerseGeometry.h"
#include <osg/TriangleIndexFunctor>
#include <osgUtil/SmoothingVisitor>
#include <osgUtil/TangentSpaceGenerator>
#include <algorithm>
#include <unordered_map>

using namespace Gui::Render;

// Constructor
OsgVerseGeometry::OsgVerseGeometry()
    : OsgVerseNode(new osg::Geometry(), true, NodeType::Geometry)
{
}

OsgVerseGeometry::OsgVerseGeometry(osg::Geometry* geometry, bool ownsNode)
    : OsgVerseNode(geometry, ownsNode, NodeType::Geometry)
{
}

OsgVerseGeometry::~OsgVerseGeometry() = default;

osg::Geometry* OsgVerseGeometry::getOsgGeometry() const
{
    return static_cast<osg::Geometry*>(getOsgNode());
}

// Vertex data methods
void OsgVerseGeometry::setVertices(const std::vector<Vec3f>& vertices)
{
    auto* geom = getOsgGeometry();
    auto* vertexArray = new osg::Vec3Array(vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        (*vertexArray)[i].set(vertices[i].x, vertices[i].y, vertices[i].z);
    }
    geom->setVertexArray(vertexArray);
}

void OsgVerseGeometry::setVertices(const float* data, size_t count)
{
    auto* geom = getOsgGeometry();
    auto* vertexArray = new osg::Vec3Array(count);
    for (size_t i = 0; i < count; ++i) {
        (*vertexArray)[i].set(data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
    }
    geom->setVertexArray(vertexArray);
}

std::vector<Vec3f> OsgVerseGeometry::getVertices() const
{
    auto* geom = getOsgGeometry();
    auto* vertexArray = dynamic_cast<osg::Vec3Array*>(geom->getVertexArray());
    if (!vertexArray) {
        return {};
    }
    
    std::vector<Vec3f> vertices(vertexArray->size());
    for (size_t i = 0; i < vertexArray->size(); ++i) {
        const auto& v = (*vertexArray)[i];
        vertices[i] = Vec3f(v.x(), v.y(), v.z());
    }
    return vertices;
}

size_t OsgVerseGeometry::getVertexCount() const
{
    auto* geom = getOsgGeometry();
    auto* vertexArray = geom->getVertexArray();
    return vertexArray ? vertexArray->getNumElements() : 0;
}

// Normal methods
void OsgVerseGeometry::setNormals(const std::vector<Vec3f>& normals)
{
    auto* geom = getOsgGeometry();
    auto* normalArray = new osg::Vec3Array(normals.size());
    for (size_t i = 0; i < normals.size(); ++i) {
        (*normalArray)[i].set(normals[i].x, normals[i].y, normals[i].z);
    }
    geom->setNormalArray(normalArray);
    geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
}

void OsgVerseGeometry::setNormals(const float* data, size_t count)
{
    auto* geom = getOsgGeometry();
    auto* normalArray = new osg::Vec3Array(count);
    for (size_t i = 0; i < count; ++i) {
        (*normalArray)[i].set(data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
    }
    geom->setNormalArray(normalArray);
    geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
}

std::vector<Vec3f> OsgVerseGeometry::getNormals() const
{
    auto* geom = getOsgGeometry();
    auto* normalArray = dynamic_cast<osg::Vec3Array*>(geom->getNormalArray());
    if (!normalArray) {
        return {};
    }
    
    std::vector<Vec3f> normals(normalArray->size());
    for (size_t i = 0; i < normalArray->size(); ++i) {
        const auto& n = (*normalArray)[i];
        normals[i] = Vec3f(n.x(), n.y(), n.z());
    }
    return normals;
}

bool OsgVerseGeometry::hasNormals() const
{
    return getOsgGeometry()->getNormalArray() != nullptr;
}

// Texture coordinate methods
void OsgVerseGeometry::setTexCoords(const std::vector<Vec2f>& texCoords, unsigned int unit)
{
    auto* geom = getOsgGeometry();
    auto* texCoordArray = new osg::Vec2Array(texCoords.size());
    for (size_t i = 0; i < texCoords.size(); ++i) {
        (*texCoordArray)[i].set(texCoords[i].x, texCoords[i].y);
    }
    geom->setTexCoordArray(unit, texCoordArray);
}

void OsgVerseGeometry::setTexCoords(const float* data, size_t count, unsigned int unit)
{
    auto* geom = getOsgGeometry();
    auto* texCoordArray = new osg::Vec2Array(count);
    for (size_t i = 0; i < count; ++i) {
        (*texCoordArray)[i].set(data[i * 2], data[i * 2 + 1]);
    }
    geom->setTexCoordArray(unit, texCoordArray);
}

std::vector<Vec2f> OsgVerseGeometry::getTexCoords(unsigned int unit) const
{
    auto* geom = getOsgGeometry();
    auto* texCoordArray = dynamic_cast<osg::Vec2Array*>(geom->getTexCoordArray(unit));
    if (!texCoordArray) {
        return {};
    }
    
    std::vector<Vec2f> texCoords(texCoordArray->size());
    for (size_t i = 0; i < texCoordArray->size(); ++i) {
        const auto& tc = (*texCoordArray)[i];
        texCoords[i] = Vec2f(tc.x(), tc.y());
    }
    return texCoords;
}

bool OsgVerseGeometry::hasTexCoords(unsigned int unit) const
{
    return getOsgGeometry()->getTexCoordArray(unit) != nullptr;
}

// Color methods
void OsgVerseGeometry::setColors(const std::vector<Color>& colors)
{
    auto* geom = getOsgGeometry();
    auto* colorArray = new osg::Vec4Array(colors.size());
    for (size_t i = 0; i < colors.size(); ++i) {
        (*colorArray)[i].set(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
    }
    geom->setColorArray(colorArray);
    geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
}

void OsgVerseGeometry::setColors(const float* data, size_t count)
{
    auto* geom = getOsgGeometry();
    auto* colorArray = new osg::Vec4Array(count);
    for (size_t i = 0; i < count; ++i) {
        (*colorArray)[i].set(data[i * 4], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]);
    }
    geom->setColorArray(colorArray);
    geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
}

std::vector<Color> OsgVerseGeometry::getColors() const
{
    auto* geom = getOsgGeometry();
    auto* colorArray = dynamic_cast<osg::Vec4Array*>(geom->getColorArray());
    if (!colorArray) {
        return {};
    }
    
    std::vector<Color> colors(colorArray->size());
    for (size_t i = 0; i < colorArray->size(); ++i) {
        const auto& c = (*colorArray)[i];
        colors[i] = Color(c.r(), c.g(), c.b(), c.a());
    }
    return colors;
}

bool OsgVerseGeometry::hasColors() const
{
    return getOsgGeometry()->getColorArray() != nullptr;
}

// Tangent and bitangent methods
void OsgVerseGeometry::setTangents(const std::vector<Vec3f>& tangents)
{
    auto* geom = getOsgGeometry();
    auto* tangentArray = new osg::Vec4Array(tangents.size());
    for (size_t i = 0; i < tangents.size(); ++i) {
        (*tangentArray)[i].set(tangents[i].x, tangents[i].y, tangents[i].z, 1.0f);
    }
    geom->setVertexAttribArray(6, tangentArray);
    geom->setVertexAttribBinding(6, osg::Geometry::BIND_PER_VERTEX);
}

std::vector<Vec3f> OsgVerseGeometry::getTangents() const
{
    auto* geom = getOsgGeometry();
    auto* tangentArray = dynamic_cast<osg::Vec4Array*>(geom->getVertexAttribArray(6));
    if (!tangentArray) {
        return {};
    }
    
    std::vector<Vec3f> tangents(tangentArray->size());
    for (size_t i = 0; i < tangentArray->size(); ++i) {
        const auto& t = (*tangentArray)[i];
        tangents[i] = Vec3f(t.x(), t.y(), t.z());
    }
    return tangents;
}

bool OsgVerseGeometry::hasTangents() const
{
    return getOsgGeometry()->getVertexAttribArray(6) != nullptr;
}

void OsgVerseGeometry::setBitangents(const std::vector<Vec3f>& bitangents)
{
    auto* geom = getOsgGeometry();
    auto* bitangentArray = new osg::Vec3Array(bitangents.size());
    for (size_t i = 0; i < bitangents.size(); ++i) {
        (*bitangentArray)[i].set(bitangents[i].x, bitangents[i].y, bitangents[i].z);
    }
    geom->setVertexAttribArray(7, bitangentArray);
    geom->setVertexAttribBinding(7, osg::Geometry::BIND_PER_VERTEX);
}

std::vector<Vec3f> OsgVerseGeometry::getBitangents() const
{
    auto* geom = getOsgGeometry();
    auto* bitangentArray = dynamic_cast<osg::Vec3Array*>(geom->getVertexAttribArray(7));
    if (!bitangentArray) {
        return {};
    }
    
    std::vector<Vec3f> bitangents(bitangentArray->size());
    for (size_t i = 0; i < bitangentArray->size(); ++i) {
        const auto& b = (*bitangentArray)[i];
        bitangents[i] = Vec3f(b.x(), b.y(), b.z());
    }
    return bitangents;
}

bool OsgVerseGeometry::hasBitangents() const
{
    return getOsgGeometry()->getVertexAttribArray(7) != nullptr;
}

// Index methods
void OsgVerseGeometry::setIndices(const std::vector<uint32_t>& indices)
{
    auto* geom = getOsgGeometry();
    auto* indexArray = new osg::DrawElementsUInt(primitiveTypeToGL(_primitiveType), indices.size());
    for (size_t i = 0; i < indices.size(); ++i) {
        (*indexArray)[i] = indices[i];
    }
    geom->addPrimitiveSet(indexArray);
}

void OsgVerseGeometry::setIndices(const uint32_t* data, size_t count)
{
    auto* geom = getOsgGeometry();
    auto* indexArray = new osg::DrawElementsUInt(primitiveTypeToGL(_primitiveType), count);
    for (size_t i = 0; i < count; ++i) {
        (*indexArray)[i] = data[i];
    }
    geom->addPrimitiveSet(indexArray);
}

std::vector<uint32_t> OsgVerseGeometry::getIndices() const
{
    auto* geom = getOsgGeometry();
    if (geom->getNumPrimitiveSets() == 0) {
        return {};
    }
    
    std::vector<uint32_t> indices;
    for (unsigned int i = 0; i < geom->getNumPrimitiveSets(); ++i) {
        auto* primitiveSet = geom->getPrimitiveSet(i);
        auto* drawElements = dynamic_cast<osg::DrawElementsUInt*>(primitiveSet);
        if (drawElements) {
            for (unsigned int j = 0; j < drawElements->size(); ++j) {
                indices.push_back((*drawElements)[j]);
            }
        }
    }
    return indices;
}

size_t OsgVerseGeometry::getIndexCount() const
{
    auto* geom = getOsgGeometry();
    size_t count = 0;
    for (unsigned int i = 0; i < geom->getNumPrimitiveSets(); ++i) {
        count += geom->getPrimitiveSet(i)->getNumIndices();
    }
    return count;
}

bool OsgVerseGeometry::hasIndices() const
{
    return getOsgGeometry()->getNumPrimitiveSets() > 0;
}

// Primitive type methods
void OsgVerseGeometry::setPrimitiveType(PrimitiveType type)
{
    _primitiveType = type;
}

OsgVerseGeometry::PrimitiveType OsgVerseGeometry::getPrimitiveType() const
{
    return _primitiveType;
}

void OsgVerseGeometry::addPrimitiveSet(PrimitiveType type, unsigned int first, unsigned int count)
{
    auto* geom = getOsgGeometry();
    GLenum mode = primitiveTypeToGL(type);
    geom->addPrimitiveSet(new osg::DrawArrays(mode, first, count));
}

void OsgVerseGeometry::clearPrimitiveSets()
{
    auto* geom = getOsgGeometry();
    geom->removePrimitiveSet(0, geom->getNumPrimitiveSets());
}

// Geometry operations
void OsgVerseGeometry::computeNormals()
{
    auto* geom = getOsgGeometry();
    osgUtil::SmoothingVisitor sv;
    sv.smooth(*geom);
}

void OsgVerseGeometry::computeTangents()
{
    auto* geom = getOsgGeometry();
    osg::ref_ptr<osgUtil::TangentSpaceGenerator> tsg = new osgUtil::TangentSpaceGenerator();
    tsg->generate(geom);
    
    geom->setVertexAttribArray(6, tsg->getTangentArray());
    geom->setVertexAttribBinding(6, osg::Geometry::BIND_PER_VERTEX);
    geom->setVertexAttribArray(7, tsg->getBinormalArray());
    geom->setVertexAttribBinding(7, osg::Geometry::BIND_PER_VERTEX);
}

BoundingBox OsgVerseGeometry::computeBoundingBox() const
{
    auto* geom = getOsgGeometry();
    const osg::BoundingBox& bb = geom->getBoundingBox();
    
    BoundingBox bbox;
    bbox.MinX = bb.xMin();
    bbox.MinY = bb.yMin();
    bbox.MinZ = bb.zMin();
    bbox.MaxX = bb.xMax();
    bbox.MaxY = bb.yMax();
    bbox.MaxZ = bb.zMax();
    return bbox;
}

void OsgVerseGeometry::optimize()
{
    // Remove duplicate vertices and optimize indices
    auto vertices = getVertices();
    auto normals = getNormals();
    auto texCoords = getTexCoords();
    auto colors = getColors();
    auto indices = getIndices();
    
    if (vertices.empty()) {
        return;
    }
    
    // Build vertex map to find duplicates
    std::unordered_map<size_t, uint32_t> vertexMap;
    std::vector<Vec3f> uniqueVertices;
    std::vector<Vec3f> uniqueNormals;
    std::vector<Vec2f> uniqueTexCoords;
    std::vector<Color> uniqueColors;
    std::vector<uint32_t> newIndices;
    
    bool hasNorm = !normals.empty();
    bool hasTex = !texCoords.empty();
    bool hasCol = !colors.empty();
    
    auto hashVertex = [&](size_t idx) -> size_t {
        size_t hash = 0;
        const auto& v = vertices[idx];
        hash ^= std::hash<float>{}(v.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<float>{}(v.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<float>{}(v.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        return hash;
    };
    
    for (size_t i = 0; i < indices.size(); ++i) {
        uint32_t idx = indices[i];
        size_t hash = hashVertex(idx);
        
        auto it = vertexMap.find(hash);
        if (it != vertexMap.end()) {
            newIndices.push_back(it->second);
        } else {
            uint32_t newIdx = static_cast<uint32_t>(uniqueVertices.size());
            vertexMap[hash] = newIdx;
            uniqueVertices.push_back(vertices[idx]);
            if (hasNorm) uniqueNormals.push_back(normals[idx]);
            if (hasTex) uniqueTexCoords.push_back(texCoords[idx]);
            if (hasCol) uniqueColors.push_back(colors[idx]);
            newIndices.push_back(newIdx);
        }
    }
    
    // Update geometry with optimized data
    clear();
    setVertices(uniqueVertices);
    if (hasNorm) setNormals(uniqueNormals);
    if (hasTex) setTexCoords(uniqueTexCoords);
    if (hasCol) setColors(uniqueColors);
    setIndices(newIndices);
}

void OsgVerseGeometry::reverseNormals()
{
    auto normals = getNormals();
    for (auto& n : normals) {
        n.x = -n.x;
        n.y = -n.y;
        n.z = -n.z;
    }
    setNormals(normals);
}

void OsgVerseGeometry::reverseWinding()
{
    auto indices = getIndices();
    if (_primitiveType == PrimitiveType::Triangles) {
        for (size_t i = 0; i < indices.size(); i += 3) {
            std::swap(indices[i + 1], indices[i + 2]);
        }
        clearPrimitiveSets();
        setIndices(indices);
    }
}

// Coin3D compatibility
void OsgVerseGeometry::setFromCoin3DGeometry(const void* coinGeometry)
{
    // TODO: Implement Coin3D geometry conversion
    // This requires access to Coin3D headers and data structures
}

// Utility methods
osg::Geode* OsgVerseGeometry::createGeode() const
{
    auto* geode = new osg::Geode();
    geode->addDrawable(getOsgGeometry());
    return geode;
}

RenderNode::Ptr OsgVerseGeometry::clone() const
{
    auto* geom = getOsgGeometry();
    auto* clonedGeom = dynamic_cast<osg::Geometry*>(geom->clone(osg::CopyOp::DEEP_COPY_ALL));
    return std::make_shared<OsgVerseGeometry>(clonedGeom, true);
}

void OsgVerseGeometry::clear()
{
    auto* geom = getOsgGeometry();
    geom->setVertexArray(nullptr);
    geom->setNormalArray(nullptr);
    geom->setColorArray(nullptr);
    for (unsigned int i = 0; i < 8; ++i) {
        geom->setTexCoordArray(i, nullptr);
    }
    geom->removePrimitiveSet(0, geom->getNumPrimitiveSets());
}

size_t OsgVerseGeometry::getMemoryUsage() const
{
    size_t usage = 0;
    auto* geom = getOsgGeometry();
    
    if (auto* vertexArray = geom->getVertexArray()) {
        usage += vertexArray->getTotalDataSize();
    }
    if (auto* normalArray = geom->getNormalArray()) {
        usage += normalArray->getTotalDataSize();
    }
    if (auto* colorArray = geom->getColorArray()) {
        usage += colorArray->getTotalDataSize();
    }
    for (unsigned int i = 0; i < 8; ++i) {
        if (auto* texCoordArray = geom->getTexCoordArray(i)) {
            usage += texCoordArray->getTotalDataSize();
        }
    }
    
    return usage;
}

// Private helper methods
GLenum OsgVerseGeometry::primitiveTypeToGL(PrimitiveType type) const
{
    switch (type) {
        case PrimitiveType::Points:         return GL_POINTS;
        case PrimitiveType::Lines:          return GL_LINES;
        case PrimitiveType::LineStrip:      return GL_LINE_STRIP;
        case PrimitiveType::LineLoop:       return GL_LINE_LOOP;
        case PrimitiveType::Triangles:      return GL_TRIANGLES;
        case PrimitiveType::TriangleStrip:  return GL_TRIANGLE_STRIP;
        case PrimitiveType::TriangleFan:    return GL_TRIANGLE_FAN;
        case PrimitiveType::Quads:          return GL_QUADS;
        case PrimitiveType::QuadStrip:      return GL_QUAD_STRIP;
        case PrimitiveType::Polygon:        return GL_POLYGON;
        default:                            return GL_TRIANGLES;
    }
}

OsgVerseGeometry::PrimitiveType OsgVerseGeometry::glToPrimitiveType(GLenum mode) const
{
    switch (mode) {
        case GL_POINTS:         return PrimitiveType::Points;
        case GL_LINES:          return PrimitiveType::Lines;
        case GL_LINE_STRIP:     return PrimitiveType::LineStrip;
        case GL_LINE_LOOP:      return PrimitiveType::LineLoop;
        case GL_TRIANGLES:      return PrimitiveType::Triangles;
        case GL_TRIANGLE_STRIP: return PrimitiveType::TriangleStrip;
        case GL_TRIANGLE_FAN:   return PrimitiveType::TriangleFan;
        case GL_QUADS:          return PrimitiveType::Quads;
        case GL_QUAD_STRIP:     return PrimitiveType::QuadStrip;
        case GL_POLYGON:        return PrimitiveType::Polygon;
        default:                return PrimitiveType::Triangles;
    }
}

void OsgVerseGeometry::updateGeometry()
{
    // Mark geometry as dirty to trigger updates
    auto* geom = getOsgGeometry();
    geom->dirtyBound();
    geom->dirtyDisplayList();
}

//===========================================================================
// OsgVerseGeometryBuilder Implementation
//===========================================================================

std::unique_ptr<OsgVerseGeometry> OsgVerseGeometryBuilder::createCube(float size)
{
    auto geom = std::unique_ptr<OsgVerseGeometry>(new OsgVerseGeometry());
    float halfSize = size * 0.5f;

    std::vector<Vec3f> vertices = {
        Vec3f(-halfSize, -halfSize, -halfSize), Vec3f(halfSize, -halfSize, -halfSize),
        Vec3f(halfSize, halfSize, -halfSize), Vec3f(-halfSize, halfSize, -halfSize),
        Vec3f(-halfSize, -halfSize, halfSize), Vec3f(halfSize, -halfSize, halfSize),
        Vec3f(halfSize, halfSize, halfSize), Vec3f(-halfSize, halfSize, halfSize)
    };

    std::vector<uint32_t> indices = {
        0, 1, 2, 0, 2, 3, 5, 4, 7, 5, 7, 6,
        4, 0, 3, 4, 3, 7, 1, 5, 6, 1, 6, 2,
        3, 2, 6, 3, 6, 7, 4, 5, 1, 4, 1, 0
    };

    geom->setVertices(vertices);
    geom->setIndices(indices);
    geom->setPrimitiveType(OsgVerseGeometry::PrimitiveType::Triangles);
    geom->computeNormals();
    return geom;
}

std::unique_ptr<OsgVerseGeometry> OsgVerseGeometryBuilder::createSphere(float radius, unsigned int segments)
{
    auto geom = std::unique_ptr<OsgVerseGeometry>(new OsgVerseGeometry());
    std::vector<Vec3f> vertices;
    std::vector<Vec3f> normals;
    std::vector<Vec2f> texCoords;
    std::vector<uint32_t> indices;

    unsigned int rings = segments / 2;
    for (unsigned int ring = 0; ring <= rings; ++ring) {
        float phi = static_cast<float>(ring) / static_cast<float>(rings) * M_PI;
        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);

        for (unsigned int seg = 0; seg <= segments; ++seg) {
            float theta = static_cast<float>(seg) / static_cast<float>(segments) * 2.0f * M_PI;
            Vec3f normal = Vec3f(sinPhi * std::cos(theta), sinPhi * std::sin(theta), cosPhi);
            vertices.push_back(Vec3f(radius * normal.x, radius * normal.y, radius * normal.z));
            normals.push_back(normal);
            texCoords.push_back({static_cast<float>(seg) / segments, static_cast<float>(ring) / rings});
        }
    }

    for (unsigned int ring = 0; ring < rings; ++ring) {
        for (unsigned int seg = 0; seg < segments; ++seg) {
            uint32_t current = ring * (segments + 1) + seg;
            uint32_t next = current + segments + 1;
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    geom->setVertices(vertices);
    geom->setNormals(normals);
    geom->setTexCoords(texCoords);
    geom->setIndices(indices);
    geom->setPrimitiveType(OsgVerseGeometry::PrimitiveType::Triangles);
    return geom;
}

std::unique_ptr<OsgVerseGeometry> OsgVerseGeometryBuilder::createCylinder(float radius, float height, unsigned int segments)
{
    auto geom = std::unique_ptr<OsgVerseGeometry>(new OsgVerseGeometry());
    std::vector<Vec3f> vertices;
    std::vector<Vec3f> normals;
    std::vector<uint32_t> indices;
    float halfHeight = height * 0.5f;

    for (unsigned int i = 0; i <= segments; ++i) {
        float theta = static_cast<float>(i) / segments * 2.0f * M_PI;
        Vec3f normal = Vec3f(std::cos(theta), std::sin(theta), 0.0f);
        vertices.push_back(Vec3f(radius * normal.x, radius * normal.y, -halfHeight));
        normals.push_back(normal);
        vertices.push_back(Vec3f(radius * normal.x, radius * normal.y, halfHeight));
        normals.push_back(normal);
    }

    for (unsigned int i = 0; i < segments; ++i) {
        uint32_t base = i * 2;
        indices.push_back(base);
        indices.push_back(base + 2);
        indices.push_back(base + 1);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    geom->setVertices(vertices);
    geom->setNormals(normals);
    geom->setIndices(indices);
    geom->setPrimitiveType(OsgVerseGeometry::PrimitiveType::Triangles);
    return geom;
}

std::unique_ptr<OsgVerseGeometry> OsgVerseGeometryBuilder::createCone(float radius, float height, unsigned int segments)
{
    auto geom = std::unique_ptr<OsgVerseGeometry>(new OsgVerseGeometry());
    std::vector<Vec3f> vertices;
    std::vector<uint32_t> indices;
    float halfHeight = height * 0.5f;

    vertices.push_back(Vec3f(0.0f, 0.0f, halfHeight));
    for (unsigned int i = 0; i <= segments; ++i) {
        float theta = static_cast<float>(i) / segments * 2.0f * M_PI;
        vertices.push_back(Vec3f(radius * std::cos(theta), radius * std::sin(theta), -halfHeight));
    }

    for (unsigned int i = 0; i < segments; ++i) {
        indices.push_back(0);
        indices.push_back(i + 1);
        indices.push_back(i + 2);
    }

    geom->setVertices(vertices);
    geom->setIndices(indices);
    geom->setPrimitiveType(OsgVerseGeometry::PrimitiveType::Triangles);
    geom->computeNormals();
    return geom;
}

std::unique_ptr<OsgVerseGeometry> OsgVerseGeometryBuilder::createPlane(float width, float height, unsigned int widthSegments, unsigned int heightSegments)
{
    auto geom = std::unique_ptr<OsgVerseGeometry>(new OsgVerseGeometry());
    std::vector<Vec3f> vertices;
    std::vector<Vec3f> normals;
    std::vector<Vec2f> texCoords;
    std::vector<uint32_t> indices;

    for (unsigned int y = 0; y <= heightSegments; ++y) {
        for (unsigned int x = 0; x <= widthSegments; ++x) {
            float u = static_cast<float>(x) / widthSegments;
            float v = static_cast<float>(y) / heightSegments;
            vertices.push_back(Vec3f((u - 0.5f) * width, (v - 0.5f) * height, 0.0f));
            normals.push_back(Vec3f(0.0f, 0.0f, 1.0f));
            texCoords.push_back({u, v});
        }
    }

    for (unsigned int y = 0; y < heightSegments; ++y) {
        for (unsigned int x = 0; x < widthSegments; ++x) {
            uint32_t base = y * (widthSegments + 1) + x;
            indices.push_back(base);
            indices.push_back(base + widthSegments + 1);
            indices.push_back(base + 1);
            indices.push_back(base + 1);
            indices.push_back(base + widthSegments + 1);
            indices.push_back(base + widthSegments + 2);
        }
    }

    geom->setVertices(vertices);
    geom->setNormals(normals);
    geom->setTexCoords(texCoords);
    geom->setIndices(indices);
    geom->setPrimitiveType(OsgVerseGeometry::PrimitiveType::Triangles);
    return geom;
}

std::unique_ptr<OsgVerseGeometry> OsgVerseGeometryBuilder::createTorus(float majorRadius, float minorRadius, unsigned int majorSegments, unsigned int minorSegments)
{
    auto geom = std::unique_ptr<OsgVerseGeometry>(new OsgVerseGeometry());
    std::vector<Vec3f> vertices;
    std::vector<Vec3f> normals;
    std::vector<uint32_t> indices;

    for (unsigned int i = 0; i <= majorSegments; ++i) {
        float theta = static_cast<float>(i) / majorSegments * 2.0f * M_PI;
        float cosTheta = std::cos(theta);
        float sinTheta = std::sin(theta);

        for (unsigned int j = 0; j <= minorSegments; ++j) {
            float phi = static_cast<float>(j) / minorSegments * 2.0f * M_PI;
            float cosPhi = std::cos(phi);
            float sinPhi = std::sin(phi);

            Vec3f normal = Vec3f(cosTheta * cosPhi, sinTheta * cosPhi, sinPhi);
            Vec3f vertex = Vec3f(
                (majorRadius + minorRadius * cosPhi) * cosTheta,
                (majorRadius + minorRadius * cosPhi) * sinTheta,
                minorRadius * sinPhi
            );

            vertices.push_back(vertex);
            normals.push_back(normal);
        }
    }

    for (unsigned int i = 0; i < majorSegments; ++i) {
        for (unsigned int j = 0; j < minorSegments; ++j) {
            uint32_t current = i * (minorSegments + 1) + j;
            uint32_t next = current + minorSegments + 1;
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    geom->setVertices(vertices);
    geom->setNormals(normals);
    geom->setIndices(indices);
    geom->setPrimitiveType(OsgVerseGeometry::PrimitiveType::Triangles);
    return geom;
}

std::unique_ptr<OsgVerseGeometry> OsgVerseGeometryBuilder::createFromPointCloud(const std::vector<Vec3f>& points, const std::vector<Color>* colors)
{
    auto geom = std::unique_ptr<OsgVerseGeometry>(new OsgVerseGeometry());
    geom->setVertices(points);
    if (colors && !colors->empty()) {
        geom->setColors(*colors);
    }
    geom->setPrimitiveType(OsgVerseGeometry::PrimitiveType::Points);
    geom->addPrimitiveSet(OsgVerseGeometry::PrimitiveType::Points, 0, points.size());
    return geom;
}

std::unique_ptr<OsgVerseGeometry> OsgVerseGeometryBuilder::createFromTriangleMesh(
    const std::vector<Vec3f>& vertices,
    const std::vector<uint32_t>& indices,
    const std::vector<Vec3f>* normals,
    const std::vector<Vec2f>* texCoords)
{
    auto geom = std::unique_ptr<OsgVerseGeometry>(new OsgVerseGeometry());
    geom->setVertices(vertices);
    geom->setIndices(indices);

    if (normals && !normals->empty()) {
        geom->setNormals(*normals);
    } else {
        geom->computeNormals();
    }

    if (texCoords && !texCoords->empty()) {
        geom->setTexCoords(*texCoords);
    }

    geom->setPrimitiveType(OsgVerseGeometry::PrimitiveType::Triangles);
    return geom;
}

