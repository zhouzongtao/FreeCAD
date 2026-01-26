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

#ifndef GUI_RENDER_RENDERNODEFACTORY_H
#define GUI_RENDER_RENDERNODEFACTORY_H

#include <memory>
#include <functional>
#include <unordered_map>
#include <mutex>

#include <FCGlobal.h>
#include "RenderTypes.h"
#include "RenderNode.h"

namespace Gui {
namespace Render {

// 前向声明抽象节点类型 / Forward declarations for abstract node types
class RenderMaterial;
class RenderGeometry;
class RenderIndexedFaceSet;
class RenderIndexedLineSet;
class RenderPointSet;
class RenderCoordinate;
class RenderDrawStyle;
class RenderPickStyle;
class RenderSelectionRoot;
class RenderBoundingBox;

/**
 * @brief 渲染节点工厂接口 / Render node factory interface
 *
 * 抽象工厂接口，用于创建后端无关的渲染节点。
 * ViewProvider 通过此接口创建节点，实现与具体后端的解耦。
 *
 * Abstract factory interface for creating backend-agnostic render nodes.
 * ViewProvider uses this interface to create nodes, decoupling from specific backends.
 *
 * 设计原则 / Design Principles:
 * 1. 所有节点创建都通过工厂进行
 * 2. 返回的是抽象层指针，调用者不需要知道具体后端
 * 3. 每个后端实现自己的工厂
 * 4. 支持运行时后端切换
 */
class GuiExport RenderNodeFactory {
public:
    using Ptr = std::shared_ptr<RenderNodeFactory>;

    /**
     * @brief 虚析构函数 / Virtual destructor
     */
    virtual ~RenderNodeFactory() = default;

    /**
     * @brief 获取后端类型 / Get backend type
     */
    virtual BackendType getBackendType() const = 0;

    /**
     * @brief 获取后端名称 / Get backend name
     */
    virtual std::string getBackendName() const = 0;

    //=========================================================================
    // 容器节点创建 / Container Node Creation
    //=========================================================================

    /**
     * @brief 创建分组节点 / Create group node
     *
     * 用于组织子节点，不进行状态隔离。
     * For organizing child nodes without state isolation.
     *
     * 注意：返回 RenderNode 以避免继承问题。可以通过 dynamic_cast
     * 转换为 RenderGroup 或后端特定类型。
     * Note: Returns RenderNode to avoid inheritance issues. Can be
     * dynamic_cast to RenderGroup or backend-specific types.
     */
    virtual std::shared_ptr<RenderNode> createGroup() = 0;

    /**
     * @brief 创建分隔节点 / Create separator node
     *
     * 类似于 Coin3D SoSeparator，对子节点的渲染状态进行隔离。
     * Similar to Coin3D SoSeparator, isolates rendering state for children.
     * 这是 ViewProvider 根节点的标准类型。
     * This is the standard type for ViewProvider root nodes.
     *
     * 注意：返回 RenderNode 以避免继承问题。可以通过 dynamic_cast
     * 转换为 RenderSeparator 或后端特定类型。
     * Note: Returns RenderNode to avoid inheritance issues. Can be
     * dynamic_cast to RenderSeparator or backend-specific types.
     */
    virtual std::shared_ptr<RenderNode> createSeparator() = 0;

    /**
     * @brief 创建切换节点 / Create switch node
     *
     * 用于显示模式切换（如线框/着色）。
     * For display mode switching (e.g., wireframe/shaded).
     */
    virtual std::shared_ptr<RenderNode> createSwitch() = 0;

    /**
     * @brief 创建变换节点 / Create transform node
     *
     * 用于平移、旋转、缩放子节点。
     * For translating, rotating, scaling child nodes.
     */
    virtual std::shared_ptr<RenderNode> createTransform() = 0;

    //=========================================================================
    // 几何节点创建 / Geometry Node Creation
    //=========================================================================

    /**
     * @brief 创建坐标节点 / Create coordinate node
     *
     * 存储顶点坐标数据。
     * Stores vertex coordinate data.
     */
    virtual std::shared_ptr<RenderNode> createCoordinate() = 0;

    /**
     * @brief 创建索引面集节点 / Create indexed face set node
     *
     * 用于渲染任意多边形网格。
     * For rendering arbitrary polygon meshes.
     */
    virtual std::shared_ptr<RenderNode> createIndexedFaceSet() = 0;

    /**
     * @brief 创建索引线集节点 / Create indexed line set node
     *
     * 用于渲染线条和边线。
     * For rendering lines and edges.
     */
    virtual std::shared_ptr<RenderNode> createIndexedLineSet() = 0;

    /**
     * @brief 创建点集节点 / Create point set node
     *
     * 用于渲染点云。
     * For rendering point clouds.
     */
    virtual std::shared_ptr<RenderNode> createPointSet() = 0;

    /**
     * @brief 创建法线节点 / Create normal node
     *
     * 存储法线数据。
     * Stores normal data.
     */
    virtual std::shared_ptr<RenderNode> createNormal() = 0;

    //=========================================================================
    // 外观节点创建 / Appearance Node Creation
    //=========================================================================

    /**
     * @brief 创建材质节点 / Create material node
     *
     * 用于设置颜色、透明度、光泽度等属性。
     * For setting color, transparency, shininess, etc.
     *
     * 注意：返回的节点可以通过 dynamic_cast 转换为后端特定类型
     * （如 Coin3DMaterial）以使用扩展功能。
     * Note: The returned node can be dynamic_cast to backend-specific types
     * (e.g., Coin3DMaterial) for extended functionality.
     */
    virtual std::shared_ptr<RenderNode> createMaterial() = 0;

    /**
     * @brief 创建绘制样式节点 / Create draw style node
     *
     * 用于设置线宽、点大小、填充模式等。
     * For setting line width, point size, fill mode, etc.
     */
    virtual std::shared_ptr<RenderNode> createDrawStyle() = 0;

    /**
     * @brief 创建拾取样式节点 / Create pick style node
     *
     * 用于控制节点的拾取行为。
     * For controlling node picking behavior.
     */
    virtual std::shared_ptr<RenderNode> createPickStyle() = 0;

    //=========================================================================
    // 选择相关节点创建 / Selection-Related Node Creation
    //=========================================================================

    /**
     * @brief 创建选择根节点 / Create selection root node
     *
     * 用于管理选择和高亮状态。
     * For managing selection and highlight states.
     * 对应 Coin3D 的 SoFCSelectionRoot。
     * Corresponds to Coin3D's SoFCSelectionRoot.
     */
    virtual std::shared_ptr<RenderNode> createSelectionRoot() = 0;

    /**
     * @brief 创建边界框节点 / Create bounding box node
     *
     * 用于渲染边界框。
     * For rendering bounding boxes.
     */
    virtual std::shared_ptr<RenderNode> createBoundingBox() = 0;

    //=========================================================================
    // 便捷方法 / Convenience Methods
    //=========================================================================

    /**
     * @brief 创建 ViewProvider 标准节点结构 / Create standard ViewProvider node structure
     *
     * 返回包含以下结构的根节点：
     * Returns a root node containing the following structure:
     *
     * ```
     * Separator (root)
     * ├── Transform
     * └── Switch (modeSwitch)
     *     ├── [mode 0 nodes will be added here]
     *     ├── [mode 1 nodes will be added here]
     *     └── ...
     * ```
     *
     * @param[out] transform 返回变换节点指针 / Returns transform node pointer
     * @param[out] modeSwitch 返回模式切换节点指针 / Returns mode switch node pointer
     * @return 根节点（RenderNode，可 dynamic_cast 到具体类型）/ Root node (use dynamic_cast for specific types)
     */
    virtual std::shared_ptr<RenderNode> createViewProviderRoot(
        std::shared_ptr<RenderNode>* transform = nullptr,
        std::shared_ptr<RenderNode>* modeSwitch = nullptr);

    /**
     * @brief 创建带材质的几何节点组 / Create geometry group with material
     *
     * @param material 材质属性 / Material properties
     * @return 包含材质和几何占位符的节点 / Node with material and geometry placeholder
     */
    virtual std::shared_ptr<RenderNode> createMaterialGroup(const Material& material);
};

//=============================================================================
// 工厂注册表 / Factory Registry
//=============================================================================

/**
 * @brief 渲染节点工厂注册表 / Render node factory registry
 *
 * 管理所有已注册的后端工厂。
 * Manages all registered backend factories.
 *
 * 使用方式 / Usage:
 * ```cpp
 * // 获取当前后端的工厂
 * auto factory = RenderNodeFactoryRegistry::instance().getFactory(BackendType::Coin3D);
 *
 * // 或获取默认工厂
 * auto factory = RenderNodeFactoryRegistry::instance().getDefaultFactory();
 *
 * // 创建节点
 * auto separator = factory->createSeparator();
 * auto material = factory->createMaterial();
 * ```
 */
class GuiExport RenderNodeFactoryRegistry {
public:
    /**
     * @brief 获取单例实例 / Get singleton instance
     */
    static RenderNodeFactoryRegistry& instance();

    /**
     * @brief 注册工厂 / Register factory
     *
     * @param type 后端类型 / Backend type
     * @param factory 工厂实例 / Factory instance
     */
    void registerFactory(BackendType type, RenderNodeFactory::Ptr factory);

    /**
     * @brief 注销工厂 / Unregister factory
     *
     * @param type 后端类型 / Backend type
     */
    void unregisterFactory(BackendType type);

    /**
     * @brief 获取工厂 / Get factory
     *
     * @param type 后端类型 / Backend type
     * @return 工厂实例，如果未注册则返回 nullptr / Factory instance, nullptr if not registered
     */
    RenderNodeFactory::Ptr getFactory(BackendType type) const;

    /**
     * @brief 获取默认工厂 / Get default factory
     *
     * 返回当前设置的默认后端工厂。
     * Returns the factory for the currently set default backend.
     */
    RenderNodeFactory::Ptr getDefaultFactory() const;

    /**
     * @brief 设置默认后端 / Set default backend
     *
     * @param type 后端类型 / Backend type
     */
    void setDefaultBackend(BackendType type);

    /**
     * @brief 获取默认后端类型 / Get default backend type
     */
    BackendType getDefaultBackend() const { return _defaultBackend; }

    /**
     * @brief 检查工厂是否已注册 / Check if factory is registered
     */
    bool hasFactory(BackendType type) const;

    /**
     * @brief 获取所有已注册的后端类型 / Get all registered backend types
     */
    std::vector<BackendType> getRegisteredBackends() const;

    /**
     * @brief 获取后端信息 / Get backend info
     */
    BackendInfo getBackendInfo(BackendType type) const;

private:
    RenderNodeFactoryRegistry() = default;
    ~RenderNodeFactoryRegistry() = default;

    // 禁止拷贝和移动 / Disable copy and move
    RenderNodeFactoryRegistry(const RenderNodeFactoryRegistry&) = delete;
    RenderNodeFactoryRegistry& operator=(const RenderNodeFactoryRegistry&) = delete;

    mutable std::mutex _mutex;
    std::unordered_map<BackendType, RenderNodeFactory::Ptr> _factories;
    BackendType _defaultBackend{BackendType::Coin3D};
};

//=============================================================================
// 工厂自动注册辅助 / Factory Auto-Registration Helper
//=============================================================================

/**
 * @brief 工厂自动注册器 / Factory auto-registrar
 *
 * 用于在静态初始化时自动注册工厂。
 * For automatic factory registration during static initialization.
 *
 * 使用方式 / Usage:
 * ```cpp
 * // 在 .cpp 文件中
 * static RenderNodeFactoryRegistrar<Coin3DNodeFactory> coin3dRegistrar(BackendType::Coin3D);
 * ```
 */
template<typename FactoryType>
class RenderNodeFactoryRegistrar {
public:
    explicit RenderNodeFactoryRegistrar(BackendType type) {
        RenderNodeFactoryRegistry::instance().registerFactory(
            type,
            std::make_shared<FactoryType>()
        );
    }
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_RENDERNODEFACTORY_H
