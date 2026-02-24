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

#ifndef GUI_VIEW3D_IVIEWER3D_H
#define GUI_VIEW3D_IVIEWER3D_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <QColor>
#include <QCursor>
#include <QImage>
#include <QPoint>
#include <QWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>

#include <FCGlobal.h>
#include <Base/BoundBox.h>
#include <Base/Placement.h>
#include <Base/Vector2D.h>
#include <Base/Vector3D.h>
#include <Base/Matrix.h>
#include <Gui/Render/Core/RenderTypes.h>

class QOpenGLWidget;

namespace Gui {

class ViewProvider;
class Document;

namespace View3D {

/**
 * @brief 相机参数结构
 */
struct CameraParams {
    Base::Vector3d position{0.0, 0.0, 10.0};
    Base::Vector3d target{0.0, 0.0, 0.0};
    Base::Vector3d upVector{0.0, 0.0, 1.0};
    double fieldOfView{45.0};
    double aspectRatio{1.0};
    double nearPlane{0.1};
    double farPlane{1000.0};
    bool orthographic{false};
    double height{10.0};  // For orthographic camera
};

/**
 * @brief 拾取类型
 */
enum class PickType {
    None = 0,   ///< 未命中
    Face,       ///< 命中面
    Edge,       ///< 命中边
    Vertex,     ///< 命中顶点
    Object      ///< 命中对象（未细分）
};

/**
 * @brief 拾取结果
 */
struct PickResult {
    bool valid{false};                          ///< 是否有效命中
    Base::Vector3d point;                       ///< 交点坐标（世界坐标系）
    Base::Vector3d normal;                      ///< 交点法线
    ViewProvider* viewProvider{nullptr};        ///< 命中的 ViewProvider
    std::string subElementName;                 ///< 子元素名称 (如 "Face1", "Edge2")
    double distance{0.0};                       ///< 从相机到交点的距离

    // 拾取类型相关字段
    PickType pickType{PickType::None};          ///< 拾取类型 (Face/Edge/Vertex)
    int primitiveIndex{-1};                     ///< 图元索引
    int faceIndex{-1};                          ///< 面索引 (当 pickType == Face)
    int edgeIndex{-1};                          ///< 边索引 (当 pickType == Edge)
    int vertexIndex{-1};                        ///< 顶点索引 (当 pickType == Vertex)

    // 辅助方法
    bool isFace() const { return pickType == PickType::Face; }
    bool isEdge() const { return pickType == PickType::Edge; }
    bool isVertex() const { return pickType == PickType::Vertex; }

    /// 按距离比较（用于排序）
    bool operator<(const PickResult& other) const {
        return distance < other.distance;
    }
};

/**
 * @brief 选择模式
 */
enum class SelectionMode {
    None,
    Lasso,
    Rectangle,
    Rubberband
};

/**
 * @brief 渲染模式
 */
enum class RenderMode {
    AsIs,
    Wireframe,
    Points,
    HiddenLine,
    NoShading,
    Shaded,
    FlatLines
};

/**
 * @brief 背景渐变类型
 */
enum class BackgroundGradientType {
    None,   ///< 无渐变（纯色）
    Linear, ///< 线性渐变（从上到下）
    Radial, ///< 径向渐变（从中心到边缘）
    Corner  ///< 角落渐变（四角双线性插值）
};

/**
 * @brief 背景渐变参数
 */
struct BackgroundGradient {
    BackgroundGradientType type{BackgroundGradientType::None};  ///< 渐变类型
    Base::Color topColor{0.4f, 0.4f, 0.6f};                     ///< 顶部颜色（线性渐变）/ 中心颜色（径向渐变）
    Base::Color bottomColor{0.1f, 0.1f, 0.2f};                  ///< 底部颜色（线性渐变）/ 边缘颜色（径向渐变）
    Base::Color topLeftColor{0.4f, 0.4f, 0.6f};                 ///< 左上角颜色（角落渐变）
    Base::Color topRightColor{0.4f, 0.4f, 0.6f};                ///< 右上角颜色（角落渐变）
    Base::Color bottomLeftColor{0.1f, 0.1f, 0.2f};              ///< 左下角颜色（角落渐变）
    Base::Color bottomRightColor{0.1f, 0.1f, 0.2f};             ///< 右下角颜色（角落渐变）
    float midPoint{0.5f};                                        ///< 渐变中点（0.0-1.0）
};

/**
 * @brief 3D 视图渲染器抽象接口
 * 
 * 定义所有 3D 渲染器必须实现的接口，支持不同的渲染后端
 * (Coin3D, OsgVerse, 未来可能的其他后端)
 * 
 * 设计原则：
 * - 后端无关：接口不依赖任何特定渲染库
 * - 完整功能：涵盖 FreeCAD 3D 视图的所有核心功能
 * - 易于实现：接口清晰，实现者容易理解
 */
class GuiExport IViewer3D {
public:
    virtual ~IViewer3D() = default;

    //-----------------------------------------------------------------------
    // 基础渲染接口
    //-----------------------------------------------------------------------

    /**
     * @brief 渲染一帧
     * 
     * 由 Qt 的 paintEvent 或渲染循环调用
     */
    virtual void render() = 0;

    /**
     * @brief 调整视图大小
     */
    virtual void resize(int width, int height) = 0;

    /**
     * @brief 获取渲染 Widget
     * 
     * @return QWidget* 可以嵌入到 Qt 界面的 widget
     */
    virtual QWidget* getWidget() = 0;

    /**
     * @brief 获取 OpenGL Widget（如果适用）
     */
    virtual QOpenGLWidget* getGLWidget() { return nullptr; }

    //-----------------------------------------------------------------------
    // 场景管理
    //-----------------------------------------------------------------------

    /**
     * @brief 设置场景根节点
     * 
     * @param root 后端特定的场景根节点（void* 以保持后端无关）
     * 
     * 注意：对于 Coin3D，这是 SoNode*
     *       对于 OsgVerse，这是 osg::Node*
     */
    virtual void setSceneGraph(void* root) = 0;

    /**
     * @brief 获取场景根节点
     */
    virtual void* getSceneGraph() = 0;

    /**
     * @brief 更新场景（标记需要重绘）
     */
    virtual void updateScene() = 0;

    //-----------------------------------------------------------------------
    // 相机控制
    //-----------------------------------------------------------------------

    /**
     * @brief 设置相机参数
     */
    virtual void setCamera(const CameraParams& params) = 0;

    /**
     * @brief 获取相机参数
     */
    virtual CameraParams getCamera() const = 0;

    /**
     * @brief 查看全部（适应所有对象）
     */
    virtual void viewAll() = 0;

    /**
     * @brief 适应选择（将相机调整到聚焦选中的对象）
     *
     * 如果有选中的对象，相机将调整位置和距离以使选中的对象
     * 填满视口。如果没有选中的对象，则调用 viewAll()。
     */
    virtual void fitSelection() = 0;

    /**
     * @brief 重置相机到默认位置
     */
    virtual void resetCamera() = 0;

    /**
     * @brief 设置相机类型
     * 
     * @param orthographic true 为正交投影，false 为透视投影
     */
    virtual void setCameraType(bool orthographic) = 0;

    /**
     * @brief 获取相机类型
     */
    virtual bool isCameraOrthographic() const = 0;

    //-----------------------------------------------------------------------
    // 事件处理
    //-----------------------------------------------------------------------

    /**
     * @brief 处理鼠标事件
     * 
     * @return true 如果事件被处理
     */
    virtual bool handleMouseEvent(QMouseEvent* event) = 0;

    /**
     * @brief 处理键盘事件
     */
    virtual bool handleKeyEvent(QKeyEvent* event) = 0;

    /**
     * @brief 处理滚轮事件
     */
    virtual bool handleWheelEvent(QWheelEvent* event) = 0;

    //-----------------------------------------------------------------------
    // 拾取和选择
    //-----------------------------------------------------------------------

    /**
     * @brief 拾取场景中的对象
     * 
     * @param pos 屏幕坐标
     * @return PickResult 拾取结果
     */
    virtual PickResult pick(const QPoint& pos) = 0;

    /**
     * @brief 设置选择模式
     */
    virtual void setSelectionMode(SelectionMode mode) = 0;

    /**
     * @brief 获取当前选择模式
     */
    virtual SelectionMode getSelectionMode() const = 0;

    /**
     * @brief 开始选择操作
     */
    virtual void startSelection(SelectionMode mode) = 0;

    /**
     * @brief 停止选择操作
     */
    virtual void stopSelection() = 0;

    /**
     * @brief 中止选择操作
     */
    virtual void abortSelection() = 0;

    /**
     * @brief 是否正在选择
     */
    virtual bool isSelecting() const = 0;

    //-----------------------------------------------------------------------
    // ViewProvider 管理
    //-----------------------------------------------------------------------

    /**
     * @brief 添加 ViewProvider
     *
     * ViewProvider 是 FreeCAD 中对象的视图表示
     */
    virtual void addViewProvider(ViewProvider* vp) = 0;

    /**
     * @brief 移除 ViewProvider
     */
    virtual void removeViewProvider(ViewProvider* vp) = 0;

    /**
     * @brief 更新 ViewProvider
     *
     * 当 ViewProvider 的几何数据变化时调用（如形状重新计算后）
     */
    virtual void updateViewProvider(ViewProvider* vp) = 0;

    /**
     * @brief 检查是否包含 ViewProvider
     */
    virtual bool hasViewProvider(ViewProvider* vp) const = 0;

    /**
     * @brief 获取所有 ViewProvider
     */
    virtual std::vector<ViewProvider*> getViewProviders() const = 0;

    //-----------------------------------------------------------------------
    // 渲染设置
    //-----------------------------------------------------------------------

    /**
     * @brief 设置渲染模式
     */
    virtual void setRenderMode(RenderMode mode) = 0;

    /**
     * @brief 获取渲染模式
     */
    virtual RenderMode getRenderMode() const = 0;

    /**
     * @brief 设置背景颜色
     */
    virtual void setBackgroundColor(const Base::Color& color) = 0;

    /**
     * @brief 获取背景颜色
     */
    virtual Base::Color getBackgroundColor() const = 0;

    /**
     * @brief 设置背景渐变
     */
    virtual void setBackgroundGradient(const BackgroundGradient& gradient) = 0;

    /**
     * @brief 获取背景渐变
     */
    virtual BackgroundGradient getBackgroundGradient() const = 0;

    /**
     * @brief 设置是否启用背光
     */
    virtual void setBacklightEnabled(bool enabled) = 0;

    /**
     * @brief 是否启用背光
     */
    virtual bool isBacklightEnabled() const = 0;

    /**
     * @brief 设置环境光强度
     *
     * 控制全局环境光的亮度，影响场景中所有对象的基础照明。
     * 较高的值使阴影区域更亮，较低的值使对比度更强。
     *
     * @param intensity 环境光强度 (0.0 - 1.0)，默认值为 0.2
     */
    virtual void setAmbientIntensity(float intensity) = 0;

    /**
     * @brief 获取环境光强度
     *
     * @return 当前环境光强度 (0.0 - 1.0)
     */
    virtual float getAmbientIntensity() const = 0;

    //-----------------------------------------------------------------------
    // 导航和交互
    //-----------------------------------------------------------------------

    /**
     * @brief 设置导航样式
     * 
     * @param style 导航样式名称（如 "CAD", "Blender", "Touchpad" 等）
     */
    virtual void setNavigationStyle(const std::string& style) = 0;

    /**
     * @brief 获取导航样式
     */
    virtual std::string getNavigationStyle() const = 0;

    /**
     * @brief 设置是否启用查看模式
     */
    virtual void setViewing(bool enable) = 0;

    /**
     * @brief 是否处于查看模式
     */
    virtual bool isViewing() const = 0;

    //-----------------------------------------------------------------------
    // 后端信息
    //-----------------------------------------------------------------------

    /**
     * @brief 获取后端类型
     */
    virtual Render::BackendType getBackendType() const = 0;

    /**
     * @brief 获取后端名称
     */
    virtual std::string getBackendName() const = 0;

    /**
     * @brief 获取后端版本
     */
    virtual std::string getBackendVersion() const = 0;

    //-----------------------------------------------------------------------
    // 统计和调试
    //-----------------------------------------------------------------------

    /**
     * @brief 获取渲染统计信息
     */
    virtual Render::RenderStats getStats() const = 0;

    /**
     * @brief 重置统计信息
     */
    virtual void resetStats() = 0;

    /**
     * @brief 设置是否显示 FPS
     */
    virtual void setFPSEnabled(bool enabled) = 0;

    /**
     * @brief 是否显示 FPS
     */
    virtual bool isFPSEnabled() const = 0;

    //-----------------------------------------------------------------------
    // 高级功能
    //-----------------------------------------------------------------------

    /**
     * @brief 截图
     * 
     * @param width 图像宽度（0 表示使用当前视图大小）
     * @param height 图像高度
     * @return QImage 截图图像
     */
    virtual QImage grabImage(int width = 0, int height = 0) = 0;

    /**
     * @brief 保存截图到文件
     */
    virtual bool saveScreenshot(const QString& filename, int width = 0, int height = 0) = 0;

    /**
     * @brief 设置编辑模式
     */
    virtual void setEditingViewProvider(ViewProvider* vp, int mode) = 0;

    /**
     * @brief 获取正在编辑的 ViewProvider
     */
    virtual ViewProvider* getEditingViewProvider() const = 0;

    /**
     * @brief 是否处于编辑模式
     */
    virtual bool isEditingViewProvider() const = 0;

    /**
     * @brief 重置编辑模式
     */
    virtual void resetEditingViewProvider() = 0;

    //-----------------------------------------------------------------------
    // 编辑模式扩展（Phase G）
    //-----------------------------------------------------------------------

    /**
     * @brief 获取焦平面上的 3D 点
     *
     * 将屏幕坐标投影到焦平面（过相机目标点、法线为视线方向的平面）
     *
     * @param x 屏幕 X 坐标
     * @param y 屏幕 Y 坐标
     * @return 焦平面上的 3D 世界坐标
     */
    virtual Base::Vector3d getPointOnFocalPlane(int x, int y) const = 0;

    /**
     * @brief 设置编辑根节点
     *
     * 创建编辑模式的场景图结构，将编辑几何体与主场景分离
     *
     * @param node 要添加到编辑根的节点（后端特定类型，void* 保持后端无关）
     *             如果为 nullptr，则将当前编辑 VP 的子节点移到编辑根
     * @param mat 编辑变换矩阵（可选）
     */
    virtual void setupEditingRoot(void* node = nullptr, const Base::Matrix4D* mat = nullptr) = 0;

    /**
     * @brief 重置编辑根节点
     *
     * 将编辑几何体移回原始 VP 根，恢复场景图结构
     *
     * @param updateLinks 是否更新 ViewProviderLink 引用
     */
    virtual void resetEditingRoot(bool updateLinks = true) = 0;

    /**
     * @brief 设置编辑变换矩阵
     *
     * @param mat 变换矩阵
     */
    virtual void setEditingTransform(const Base::Matrix4D& mat) = 0;

    //-----------------------------------------------------------------------
    // Seek 功能（Phase G）
    //-----------------------------------------------------------------------

    /**
     * @brief 飞行到屏幕点击位置
     *
     * 在屏幕坐标处拾取 3D 点，然后将相机动画飞行到该点
     *
     * @param screenX 屏幕 X 坐标
     * @param screenY 屏幕 Y 坐标
     * @return true 如果成功拾取到点并开始动画
     */
    virtual bool seekToPoint(int screenX, int screenY) = 0;

    /**
     * @brief 飞行到世界坐标点
     *
     * 将相机动画飞行到指定的世界坐标位置
     *
     * @param worldPos 目标世界坐标
     */
    virtual void seekToPoint(const Base::Vector3d& worldPos) = 0;

    //-----------------------------------------------------------------------
    // 拾取半径（Phase G）
    //-----------------------------------------------------------------------

    /**
     * @brief 获取拾取半径（像素）
     */
    virtual float getPickRadius() const = 0;

    /**
     * @brief 设置拾取半径（像素）
     */
    virtual void setPickRadius(float radius) = 0;

    //-----------------------------------------------------------------------
    // 渲染模式覆盖（Phase H）
    //-----------------------------------------------------------------------

    /**
     * @brief 设置全局渲染模式覆盖
     *
     * 覆盖所有 ViewProvider 的显示模式
     *
     * @param mode 模式字符串（如 "Wireframe", "Shaded", "Flat Lines" 等）
     *             空字符串表示取消覆盖
     */
    virtual void setOverrideMode(const std::string& mode) = 0;

    /**
     * @brief 获取当前渲染模式覆盖
     */
    virtual std::string getOverrideMode() const = 0;

    //-----------------------------------------------------------------------
    // 坐标投影系统 / Coordinate Projection
    //-----------------------------------------------------------------------

    /** Get view direction (normalized) */
    virtual Base::Vector3d getViewDirection() const { return Base::Vector3d(0, 0, -1); }

    /** Get up direction */
    virtual Base::Vector3d getUpDirection() const { return Base::Vector3d(0, 1, 0); }

    /** Project 3D world point to 2D screen coordinates */
    virtual QPoint getPointOnViewport(const Base::Vector3d& pt) const { return QPoint(0, 0); }

    /** Get 3D point on a line closest to the screen point */
    virtual Base::Vector3d getPointOnLine(const QPoint& screenPos, const Base::Vector3d& axisCenter, const Base::Vector3d& axis) const { return Base::Vector3d(); }

    /** Get 3D point on the XY plane of a placement */
    virtual Base::Vector3d getPointOnXYPlaneOfPlacement(const QPoint& screenPos, const Base::Placement& plc) const { return Base::Vector3d(); }

    /** Project screen 2D point to a 3D ray (two points on near/far planes) */
    virtual void projectPointToLine(const QPoint& screenPos, Base::Vector3d& pt1, Base::Vector3d& pt2) const { pt1 = pt2 = Base::Vector3d(); }

    /** Get normalized screen position [0..1] */
    virtual Base::Vector2d getNormalizedPosition(const QPoint& screenPos) const { return Base::Vector2d(0.5, 0.5); }

    /** Project normalized 2D point onto near plane */
    virtual Base::Vector3d projectOnNearPlane(const Base::Vector2d& pt) const { return Base::Vector3d(); }

    /** Project normalized 2D point onto far plane */
    virtual Base::Vector3d projectOnFarPlane(const Base::Vector2d& pt) const { return Base::Vector3d(); }

    /** Get center point on focal plane */
    virtual Base::Vector3d getCenterPointOnFocalPlane() const { return Base::Vector3d(); }

    /** Get near plane (point + normal) */
    virtual void getNearPlane(Base::Vector3d& pt, Base::Vector3d& normal) const { pt = normal = Base::Vector3d(); }

    /** Get far plane (point + normal) */
    virtual void getFarPlane(Base::Vector3d& pt, Base::Vector3d& normal) const { pt = normal = Base::Vector3d(); }

    /** Get viewport dimensions in world units */
    virtual void getDimensions(float& height, float& width) const { height = width = 0; }

    /** Get max viewport dimension */
    virtual float getMaxDimension() const { return 0; }

    /** Get scene bounding box (min and max corners) */
    virtual void getBoundingBox(Base::Vector3d& min, Base::Vector3d& max) const { min = max = Base::Vector3d(); }

    //-----------------------------------------------------------------------
    // 事件回调系统 / Event Callback System
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

    /** Event callback function signature: (eventType, event, userData) -> handled */
    using EventCallbackFunc = std::function<bool(EventType, void*, void*)>;

    /** Add an event callback */
    virtual void addEventCallback(EventType type, EventCallbackFunc cb, void* userData = nullptr) { (void)type; (void)cb; (void)userData; }

    /** Remove an event callback */
    virtual void removeEventCallback(EventType type, EventCallbackFunc cb, void* userData = nullptr) { (void)type; (void)cb; (void)userData; }

    //-----------------------------------------------------------------------
    // 图形覆盖层 / Graphics Overlay
    //-----------------------------------------------------------------------

    /** Add a 2D graphics overlay item (backend-specific) */
    virtual void addGraphicsItem(void* item) { (void)item; }

    /** Remove a 2D graphics overlay item */
    virtual void removeGraphicsItem(void* item) { (void)item; }

    /** Clear all graphics overlay items */
    virtual void clearGraphicsItems() {}

    //-----------------------------------------------------------------------
    // 编辑模式扩展 / Editing Mode Extensions
    //-----------------------------------------------------------------------

    /** Set editing flag */
    virtual void setEditing(bool edit) { (void)edit; }

    /** Check if in editing mode */
    virtual bool isEditing() const { return isEditingViewProvider(); }

    /** Set cursor for editing mode */
    virtual void setEditingCursor(const QCursor& cursor) { (void)cursor; }

    /** Set cursor for component selection */
    virtual void setComponentCursor(const QCursor& cursor) { (void)cursor; }

    /** Redirect events to scene graph (for editing mode) */
    virtual void setRedirectToSceneGraph(bool redirect) { (void)redirect; }

    /** Check if events are redirected to scene graph */
    virtual bool isRedirectedToSceneGraph() const { return false; }

    //-----------------------------------------------------------------------
    // 选择扩展 / Selection Extensions
    //-----------------------------------------------------------------------

    /** Enable/disable selection */
    virtual void setSelectionEnabled(bool enable) { (void)enable; }

    /** Check if selection is enabled */
    virtual bool isSelectionEnabled() const { return true; }

    /** Perform box zoom */
    virtual void boxZoom(int x1, int y1, int x2, int y2) { (void)x1; (void)y1; (void)x2; (void)y2; }

    /** Scale camera (zoom in/out by factor) */
    virtual void scale(float factor) { (void)factor; }

    /** Enable/disable camera animation */
    virtual void setAnimationEnabled(bool enabled) { (void)enabled; }

    /** Check if camera animation is enabled */
    virtual bool isAnimationEnabled() const { return false; }

    /** Save current camera as home position */
    virtual void saveHomePosition() {}

    /** Check if a home position has been saved */
    virtual bool hasHomePosition() const { return false; }

    /** Reset camera to saved home position */
    virtual void resetToHomePosition() {}

    //-----------------------------------------------------------------------
    // 选择多边形 / Selection Polygon
    //-----------------------------------------------------------------------

    /** Get selection polygon vertices in screen coordinates (pixel coords) */
    virtual std::vector<std::pair<int,int>> getSelectionPolygon(bool* isClosed = nullptr) const {
        if (isClosed) *isClosed = false;
        return {};
    }

    /** Get selection polygon vertices in normalized device coordinates [0..1] */
    virtual std::vector<std::pair<float,float>> getSelectionPolygonNormalized(bool* isClosed = nullptr) const {
        if (isClosed) *isClosed = false;
        return {};
    }

    //-----------------------------------------------------------------------
    // 射线拾取 / Ray Picking
    //-----------------------------------------------------------------------

    /** Get intersection point of screen ray with ViewProvider geometry */
    virtual Base::Vector3d getPointOnRay(const QPoint& screenPos, const ViewProvider* vp) const {
        (void)screenPos; (void)vp;
        return Base::Vector3d();
    }

    /** Get intersection point of 3D ray with ViewProvider geometry */
    virtual Base::Vector3d getPointOnRay(const Base::Vector3d& rayOrigin, const Base::Vector3d& rayDir, const ViewProvider* vp) const {
        (void)rayOrigin; (void)rayDir; (void)vp;
        return Base::Vector3d();
    }

    //-----------------------------------------------------------------------
    // 视口投影到放置平面 / Viewport on Placement Plane
    //-----------------------------------------------------------------------

    /** Get viewport bounding box projected onto XY plane of a placement */
    virtual Base::BoundBox2d getViewportOnXYPlaneOfPlacement(const Base::Placement& plc) const {
        (void)plc;
        return Base::BoundBox2d(0, 0, 0, 0);
    }

    //-----------------------------------------------------------------------
    // 渲染扩展 / Rendering Extensions
    //-----------------------------------------------------------------------

    /** Save picture with multi-sampling */
    virtual void savePicture(int width, int height, int samples, const QColor& bg, QImage& img) const {
        (void)samples; (void)bg;
        img = const_cast<IViewer3D*>(this)->grabImage(width, height);
    }

    /** Align camera to selected face normal */
    virtual void alignToSelection() { fitSelection(); }

    //-----------------------------------------------------------------------
    // UI 控制 / UI Control
    //-----------------------------------------------------------------------

    /** Enable/disable popup menu */
    virtual void setPopupMenuEnabled(bool on) { (void)on; }

    /** Check if popup menu is enabled */
    virtual bool isPopupMenuEnabled() const { return true; }
};

} // namespace View3D
} // namespace Gui

#endif // GUI_VIEW3D_IVIEWER3D_H
