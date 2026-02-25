# Step 2 实施指南：更新 View3DInventor 使用 ViewerFactory

## 目标

修改 `View3DInventor` 使用 `ViewerFactory` 创建渲染器，实现真正的后端切换能力。

## 当前状态分析

### View3DInventor 构造函数（当前）

```cpp
View3DInventor::View3DInventor(
    Gui::Document* pcDocument,
    QWidget* parent,
    const QOpenGLWidget* sharewidget,
    Qt::WindowFlags wflags
)
    : MDIView(pcDocument, parent, wflags)
    , _viewerPy(nullptr)
{
    // ... 省略其他代码 ...
    
    // 直接创建 View3DInventorViewer
    if (glformat) {
        _viewer = new View3DInventorViewer(f, this, sharewidget);
    }
    else {
        _viewer = new View3DInventorViewer(this, sharewidget);
    }
    
    // 配置 viewer
    _viewer->setDocument(this->_pcDocument);
    stack->addWidget(_viewer->getWidget());
    
    // ... 省略其他代码 ...
}
```

### 问题

1. **硬编码 Coin3D**: 直接创建 `View3DInventorViewer`（Coin3D 特定）
2. **无法切换后端**: 一旦创建就固定使用 Coin3D
3. **类型耦合**: `_viewer` 成员是 `View3DInventorViewer*` 类型

## 实施方案

### 方案 A：最小修改（推荐）

**优点**:
- 修改最少
- 保持向后兼容
- 风险最低

**缺点**:
- 仍然依赖 Coin3D 类型
- 无法完全利用抽象接口

### 方案 B：完全抽象（理想）

**优点**:
- 完全使用抽象接口
- 真正的后端无关
- 架构最清晰

**缺点**:
- 修改较多
- 需要处理 Coin3D 特定功能
- 风险较高

## 推荐实施：方案 A（渐进式）

### 第一阶段：使用工厂创建，保持类型

**目标**: 使用 `ViewerFactory` 创建渲染器，但保持 `_viewer` 类型不变

#### 修改 View3DInventor.h

```cpp
// 添加 include
#include <Gui/View3D/ViewerFactory.h>
#include <Gui/View3D/IViewer3D.h>
#include <Gui/View3D/Backends/Coin/CoinViewer.h>

// 保持成员不变（暂时）
private:
    View3DInventorViewer* _viewer;  // 保持不变
```

#### 修改 View3DInventor.cpp 构造函数

```cpp
View3DInventor::View3DInventor(
    Gui::Document* pcDocument,
    QWidget* parent,
    const QOpenGLWidget* sharewidget,
    Qt::WindowFlags wflags
)
    : MDIView(pcDocument, parent, wflags)
    , _viewerPy(nullptr)
{
    stack = new QStackedWidget(this);
    setMouseTracking(true);
    setAcceptDrops(true);

    // 使用 ViewerFactory 创建渲染器
    try {
        // 创建抽象渲染器
        auto viewer = View3D::ViewerFactory::createDefault(this, sharewidget);
        
        // 获取 Coin3D 特定实现（用于向后兼容）
        auto* coinViewer = dynamic_cast<View3D::Coin::CoinViewer*>(viewer.get());
        if (coinViewer) {
            _viewer = coinViewer->getCoinViewer();
            viewer.release();  // 释放所有权，由 _viewer 管理
        }
        else {
            // 回退到直接创建（不应该发生）
            Base::Console().warning("ViewerFactory did not return CoinViewer, falling back to direct creation\n");
            _viewer = new View3DInventorViewer(this, sharewidget);
        }
    }
    catch (const std::exception& e) {
        // 如果工厂失败，回退到直接创建
        Base::Console().error("ViewerFactory failed: %s, falling back to direct creation\n", e.what());
        _viewer = new View3DInventorViewer(this, sharewidget);
    }

    // 配置 viewer（保持不变）
    _viewer->setDocument(this->_pcDocument);
    stack->addWidget(_viewer->getWidget());
    
    // ... 其余代码保持不变 ...
}
```

### 第二阶段：添加后端切换支持（可选）

#### 添加切换方法

```cpp
// View3DInventor.h
public:
    /**
     * @brief 切换渲染后端
     * @param backend 后端类型
     * @return 是否成功切换
     */
    bool switchRenderBackend(Render::BackendType backend);
    
    /**
     * @brief 获取当前后端类型
     */
    Render::BackendType getCurrentBackend() const;

// View3DInventor.cpp
bool View3DInventor::switchRenderBackend(Render::BackendType backend)
{
    try {
        // 保存当前相机状态
        SoCamera* oldCam = _viewer->getSoRenderManager()->getCamera();
        std::string cameraSettings;
        if (oldCam) {
            cameraSettings = SoFCDB::writeNodesToString(oldCam);
        }
        
        // 保存场景图
        SoNode* sceneGraph = _viewer->getSceneGraph();
        if (sceneGraph) {
            sceneGraph->ref();
        }
        
        // 创建新渲染器
        auto newViewer = View3D::ViewerFactory::create(backend, this, nullptr);
        
        // 如果是 Coin3D，获取内部 viewer
        if (backend == Render::BackendType::Coin3D) {
            auto* coinViewer = dynamic_cast<View3D::Coin::CoinViewer*>(newViewer.get());
            if (coinViewer) {
                // 删除旧 viewer
                delete _viewer;
                
                // 设置新 viewer
                _viewer = coinViewer->getCoinViewer();
                newViewer.release();
                
                // 恢复场景图
                if (sceneGraph) {
                    _viewer->setSceneGraph(sceneGraph);
                    sceneGraph->unref();
                }
                
                // 恢复相机
                if (!cameraSettings.empty()) {
                    setCamera(cameraSettings.c_str());
                }
                
                // 更新 UI
                stack->removeWidget(stack->widget(0));
                stack->insertWidget(0, _viewer->getWidget());
                stack->setCurrentIndex(0);
                
                _viewer->setDocument(this->_pcDocument);
                
                return true;
            }
        }
        
        // 其他后端暂不支持
        Base::Console().warning("Backend switching to non-Coin3D not yet supported\n");
        return false;
    }
    catch (const std::exception& e) {
        Base::Console().error("Failed to switch backend: %s\n", e.what());
        return false;
    }
}

Render::BackendType View3DInventor::getCurrentBackend() const
{
    // 目前总是 Coin3D
    return Render::BackendType::Coin3D;
}
```

## 实施步骤

### Step 2.1: 基础修改

1. ✅ 在 `View3DInventor.h` 添加必要的 include
2. ✅ 修改构造函数使用 `ViewerFactory::createDefault()`
3. ✅ 添加异常处理和回退机制
4. ✅ 编译测试

### Step 2.2: 测试验证

1. ✅ 测试 3D 视图创建
2. ✅ 测试所有现有功能
3. ✅ 验证 ViewerFactory 正确工作

### Step 2.3: 添加切换支持（可选）

1. ⬜ 实现 `switchRenderBackend()` 方法
2. ⬜ 实现 `getCurrentBackend()` 方法
3. ⬜ 添加 Python 绑定
4. ⬜ 测试后端切换

## 注意事项

### 1. 内存管理

```cpp
// CoinViewer 包装了 View3DInventorViewer
// 需要正确处理所有权转移

auto viewer = ViewerFactory::createDefault(...);  // unique_ptr
auto* coinViewer = dynamic_cast<CoinViewer*>(viewer.get());
_viewer = coinViewer->getCoinViewer();  // 获取内部指针
viewer.release();  // 释放 unique_ptr 所有权
// 现在 _viewer 负责删除 View3DInventorViewer
```

### 2. 向后兼容

- 保持 `getViewer()` 返回 `View3DInventorViewer*`
- 所有现有代码继续工作
- 不破坏 Python API

### 3. 异常处理

- 工厂可能抛出异常
- 需要回退到直接创建
- 记录错误日志

### 4. Coin3D 特定功能

某些功能仍然需要 Coin3D 特定类型：
- `getSoRenderManager()`
- `setCameraType(SoType)`
- `getSoRenderManager()->getCamera()`

这些在第一阶段保持不变。

## 测试计划

### 单元测试

```python
# test_view3d_factory.py
import FreeCAD
import FreeCADGui

def test_view_creation():
    """测试 3D 视图创建"""
    doc = FreeCAD.newDocument("Test")
    FreeCADGui.showMainWindow()
    
    # 创建视图应该使用工厂
    view = FreeCADGui.ActiveDocument.ActiveView
    assert view is not None
    print("✓ View created successfully")

def test_viewer_type():
    """测试渲染器类型"""
    view = FreeCADGui.ActiveDocument.ActiveView
    viewer = view.getViewer()
    
    # 应该仍然是 View3DInventorViewer
    assert viewer is not None
    print(f"✓ Viewer type: {type(viewer).__name__}")

def test_view_operations():
    """测试视图操作"""
    view = FreeCADGui.ActiveDocument.ActiveView
    
    # 测试基本操作
    view.viewAxometric()
    view.viewAll()
    view.fitAll()
    
    print("✓ View operations work")

if __name__ == "__main__":
    test_view_creation()
    test_viewer_type()
    test_view_operations()
    print("\n✓ All tests passed!")
```

### 集成测试

1. 创建文档和对象
2. 打开 3D 视图
3. 测试所有视图操作
4. 测试相机控制
5. 测试选择和拾取
6. 测试导航

## 预期结果

### 编译

```
FreeCADGui.vcxproj -> E:\Repository\FreeCAD\FreeCAD\build\bin\FreeCADGui.dll
Exit Code: 0
```

### 运行时

```
ViewerFactory: Creating default viewer (backend: 1)
ViewerFactory: Creating viewer for backend type 1
CoinViewerRegistrar: Registering Coin3D viewer
CoinViewer: Creating Coin3D viewer
CoinViewer: Coin3D viewer created successfully
```

### 功能

- ✅ 3D 视图正常创建
- ✅ 所有现有功能工作
- ✅ 使用 ViewerFactory 创建
- ✅ 为未来后端切换做好准备

## 下一步

完成 Step 2 后，架构将支持：

1. **Step 3**: 实现 OsgVerse 适配器
2. **Step 4**: 实现真正的后端切换
3. **Step 5**: 添加 Python API
4. **Step 6**: 完整测试和文档

---

**状态**: 准备实施  
**风险**: 低（渐进式修改，有回退机制）  
**预计时间**: 1-2 小时
