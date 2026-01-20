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

#ifndef GUI_VIEW3D_COIN_COINVIEWER_H
#define GUI_VIEW3D_COIN_COINVIEWER_H

#include <memory>
#include <set>

#include <FCGlobal.h>
#include <Gui/View3D/IViewer3D.h>

class QOpenGLWidget;

namespace Gui {

class View3DInventorViewer;
class ViewProvider;

namespace View3D {
namespace Coin {

/**
 * @brief Coin3D 渲染器实现
 * 
 * 这是一个适配器类，将现有的 View3DInventorViewer 包装成 IViewer3D 接口
 * 
 * 设计目标：
 * - 最小化代码修改：尽可能重用 View3DInventorViewer 的现有功能
 * - 保持兼容性：确保所有现有功能正常工作
 * - 清晰的接口：提供统一的 IViewer3D 接口
 * 
 * 实现策略：
 * - 组合而非继承：包含 View3DInventorViewer 实例
 * - 委托模式：大部分方法直接委托给 View3DInventorViewer
 * - 适配转换：在需要时进行类型和参数转换
 */
class GuiExport CoinViewer : public IViewer3D {
public:
    /**
     * @brief 构造函数
     * 
     * @param parent 父 widget
     * @param shareWidget 共享 OpenGL 上下文的 widget
     */
    explicit CoinViewer(QWidget* parent = nullptr, const QOpenGLWidget* shareWidget = nullptr);

    /**
     * @brief 析构函数
     */
    ~CoinViewer() override;

    //-----------------------------------------------------------------------
    // IViewer3D 接口实现
    //-----------------------------------------------------------------------

    // 基础渲染接口
    void render() override;
    void resize(int width, int height) override;
    QWidget* getWidget() override;
    QOpenGLWidget* getGLWidget() override;

    // 场景管理
    void setSceneGraph(void* root) override;
    void* getSceneGraph() override;
    void updateScene() override;

    // 相机控制
    void setCamera(const CameraParams& params) override;
    CameraParams getCamera() const override;
    void viewAll() override;
    void resetCamera() override;
    void setCameraType(bool orthographic) override;
    bool isCameraOrthographic() const override;

    // 事件处理
    bool handleMouseEvent(QMouseEvent* event) override;
    bool handleKeyEvent(QKeyEvent* event) override;
    bool handleWheelEvent(QWheelEvent* event) override;

    // 拾取和选择
    PickResult pick(const QPoint& pos) override;
    void setSelectionMode(SelectionMode mode) override;
    SelectionMode getSelectionMode() const override;
    void startSelection(SelectionMode mode) override;
    void stopSelection() override;
    void abortSelection() override;
    bool isSelecting() const override;

    // ViewProvider 管理
    void addViewProvider(ViewProvider* vp) override;
    void removeViewProvider(ViewProvider* vp) override;
    bool hasViewProvider(ViewProvider* vp) const override;
    std::vector<ViewProvider*> getViewProviders() const override;

    // 渲染设置
    void setRenderMode(RenderMode mode) override;
    RenderMode getRenderMode() const override;
    void setBackgroundColor(const Base::Color& color) override;
    Base::Color getBackgroundColor() const override;
    void setBacklightEnabled(bool enabled) override;
    bool isBacklightEnabled() const override;

    // 导航和交互
    void setNavigationStyle(const std::string& style) override;
    std::string getNavigationStyle() const override;
    void setViewing(bool enable) override;
    bool isViewing() const override;

    // 后端信息
    Render::BackendType getBackendType() const override {
        return Render::BackendType::Coin3D;
    }
    std::string getBackendName() const override {
        return "Coin3D";
    }
    std::string getBackendVersion() const override;

    // 统计和调试
    Render::RenderStats getStats() const override;
    void resetStats() override;
    void setFPSEnabled(bool enabled) override;
    bool isFPSEnabled() const override;

    // 高级功能
    QImage grabImage(int width = 0, int height = 0) override;
    bool saveScreenshot(const QString& filename, int width = 0, int height = 0) override;
    void setEditingViewProvider(ViewProvider* vp, int mode) override;
    ViewProvider* getEditingViewProvider() const override;
    bool isEditingViewProvider() const override;
    void resetEditingViewProvider() override;

    //-----------------------------------------------------------------------
    // Coin3D 特定接口（可选）
    //-----------------------------------------------------------------------

    /**
     * @brief 获取内部的 View3DInventorViewer 实例
     * 
     * 用于需要直接访问 Coin3D 特定功能的情况
     * 
     * @return View3DInventorViewer* 内部 viewer 实例
     */
    View3DInventorViewer* getCoinViewer() const {
        return _coinViewer;
    }

private:
    // 内部使用现有的 View3DInventorViewer
    View3DInventorViewer* _coinViewer;

    // 辅助方法：转换选择模式
    int convertSelectionMode(SelectionMode mode);
    SelectionMode convertSelectionMode(int mode);
};

/**
 * @brief Coin3D 渲染器注册器
 * 
 * 自动注册 Coin3D 渲染器到工厂
 */
class CoinViewerRegistrar {
public:
    CoinViewerRegistrar();
};

} // namespace Coin
} // namespace View3D
} // namespace Gui

#endif // GUI_VIEW3D_COIN_COINVIEWER_H
