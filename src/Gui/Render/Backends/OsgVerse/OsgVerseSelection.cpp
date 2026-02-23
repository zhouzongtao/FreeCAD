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

#include "PreCompiled.h"
#include "OsgVerseSelection.h"

#include <osg/LineWidth>
#include <osg/Material>
#include <osg/PolygonMode>
#include <osg/PolygonOffset>
#include <osg/PrimitiveSet>

#include <Base/Console.h>

namespace Gui {
namespace Render {

//=============================================================================
// OsgVerseSelectionRoot 实现
//=============================================================================

OsgVerseSelectionRoot::OsgVerseSelectionRoot()
    : OsgVerseGroup()
{
    setName("SelectionRoot");
    createHighlightStateSet();
}

OsgVerseSelectionRoot::~OsgVerseSelectionRoot()
{
}

void OsgVerseSelectionRoot::setSelectionState(SelectionState state)
{
    if (_selectionState == state) {
        return;
    }

    Base::Console().log("OsgVerseSelectionRoot::setSelectionState: %d -> %d\n",
                        static_cast<int>(_selectionState), static_cast<int>(state));

    _selectionState = state;
    updateHighlight();
}

void OsgVerseSelectionRoot::setHighlightMode(HighlightMode mode)
{
    _highlightMode = mode;
    updateHighlight();
}

void OsgVerseSelectionRoot::setSelectionColor(float r, float g, float b, float a)
{
    _selectionColor = osg::Vec4(r, g, b, a);
    if (_selectionState == SelectionState::Selected) {
        updateHighlight();
    }
}

void OsgVerseSelectionRoot::setPreselectionColor(float r, float g, float b, float a)
{
    _preselectionColor = osg::Vec4(r, g, b, a);
    if (_selectionState == SelectionState::Preselected) {
        updateHighlight();
    }
}

void OsgVerseSelectionRoot::setOutlineWidth(float width)
{
    _outlineWidth = width;
    updateHighlight();
}

void OsgVerseSelectionRoot::setSelectionEnabled(bool enabled)
{
    _selectionEnabled = enabled;
    if (!enabled && _selectionState == SelectionState::Selected) {
        setSelectionState(SelectionState::None);
    }
}

void OsgVerseSelectionRoot::setPreselectionEnabled(bool enabled)
{
    _preselectionEnabled = enabled;
    if (!enabled && _selectionState == SelectionState::Preselected) {
        setSelectionState(SelectionState::None);
    }
}

void OsgVerseSelectionRoot::createHighlightStateSet()
{
    _highlightStateSet = new osg::StateSet();

    // 将在 updateHighlight() 中配置
}

void OsgVerseSelectionRoot::updateHighlight()
{
    osg::Group* group = getOsgGroup();
    if (!group) {
        Base::Console().warning("OsgVerseSelectionRoot::updateHighlight: No osg::Group!\n");
        return;
    }

    Base::Console().log("OsgVerseSelectionRoot::updateHighlight: state=%d, mode=%d\n",
                        static_cast<int>(_selectionState), static_cast<int>(_highlightMode));

    if (_selectionState == SelectionState::None) {
        // 移除高亮 - 恢复原始 uniform
        removeOutlineEffect();

        // 清除 highlight uniform，恢复原始颜色
        if (_highlightStateSet) {
            _highlightStateSet->removeUniform("u_baseColor");
            _highlightStateSet->removeUniform("u_colorMode");
        }
        group->setStateSet(nullptr);
        Base::Console().log("OsgVerseSelectionRoot::updateHighlight: Cleared highlight\n");
        return;
    }

    // 确定使用的颜色
    osg::Vec4 highlightColor;
    if (_selectionState == SelectionState::Selected) {
        if (!_selectionEnabled) {
            return;
        }
        highlightColor = _selectionColor;
        Base::Console().log("OsgVerseSelectionRoot::updateHighlight: Selection color (%.2f, %.2f, %.2f)\n",
                           highlightColor.r(), highlightColor.g(), highlightColor.b());
    }
    else if (_selectionState == SelectionState::Preselected) {
        if (!_preselectionEnabled) {
            return;
        }
        highlightColor = _preselectionColor;
        Base::Console().log("OsgVerseSelectionRoot::updateHighlight: Preselection color (%.2f, %.2f, %.2f)\n",
                           highlightColor.r(), highlightColor.g(), highlightColor.b());
    }

    // 根据高亮模式应用效果
    if (_highlightMode == HighlightMode::None) {
        group->setStateSet(nullptr);
        removeOutlineEffect();
        return;
    }

    // 创建或更新 StateSet
    if (!_highlightStateSet) {
        _highlightStateSet = new osg::StateSet();
    }

    if (_highlightMode == HighlightMode::Color || _highlightMode == HighlightMode::Both) {
        // 颜色高亮 - 使用 uniform (macOS GL 2.1 兼容方式)
        // 注意：shader 使用 u_baseColor 和 u_colorMode
        osg::ref_ptr<osg::Uniform> baseColorUniform = new osg::Uniform("u_baseColor", highlightColor);
        _highlightStateSet->addUniform(baseColorUniform.get());

        // u_colorMode = 0 表示使用 uniform 颜色
        osg::ref_ptr<osg::Uniform> colorModeUniform = new osg::Uniform("u_colorMode", 0);
        _highlightStateSet->addUniform(colorModeUniform.get());

        // 设置 OVERRIDE 确保这个 StateSet 覆盖子节点的设置
        _highlightStateSet->setRenderBinDetails(10, "RenderBin");
    }

    if (_highlightMode == HighlightMode::Outline || _highlightMode == HighlightMode::Both) {
        applyOutlineEffect();
    }
    else {
        removeOutlineEffect();
    }

    group->setStateSet(_highlightStateSet.get());
    Base::Console().log("OsgVerseSelectionRoot::updateHighlight: Applied StateSet to group\n");
}

void OsgVerseSelectionRoot::applyOutlineEffect()
{
    if (!_highlightStateSet) {
        return;
    }

    // 使用线条宽度增加轮廓效果
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth(_outlineWidth);
    _highlightStateSet->setAttributeAndModes(lineWidth.get(), osg::StateAttribute::ON);

    // 使用 PolygonOffset 避免 z-fighting
    osg::ref_ptr<osg::PolygonOffset> offset = new osg::PolygonOffset(-1.0f, -1.0f);
    _highlightStateSet->setAttributeAndModes(offset.get(), osg::StateAttribute::ON);
}

void OsgVerseSelectionRoot::removeOutlineEffect()
{
    if (!_highlightStateSet) {
        return;
    }

    _highlightStateSet->removeAttribute(osg::StateAttribute::LINEWIDTH);
    _highlightStateSet->removeAttribute(osg::StateAttribute::POLYGONOFFSET);
}

//=============================================================================
// OsgVerseBoundingBox 实现
//=============================================================================

OsgVerseBoundingBox::OsgVerseBoundingBox()
    : OsgVerseGroup()
{
    setName("BoundingBox");
    createGeometry();
}

OsgVerseBoundingBox::OsgVerseBoundingBox(const Vec3f& minCorner, const Vec3f& maxCorner)
    : OsgVerseGroup()
    , _minCorner(minCorner)
    , _maxCorner(maxCorner)
{
    setName("BoundingBox");
    createGeometry();
}

OsgVerseBoundingBox::~OsgVerseBoundingBox()
{
}

void OsgVerseBoundingBox::setBounds(const Vec3f& minCorner, const Vec3f& maxCorner)
{
    _minCorner = minCorner;
    _maxCorner = maxCorner;
    updateGeometry();
}

void OsgVerseBoundingBox::setBounds(float xMin, float yMin, float zMin,
                                     float xMax, float yMax, float zMax)
{
    _minCorner = Vec3f(xMin, yMin, zMin);
    _maxCorner = Vec3f(xMax, yMax, zMax);
    updateGeometry();
}

Vec3f OsgVerseBoundingBox::getCenter() const
{
    return Vec3f(
        (_minCorner.x + _maxCorner.x) * 0.5f,
        (_minCorner.y + _maxCorner.y) * 0.5f,
        (_minCorner.z + _maxCorner.z) * 0.5f
    );
}

Vec3f OsgVerseBoundingBox::getSize() const
{
    return Vec3f(
        _maxCorner.x - _minCorner.x,
        _maxCorner.y - _minCorner.y,
        _maxCorner.z - _minCorner.z
    );
}

void OsgVerseBoundingBox::setColor(float r, float g, float b, float a)
{
    _color = osg::Vec4(r, g, b, a);

    if (_colors && _colors->size() > 0) {
        (*_colors)[0] = _color;
        _colors->dirty();
        if (_geometry) {
            _geometry->dirtyDisplayList();
        }
    }
}

void OsgVerseBoundingBox::setColor(const osg::Vec4& color)
{
    setColor(color.r(), color.g(), color.b(), color.a());
}

void OsgVerseBoundingBox::setLineWidth(float width)
{
    _lineWidth = width;

    if (_geode) {
        osg::StateSet* ss = _geode->getOrCreateStateSet();
        osg::ref_ptr<osg::LineWidth> lw = new osg::LineWidth(width);
        ss->setAttributeAndModes(lw.get(), osg::StateAttribute::ON);
    }
}

void OsgVerseBoundingBox::setVisible(bool visible)
{
    _visible = visible;

    osg::Group* group = getOsgGroup();
    if (group) {
        group->setNodeMask(visible ? ~0u : 0u);
    }
}

void OsgVerseBoundingBox::createGeometry()
{
    // 创建 Geode
    _geode = new osg::Geode();

    // 创建几何体
    _geometry = new osg::Geometry();

    // 创建 8 个顶点
    _vertices = new osg::Vec3Array(8);
    updateGeometry();  // 填充顶点数据

    _geometry->setVertexArray(_vertices.get());

    // 创建颜色数组 (单一颜色)
    _colors = new osg::Vec4Array(1);
    (*_colors)[0] = _color;
    _geometry->setColorArray(_colors.get(), osg::Array::BIND_OVERALL);

    // 创建 12 条边的索引
    // 边界框有 12 条边：
    // 底面 4 条，顶面 4 条，连接底面和顶面的 4 条
    osg::ref_ptr<osg::DrawElementsUShort> edges =
        new osg::DrawElementsUShort(osg::PrimitiveSet::LINES, 24);

    // 底面边 (z = min)
    (*edges)[0] = 0; (*edges)[1] = 1;   // 0-1
    (*edges)[2] = 1; (*edges)[3] = 2;   // 1-2
    (*edges)[4] = 2; (*edges)[5] = 3;   // 2-3
    (*edges)[6] = 3; (*edges)[7] = 0;   // 3-0

    // 顶面边 (z = max)
    (*edges)[8] = 4; (*edges)[9] = 5;   // 4-5
    (*edges)[10] = 5; (*edges)[11] = 6; // 5-6
    (*edges)[12] = 6; (*edges)[13] = 7; // 6-7
    (*edges)[14] = 7; (*edges)[15] = 4; // 7-4

    // 连接边 (垂直)
    (*edges)[16] = 0; (*edges)[17] = 4; // 0-4
    (*edges)[18] = 1; (*edges)[19] = 5; // 1-5
    (*edges)[20] = 2; (*edges)[21] = 6; // 2-6
    (*edges)[22] = 3; (*edges)[23] = 7; // 3-7

    _geometry->addPrimitiveSet(edges.get());

    // 禁用光照
    osg::StateSet* ss = _geode->getOrCreateStateSet();
    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    // 设置线宽
    osg::ref_ptr<osg::LineWidth> lw = new osg::LineWidth(_lineWidth);
    ss->setAttributeAndModes(lw.get(), osg::StateAttribute::ON);

    // 添加几何体到 Geode
    _geode->addDrawable(_geometry.get());

    // 添加 Geode 到 Group
    getOsgGroup()->addChild(_geode.get());
}

void OsgVerseBoundingBox::updateGeometry()
{
    if (!_vertices) {
        return;
    }

    float xMin = _minCorner.x, yMin = _minCorner.y, zMin = _minCorner.z;
    float xMax = _maxCorner.x, yMax = _maxCorner.y, zMax = _maxCorner.z;

    // 8 个顶点的布局:
    //
    //        7 -------- 6
    //       /|         /|
    //      / |        / |
    //     4 -------- 5  |
    //     |  3 ------|-- 2
    //     | /        | /
    //     |/         |/
    //     0 -------- 1
    //
    // 底面 (z = min): 0, 1, 2, 3
    // 顶面 (z = max): 4, 5, 6, 7

    (*_vertices)[0] = osg::Vec3(xMin, yMin, zMin);
    (*_vertices)[1] = osg::Vec3(xMax, yMin, zMin);
    (*_vertices)[2] = osg::Vec3(xMax, yMax, zMin);
    (*_vertices)[3] = osg::Vec3(xMin, yMax, zMin);
    (*_vertices)[4] = osg::Vec3(xMin, yMin, zMax);
    (*_vertices)[5] = osg::Vec3(xMax, yMin, zMax);
    (*_vertices)[6] = osg::Vec3(xMax, yMax, zMax);
    (*_vertices)[7] = osg::Vec3(xMin, yMax, zMax);

    _vertices->dirty();

    if (_geometry) {
        _geometry->dirtyDisplayList();
        _geometry->dirtyBound();
    }
}

} // namespace Render
} // namespace Gui
