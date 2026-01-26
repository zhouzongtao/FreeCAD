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

#include "GeometryNodeBuilder.h"

#include <Base/Console.h>

// Coin3D 后端头文件 / Coin3D backend headers
#ifdef RENDER_HAS_COIN3D_BACKEND
#include "../Backends/Coin3D/Coin3DNode.h"
#include "../Backends/Coin3D/Coin3DMaterial.h"
#include "../Backends/Coin3D/Coin3DGeometry.h"
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoShapeHints.h>
#endif

namespace Gui {
namespace Render {

GeometryNodeBuilder::GeometryNodeBuilder(RenderNodeFactory* factory)
    : _factory(factory)
{
}

RenderNode::Ptr GeometryNodeBuilder::buildShadedNode(const GeometryData& data)
{
    if (!_factory || !data.hasFaces()) {
        return nullptr;
    }

    // 创建分隔节点作为容器 / Create separator as container
    auto separator = _factory->createSeparator();
    if (!separator) {
        return nullptr;
    }

    // 根据后端类型填充节点 / Fill nodes based on backend type
    if (_factory->getBackendType() == BackendType::Coin3D) {
#ifdef RENDER_HAS_COIN3D_BACKEND
        auto* coinSep = dynamic_cast<Coin3DSeparator*>(separator.get());
        if (!coinSep) {
            return nullptr;
        }

        SoSeparator* soSep = coinSep->getSoSeparator();

        // 添加形状提示（法线方向和剔除）/ Add shape hints (normals and culling)
        auto* hints = new SoShapeHints();
        hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
        hints->shapeType = _options.enableCulling ?
                          SoShapeHints::SOLID : SoShapeHints::UNKNOWN_SHAPE_TYPE;
        soSep->addChild(hints);

        // 添加材质 / Add material
        auto* material = new SoMaterial();
        material->diffuseColor.setValue(_options.diffuseColor.r,
                                        _options.diffuseColor.g,
                                        _options.diffuseColor.b);
        material->transparency = _options.transparency;
        soSep->addChild(material);

        // 添加坐标 / Add coordinates
        auto* coords = new SoCoordinate3();
        coords->point.setNum(static_cast<int>(data.getVertexCount()));
        SbVec3f* coordPtr = coords->point.startEditing();
        for (size_t i = 0; i < data.getVertexCount(); ++i) {
            coordPtr[i].setValue(data.vertices[i * 3],
                                data.vertices[i * 3 + 1],
                                data.vertices[i * 3 + 2]);
        }
        coords->point.finishEditing();
        soSep->addChild(coords);

        // 添加法线（如果有）/ Add normals (if available)
        if (_options.useVertexNormals && !data.normals.empty()) {
            auto* normals = new SoNormal();
            normals->vector.setNum(static_cast<int>(data.normals.size() / 3));
            SbVec3f* normalPtr = normals->vector.startEditing();
            for (size_t i = 0; i < data.normals.size() / 3; ++i) {
                normalPtr[i].setValue(data.normals[i * 3],
                                     data.normals[i * 3 + 1],
                                     data.normals[i * 3 + 2]);
            }
            normals->vector.finishEditing();
            soSep->addChild(normals);

            // 法线绑定 / Normal binding
            auto* normalBinding = new SoNormalBinding();
            switch (data.normalBinding) {
                case GeometryData::NormalBinding::PerVertex:
                    normalBinding->value = SoNormalBinding::PER_VERTEX_INDEXED;
                    break;
                case GeometryData::NormalBinding::PerFace:
                    normalBinding->value = SoNormalBinding::PER_FACE;
                    break;
                default:
                    normalBinding->value = SoNormalBinding::PER_VERTEX_INDEXED;
                    break;
            }
            soSep->addChild(normalBinding);
        }

        // 添加索引面集 / Add indexed face set
        auto* faceSet = new SoIndexedFaceSet();
        faceSet->coordIndex.setNum(static_cast<int>(data.faceIndices.size()));
        int32_t* indexPtr = faceSet->coordIndex.startEditing();
        for (size_t i = 0; i < data.faceIndices.size(); ++i) {
            indexPtr[i] = data.faceIndices[i];
        }
        faceSet->coordIndex.finishEditing();
        soSep->addChild(faceSet);
#endif
    }
    // TODO: 添加 OsgVerse 后端支持
    // TODO: Add OsgVerse backend support

    return separator;
}

RenderNode::Ptr GeometryNodeBuilder::buildWireframeNode(const GeometryData& data)
{
    if (!_factory || !data.hasLines()) {
        return nullptr;
    }

    auto separator = _factory->createSeparator();
    if (!separator) {
        return nullptr;
    }

    if (_factory->getBackendType() == BackendType::Coin3D) {
#ifdef RENDER_HAS_COIN3D_BACKEND
        auto* coinSep = dynamic_cast<Coin3DSeparator*>(separator.get());
        if (!coinSep) {
            return nullptr;
        }

        SoSeparator* soSep = coinSep->getSoSeparator();

        // 添加绘制样式 / Add draw style
        auto* drawStyle = new SoDrawStyle();
        drawStyle->lineWidth = _options.lineWidth;
        soSep->addChild(drawStyle);

        // 添加材质（线颜色）/ Add material (line color)
        auto* material = new SoMaterial();
        material->diffuseColor.setValue(_options.lineColor.r,
                                        _options.lineColor.g,
                                        _options.lineColor.b);
        soSep->addChild(material);

        // 添加坐标 / Add coordinates
        auto* coords = new SoCoordinate3();
        coords->point.setNum(static_cast<int>(data.getVertexCount()));
        SbVec3f* coordPtr = coords->point.startEditing();
        for (size_t i = 0; i < data.getVertexCount(); ++i) {
            coordPtr[i].setValue(data.vertices[i * 3],
                                data.vertices[i * 3 + 1],
                                data.vertices[i * 3 + 2]);
        }
        coords->point.finishEditing();
        soSep->addChild(coords);

        // 添加索引线集 / Add indexed line set
        auto* lineSet = new SoIndexedLineSet();
        lineSet->coordIndex.setNum(static_cast<int>(data.lineIndices.size()));
        int32_t* indexPtr = lineSet->coordIndex.startEditing();
        for (size_t i = 0; i < data.lineIndices.size(); ++i) {
            indexPtr[i] = data.lineIndices[i];
        }
        lineSet->coordIndex.finishEditing();
        soSep->addChild(lineSet);
#endif
    }

    return separator;
}

RenderNode::Ptr GeometryNodeBuilder::buildPointsNode(const GeometryData& data)
{
    if (!_factory || !data.hasPoints()) {
        return nullptr;
    }

    auto separator = _factory->createSeparator();
    if (!separator) {
        return nullptr;
    }

    if (_factory->getBackendType() == BackendType::Coin3D) {
#ifdef RENDER_HAS_COIN3D_BACKEND
        auto* coinSep = dynamic_cast<Coin3DSeparator*>(separator.get());
        if (!coinSep) {
            return nullptr;
        }

        SoSeparator* soSep = coinSep->getSoSeparator();

        // 添加绘制样式 / Add draw style
        auto* drawStyle = new SoDrawStyle();
        drawStyle->pointSize = _options.pointSize;
        soSep->addChild(drawStyle);

        // 添加材质（点颜色）/ Add material (point color)
        auto* material = new SoMaterial();
        material->diffuseColor.setValue(_options.pointColor.r,
                                        _options.pointColor.g,
                                        _options.pointColor.b);
        soSep->addChild(material);

        // 添加坐标 / Add coordinates
        auto* coords = new SoCoordinate3();
        // 如果有点索引，只添加索引指定的点；否则添加所有顶点
        // If there are point indices, add only indexed points; otherwise add all vertices
        if (!data.pointIndices.empty()) {
            coords->point.setNum(static_cast<int>(data.pointIndices.size()));
            SbVec3f* coordPtr = coords->point.startEditing();
            for (size_t i = 0; i < data.pointIndices.size(); ++i) {
                int32_t idx = data.pointIndices[i];
                coordPtr[i].setValue(data.vertices[idx * 3],
                                    data.vertices[idx * 3 + 1],
                                    data.vertices[idx * 3 + 2]);
            }
            coords->point.finishEditing();
        } else {
            coords->point.setNum(static_cast<int>(data.getVertexCount()));
            SbVec3f* coordPtr = coords->point.startEditing();
            for (size_t i = 0; i < data.getVertexCount(); ++i) {
                coordPtr[i].setValue(data.vertices[i * 3],
                                    data.vertices[i * 3 + 1],
                                    data.vertices[i * 3 + 2]);
            }
            coords->point.finishEditing();
        }
        soSep->addChild(coords);

        // 添加点集 / Add point set
        auto* pointSet = new SoPointSet();
        soSep->addChild(pointSet);
#endif
    }

    return separator;
}

RenderNode::Ptr GeometryNodeBuilder::buildFlatLinesNode(const GeometryData& data)
{
    if (!_factory) {
        return nullptr;
    }

    auto separator = _factory->createSeparator();
    if (!separator) {
        return nullptr;
    }

    if (_factory->getBackendType() == BackendType::Coin3D) {
#ifdef RENDER_HAS_COIN3D_BACKEND
        auto* coinSep = dynamic_cast<Coin3DSeparator*>(separator.get());
        if (!coinSep) {
            return nullptr;
        }

        SoSeparator* soSep = coinSep->getSoSeparator();

        // 添加坐标（共享）/ Add coordinates (shared)
        auto* coords = new SoCoordinate3();
        coords->point.setNum(static_cast<int>(data.getVertexCount()));
        SbVec3f* coordPtr = coords->point.startEditing();
        for (size_t i = 0; i < data.getVertexCount(); ++i) {
            coordPtr[i].setValue(data.vertices[i * 3],
                                data.vertices[i * 3 + 1],
                                data.vertices[i * 3 + 2]);
        }
        coords->point.finishEditing();
        soSep->addChild(coords);

        // 1. 添加着色面 / Add shaded faces
        if (data.hasFaces()) {
            auto* faceSep = new SoSeparator();

            // 形状提示 / Shape hints
            auto* hints = new SoShapeHints();
            hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
            hints->shapeType = _options.enableCulling ?
                              SoShapeHints::SOLID : SoShapeHints::UNKNOWN_SHAPE_TYPE;
            faceSep->addChild(hints);

            // 材质 / Material
            auto* material = new SoMaterial();
            material->diffuseColor.setValue(_options.diffuseColor.r,
                                            _options.diffuseColor.g,
                                            _options.diffuseColor.b);
            material->transparency = _options.transparency;
            faceSep->addChild(material);

            // 法线（如果有）/ Normals (if available)
            if (_options.useVertexNormals && !data.normals.empty()) {
                auto* normals = new SoNormal();
                normals->vector.setNum(static_cast<int>(data.normals.size() / 3));
                SbVec3f* normalPtr = normals->vector.startEditing();
                for (size_t i = 0; i < data.normals.size() / 3; ++i) {
                    normalPtr[i].setValue(data.normals[i * 3],
                                         data.normals[i * 3 + 1],
                                         data.normals[i * 3 + 2]);
                }
                normals->vector.finishEditing();
                faceSep->addChild(normals);

                auto* normalBinding = new SoNormalBinding();
                normalBinding->value = SoNormalBinding::PER_VERTEX_INDEXED;
                faceSep->addChild(normalBinding);
            }

            // 多边形偏移（避免 Z-fighting）/ Polygon offset (avoid Z-fighting)
            auto* offset = new SoPolygonOffset();
            offset->factor = 1.0f;
            offset->units = 1.0f;
            offset->styles = SoPolygonOffset::FILLED;
            offset->on = TRUE;
            faceSep->addChild(offset);

            // 索引面集 / Indexed face set
            auto* faceSet = new SoIndexedFaceSet();
            faceSet->coordIndex.setNum(static_cast<int>(data.faceIndices.size()));
            int32_t* indexPtr = faceSet->coordIndex.startEditing();
            for (size_t i = 0; i < data.faceIndices.size(); ++i) {
                indexPtr[i] = data.faceIndices[i];
            }
            faceSet->coordIndex.finishEditing();
            faceSep->addChild(faceSet);

            soSep->addChild(faceSep);
        }

        // 2. 添加边线 / Add edge lines
        if (data.hasLines()) {
            auto* lineSep = new SoSeparator();

            // 绘制样式 / Draw style
            auto* drawStyle = new SoDrawStyle();
            drawStyle->lineWidth = _options.lineWidth;
            lineSep->addChild(drawStyle);

            // 材质（线颜色）/ Material (line color)
            auto* material = new SoMaterial();
            material->diffuseColor.setValue(_options.lineColor.r,
                                            _options.lineColor.g,
                                            _options.lineColor.b);
            lineSep->addChild(material);

            // 索引线集 / Indexed line set
            auto* lineSet = new SoIndexedLineSet();
            lineSet->coordIndex.setNum(static_cast<int>(data.lineIndices.size()));
            int32_t* indexPtr = lineSet->coordIndex.startEditing();
            for (size_t i = 0; i < data.lineIndices.size(); ++i) {
                indexPtr[i] = data.lineIndices[i];
            }
            lineSet->coordIndex.finishEditing();
            lineSep->addChild(lineSet);

            soSep->addChild(lineSep);
        }
#endif
    }

    return separator;
}

RenderNode::Ptr GeometryNodeBuilder::buildAllModes(const GeometryData& data)
{
    if (!_factory) {
        return nullptr;
    }

    auto switchNode = _factory->createSwitch();
    if (!switchNode) {
        return nullptr;
    }

    if (_factory->getBackendType() == BackendType::Coin3D) {
#ifdef RENDER_HAS_COIN3D_BACKEND
        auto* coinSwitch = dynamic_cast<Coin3DSwitch*>(switchNode.get());
        if (!coinSwitch) {
            return nullptr;
        }

        // Mode 0: Flat Lines (默认)
        auto flatLines = buildFlatLinesNode(data);
        if (flatLines) {
            coinSwitch->addChild(flatLines);
        }

        // Mode 1: Shaded
        auto shaded = buildShadedNode(data);
        if (shaded) {
            coinSwitch->addChild(shaded);
        }

        // Mode 2: Wireframe
        auto wireframe = buildWireframeNode(data);
        if (wireframe) {
            coinSwitch->addChild(wireframe);
        }

        // Mode 3: Points
        auto points = buildPointsNode(data);
        if (points) {
            coinSwitch->addChild(points);
        }

        // 默认显示 Flat Lines / Default to Flat Lines
        coinSwitch->setWhichChild(0);
#endif
    }

    return switchNode;
}

RenderNode::Ptr GeometryNodeBuilder::createMaterialNode(const Color& color, float transparency)
{
    if (!_factory) {
        return nullptr;
    }

    auto material = _factory->createMaterial();
    if (!material) {
        return nullptr;
    }

    if (_factory->getBackendType() == BackendType::Coin3D) {
#ifdef RENDER_HAS_COIN3D_BACKEND
        auto* coinMat = dynamic_cast<Coin3DMaterial*>(material.get());
        if (coinMat) {
            coinMat->setDiffuseColor(color.r, color.g, color.b);
            coinMat->setTransparency(transparency);
        }
#endif
    }

    return material;
}

RenderNode::Ptr GeometryNodeBuilder::createDrawStyleNode(float lineWidth, float pointSize)
{
    if (!_factory) {
        return nullptr;
    }

    auto drawStyle = _factory->createDrawStyle();
    if (!drawStyle) {
        return nullptr;
    }

    // TODO: 设置绘制样式属性
    // TODO: Set draw style properties
    (void)lineWidth;
    (void)pointSize;

    return drawStyle;
}

RenderNode::Ptr GeometryNodeBuilder::createCoordinateNode(const GeometryData& data)
{
    if (!_factory || data.isEmpty()) {
        return nullptr;
    }

    auto coords = _factory->createCoordinate();
    if (!coords) {
        return nullptr;
    }

    // TODO: 填充坐标数据
    // TODO: Fill coordinate data

    return coords;
}

RenderNode::Ptr GeometryNodeBuilder::createNormalNode(const GeometryData& data)
{
    if (!_factory || data.normals.empty()) {
        return nullptr;
    }

    auto normals = _factory->createNormal();
    if (!normals) {
        return nullptr;
    }

    // TODO: 填充法线数据
    // TODO: Fill normal data

    return normals;
}

} // namespace Render
} // namespace Gui
