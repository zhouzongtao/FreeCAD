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

#include <memory>
#include <string>
#include <vector>

#include <QWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>

#include <FCGlobal.h>
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
};

} // namespace View3D
} // namespace Gui

#endif // GUI_VIEW3D_IVIEWER3D_H
