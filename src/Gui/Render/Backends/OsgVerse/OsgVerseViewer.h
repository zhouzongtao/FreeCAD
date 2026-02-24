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

#ifndef GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEVIEWER_H
#define GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEVIEWER_H

#include <memory>
#include <array>
#include <functional>
#include <list>
#include <set>
#include <map>
#include <vector>
#include <QOpenGLWidget>
#include <QCursor>
#include <osg/ref_ptr>
#include <osg/Node>
#include <osg/ClipPlane>
#include <osg/ClipNode>

#include <FCGlobal.h>
#include <Base/Vector3D.h>
#include <Base/Tools2D.h>
#include <Base/BoundBox.h>
#include <Base/Matrix.h>
#include <Base/Placement.h>
#include "../../Core/RenderViewer.h"
#include "OsgVerseEngine.h"

class QDragEnterEvent;
class QDropEvent;
class QColor;
class QImage;

// OsgVerse / OSG 前向声明 / Forward declarations
namespace osgViewer {
    class Viewer;
    class GraphicsWindowEmbedded;
}

namespace osgGA {
    class CameraManipulator;
}

namespace osg {
    class Camera;
    class Group;
    class Geometry;
    class MatrixTransform;
}

// FreeCAD 前向声明 / FreeCAD forward declarations
namespace Gui {
    class ViewProvider;
}

namespace Gui {
namespace Render {

/**
 * @brief OsgVerse 查看器 / OsgVerse viewer
 *
 * 基于 OpenSceneGraph 的 3D 查看器实现。
 * 3D viewer implementation based on OpenSceneGraph.
 *
 * 设计目标 / Design Goals:
 * - 与 Qt 无缝集成
 * - 提供与 View3DInventorViewer 相似的交互体验
 * - 支持现代渲染特性（PBR、HDR 等）
 * - 高性能渲染和流畅的交互
 *
 * 技术特性 / Technical Features:
 * - 基于 QOpenGLWidget 的 Qt 集成
 * - 支持多线程渲染
 * - 内置相机操纵器
 * - 事件处理和交互
 */
class GuiExport OsgVerseViewer : public RenderViewer {
public:
    /**
     * @brief 构造函数 / Constructor
     */
    OsgVerseViewer();

    /**
     * @brief 析构函数 / Destructor
     */
    ~OsgVerseViewer() override;

    //-----------------------------------------------------------------------
    // 场景图管理 / Scene Graph Management
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

    //-----------------------------------------------------------------------
    // 相机控制 / Camera Control
    //-----------------------------------------------------------------------

    void setCamera(const CameraParams& params) override;
    CameraParams getCamera() const override;
    void resetCamera() override;
    void fitAll() override;
    void fitSelection() override;

    //-----------------------------------------------------------------------
    // 灯光控制 / Light Control
    //-----------------------------------------------------------------------

    void setAmbientIntensity(float intensity) override;
    float getAmbientIntensity() const override;
    void setBacklightEnabled(bool enabled) override;
    bool isBacklightEnabled() const override;

    //-----------------------------------------------------------------------
    // 窗口集成 / Window Integration
    //-----------------------------------------------------------------------

    QWidget* getWidget() const override;
    void resize(int width, int height) override;
    void onResize(int width, int height) override;

    //-----------------------------------------------------------------------
    // 截图功能 / Screenshot Functionality
    //-----------------------------------------------------------------------

    QImage grabImage(int width, int height) override;
    bool saveScreenshot(const QString& filename, int width, int height) override;

    //-----------------------------------------------------------------------
    // 统计信息 / Statistics
    //-----------------------------------------------------------------------

    RenderStats getStats() const override;
    void resetStats() const override;

    //-----------------------------------------------------------------------
    // ViewProvider 管理 / ViewProvider Management
    //-----------------------------------------------------------------------

    /**
     * @brief 添加 ViewProvider 到场景 / Add ViewProvider to scene
     * @param vp ViewProvider 指针 / ViewProvider pointer
     */
    void addViewProvider(Gui::ViewProvider* vp);

    /**
     * @brief 从场景移除 ViewProvider / Remove ViewProvider from scene
     * @param vp ViewProvider 指针 / ViewProvider pointer
     */
    void removeViewProvider(Gui::ViewProvider* vp);

    /**
     * @brief 更新 ViewProvider 的显示 / Update ViewProvider display
     * @param vp ViewProvider 指针 / ViewProvider pointer
     */
    void updateViewProvider(Gui::ViewProvider* vp);

    /**
     * @brief 检查是否包含指定 ViewProvider / Check if contains ViewProvider
     * @param vp ViewProvider 指针 / ViewProvider pointer
     * @return 是否包含 / Whether contains
     */
    bool hasViewProvider(Gui::ViewProvider* vp) const;

    /**
     * @brief 获取所有 ViewProvider / Get all ViewProviders
     * @return ViewProvider 列表 / ViewProvider list
     */
    std::vector<Gui::ViewProvider*> getViewProviders() const;

    //-----------------------------------------------------------------------
    // 兼容性接口 / Compatibility Interface
    //-----------------------------------------------------------------------

    void* getNativePointer() const override;
    BackendType getBackendType() const override { return BackendType::OsgVerse; }
    void setEventCallback(EventCallback callback) override;

    //-----------------------------------------------------------------------
    // 查看器模式 / Viewer Modes
    //-----------------------------------------------------------------------

    ViewerMode getViewerMode() const override;
    void setViewerMode(ViewerMode mode) override;
    bool isAnimating() const override;
    void stopAnimation() override;

    //-----------------------------------------------------------------------
    // OsgVerse 特定接口 / OsgVerse-specific Interface
    //-----------------------------------------------------------------------

    /**
     * @brief 获取 OSG 查看器 / Get OSG viewer
     */
    osgViewer::Viewer* getOsgViewer() const { return _viewer; }

    /**
     * @brief 获取渲染引擎 / Get render engine
     */
    OsgVerseEngine* getEngine() const { return _engine.get(); }

    //-----------------------------------------------------------------------
    // 相机动画 / Camera Animation
    //-----------------------------------------------------------------------

    /**
     * @brief 设置相机动画持续时间 / Set camera animation duration
     */
    void setAnimationDuration(float duration) { _animationDuration = duration; }

    /**
     * @brief 获取相机动画持续时间 / Get camera animation duration
     */
    float getAnimationDuration() const { return _animationDuration; }

    /**
     * @brief 启用/禁用相机动画 / Enable/disable camera animation
     */
    void setAnimationEnabled(bool enabled) { _animationEnabled = enabled; }

    /**
     * @brief 检查相机动画是否启用 / Check if camera animation is enabled
     */
    bool isAnimationEnabled() const { return _animationEnabled; }

    //-----------------------------------------------------------------------
    // 预设视角 / Preset Views
    //-----------------------------------------------------------------------

    /**
     * @brief 预设视角枚举 / Preset view enumeration
     */
    enum class PresetView {
        Default,    ///< 默认视角 / Default view
        Top,       ///< 俯视 / Top view
        Front,      ///< 正视 / Front view
        Right,      ///< 右视 / Right view
        Left,       ///< 左视 / Left view
        Bottom,     ///< 底视 / Bottom view
        Rear,       ///< 后视 / Rear view
        Iso,        ///< 等轴测视角 / Isometric view
        User        ///< 用户自定义视角 / User custom view
    };

    /**
     * @brief 设置预设视角 / Set preset view
     */
    void setPresetView(PresetView view);

    /**
     * @brief 获取当前预设视角 / Get current preset view
     */
    PresetView getPresetView() const { return _presetView; }

    /**
     * @brief 保存当前视角为预设 / Save current view as preset
     */
    void saveCurrentViewAsPreset(const std::string& name);

    /**
     * @brief 切换到预设视角 / Switch to preset view
     */
    void switchToPresetView(const std::string& name);

    /**
     * @brief 获取预设视角名称 / Get preset view name
     */
    std::string getPresetViewName() const;

    /**
     * @brief 设置相机操纵器 / Set camera manipulator
     */
    void setCameraManipulator(void* manipulator);

    /**
     * @brief 启用/禁用统计显示 / Enable/disable statistics display
     */
    void setStatsEnabled(bool enabled);

    /**
     * @brief 检查统计显示是否启用 / Check if statistics display is enabled
     */
    bool isStatsEnabled() const { return _statsEnabled; }

    //-----------------------------------------------------------------------
    // Selection support / 选择支持
    //-----------------------------------------------------------------------

    /**
     * @brief Find ViewProvider from an OSG node path
     *
     * Walks up the node hierarchy to find the ViewProvider that owns
     * the given node. Used by the picking system.
     */
    Gui::ViewProvider* findViewProviderForNode(osg::Node* node) const;

    /**
     * @brief Get the OSG node for a ViewProvider
     */
    osg::Node* getNodeForViewProvider(Gui::ViewProvider* vp) const;

    /**
     * @brief Get the node-to-VP mapping (for picking service)
     */
    const std::map<osg::Node*, Gui::ViewProvider*>& getNodeToVPMap() const { return _nodeToVPMap; }

    //-----------------------------------------------------------------------
    // Selection integration / 选择集成
    //-----------------------------------------------------------------------

    /**
     * @brief Handle preselection on mouse move
     * @param screenX Screen X coordinate
     * @param screenY Screen Y coordinate
     */
    void handlePreselection(int screenX, int screenY);

    /**
     * @brief Handle click selection
     * @param screenX Screen X coordinate
     * @param screenY Screen Y coordinate
     * @param ctrlPressed Whether Ctrl is held (multi-select)
     */
    void handleSelection(int screenX, int screenY, bool ctrlPressed);

    /**
     * @brief Update selection highlight for a ViewProvider
     */
    void updateSelectionHighlight(Gui::ViewProvider* vp, bool selected);

    /**
     * @brief Update preselection highlight for a ViewProvider
     */
    void updatePreselectionHighlight(Gui::ViewProvider* vp, bool preselected);

    /**
     * @brief Clear all selection highlights
     */
    void clearSelectionHighlights();

    //-----------------------------------------------------------------------
    // Editing mode / 编辑模式
    //-----------------------------------------------------------------------

    /**
     * @brief Enter editing mode for a ViewProvider
     * @param vp The ViewProvider to edit
     * @param mode The editing mode
     */
    void setEditingViewProvider(Gui::ViewProvider* vp, int mode);

    /**
     * @brief Get the currently editing ViewProvider
     */
    Gui::ViewProvider* getEditingViewProvider() const { return _editingVP; }

    /**
     * @brief Check if in editing mode
     */
    bool isEditing() const { return _editingVP != nullptr; }

    /**
     * @brief Exit editing mode
     */
    void resetEditingViewProvider();

    /**
     * @brief Handle box/region selection
     * @param x1, y1 First corner (screen coords)
     * @param x2, y2 Second corner (screen coords)
     * @param ctrlPressed Whether Ctrl is held
     */
    void handleBoxSelection(int x1, int y1, int x2, int y2, bool ctrlPressed);

    /**
     * @brief Enable/disable rubber band selection mode on the widget
     */
    void setRubberBandEnabled(bool enabled);

    //-----------------------------------------------------------------------
    // Gradient Background / 渐变背景
    //-----------------------------------------------------------------------

    /**
     * @brief Set gradient background colors
     * @param topR/topG/topB Top color RGB [0..1]
     * @param botR/botG/botB Bottom color RGB [0..1]
     */
    void setGradientBackground(float topR, float topG, float topB,
                               float botR, float botG, float botB);

    //-----------------------------------------------------------------------
    // Axis Cross / 坐标轴十字
    //-----------------------------------------------------------------------

    /**
     * @brief Enable/disable the axis cross display
     */
    void setAxisCrossEnabled(bool enabled);
    bool isAxisCrossEnabled() const { return _axisCrossEnabled; }

    //-----------------------------------------------------------------------
    // Navigation Style System / 导航风格系统
    //-----------------------------------------------------------------------

    /** Navigation action types */
    enum class NavAction {
        None,
        Rotate,
        Pan,
        Zoom,
        Seek,
        BoxZoom,
        BoxSelect
    };

    /**
     * @brief Set navigation style
     * @param style "CAD", "Blender", "Inventor", "Trackball"
     */
    void setNavigationStyle(const std::string& style);
    std::string getNavigationStyle() const { return _navigationStyle; }

    /** Determine navigation action from mouse event */
    NavAction getNavAction(QMouseEvent* event) const;

    /** Scale camera (zoom in/out by factor) */
    void scale(float factor);

    /** Move camera to a world position (animated) */
    void moveCameraTo(const Base::Vector3d& target);

    /** Get list of supported navigation types */
    static std::vector<std::string> listNavigationTypes();

    //-----------------------------------------------------------------------
    // Context menu / 右键菜单
    //-----------------------------------------------------------------------

    /**
     * @brief Show context menu at the given position
     */
    void showContextMenu(const QPoint& globalPos);

    //-----------------------------------------------------------------------
    // Phase G: 焦平面投影 / Focal Plane Projection
    //-----------------------------------------------------------------------

    /**
     * @brief 获取焦平面上的 3D 点 / Get 3D point on focal plane
     *
     * 将屏幕坐标投影到焦平面（过相机目标点、法线为视线方向的平面）
     *
     * @param x 屏幕 X 坐标 / Screen X coordinate
     * @param y 屏幕 Y 坐标 / Screen Y coordinate
     * @return 焦平面上的 3D 世界坐标 / 3D world coordinate on focal plane
     */
    Base::Vector3d getPointOnFocalPlane(int x, int y) const;

    //-----------------------------------------------------------------------
    // Coordinate Projection System / 坐标投影系统
    //-----------------------------------------------------------------------

    /** Get view direction (normalized, from eye toward target) */
    Base::Vector3d getViewDirection() const;

    /** Get up direction */
    Base::Vector3d getUpDirection() const;

    /** Get camera orientation as quaternion (x, y, z, w) */
    void getCameraOrientation(double& x, double& y, double& z, double& w) const;

    /** Project 3D world point to 2D screen coordinates */
    QPoint getPointOnViewport(const Base::Vector3d& pt) const;

    /** Get 3D point on a line closest to the screen point */
    Base::Vector3d getPointOnLine(const QPoint& screenPos, const Base::Vector3d& axisCenter, const Base::Vector3d& axis) const;

    /** Get 3D point on the XY plane of a placement */
    Base::Vector3d getPointOnXYPlaneOfPlacement(const QPoint& screenPos, const Base::Placement& plc) const;

    /** Get normalized screen position [0..1] */
    Base::Vector2d getNormalizedPosition(const QPoint& screenPos) const;

    /** Project normalized 2D point onto near plane */
    Base::Vector3d projectOnNearPlane(const Base::Vector2d& pt) const;

    /** Project normalized 2D point onto far plane */
    Base::Vector3d projectOnFarPlane(const Base::Vector2d& pt) const;

    /** Project screen 2D point to a 3D ray (two points) */
    void projectPointToLine(const QPoint& screenPos, Base::Vector3d& pt1, Base::Vector3d& pt2) const;

    /** Get center point on focal plane */
    Base::Vector3d getCenterPointOnFocalPlane() const;

    /** Get near plane (point + normal) */
    void getNearPlane(Base::Vector3d& pt, Base::Vector3d& normal) const;

    /** Get far plane (point + normal) */
    void getFarPlane(Base::Vector3d& pt, Base::Vector3d& normal) const;

    /** Get viewport dimensions in world units at focal distance */
    void getDimensions(float& height, float& width) const;

    /** Get max viewport dimension */
    float getMaxDimension() const;

    /** Get scene bounding box */
    void getBoundingBox(Base::Vector3d& min, Base::Vector3d& max) const;

    //-----------------------------------------------------------------------
    // Phase G: 编辑根节点 / Editing Root Node
    //-----------------------------------------------------------------------

    /**
     * @brief 设置编辑根节点 / Setup editing root node
     */
    void setupEditingRoot(void* node = nullptr, const Base::Matrix4D* mat = nullptr);

    /**
     * @brief 重置编辑根节点 / Reset editing root node
     */
    void resetEditingRoot(bool updateLinks = true);

    /**
     * @brief 设置编辑变换矩阵 / Set editing transform matrix
     */
    void setEditingTransform(const Base::Matrix4D& mat);

    //-----------------------------------------------------------------------
    // Phase G: Seek 功能
    //-----------------------------------------------------------------------

    /**
     * @brief 飞行到屏幕点击位置 / Seek to screen position
     */
    bool seekToPoint(int screenX, int screenY);

    /**
     * @brief 飞行到世界坐标点 / Seek to world position
     */
    void seekToPoint(const Base::Vector3d& worldPos);

    //-----------------------------------------------------------------------
    // Phase G: 拾取半径 / Pick Radius
    //-----------------------------------------------------------------------

    float getPickRadius() const { return _pickRadius; }
    void setPickRadius(float radius) { _pickRadius = radius; }

    //-----------------------------------------------------------------------
    // Phase H: 渲染模式覆盖 / Override Mode
    //-----------------------------------------------------------------------

    void setOverrideMode(const std::string& mode);
    std::string getOverrideMode() const { return _overrideMode; }

    //-----------------------------------------------------------------------
    // Phase H: 惯性旋转 / Spin Animation
    //-----------------------------------------------------------------------

    void setSpinAnimationEnabled(bool enabled) { _spinEnabled = enabled; }
    bool isSpinAnimationEnabled() const { return _spinEnabled; }

    //-----------------------------------------------------------------------
    // Phase I: 裁剪面 / Clip Planes
    //-----------------------------------------------------------------------

    void setClipPlane(int index, double a, double b, double c, double d);
    void removeClipPlane(int index);
    bool isClipPlaneEnabled(int index) const;
    void toggleClipPlane(int index);

    //-----------------------------------------------------------------------
    // Event Callback System / 事件回调系统
    //-----------------------------------------------------------------------

    /** Event types for callback registration */
    enum class EventType {
        MouseButtonPress,
        MouseButtonRelease,
        MouseMove,
        KeyPress,
        KeyRelease,
        Wheel,
        Any
    };

    /** Callback function: (eventType, QEvent*, userData) -> handled */
    using EventCallbackFunc = std::function<bool(EventType, void*, void*)>;

    struct EventCallbackEntry {
        EventType type;
        EventCallbackFunc callback;
        void* userData;
    };

    /** Register an event callback */
    void addEventCallback(EventType type, EventCallbackFunc cb, void* userData = nullptr);

    /** Unregister an event callback */
    void removeEventCallback(EventType type, EventCallbackFunc cb, void* userData = nullptr);

    /** Dispatch event to registered callbacks */
    bool dispatchEventCallbacks(EventType type, void* event);

    //-----------------------------------------------------------------------
    // Editing Mode Extensions / 编辑模式扩展
    //-----------------------------------------------------------------------

    /** Set editing flag */
    void setEditing(bool edit);

    /** Set cursor for editing mode */
    void setEditingCursor(const QCursor& cursor);

    /** Set cursor for component selection */
    void setComponentCursor(const QCursor& cursor);

    /** Redirect events to scene graph */
    void setRedirectToSceneGraph(bool redirect);
    bool isRedirectedToSceneGraph() const { return _redirectToSceneGraph; }

    /** Enable/disable selection */
    void setSelectionEnabled(bool enable);
    bool isSelectionEnabled() const { return _selectionEnabled; }

    /** Enable/disable popup menu */
    void setPopupMenuEnabled(bool on);
    bool isPopupMenuEnabled() const { return _popupMenuEnabled; }

    //-----------------------------------------------------------------------
    // Graphics Overlay System / 图形覆盖层系统
    //-----------------------------------------------------------------------

    /** Opaque handle for graphics items */
    using GraphicsItemHandle = void*;

    /** Add a 2D graphics overlay item */
    void addGraphicsItem(GraphicsItemHandle item);

    /** Remove a 2D graphics overlay item */
    void removeGraphicsItem(GraphicsItemHandle item);

    /** Clear all graphics overlay items */
    void clearGraphicsItems();

    /** Get all graphics items */
    const std::vector<GraphicsItemHandle>& getGraphicsItems() const { return _graphicsItems; }

    //-----------------------------------------------------------------------
    // Dimension Annotations / 尺寸标注
    //-----------------------------------------------------------------------

    /** Add a 3D dimension annotation */
    void addDimension3d(const Base::Vector3d& p1, const Base::Vector3d& p2, const Base::Vector3d& textPos);

    /** Add a delta dimension annotation */
    void addDimensionDelta(const Base::Vector3d& p1, const Base::Vector3d& p2, const Base::Vector3d& textPos);

    /** Turn all dimensions on */
    void turnAllDimensionsOn();

    /** Turn all dimensions off */
    void turnAllDimensionsOff();

    /** Erase all dimensions */
    void eraseAllDimensions();

    /** Set dimensions visibility */
    void setDimensionsVisible(bool visible);
    bool areDimensionsVisible() const { return _dimensionsVisible; }

    //-----------------------------------------------------------------------
    // Save Picture / 保存图片
    //-----------------------------------------------------------------------

    /** Save picture with multi-sampling support */
    void savePicture(int width, int height, int samples, const QColor& bg, QImage& img) const;

    //-----------------------------------------------------------------------
    // Box Zoom / 框选缩放
    //-----------------------------------------------------------------------

    /** Zoom to fit a screen rectangle */
    void boxZoom(int x1, int y1, int x2, int y2);

    //-----------------------------------------------------------------------
    // Selection Polygon / 选择多边形
    //-----------------------------------------------------------------------

    /** Get selection polygon vertices in screen coordinates */
    std::vector<std::pair<int,int>> getSelectionPolygon(bool* isClosed = nullptr) const;

    /** Get selection polygon vertices in normalized device coordinates */
    std::vector<std::pair<float,float>> getSelectionPolygonNormalized(bool* isClosed = nullptr) const;

    //-----------------------------------------------------------------------
    // Ray Picking / 射线拾取
    //-----------------------------------------------------------------------

    /** Get intersection point of screen ray with ViewProvider geometry */
    Base::Vector3d getPointOnRay(const QPoint& screenPos, const Gui::ViewProvider* vp) const;

    /** Get intersection point of 3D ray with ViewProvider geometry */
    Base::Vector3d getPointOnRay(const Base::Vector3d& rayOrigin, const Base::Vector3d& rayDir, const Gui::ViewProvider* vp) const;

    //-----------------------------------------------------------------------
    // Viewport on Placement Plane / 视口投影到放置平面
    //-----------------------------------------------------------------------

    /** Get viewport bounding box projected onto XY plane of a placement */
    Base::BoundBox2d getViewportOnXYPlaneOfPlacement(const Base::Placement& plc) const;

    //-----------------------------------------------------------------------
    // Align to Selection / 对齐到选择
    //-----------------------------------------------------------------------

    /** Align camera to selected face normal */
    void alignToSelection();

private:
    /**
     * @brief 初始化查看器 / Initialize viewer
     */
    void initializeViewer();

    /**
     * @brief 初始化 Qt 窗口 / Initialize Qt widget
     */
    void initializeWidget();

    /**
     * @brief 设置默认相机 / Setup default camera
     */
    void setupDefaultCamera();

    /**
     * @brief 设置默认光照 / Setup default lighting
     */
    void setupDefaultLighting();

    /**
     * @brief Setup gradient background HUD camera
     */
    void setupGradientBackground();

    /**
     * @brief Setup axis cross HUD camera
     */
    void setupAxisCross();

    /**
     * @brief Update axis cross rotation to match main camera
     */
    void updateAxisCross();

    /**
     * @brief 确保已初始化 / Ensure initialized
     * 
     * 延迟初始化模式：只在第一次使用时才初始化
     * Lazy initialization: only initialize on first use
     */
    void ensureInitialized();

    /**
     * @brief 检查前置条件 / Check prerequisites
     * 
     * @return true 如果所有前置条件满足 / if all prerequisites are met
     */
    bool checkPrerequisites();

    /**
     * @brief Qt 窗口类 / Qt widget class
     */
    class ViewerWidget;

    // 核心组件 / Core components
    std::unique_ptr<OsgVerseEngine> _engine;    ///< 渲染引擎 / Render engine
    osgViewer::Viewer* _viewer{nullptr};        ///< OSG 查看器 / OSG viewer
    ViewerWidget* _widget{nullptr};             ///< Qt 窗口 / Qt widget
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> _graphicsWindow; ///< 嵌入式图形窗口 / Embedded graphics window

    // 初始化状态 / Initialization state
    bool _initialized{false};                   ///< 是否已初始化 / Whether initialized
    bool _initializationFailed{false};          ///< 初始化是否失败 / Whether initialization failed
    bool _firstFrame{true};                     ///< 是否是第一帧 / Whether first frame
    bool _pendingFitAll{true};                  ///< 延迟fitAll / Deferred fitAll on first geometry

    //-----------------------------------------------------------------------
    // 相机动画内部方法 / Camera Animation Internal Methods
    //-----------------------------------------------------------------------

    /**
     * @brief 启动相机动画 / Start camera animation
     * @param targetEye 目标相机位置 / Target camera position
     * @param targetCenter 目标中心点 / Target center point
     */
    void startCameraAnimation(const Vec3d& targetEye, const Vec3d& targetCenter);

    /**
     * @brief 更新相机动画 / Update camera animation
     * @param currentTime 当前时间 / Current time
     */
    void updateCameraAnimation(double currentTime);

    /**
     * @brief 停止相机动画 / Stop camera animation
     */
    void stopCameraAnimation();

    /**
     * @brief 更新惯性旋转 / Update spin animation
     */
    void updateSpinAnimation();

    // 相机动画成员 / Camera animation members
    float _animationDuration{1.0f};             ///< 动画持续时间（秒）/ Animation duration (seconds)
    bool _animationEnabled{false};              ///< 动画是否启用 / Whether animation is enabled
    Vec3d _animationTargetEye;          ///< 动画目标位置 / Animation target position
    Vec3d _animationTargetCenter;       ///< 动画目标中心 / Animation target center
    float _animationStartTime{0.0f};        ///< 动画开始时间 / Animation start time
    bool _animationComplete{true};          ///< 动画是否完成 / Whether animation is complete

    // 预设视角成员 / Preset view members
    PresetView _presetView{PresetView::Default};  ///< 当前预设视角 / Current preset view
    std::vector<std::string> _savedViewNames;    ///< 保存的视角名称列表 / Saved view names
    std::map<std::string, CameraParams> _savedViewParams; ///< 保存的视角参数 / Saved view parameters

    // 相机动画目标位置 / Camera animation target
    struct CameraAnimationTarget {
        Vec3d eye;
        Vec3d center;
        float distance;
    };
    CameraAnimationTarget _animationTarget;

    // 场景状态 / Scene state
    RenderNode::Ptr _sceneRoot;
    RenderMode _renderMode{RenderMode::Default};
    Color _backgroundColor{0.2f, 0.2f, 0.3f, 1.0f};

    // 相机状态 / Camera state
    CameraParams _cameraParams;
    float _ambientIntensity{0.3f};
    bool _backlightEnabled{true};

    // 查看器状态 / Viewer state
    ViewerMode _viewerMode{ViewerMode::Idle};
    bool _isAnimating{false};
    bool _statsEnabled{false};

    // 事件回调 / Event callback
    EventCallback _eventCallback;

    // ViewProvider 管理 / ViewProvider Management
    std::set<Gui::ViewProvider*> _viewProviders;                       ///< ViewProvider 集合 / ViewProvider set
    std::map<Gui::ViewProvider*, osg::ref_ptr<osg::Node>> _vpToNodeMap; ///< VP到OSG节点的映射 / VP to OSG node mapping
    std::map<osg::Node*, Gui::ViewProvider*> _nodeToVPMap;            ///< OSG节点到VP的映射 / OSG node to VP mapping

    // NaviCube
    std::unique_ptr<class OsgVerseNaviCube> _naviCube;                ///< 导航立方体 / Navigation cube
    bool _naviCubeEnabled{true};                                       ///< NaviCube是否启用 / Whether NaviCube is enabled

    // Navigation style / 导航风格
    std::string _navigationStyle{"CAD"};                               ///< Current navigation style

    // Selection state / 选择状态
    std::unique_ptr<class OsgVersePickingService> _pickingService;    ///< Picking service
    Gui::ViewProvider* _preselectedVP{nullptr};                        ///< Currently preselected VP
    std::string _preselectedElement;                                   ///< Preselected sub-element

    // Editing state / 编辑状态
    Gui::ViewProvider* _editingVP{nullptr};                            ///< Currently editing VP
    int _editingMode{0};                                               ///< Editing mode
    osg::ref_ptr<osg::Group> _editingRoot;                             ///< Editing scene root

    // Phase G: Editing root / 编辑根节点
    osg::ref_ptr<osg::Group> _editingRootNode;                         ///< Editing root group node
    osg::ref_ptr<osg::MatrixTransform> _editingTransform;              ///< Editing transform node

    // Phase G: Pick radius / 拾取半径
    float _pickRadius{5.0f};                                           ///< Pick radius in pixels

    // Phase H: Override mode / 渲染模式覆盖
    std::string _overrideMode;                                         ///< Current override mode string

    // Phase H: Spin animation / 惯性旋转
    bool _spinEnabled{true};                                           ///< Whether spin animation is enabled
    bool _isSpinning{false};                                           ///< Whether currently spinning
    double _spinVelocityX{0.0};                                        ///< Spin velocity X
    double _spinVelocityY{0.0};                                        ///< Spin velocity Y
    double _spinDecay{0.95};                                           ///< Spin decay factor per frame

    // Phase I: Clip planes / 裁剪面
    std::array<osg::ref_ptr<osg::ClipPlane>, 6> _clipPlanes;          ///< Clip planes
    osg::ref_ptr<osg::ClipNode> _clipNode;                             ///< Clip node
    std::array<bool, 6> _clipPlaneEnabled{};                           ///< Clip plane enabled flags

    // Gradient background / 渐变背景
    osg::ref_ptr<osg::Camera> _gradientCamera;                         ///< Gradient HUD camera
    osg::ref_ptr<osg::Geometry> _gradientGeom;                         ///< Gradient quad geometry

    // Axis cross / 坐标轴十字
    osg::ref_ptr<osg::Camera> _axisCrossCamera;                        ///< Axis cross HUD camera
    osg::ref_ptr<osg::MatrixTransform> _axisCrossTransform;            ///< Rotation transform for axes
    bool _axisCrossEnabled{true};                                       ///< Whether axis cross is enabled

    // Event callback system / 事件回调系统
    std::list<EventCallbackEntry> _eventCallbacks;

    // Editing extensions / 编辑模式扩展
    bool _editingFlag{false};
    QCursor _editingCursor;
    QCursor _componentCursor;
    bool _redirectToSceneGraph{false};
    bool _selectionEnabled{true};
    bool _popupMenuEnabled{true};

    // Graphics overlay / 图形覆盖层
    std::vector<GraphicsItemHandle> _graphicsItems;

    // Dimensions / 尺寸标注
    bool _dimensionsVisible{true};

    // Selection polygon / 选择多边形
    mutable std::vector<std::pair<int,int>> _selectionPolygon;
    mutable bool _selectionPolygonClosed{false};
};

/**
 * @brief OsgVerse 查看器的 Qt 窗口部件 / Qt widget for OsgVerse viewer
 *
 * 集成 OSG 渲染到 Qt 窗口系统。
 * Integrates OSG rendering into Qt widget system.
 */
class OsgVerseViewer::ViewerWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    ViewerWidget(OsgVerseViewer* osgVerseViewer,
                 osgViewer::Viewer* viewer,
                 osgViewer::GraphicsWindowEmbedded* graphicsWindow,
                 QWidget* parent = nullptr);
    ~ViewerWidget() override;

    osgViewer::GraphicsWindowEmbedded* getGraphicsWindow() const { return _graphicsWindow.get(); }

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    // 鼠标事件 / Mouse events
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    // 键盘事件 / Keyboard events
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    // 绘制事件 / Paint event (for rubber band overlay)
    void paintEvent(QPaintEvent* event) override;

    // Drag-drop events
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    OsgVerseViewer* _osgVerseViewer;  ///< 指向外部OsgVerseViewer的指针 / Pointer to outer OsgVerseViewer
    osgViewer::Viewer* _viewer;
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> _graphicsWindow;
    bool _firstFrame{true};

    // Rubber band selection state
    bool _rubberBandActive{false};
    QPoint _rubberBandStart;
    QPoint _rubberBandEnd;

    // Last mouse position for spin velocity calculation
    double _lastMouseX{0.0};
    double _lastMouseY{0.0};
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEVIEWER_H
