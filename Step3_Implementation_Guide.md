# Step 3 实施指南：实现 OsgVerse Viewer

## 目标

创建 OsgVerseViewer 实现 IViewer3D 接口，使其能够通过 ViewerFactory 创建。

## 当前状态分析

### 现有代码

1. **src/Gui/Render/Backends/OsgVerse/OsgVerseViewer.h/cpp**
   - 基于 `RenderViewer` 接口（旧的抽象层）
   - 已经实现了基本的 OSG 渲染功能
   - 使用 `osgViewer::Viewer` 和 `GraphicsWindowEmbedded`

2. **需要的新代码**
   - **src/Gui/View3D/Backends/OsgVerse/OsgVerseViewer.h/cpp**
   - 基于 `IViewer3D` 接口（新的抽象层）
   - 适配器模式，包装现有的 OsgVerse 渲染代码

## 实施方案

### 方案 A：完整实现（推荐暂缓）

直接实现所有 IViewer3D 接口方法。

**优点**: 功能完整
**缺点**: 工作量大，风险高

### 方案 B：最小实现（推荐）

实现最基本的功能，让 3D 视图能够创建和显示。

**优点**: 
- 快速验证架构
- 风险低
- 可以逐步完善

**缺点**: 
- 功能不完整
- 需要后续迭代

## 推荐实施：方案 B（最小可行实现）

### 第一阶段：基础框架

#### 1. 创建文件结构

```
src/Gui/View3D/Backends/OsgVerse/
├── OsgVerseViewerImpl.h      # 新的 IViewer3D 实现
└── OsgVerseViewerImpl.cpp
```

#### 2. 最小接口实现

只实现必需的方法：

```cpp
class OsgVerseViewerImpl : public IViewer3D {
public:
    OsgVerseViewerImpl(QWidget* parent, const QOpenGLWidget* shareWidget);
    ~OsgVerseViewerImpl() override;

    // 必需的基础方法
    void render() override;
    void resize(int width, int height) override;
    QWidget* getWidget() override;
    QOpenGLWidget* getGLWidget() override;
    
    void setSceneGraph(void* root) override;
    void* getSceneGraph() override;
    void updateScene() override;
    
    void setCamera(const CameraParams& params) override;
    CameraParams getCamera() const override;
    void viewAll() override;
    void resetCamera() override;
    
    // 后端信息
    Render::BackendType getBackendType() const override { 
        return Render::BackendType::OsgVerse; 
    }
    std::string getBackendName() const override { 
        return "OsgVerse"; 
    }
    std::string getBackendVersion() const override;
    
    // 其他方法返回默认值或空实现
    // ...
    
private:
    QOpenGLWidget* _widget;  // 简单的 OpenGL widget
    osg::ref_ptr<osgViewer::Viewer> _viewer;
    osg::ref_ptr<osg::Group> _sceneRoot;
};
```

### 第二阶段：注册到工厂

在 Application.cpp 中添加注册：

```cpp
#ifdef BUILD_WITH_OSGVERSE
    View3D::ViewerFactory::registerCreator(
        Render::BackendType::OsgVerse,
        [](QWidget* parent, const QOpenGLWidget* shareWidget) {
            return std::make_unique<View3D::OsgVerse::OsgVerseViewerImpl>(parent, shareWidget);
        }
    );
    Base::Console().log("Application: OsgVerse viewer registered\n");
#endif
```

### 第三阶段：测试验证

1. 编译测试
2. 切换到 OsgVerse 后端
3. 验证 3D 视图创建
4. 测试基本渲染

## 详细实施步骤

### Step 3.1: 创建最小实现

**文件**: `src/Gui/View3D/Backends/OsgVerse/OsgVerseViewerImpl.h`

```cpp
#ifndef GUI_VIEW3D_OSGVERSE_OSGVERSEVIEWERIMPL_H
#define GUI_VIEW3D_OSGVERSE_OSGVERSEVIEWERIMPL_H

#include <memory>
#include <QOpenGLWidget>
#include <osg/ref_ptr>
#include <osg/Group>
#include <osgViewer/Viewer>

#include <FCGlobal.h>
#include <Gui/View3D/IViewer3D.h>

namespace Gui {

class ViewProvider;

namespace View3D {
namespace OsgVerse {

/**
 * @brief OsgVerse 渲染器实现（最小版本）
 * 
 * 这是一个最小可行实现，用于验证架构。
 * 后续会逐步完善功能。
 */
class GuiExport OsgVerseViewerImpl : public IViewer3D {
public:
    explicit OsgVerseViewerImpl(QWidget* parent = nullptr, 
                                const QOpenGLWidget* shareWidget = nullptr);
    ~OsgVerseViewerImpl() override;

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

    // 事件处理（暂时返回 false）
    bool handleMouseEvent(QMouseEvent* event) override { return false; }
    bool handleKeyEvent(QKeyEvent* event) override { return false; }
    bool handleWheelEvent(QWheelEvent* event) override { return false; }

    // 拾取和选择（暂时返回空）
    PickResult pick(const QPoint& pos) override { return PickResult(); }
    void setSelectionMode(SelectionMode mode) override {}
    SelectionMode getSelectionMode() const override { return SelectionMode::None; }
    void startSelection(SelectionMode mode) override {}
    void stopSelection() override {}
    void abortSelection() override {}
    bool isSelecting() const override { return false; }

    // ViewProvider 管理（暂时空实现）
    void addViewProvider(ViewProvider* vp) override {}
    void removeViewProvider(ViewProvider* vp) override {}
    bool hasViewProvider(ViewProvider* vp) const override { return false; }
    std::vector<ViewProvider*> getViewProviders() const override { return {}; }

    // 渲染设置
    void setRenderMode(RenderMode mode) override {}
    RenderMode getRenderMode() const override { return RenderMode::Shaded; }
    void setBackgroundColor(const Base::Color& color) override;
    Base::Color getBackgroundColor() const override;
    void setBacklightEnabled(bool enabled) override {}
    bool isBacklightEnabled() const override { return false; }

    // 导航和交互
    void setNavigationStyle(const std::string& style) override {}
    std::string getNavigationStyle() const override { return "Unknown"; }
    void setViewing(bool enable) override {}
    bool isViewing() const override { return true; }

    // 后端信息
    Render::BackendType getBackendType() const override { 
        return Render::BackendType::OsgVerse; 
    }
    std::string getBackendName() const override { 
        return "OsgVerse"; 
    }
    std::string getBackendVersion() const override;

    // 统计和调试
    Render::RenderStats getStats() const override { return Render::RenderStats(); }
    void resetStats() override {}
    void setFPSEnabled(bool enabled) override {}
    bool isFPSEnabled() const override { return false; }

    // 高级功能
    QImage grabImage(int width = 0, int height = 0) override;
    bool saveScreenshot(const QString& filename, int width = 0, int height = 0) override;
    void setEditingViewProvider(ViewProvider* vp, int mode) override {}
    ViewProvider* getEditingViewProvider() const override { return nullptr; }
    bool isEditingViewProvider() const override { return false; }
    void resetEditingViewProvider() override {}

private:
    class ViewerWidget;
    
    ViewerWidget* _widget;
    osg::ref_ptr<osgViewer::Viewer> _viewer;
    osg::ref_ptr<osg::Group> _sceneRoot;
    Base::Color _backgroundColor;
    bool _initialized;
    
    void initialize();
};

} // namespace OsgVerse
} // namespace View3D
} // namespace Gui

#endif // GUI_VIEW3D_OSGVERSE_OSGVERSEVIEWERIMPL_H
```

### Step 3.2: 实现基础功能

**文件**: `src/Gui/View3D/Backends/OsgVerse/OsgVerseViewerImpl.cpp`

实现最基本的功能：
- 创建 OSG viewer
- 创建 Qt widget
- 基本的渲染循环
- 简单的相机控制

### Step 3.3: 更新 CMakeLists.txt

```cmake
# src/Gui/View3D/CMakeLists.txt

# 如果启用 OsgVerse，添加 OsgVerse 后端
if(BUILD_WITH_OSGVERSE)
    set(View3D_OsgVerse_SRCS
        Backends/OsgVerse/OsgVerseViewerImpl.h
        Backends/OsgVerse/OsgVerseViewerImpl.cpp
    )
    
    target_sources(FreeCADGui PRIVATE
        ${View3D_OsgVerse_SRCS}
    )
    
    message(STATUS "View3D: OsgVerse backend enabled")
endif()
```

### Step 3.4: 注册到 Application.cpp

```cpp
#ifdef BUILD_WITH_OSGVERSE
    #include "View3D/Backends/OsgVerse/OsgVerseViewerImpl.h"
    
    View3D::ViewerFactory::registerCreator(
        Render::BackendType::OsgVerse,
        [](QWidget* parent, const QOpenGLWidget* shareWidget) {
            return std::make_unique<View3D::OsgVerse::OsgVerseViewerImpl>(parent, shareWidget);
        }
    );
    Base::Console().log("Application: OsgVerse viewer registered\n");
#endif
```

## 注意事项

### 1. 与现有 OsgVerseViewer 的关系

- **现有**: `src/Gui/Render/Backends/OsgVerse/OsgVerseViewer` (基于 RenderViewer)
- **新的**: `src/Gui/View3D/Backends/OsgVerse/OsgVerseViewerImpl` (基于 IViewer3D)

两者可以共存，新的是适配器。

### 2. 场景图转换

```cpp
void setSceneGraph(void* root) override {
    // root 可能是 SoNode* (Coin3D) 或 osg::Node* (OSG)
    // 需要类型检查和转换
    if (auto* osgNode = static_cast<osg::Node*>(root)) {
        _sceneRoot->removeChildren(0, _sceneRoot->getNumChildren());
        _sceneRoot->addChild(osgNode);
    }
}
```

### 3. 相机参数转换

```cpp
void setCamera(const CameraParams& params) override {
    // 将 IViewer3D 的 CameraParams 转换为 OSG 相机参数
    // ...
}
```

## 测试计划

### 单元测试

```python
# test_osgverse_viewer.py
import FreeCAD
import FreeCADGui

def test_osgverse_viewer_creation():
    """测试 OsgVerse viewer 创建"""
    # 切换到 OsgVerse
    FreeCADGui.switchRenderBackend("OsgVerse")
    
    # 创建文档和视图
    doc = FreeCAD.newDocument("Test")
    box = doc.addObject("Part::Box", "Box")
    doc.recompute()
    
    # 验证视图创建
    view = FreeCADGui.ActiveDocument.ActiveView
    assert view is not None
    print("✓ OsgVerse viewer created")

if __name__ == "__main__":
    test_osgverse_viewer_creation()
```

## 预期结果

### 编译

```
OsgVerseViewerImpl.cpp
FreeCADGui.vcxproj -> E:\Repository\FreeCAD\FreeCAD\build\bin\FreeCADGui.dll
Exit Code: 0
```

### 运行时

```
Application: Registering viewer backends...
ViewerFactory: Registered creator for backend type 1
Application: Coin3D viewer registered
ViewerFactory: Registered creator for backend type 2
Application: OsgVerse viewer registered

View3DInventor: Creating viewer using ViewerFactory
ViewerFactory: Creating default viewer (backend: 2)
ViewerFactory: Creating viewer for backend type 2
OsgVerseViewerImpl: Creating OsgVerse viewer
OsgVerseViewerImpl: OsgVerse viewer created successfully
View3DInventor: Successfully created viewer via factory
```

## 下一步

完成 Step 3 后：

1. **Step 3.1**: 最小实现 + 编译测试
2. **Step 3.2**: 基础渲染功能
3. **Step 3.3**: 相机控制
4. **Step 3.4**: 完整功能（逐步）

---

**状态**: 准备实施  
**风险**: 中等（需要 OSG 知识）  
**预计时间**: 2-4 小时（最小实现）
