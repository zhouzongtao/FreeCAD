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

#ifndef GUI_VIEWPROVIDERRENDERPROXY_H
#define GUI_VIEWPROVIDERRENDERPROXY_H

#include <memory>
#include <string>
#include <vector>

#include <FCGlobal.h>
#include "Render/Core/RenderTypes.h"
#include "Render/Core/RenderNode.h"

// 前向声明 / Forward declarations
class SoNode;
class SoSeparator;
class SoSwitch;
class SoTransform;
namespace App {
class Material;
}

namespace Gui {

class ViewProvider;

/**
 * @brief ViewProvider 渲染代理 / ViewProvider render proxy
 *
 * 为 ViewProvider 提供渲染抽象层的接口，使其能够与不同的渲染后端交互。
 * Provides render abstraction layer interface for ViewProvider to interact
 * with different rendering backends.
 *
 * 设计目的 / Design Purpose:
 * - 隔离 ViewProvider 与具体渲染后端的直接依赖
 * - 提供统一的渲染操作接口
 * - 支持运行时后端切换
 * - 保持与现有 Coin3D 代码的兼容性
 */
class GuiExport ViewProviderRenderProxy {
public:
    /**
     * @brief 构造函数 / Constructor
     *
     * @param viewProvider 关联的 ViewProvider / Associated ViewProvider
     */
    explicit ViewProviderRenderProxy(ViewProvider* viewProvider);

    /**
     * @brief 析构函数 / Destructor
     */
    ~ViewProviderRenderProxy();

    // 禁止拷贝，允许移动 / Disable copy, allow move
    FC_DISABLE_COPY(ViewProviderRenderProxy)
    FC_DEFAULT_MOVE(ViewProviderRenderProxy)

    //-----------------------------------------------------------------------
    // 场景图根节点 / Scene Graph Root Node
    //-----------------------------------------------------------------------


    /**
     * @brief 获取渲染根节点 / Get render root node
     *
     * 返回抽象的渲染根节点，用于与渲染引擎交互。
     * Returns the abstract render root node for interaction with render engine.
     */
    Render::RenderNode* getRenderRoot() const;

    /**
     * @brief 设置渲染根节点 / Set render root node
     */
    void setRenderRoot(Render::RenderNode::Ptr root);

    /**
     * @brief 获取 Coin3D 根节点（兼容性）/ Get Coin3D root node (compatibility)
     *
     * 用于与现有 View3DInventorViewer 集成。
     * For integration with existing View3DInventorViewer.
     */
    SoSeparator* getCoinRoot() const;

    /**
     * @brief 设置 Coin3D 根节点（兼容性）/ Set Coin3D root node (compatibility)
     */
    void setCoinRoot(SoSeparator* root);

    //-----------------------------------------------------------------------
    // 显示模式 / Display Mode
    //-----------------------------------------------------------------------

    /**
     * @brief 设置显示模式 / Set display mode
     *
     * @param mode 显示模式名称 / Display mode name
     * @return 是否成功 / Whether successful
     */
    bool setDisplayMode(const char* mode);

    /**
     * @brief 获取显示模式 / Get display mode
     */
    const char* getDisplayMode() const { return _currentDisplayMode.c_str(); }

    /**
     * @brief 获取所有可用的显示模式 / Get all available display modes
     */
    std::vector<std::string> getDisplayModes() const;

    /**
     * @brief 添加显示模式 / Add display mode
     *
     * @param mode 模式名称 / Mode name
     * @param node 对应的渲染节点 / Corresponding render node
     */
    void addDisplayMode(const std::string& mode, Render::RenderNode::Ptr node);

    //-----------------------------------------------------------------------
    // 可见性 / Visibility
    //-----------------------------------------------------------------------

    /**
     * @brief 设置可见性 / Set visibility
     */
    void setVisible(bool visible);

    /**
     * @brief 获取可见性 / Get visibility
     */
    bool isVisible() const { return _visible; }

    /**
     * @brief 切换可见性 / Toggle visibility
     */
    void toggleVisibility();

    //-----------------------------------------------------------------------
    // 变换 / Transformation
    //-----------------------------------------------------------------------

    /**
     * @brief 设置变换 / Set transformation
     *
     * @param matrix 变换矩阵 / Transformation matrix
     */
    void setTransform(const Base::Matrix4D& matrix);

    /**
     * @brief 获取变换 / Get transformation
     */
    Base::Matrix4D getTransform() const;

    /**
     * @brief 重置变换 / Reset transformation
     */
    void resetTransform();

    //-----------------------------------------------------------------------
    // 材质 / Material
    //-----------------------------------------------------------------------

    /**
     * @brief 设置材质 / Set material
     *
     * @param material 材质对象 / Material object
     */
    void setMaterial(const Render::Material& material);

    /**
     * @brief 获取材质 / Get material
     */
    Render::Material getMaterial() const;

    /**
     * @brief 设置材质样式 / Set material style
     *
     * @param style 样式名称 / Style name
     */
    void setMaterialStyle(const std::string& style);

    /**
     * @brief 获取材质样式 / Get material style
     */
    const std::string& getMaterialStyle() const { return _materialStyle; }

    //-----------------------------------------------------------------------
    // 选择状态 / Selection State
    //-----------------------------------------------------------------------

    /**
     * @brief 设置选择状态 / Set selection state
     */
    void setSelected(bool selected);

    /**
     * @brief 获取选择状态 / Get selection state
     */
    bool isSelected() const { return _selected; }

    /**
     * @brief 切换选择状态 / Toggle selection state
     */
    void toggleSelection();

    /**
     * @brief 设置高亮颜色 / Set highlight color
     */
    void setHighlightColor(const Render::Color& color);

    /**
     * @brief 获取高亮颜色 / Get highlight color
     */
    Render::Color getHighlightColor() const { return _highlightColor; }

    //-----------------------------------------------------------------------
    // 边界框 / Bounding Box
    //-----------------------------------------------------------------------

    /**
     * @brief 获取边界框 / Get bounding box
     */
    Render::BoundingBox getBoundingBox() const;

    /**
     * @brief 更新边界框 / Update bounding box
     */
    void updateBoundingBox();

    //-----------------------------------------------------------------------
    // 渲染状态 / Render State
    //-----------------------------------------------------------------------

    /**
     * @brief 设置渲染模式 / Set render mode
     */
    void setRenderMode(Render::RenderMode mode);

    /**
     * @brief 获取渲染模式 / Get render mode
     */
    Render::RenderMode getRenderMode() const { return _renderMode; }

    /**
     * @brief 标记为需要更新 / Mark as needing update
     */
    void touch();

    //-----------------------------------------------------------------------
    // 子节点管理 / Child Management
    //-----------------------------------------------------------------------

    /**
     * @brief 添加子节点 / Add child node
     */
    void addChild(Render::RenderNode::Ptr child);

    /**
     * @brief 移除子节点 / Remove child node
     */
    bool removeChild(Render::RenderNode* child);

    /**
     * @brief 获取子节点数量 / Get child count
     */
    size_t getChildCount() const;

    /**
     * @brief 获取子节点 / Get child node
     */
    Render::RenderNode* getChild(size_t index) const;

    //-----------------------------------------------------------------------
    // 兼容性方法 / Compatibility Methods
    //-----------------------------------------------------------------------

    /**
     * @brief 获取关联的 ViewProvider / Get associated ViewProvider
     */
    ViewProvider* getViewProvider() const { return _viewProvider; }

    /**
     * @brief 检查是否使用 Coin3D / Check if using Coin3D
     */
    bool isUsingCoin3D() const;

    /**
     * @brief 检查是否使用 OsgVerse / Check if using OsgVerse
     */
    bool isUsingOsgVerse() const;

private:
    // 关联的 ViewProvider
    ViewProvider* _viewProvider;

    // 渲染节点
    Render::RenderNodePtr _renderRoot;
    Render::RenderNodePtr _rootSeparator;  // 实际上是 RenderSeparator 或其派生类
    Render::RenderNodePtr _frontRoot;
    Render::RenderNodePtr _backRoot;

    // Coin3D 节点（兼容性）
    SoSeparator* _coinRoot{nullptr};
    SoSwitch* _modeSwitch{nullptr};
    SoTransform* _transformNode{nullptr};

    // 状态
    std::string _currentDisplayMode;
    bool _visible{true};
    bool _selected{false};
    Render::RenderMode _renderMode{Render::RenderMode::Default};
    std::string _materialStyle;
    Render::Color _highlightColor{0.3f, 0.3f, 1.0f, 1.0f};  // 默认蓝色高亮 / Default blue highlight

    // 显示模式映射
    std::unordered_map<std::string, Render::RenderNode::Ptr> _displayModes;
};

} // namespace Gui

#endif // GUI_VIEWPROVIDERRENDERPROXY_H
