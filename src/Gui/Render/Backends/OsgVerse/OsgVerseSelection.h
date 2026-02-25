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

#ifndef GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSESELECTION_H
#define GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSESELECTION_H

#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/StateSet>
#include <osg/Material>
#include <osg/LineWidth>
#include <osg/PolygonMode>
#include <osg/PolygonOffset>

#include <FCGlobal.h>
#include "OsgVerseNode.h"

namespace Gui {
namespace Render {

/**
 * @brief OsgVerse 选择根节点 / OsgVerse Selection Root Node
 *
 * 管理选择状态和高亮显示的特殊分组节点。
 * Special group node that manages selection state and highlighting.
 *
 * 功能 / Features:
 * - 选择高亮 (轮廓线或颜色改变)
 * - 预选高亮
 * - 选择状态跟踪
 *
 * 参考 Coin3D 的 SoFCUnifiedSelection 实现。
 * References Coin3D's SoFCUnifiedSelection implementation.
 */
class GuiExport OsgVerseSelectionRoot : public OsgVerseGroup {
public:
    /**
     * @brief 选择状态枚举
     */
    enum class SelectionState {
        None,           ///< 未选中
        Preselected,    ///< 预选中 (鼠标悬停)
        Selected        ///< 已选中
    };

    /**
     * @brief 高亮模式枚举
     */
    enum class HighlightMode {
        None,           ///< 无高亮
        Outline,        ///< 轮廓线高亮
        Color,          ///< 颜色改变高亮
        Both            ///< 轮廓线 + 颜色
    };

    /**
     * @brief 构造函数
     */
    OsgVerseSelectionRoot();

    /**
     * @brief 析构函数
     */
    ~OsgVerseSelectionRoot() override;

    //=========================================================================
    // 选择状态管理
    //=========================================================================

    /**
     * @brief 设置选择状态
     */
    void setSelectionState(SelectionState state);

    /**
     * @brief 获取选择状态
     */
    SelectionState getSelectionState() const { return _selectionState; }

    /**
     * @brief 检查是否被选中
     */
    bool isSelected() const { return _selectionState == SelectionState::Selected; }

    /**
     * @brief 检查是否被预选
     */
    bool isPreselected() const { return _selectionState == SelectionState::Preselected; }

    //=========================================================================
    // 高亮设置
    //=========================================================================

    /**
     * @brief 设置高亮模式
     */
    void setHighlightMode(HighlightMode mode);

    /**
     * @brief 获取高亮模式
     */
    HighlightMode getHighlightMode() const { return _highlightMode; }

    /**
     * @brief 设置选中颜色
     */
    void setSelectionColor(float r, float g, float b, float a = 1.0f);

    /**
     * @brief 设置预选颜色
     */
    void setPreselectionColor(float r, float g, float b, float a = 1.0f);

    /**
     * @brief 设置轮廓线宽度
     */
    void setOutlineWidth(float width);

    /**
     * @brief 获取轮廓线宽度
     */
    float getOutlineWidth() const { return _outlineWidth; }

    //=========================================================================
    // 启用/禁用
    //=========================================================================

    /**
     * @brief 启用选择功能
     */
    void setSelectionEnabled(bool enabled);

    /**
     * @brief 检查选择功能是否启用
     */
    bool isSelectionEnabled() const { return _selectionEnabled; }

    /**
     * @brief 启用预选功能
     */
    void setPreselectionEnabled(bool enabled);

    /**
     * @brief 检查预选功能是否启用
     */
    bool isPreselectionEnabled() const { return _preselectionEnabled; }

private:
    /**
     * @brief 更新高亮状态
     */
    void updateHighlight();

    /**
     * @brief 创建高亮 StateSet
     */
    void createHighlightStateSet();

    /**
     * @brief 应用轮廓线效果
     */
    void applyOutlineEffect();

    /**
     * @brief 移除轮廓线效果
     */
    void removeOutlineEffect();

    SelectionState _selectionState{SelectionState::None};
    HighlightMode _highlightMode{HighlightMode::Color};

    osg::Vec4 _selectionColor{0.1f, 0.8f, 0.1f, 1.0f};      ///< 绿色选中
    osg::Vec4 _preselectionColor{0.8f, 0.8f, 0.1f, 1.0f};   ///< 黄色预选

    float _outlineWidth{2.0f};

    bool _selectionEnabled{true};
    bool _preselectionEnabled{true};

    osg::ref_ptr<osg::StateSet> _highlightStateSet;
    osg::ref_ptr<osg::StateSet> _originalStateSet;
};

/**
 * @brief OsgVerse 边界框节点 / OsgVerse Bounding Box Node
 *
 * 显示对象边界框的线框节点。
 * Displays wireframe bounding box of objects.
 *
 * 用于：
 * - 选择可视化
 * - 调试显示
 * - 对象边界指示
 */
class GuiExport OsgVerseBoundingBox : public OsgVerseGroup {
public:
    /**
     * @brief 构造函数 - 创建空边界框
     */
    OsgVerseBoundingBox();

    /**
     * @brief 构造函数 - 创建指定范围的边界框
     *
     * @param minCorner 最小角点
     * @param maxCorner 最大角点
     */
    OsgVerseBoundingBox(const Vec3f& minCorner, const Vec3f& maxCorner);

    /**
     * @brief 析构函数
     */
    ~OsgVerseBoundingBox() override;

    //=========================================================================
    // 边界设置
    //=========================================================================

    /**
     * @brief 设置边界范围
     */
    void setBounds(const Vec3f& minCorner, const Vec3f& maxCorner);

    /**
     * @brief 设置边界范围 (使用 osg::BoundingBox)
     */
    void setBounds(float xMin, float yMin, float zMin,
                   float xMax, float yMax, float zMax);

    /**
     * @brief 获取最小角点
     */
    Vec3f getMinCorner() const { return _minCorner; }

    /**
     * @brief 获取最大角点
     */
    Vec3f getMaxCorner() const { return _maxCorner; }

    /**
     * @brief 获取中心点
     */
    Vec3f getCenter() const;

    /**
     * @brief 获取尺寸
     */
    Vec3f getSize() const;

    //=========================================================================
    // 外观设置
    //=========================================================================

    /**
     * @brief 设置线条颜色
     */
    void setColor(float r, float g, float b, float a = 1.0f);

    /**
     * @brief 设置线条颜色 (使用 Vec4)
     */
    void setColor(const osg::Vec4& color);

    /**
     * @brief 获取线条颜色
     */
    osg::Vec4 getColor() const { return _color; }

    /**
     * @brief 设置线条宽度
     */
    void setLineWidth(float width);

    /**
     * @brief 获取线条宽度
     */
    float getLineWidth() const { return _lineWidth; }

    /**
     * @brief 设置可见性
     */
    void setVisible(bool visible);

    /**
     * @brief 检查是否可见
     */
    bool isVisible() const { return _visible; }

private:
    /**
     * @brief 创建边界框几何体
     */
    void createGeometry();

    /**
     * @brief 更新边界框几何体
     */
    void updateGeometry();

    Vec3f _minCorner{0.0f, 0.0f, 0.0f};
    Vec3f _maxCorner{1.0f, 1.0f, 1.0f};

    osg::Vec4 _color{1.0f, 1.0f, 1.0f, 1.0f};  ///< 默认白色
    float _lineWidth{1.0f};
    bool _visible{true};

    osg::ref_ptr<osg::Geode> _geode;
    osg::ref_ptr<osg::Geometry> _geometry;
    osg::ref_ptr<osg::Vec3Array> _vertices;
    osg::ref_ptr<osg::Vec4Array> _colors;
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSESELECTION_H
