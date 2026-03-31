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

#ifndef GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEENGINE_H
#define GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEENGINE_H

#include <memory>
#include <string>

#include <osg/Matrix>
#include <FCGlobal.h>
#include "../../Core/RenderEngine.h"
#include "../../Core/RenderNode.h"

// Forward declaration
namespace Gui {
namespace Render {
class OsgVerseShadowMap;
class PostProcessChain;
class OsgVerseBloom;
class OsgVerseSSAO;
class OsgVerseTAA;
}
}

// OsgVerse / OSG 前向声明 / Forward declarations
namespace osg {
    class Node;
    class Group;
    class Camera;
    class Light;
}

namespace osgViewer {
    class Viewer;
}

namespace Gui {
namespace Render {

/**
 * @brief OsgVerse 渲染引擎 / OsgVerse rendering engine
 *
 * OsgVerse (基于 OpenSceneGraph) 渲染后端的包装器类。
 * Wrapper class for the OsgVerse (based on OpenSceneGraph) rendering backend.
 *
 * 设计目标 / Design Goals:
 * - 提供现代化的渲染能力（PBR、HDR、后处理等）
 * - 与 Coin3D 后端保持接口一致性
 * - 支持高性能渲染和大规模场景
 * - 提供平滑的迁移路径
 *
 * 技术特性 / Technical Features:
 * - 基于 OpenSceneGraph 3.6+ 和 OsgVerse 扩展
 * - 支持 PBR 材质和现代渲染管线
 * - 内置后处理效果（SSAO、Bloom、TAA 等）
 * - 优化的场景图管理和剔除
 * - 多线程渲染支持
 */
class GuiExport OsgVerseEngine : public RenderEngine {
public:
    /**
     * @brief 构造函数 / Constructor
     */
    OsgVerseEngine();

    /**
     * @brief 析构函数 / Destructor
     */
    ~OsgVerseEngine() override;

    //-----------------------------------------------------------------------
    // 引擎信息 / Engine Information
    //-----------------------------------------------------------------------

    BackendType getType() const override { return BackendType::OsgVerse; }
    std::string getName() const override;
    std::string getVersion() const override;
    BackendInfo getInfo() const override;
    bool isAvailable() const override;

    bool initialize() override;
    void shutdown() override;

    //-----------------------------------------------------------------------
    // 节点创建 / Node Creation
    //-----------------------------------------------------------------------

    RenderGroup::Ptr createGroup() override;
    RenderSeparator::Ptr createSeparator() override;
    RenderNode::Ptr createTransform() override;
    RenderNode::Ptr createSwitch() override;
    RenderNode::Ptr createMaterial() override;
    RenderNode::Ptr createGeometry() override;
    RenderNode::Ptr createCamera() override;
    RenderNode::Ptr createLight(LightType type) override;

    //-----------------------------------------------------------------------
    // 场景管理 / Scene Management
    //-----------------------------------------------------------------------

    void setSceneRoot(RenderNode::Ptr root) override;
    RenderNode::Ptr getSceneRoot() const override;
    void updateScene() override;

    //-----------------------------------------------------------------------
    // 渲染控制 / Rendering Control
    //-----------------------------------------------------------------------

    void render() override;
    void setRenderMode(RenderMode mode) override;
    RenderMode getRenderMode() const override;
    void setBackgroundColor(const Color& color) override;
    Color getBackgroundColor() const override;
    RenderStats getStats() const override;
    void resetStats() override;

    //-----------------------------------------------------------------------
    // 资源管理 / Resource Management
    //-----------------------------------------------------------------------

    void releaseUnusedResources() override;
    size_t getMemoryUsage() const override;

    //-----------------------------------------------------------------------
    // 兼容性接口 / Compatibility Interface
    //-----------------------------------------------------------------------

    /**
     * @brief 获取原生 OsgVerse 指针 / Get native OsgVerse pointer
     *
     * 返回底层 osgViewer::Viewer
     * Returns underlying osgViewer::Viewer
     */
    void* getNativePointer() const override { return _viewer; }

    void setEventCallback(EventCallback callback) override { _eventCallback = std::move(callback); }

    //-----------------------------------------------------------------------
    // OsgVerse 特定接口 / OsgVerse-specific Interface
    //-----------------------------------------------------------------------

    /**
     * @brief 从 osg::Node 创建包装的 RenderNode / Create wrapped RenderNode from osg::Node
     *
     * 将现有的 OSG 节点包装到抽象接口中。
     * Wraps an existing OSG node into the abstract interface.
     *
     * @param osgNode OSG 节点指针 / OSG node pointer
     * @param takeOwnership 是否获取所有权 / Whether to take ownership
     * @return 包装后的 RenderNode / Wrapped RenderNode
     */
    RenderNode::Ptr wrapNode(osg::Node* osgNode, bool takeOwnership = false);

    /**
     * @brief 从 RenderNode 提取 osg::Node / Extract osg::Node from RenderNode
     *
     * 获取包装器内的底层 OSG 节点。
     * Get the underlying OSG node inside the wrapper.
     *
     * @param renderNode 抽象渲染节点 / Abstract render node
     * @return OSG 节点指针，失败返回 nullptr / OSG node pointer, nullptr on failure
     */
    osg::Node* unwrapNode(RenderNode* renderNode) const;

    /**
     * @brief 设置 OSG 查看器 / Set OSG viewer
     *
     * 用于与 OsgVerseViewer 集成。
     * For integration with OsgVerseViewer.
     */
    void setViewer(osgViewer::Viewer* viewer);

    /**
     * @brief 获取 OSG 查看器 / Get OSG viewer
     */
    osgViewer::Viewer* getViewer() const { return _viewer; }

    /**
     * @brief 获取 OSG 场景根节点 / Get OSG scene root node
     *
     * 直接返回 OSG 的 Group 节点，用于添加光源等场景元素。
     * Returns the OSG Group node directly for adding lights and other scene elements.
     */
    osg::Group* getOsgSceneRoot() const { return _sceneRoot; }

    /**
     * @brief 启用/禁用 PBR 渲染 / Enable/disable PBR rendering
     */
    void setPBREnabled(bool enabled);

    /**
     * @brief 检查 PBR 是否启用 / Check if PBR is enabled
     */
    bool isPBREnabled() const { return _pbrEnabled; }

    /**
     * @brief 启用/禁用 HDR 渲染 / Enable/disable HDR rendering
     */
    void setHDREnabled(bool enabled);

    /**
     * @brief 检查 HDR 是否启用 / Check if HDR is enabled
     */
    bool isHDREnabled() const { return _hdrEnabled; }

    /**
     * @brief 设置抗锯齿模式 / Set anti-aliasing mode
     */
    void setAntiAliasingMode(int samples);

    /**
     * @brief 获取抗锯齿采样数 / Get anti-aliasing samples
     */
    int getAntiAliasingSamples() const { return _aaSamples; }

    /**
     * @brief 启用/禁用阴影 / Enable/disable shadows
     */
    void setShadowsEnabled(bool enabled);

    /**
     * @brief 检查阴影是否启用 / Check if shadows are enabled
     */
    bool areShadowsEnabled() const { return _shadowsEnabled; }

    /**
     * @brief 设置阴影质量 / Set shadow quality
     *
     * @param quality 质量等级 0-3 / Quality level 0-3
     */
    void setShadowQuality(int quality);

    /**
     * @brief 获取阴影质量 / Get shadow quality
     */
    int getShadowQuality() const { return _shadowQuality; }

    /**
     * @brief Enable/disable bloom effect
     */
    void setBloomEnabled(bool enabled);

    /**
     * @brief Check if bloom is enabled
     */
    bool isBloomEnabled() const;

    /**
     * @brief Set bloom brightness threshold
     */
    void setBloomThreshold(float threshold);

    /**
     * @brief Set bloom intensity
     */
    void setBloomIntensity(float intensity);

    /**
     * @brief Enable/disable SSAO effect
     */
    void setSSAOEnabled(bool enabled);

    /**
     * @brief Check if SSAO is enabled
     */
    bool isSSAOEnabled() const;

    /**
     * @brief Set SSAO sample radius
     */
    void setSSAORadius(float radius);

    /**
     * @brief Set SSAO intensity
     */
    void setSSAOIntensity(float intensity);

    /**
     * @brief Enable/disable TAA
     */
    void setTAAEnabled(bool enabled);

    /**
     * @brief Check if TAA is enabled
     */
    bool isTAAEnabled() const;

    /**
     * @brief Set TAA blend factor (0.01-1.0, default 0.1)
     */
    void setTAABlendFactor(float factor);

private:
    /**
     * @brief 初始化渲染管线 / Initialize rendering pipeline
     */
    void initializeRenderingPipeline();

    /**
     * @brief 设置后处理效果 / Setup post-processing effects
     */
    void setupPostProcessing();

public:
    /**
     * @brief Deferred post-processing initialization
     *
     * Must be called from paintGL() on the first frame when GL context is
     * guaranteed available. FBO cameras need a valid GraphicsContext to avoid
     * crashes in OSG's Renderer::compile().
     */
    void initializePostProcessingDeferred();

    /**
     * @brief Check if deferred post-processing init is pending
     */
    bool isPostProcessPending() const { return _postProcessPending; }

private:

    /**
     * @brief 更新渲染统计 / Update rendering statistics
     */
    void updateStats();

    // OsgVerse 内部状态 / OsgVerse internal state
    osgViewer::Viewer* _viewer{nullptr};    ///< OSG 查看器 / OSG viewer
    osg::Group* _sceneRoot{nullptr};        ///< 场景根节点 / Scene root node

    // 渲染状态 / Rendering state
    RenderMode _renderMode{RenderMode::Default};
    Color _backgroundColor{0.2f, 0.2f, 0.3f, 1.0f};
    RenderNode::Ptr _abstractSceneRoot;

    // 渲染特性 / Rendering features
    bool _pbrEnabled{true};                 ///< PBR 渲染 / PBR rendering
    bool _hdrEnabled{true};                 ///< HDR 渲染 / HDR rendering
    bool _shadowsEnabled{false};            ///< 阴影 / Shadows (disabled: shadow map creates scene graph cycle)
    int _shadowQuality{2};                  ///< 阴影质量 / Shadow quality
    int _aaSamples{4};                      ///< 抗锯齿采样数 / AA samples

    // 统计信息 / Statistics
    mutable RenderStats _stats;
    uint32_t _statsUpdateInterval{60};     ///< 场景遍历间隔帧数 / Frames between scene traversals
    uint32_t _framesSinceLastTraversal{0}; ///< 上次遍历后的帧数 / Frames since last traversal

    // 事件回调 / Event callback
    EventCallback _eventCallback;

    // Shadow mapping
    std::unique_ptr<OsgVerseShadowMap> _shadowMap;

    // Post-processing
    std::unique_ptr<PostProcessChain> _postProcessChain;

    // Bloom effect
    std::unique_ptr<OsgVerseBloom> _bloom;

    // TAA
    std::unique_ptr<OsgVerseTAA> _taa;
    osg::Matrix _prevMVP;
    int _frameCount{0};

    // SSAO effect
    std::unique_ptr<OsgVerseSSAO> _ssao;

    // 初始化状态 / Initialization state
    bool _initialized{false};
    bool _postProcessPending{false};  ///< Post-processing deferred init pending
};

//===========================================================================
// Manual Registration Function
//===========================================================================

/**
 * @brief 手动注册 OsgVerse 引擎到工厂
 * Manually register OsgVerse engine to factory
 * 
 * 此函数应在 RenderManager 初始化时调用，而不是使用静态自动注册
 * This function should be called during RenderManager initialization
 * instead of using static auto-registration
 */
GuiExport void registerOsgVerseEngine();

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEENGINE_H
