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

#ifndef GUI_RENDER_RENDERNODE_H
#define GUI_RENDER_RENDERNODE_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include <functional>

#include <FCGlobal.h>
#include "RenderTypes.h"

namespace Gui {

class ViewProvider;

namespace Render {

/**
 * @brief 选择状态信息 / Selection state information
 */
struct SelectionState {
    bool selected{false};           ///< 是否被选中 / Is selected
    bool highlighted{false};        ///< 是否高亮（预选中）/ Is highlighted (preselected)
    bool selectable{true};          ///< 是否可选中 / Is selectable
    std::string selectedElement;    ///< 选中的子元素名称 / Selected sub-element name
    std::string highlightedElement; ///< 高亮的子元素名称 / Highlighted sub-element name
};

/**
 * @brief 选择颜色配置 / Selection color configuration
 */
struct SelectionColors {
    float highlightColor[4]{0.9f, 0.9f, 0.0f, 1.0f};  ///< 高亮颜色 / Highlight color (yellow)
    float selectionColor[4]{0.0f, 0.8f, 0.0f, 1.0f};  ///< 选择颜色 / Selection color (green)
};

/**
 * @brief 渲染节点基类 / Base class for render nodes
 *
 * 场景图中的基础节点类，支持节点层次结构、引用计数和类型安全转换。
 * Base node class in the scene graph, supporting hierarchy, reference counting,
 * and type-safe conversions.
 *
 * 设计理念 / Design Philosophy:
 * - 提供与 Coin3D SoNode 兼容的接口
 * - 支持多种后端的统一抽象
 * - 使用 C++20 Concepts 实现类型安全的节点转换
 * - 零开销抽象，编译时类型检查
 */
class GuiExport RenderNode {
public:
    /// 引用计数智能指针类型 / Reference-counted smart pointer type
    using Ptr = std::shared_ptr<RenderNode>;
    using WeakPtr = std::weak_ptr<RenderNode>;

    /**
     * @brief 构造函数 / Constructor
     * @param type 节点类型 / Node type
     */
    explicit RenderNode(NodeType type = NodeType::Node);

    /**
     * @brief 虚析构函数 / Virtual destructor
     */
    virtual ~RenderNode();

    // 禁止拷贝，允许移动 / Disable copy, allow move
    FC_DISABLE_COPY(RenderNode)
    FC_DEFAULT_MOVE(RenderNode)

    /**
     * @brief 获取节点类型 / Get node type
     */
    virtual NodeType getNodeType() const { return _nodeType; }

    /**
     * @brief 获取节点名称 / Get node name
     */
    const std::string& getName() const { return _name; }

    /**
     * @brief 设置节点名称 / Set node name
     */
    void setName(const std::string& name) { _name = name; }

    /**
     * @brief 获取用户数据 / Get user data
     */
    void* getUserData() const { return _userData; }

    /**
     * @brief 设置用户数据 / Set user data
     */
    void setUserData(void* data) { _userData = data; }

    /**
     * @brief 添加引用 / Add reference
     *
     * 手动引用计数（兼容 Coin3D 风格）
     * Manual reference counting (Coin3D compatible)
     */
    void ref() const;

    /**
     * @brief 移除引用 / Remove reference
     */
    void unref() const;

    /**
     * @brief 获取引用计数 / Get reference count
     */
    int getRefCount() const { return _refCount; }

    /**
     * @brief 类型安全的向下转换 / Type-safe downcast
     *
     * 使用 C++20 Concepts 实现编译时类型检查
     * Uses C++20 Concepts for compile-time type checking
     */
    template<typename T>
    T* cast() {
        if (T::staticGetNodeType() == _nodeType) {
            return static_cast<T*>(this);
        }
        return nullptr;
    }

    template<typename T>
    const T* cast() const {
        if (T::staticGetNodeType() == _nodeType) {
            return static_cast<const T*>(this);
        }
        return nullptr;
    }

    /**
     * @brief 获取父节点 / Get parent node
     */
    RenderNode* getParent() const { return _parent; }

    /**
     * @brief 获取节点路径 / Get node path to root
     */
    std::vector<RenderNode*> getPath() const;

    /**
     * @brief 触发自定义事件 / Trigger custom event
     */
    virtual void onEvent(const std::string& event, void* data = nullptr);

    /**
     * @brief 触摸节点（标记为已修改） / Touch node (mark as modified)
     */
    virtual void touch();

    /**
     * @brief 获取节点是否被修改 / Check if node was modified
     */
    bool isTouched() const { return _touched; }

    /**
     * @brief 设置可见性 / Set visibility
     */
    virtual void setVisible(bool visible) { _visible = visible; }

    /**
     * @brief 获取可见性 / Get visibility
     */
    bool isVisible() const { return _visible; }

    /**
     * @brief 设置渲染模式 / Set render mode
     */
    virtual void setRenderMode(RenderMode mode) { _renderMode = mode; }

    /**
     * @brief 获取渲染模式 / Get render mode
     */
    RenderMode getRenderMode() const { return _renderMode; }

    /**
     * @brief 获取边界框 / Get bounding box
     */
    virtual BoundingBox getBoundingBox() const;

    //-----------------------------------------------------------------------
    // 选择系统接口 / Selection System Interface
    //-----------------------------------------------------------------------

    /**
     * @brief 设置是否可选中 / Set selectable
     */
    virtual void setSelectable(bool selectable) { _selectionState.selectable = selectable; }

    /**
     * @brief 获取是否可选中 / Get selectable
     */
    bool isSelectable() const { return _selectionState.selectable; }

    /**
     * @brief 设置选中状态 / Set selected state
     * @param selected 是否选中 / Whether selected
     * @param element 子元素名称（如 "Face1", "Edge2"）/ Sub-element name
     */
    virtual void setSelected(bool selected, const std::string& element = "");

    /**
     * @brief 获取是否选中 / Get selected state
     */
    bool isSelected() const { return _selectionState.selected; }

    /**
     * @brief 获取选中的子元素 / Get selected sub-element
     */
    const std::string& getSelectedElement() const { return _selectionState.selectedElement; }

    /**
     * @brief 设置高亮状态（预选中）/ Set highlighted state (preselection)
     * @param highlighted 是否高亮 / Whether highlighted
     * @param element 子元素名称 / Sub-element name
     */
    virtual void setHighlighted(bool highlighted, const std::string& element = "");

    /**
     * @brief 获取是否高亮 / Get highlighted state
     */
    bool isHighlighted() const { return _selectionState.highlighted; }

    /**
     * @brief 获取高亮的子元素 / Get highlighted sub-element
     */
    const std::string& getHighlightedElement() const { return _selectionState.highlightedElement; }

    /**
     * @brief 设置高亮颜色 / Set highlight color
     */
    virtual void setHighlightColor(float r, float g, float b, float a = 1.0f);

    /**
     * @brief 获取高亮颜色 / Get highlight color
     */
    void getHighlightColor(float& r, float& g, float& b, float& a) const;

    /**
     * @brief 设置选择颜色 / Set selection color
     */
    virtual void setSelectionColor(float r, float g, float b, float a = 1.0f);

    /**
     * @brief 获取选择颜色 / Get selection color
     */
    void getSelectionColor(float& r, float& g, float& b, float& a) const;

    /**
     * @brief 获取选择状态 / Get full selection state
     */
    const SelectionState& getSelectionState() const { return _selectionState; }

    /**
     * @brief 获取拾取的子元素 / Get picked element at point
     * @param x 屏幕 X 坐标 / Screen X coordinate
     * @param y 屏幕 Y 坐标 / Screen Y coordinate
     * @param subname 输出子元素名称 / Output sub-element name
     * @return 是否拾取到元素 / Whether an element was picked
     */
    virtual bool getElementAtPoint(int x, int y, std::string& subname) const {
        (void)x; (void)y; (void)subname;
        return false;
    }

    /**
     * @brief 获取选择形状（用于高亮显示）/ Get selection shape for highlighting
     * @param element 子元素名称 / Sub-element name
     * @return 形状顶点列表 / Shape vertex list
     */
    virtual std::vector<std::array<float, 3>> getSelectionShape(const std::string& element) const {
        (void)element;
        return {};
    }

    //-----------------------------------------------------------------------
    // ViewProvider 关联 / ViewProvider Association
    //-----------------------------------------------------------------------

    /**
     * @brief 设置关联的 ViewProvider / Set associated ViewProvider
     */
    void setViewProvider(ViewProvider* vp) { _viewProvider = vp; }

    /**
     * @brief 获取关联的 ViewProvider / Get associated ViewProvider
     */
    ViewProvider* getViewProvider() const { return _viewProvider; }

    /**
     * @brief 克隆节点 / Clone node
     */
    virtual RenderNode::Ptr clone() const;

    /**
     * @brief 获取底层后端节点（用于兼容性）/ Get backend-specific node
     *
     * 允许直接访问底层 Coin3D SoNode 或 OSG Node
     * Allows direct access to underlying Coin3D SoNode or OSG Node
     */
    template<typename BackendNode>
    BackendNode* getBackendNode() {
        return static_cast<BackendNode*>(getBackendNodeImpl());
    }

    /**
     * @brief 获取后端类型 / Get backend type
     */
    BackendType getBackendType() const { return _backendType; }

protected:
    /**
     * @brief 派生类重写以提供实际后端节点指针
     * Derived classes override to provide actual backend node pointer
     */
    virtual void* getBackendNodeImpl() { return nullptr; }
    virtual const void* getBackendNodeImpl() const { return nullptr; }

    /**
     * @brief 设置父节点 / Set parent node
     *
     * 由 RenderGroup 调用，不应直接调用
     * Called by RenderGroup, should not be called directly
     */
    void setParent(RenderNode* parent) { _parent = parent; }

    /**
     * @brief 设置后端类型 / Set backend type
     */
    void setBackendType(BackendType type) { _backendType = type; }

    friend class RenderGroup;
    friend class Coin3DGroup;  // Allow Coin3D backend to manage parent-child relationships

protected:
    NodeType _nodeType;
    std::string _name;
    void* _userData{nullptr};
    mutable int _refCount{0};
    RenderNode* _parent{nullptr};
    bool _visible{true};
    bool _touched{false};
    RenderMode _renderMode{RenderMode::Default};
    BackendType _backendType{BackendType::None};

    // Selection state
    SelectionState _selectionState;
    SelectionColors _selectionColors;
    ViewProvider* _viewProvider{nullptr};
};

/**
 * @brief 宏定义：声明节点类型 / Macro: Declare node type
 *
 * 简化节点类型的静态类型信息声明
 * Simplifies static type information declaration for node types
 */
#define RENDER_NODE_STATIC_TYPE(Type) \
    static NodeType staticGetNodeType() { return Type; } \
    NodeType getNodeType() const override { return Type; }

/**
 * @brief 分组节点 / Group node
 *
 * 可以包含多个子节点的基础容器类。
 * Base container class that can hold multiple child nodes.
 */
class GuiExport RenderGroup : public RenderNode {
public:
    RENDER_NODE_STATIC_TYPE(NodeType::Group)

    /**
     * @brief 构造函数 / Constructor
     */
    RenderGroup();

    /**
     * @brief 析构函数 / Destructor
     */
    ~RenderGroup() override;

    /**
     * @brief 添加子节点 / Add child node
     */
    virtual void addChild(RenderNode::Ptr child);

    /**
     * @brief 插入子节点 / Insert child node at position
     */
    virtual void insertChild(size_t index, RenderNode::Ptr child);

    /**
     * @brief 移除子节点 / Remove child node
     */
    virtual bool removeChild(RenderNode* child);

    /**
     * @brief 移除所有子节点 / Remove all children
     */
    virtual void removeAllChildren();

    /**
     * @brief 替换子节点 / Replace child node
     */
    virtual bool replaceChild(RenderNode* oldChild, RenderNode::Ptr newChild);

    /**
     * @brief 获取子节点数量 / Get child count
     */
    size_t getNumChildren() const { return _children.size(); }

    /**
     * @brief 获取子节点 / Get child node
     */
    RenderNode* getChild(size_t index) const;
    RenderNode::Ptr getChildPtr(size_t index) const;

    /**
     * @brief 查找子节点 / Find child by name
     */
    RenderNode* findChild(const std::string& name) const;

    /**
     * @brief 查找子节点（递归）/ Find child recursively
     */
    RenderNode* findChildRecursive(const std::string& name) const;

    /**
     * @brief 获取子节点索引 / Get child index
     */
    int findChildIndex(const RenderNode* child) const;

    /**
     * @brief 检查是否包含子节点 / Check if contains child
     */
    bool hasChild(const RenderNode* child) const;

    /**
     * @brief 获取边界框（合并所有子节点）/ Get bounding box (merge all children)
     */
    BoundingBox getBoundingBox() const override;

    /**
     * @brief 克隆节点 / Clone node
     */
    RenderNode::Ptr clone() const override;

    /**
     * @brief 子节点迭代器支持 / Child iterator support
     */
    using iterator = std::vector<RenderNode::Ptr>::iterator;
    using const_iterator = std::vector<RenderNode::Ptr>::const_iterator;

    iterator begin() { return _children.begin(); }
    iterator end() { return _children.end(); }
    const_iterator begin() const { return _children.cbegin(); }
    const_iterator end() const { return _children.cend(); }
    const_iterator cbegin() const { return _children.cbegin(); }
    const_iterator cend() const { return _children.cend(); }

protected:
    std::vector<RenderNode::Ptr> _children;
};

/**
 * @brief 分隔节点 / Separator node
 *
 * 类似于 SoSeparator，对子节点进行渲染状态隔离。
 * Similar to SoSeparator, isolates rendering state for children.
 */
class GuiExport RenderSeparator : public RenderGroup {
public:
    RENDER_NODE_STATIC_TYPE(NodeType::Separator)

    RenderSeparator();
    ~RenderSeparator() override;

    /**
     * @brief 克隆节点 / Clone node
     */
    RenderNode::Ptr clone() const override;
};

/**
 * @brief 材质节点 / Material node
 *
 * 抽象材质节点，定义材质属性接口。
 * Abstract material node defining material property interface.
 */
class GuiExport RenderMaterial : public RenderNode {
public:
    RENDER_NODE_STATIC_TYPE(NodeType::Material)

    RenderMaterial() : RenderNode(NodeType::Material) {}
    ~RenderMaterial() override = default;

    //-----------------------------------------------------------------------
    // 环境光颜色 / Ambient Color
    //-----------------------------------------------------------------------

    /**
     * @brief 设置环境光颜色 / Set ambient color
     */
    virtual void setAmbientColor(float r, float g, float b) = 0;

    /**
     * @brief 获取环境光颜色 / Get ambient color
     */
    virtual void getAmbientColor(float& r, float& g, float& b) const = 0;

    //-----------------------------------------------------------------------
    // 漫反射颜色 / Diffuse Color
    //-----------------------------------------------------------------------

    /**
     * @brief 设置漫反射颜色 / Set diffuse color
     */
    virtual void setDiffuseColor(float r, float g, float b) = 0;

    /**
     * @brief 获取漫反射颜色 / Get diffuse color
     */
    virtual void getDiffuseColor(float& r, float& g, float& b) const = 0;

    //-----------------------------------------------------------------------
    // 镜面反射颜色 / Specular Color
    //-----------------------------------------------------------------------

    /**
     * @brief 设置镜面反射颜色 / Set specular color
     */
    virtual void setSpecularColor(float r, float g, float b) = 0;

    /**
     * @brief 获取镜面反射颜色 / Get specular color
     */
    virtual void getSpecularColor(float& r, float& g, float& b) const = 0;

    //-----------------------------------------------------------------------
    // 自发光颜色 / Emissive Color
    //-----------------------------------------------------------------------

    /**
     * @brief 设置自发光颜色 / Set emissive color
     */
    virtual void setEmissiveColor(float r, float g, float b) = 0;

    /**
     * @brief 获取自发光颜色 / Get emissive color
     */
    virtual void getEmissiveColor(float& r, float& g, float& b) const = 0;

    //-----------------------------------------------------------------------
    // 光泽度和透明度 / Shininess and Transparency
    //-----------------------------------------------------------------------

    /**
     * @brief 设置光泽度 / Set shininess [0-1]
     */
    virtual void setShininess(float shininess) = 0;

    /**
     * @brief 获取光泽度 / Get shininess
     */
    virtual float getShininess() const = 0;

    /**
     * @brief 设置透明度 / Set transparency [0-1]
     */
    virtual void setTransparency(float transparency) = 0;

    /**
     * @brief 获取透明度 / Get transparency
     */
    virtual float getTransparency() const = 0;
};

/**
 * @brief 命名类型别名 / Named type aliases
 *
 * 兼容现有 FreeCAD 代码中的命名习惯
 * Compatible with existing FreeCAD naming conventions
 */
using RenderNodePtr = RenderNode::Ptr;
using RenderGroupPtr = std::shared_ptr<RenderGroup>;
using RenderSeparatorPtr = std::shared_ptr<RenderSeparator>;
using RenderMaterialPtr = std::shared_ptr<RenderMaterial>;

/**
 * @brief Concept 检查节点类型 / Concept for node type checking
 *
 * C++20 Concept 用于编译时节点类型验证
 * C++20 Concept for compile-time node type validation
 */
template<typename T>
concept RenderNodeType = std::derived_from<T, RenderNode>;

/**
 * @brief 安全的节点转换函数 / Safe node cast function
 *
 * 使用模板和 Concepts 实现类型安全的节点转换
 * Type-safe node cast using templates and Concepts
 */
template<RenderNodeType T>
std::shared_ptr<T> node_cast(const RenderNode::Ptr& node) {
    if (node && T::staticGetNodeType() == node->getNodeType()) {
        return std::static_pointer_cast<T>(node);
    }
    return nullptr;
}

template<RenderNodeType T>
T* node_raw_cast(RenderNode* node) {
    if (node && T::staticGetNodeType() == node->getNodeType()) {
        return static_cast<T*>(node);
    }
    return nullptr;
}

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_RENDERNODE_H
