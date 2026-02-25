# 第一步实施指南：接口设计 + Coin3D 适配器

## 📋 概述

本文档详细说明如何实施方案 C 的第一步：创建抽象接口层并实现 Coin3D 适配器。

**目标**：
- ✅ 建立清晰的抽象接口
- ✅ 包装现有 Coin3D 功能
- ✅ 确保现有功能不受影响
- ✅ 为 OsgVerse 实现打好基础

**预计时间**：3-5 天

## 📁 已创建的文件

### 核心接口层
```
src/Gui/View3D/
├── IViewer3D.h              ✅ 已创建 - 核心接口定义
├── ViewerFactory.h          ✅ 已创建 - 工厂类头文件
├── ViewerFactory.cpp        ✅ 已创建 - 工厂类实现
└── CMakeLists.txt           ✅ 已创建 - 构建配置
```

### Coin3D 适配器
```
src/Gui/View3D/Backends/Coin/
├── CoinViewer.h             ✅ 已创建 - Coin3D 适配器头文件
└── CoinViewer.cpp           ✅ 已创建 - Coin3D 适配器实现
```

## 🔧 集成步骤

### 步骤 1：修改 src/Gui/CMakeLists.txt

在 `src/Gui/CMakeLists.txt` 中添加 View3D 子目录：

```cmake
# 在文件末尾添加
add_subdirectory(View3D)
```

### 步骤 2：添加 PreCompiled.h 支持

创建 `src/Gui/View3D/PreCompiled.h`：

```cpp
#ifndef GUI_VIEW3D_PRECOMPILED_H
#define GUI_VIEW3D_PRECOMPILED_H

#include <FCConfig.h>

// Qt headers
#include <QWidget>
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QImage>

// Standard library
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

// FreeCAD Base
#include <Base/Console.h>
#include <Base/Vector3D.h>
#include <Base/Matrix.h>
#include <Base/Color.h>

// Coin3D (for Coin backend)
#ifdef BUILD_COIN3D
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/SoDB.h>
#endif

#endif // GUI_VIEW3D_PRECOMPILED_H
```

创建 `src/Gui/View3D/PreCompiled.cpp`：

```cpp
#include "PreCompiled.h"
```

### 步骤 3：更新 CMakeLists.txt 添加 PreCompiled

修改 `src/Gui/View3D/CMakeLists.txt`：

```cmake
set(View3D_SRCS
    PreCompiled.h
    PreCompiled.cpp
    IViewer3D.h
    ViewerFactory.h
    ViewerFactory.cpp
)
```

### 步骤 4：编译测试

```cmd
cd build
cmake --build . --config Release --target FreeCADGui -- /maxcpucount:1
```

**预期结果**：
- 编译成功
- 没有链接错误
- 所有新文件都被正确编译

### 步骤 5：验证注册

创建测试脚本 `test_viewer_factory.py`：

```python
import FreeCAD
import FreeCADGui

# 测试工厂是否正确注册
print("Testing ViewerFactory...")

# 检查 Coin3D 后端是否注册
# 注意：这需要在 C++ 中暴露相应的 Python 接口
# 目前我们通过创建视图来间接测试

# 创建新文档
doc = FreeCAD.newDocument()

# 创建 3D 视图（应该使用新的工厂）
FreeCADGui.activeDocument().activeView()

print("ViewerFactory test completed!")
```

## 📊 接口设计说明

### IViewer3D 接口

**设计原则**：
1. **后端无关**：不依赖任何特定渲染库
2. **完整功能**：涵盖所有核心 3D 视图功能
3. **易于实现**：清晰的接口，容易理解

**主要接口组**：

1. **基础渲染**：`render()`, `resize()`, `getWidget()`
2. **场景管理**：`setSceneGraph()`, `getSceneGraph()`, `updateScene()`
3. **相机控制**：`setCamera()`, `getCamera()`, `viewAll()`, `resetCamera()`
4. **事件处理**：`handleMouseEvent()`, `handleKeyEvent()`, `handleWheelEvent()`
5. **拾取选择**：`pick()`, `startSelection()`, `stopSelection()`
6. **ViewProvider**：`addViewProvider()`, `removeViewProvider()`, `hasViewProvider()`
7. **渲染设置**：`setRenderMode()`, `setBackgroundColor()`, `setBacklightEnabled()`
8. **导航交互**：`setNavigationStyle()`, `setViewing()`
9. **后端信息**：`getBackendType()`, `getBackendName()`, `getBackendVersion()`
10. **高级功能**：`grabImage()`, `saveScreenshot()`, `setEditingViewProvider()`

### ViewerFactory 工厂

**职责**：
- 创建不同后端的 IViewer3D 实现
- 管理后端注册
- 提供默认后端选择

**使用方式**：
```cpp
// 创建默认后端
auto viewer = ViewerFactory::createDefault(parent);

// 创建指定后端
auto coinViewer = ViewerFactory::create(BackendType::Coin3D, parent);
```

### CoinViewer 适配器

**设计策略**：
- **组合而非继承**：包含 View3DInventorViewer 实例
- **委托模式**：大部分方法直接委托给 View3DInventorViewer
- **最小修改**：尽可能重用现有功能

**关键实现**：

1. **构造函数**：创建 View3DInventorViewer 实例
2. **场景管理**：直接委托给 View3DInventorViewer
3. **相机控制**：转换参数格式后委托
4. **事件处理**：View3DInventorViewer 自动处理
5. **ViewProvider**：直接委托

## ⚠️ 注意事项

### 1. 不要破坏现有功能

**关键**：CoinViewer 只是一个包装器，所有功能都委托给 View3DInventorViewer。

**验证方法**：
- 所有现有测试应该通过
- 现有 Python 脚本应该正常工作
- 用户界面行为应该完全一致

### 2. 类型转换

某些接口需要类型转换：

```cpp
// 场景图：void* ↔ SoNode*
void* getSceneGraph() override {
    return _coinViewer->getSceneGraph();
}

// 相机参数：CameraParams ↔ SoCamera
void setCamera(const CameraParams& params) override {
    // 转换参数
    SbVec3f pos(params.position.x, params.position.y, params.position.z);
    // ...
}
```

### 3. 选择模式映射

```cpp
int CoinViewer::convertSelectionMode(SelectionMode mode) {
    switch (mode) {
        case SelectionMode::Lasso:
            return View3DInventorViewer::Lasso;
        case SelectionMode::Rectangle:
            return View3DInventorViewer::Rectangle;
        // ...
    }
}
```

### 4. 自动注册

使用全局静态对象自动注册：

```cpp
// 在 CoinViewer.cpp 末尾
static CoinViewerRegistrar g_coinViewerRegistrar;
```

这确保在程序启动时自动注册 Coin3D 后端。

## 🧪 测试计划

### 单元测试

创建 `tests/src/Gui/View3D/TestCoinViewer.cpp`：

```cpp
#include <gtest/gtest.h>
#include <Gui/View3D/Backends/Coin/CoinViewer.h>
#include <Gui/View3D/ViewerFactory.h>

using namespace Gui::View3D;

TEST(CoinViewer, Creation) {
    auto viewer = std::make_unique<Coin::CoinViewer>();
    ASSERT_NE(viewer, nullptr);
    ASSERT_NE(viewer->getWidget(), nullptr);
}

TEST(CoinViewer, BackendInfo) {
    auto viewer = std::make_unique<Coin::CoinViewer>();
    EXPECT_EQ(viewer->getBackendType(), Render::BackendType::Coin3D);
    EXPECT_EQ(viewer->getBackendName(), "Coin3D");
    EXPECT_FALSE(viewer->getBackendVersion().empty());
}

TEST(ViewerFactory, Registration) {
    EXPECT_TRUE(ViewerFactory::isRegistered(Render::BackendType::Coin3D));
}

TEST(ViewerFactory, CreateCoin3D) {
    auto viewer = ViewerFactory::create(Render::BackendType::Coin3D);
    ASSERT_NE(viewer, nullptr);
    EXPECT_EQ(viewer->getBackendType(), Render::BackendType::Coin3D);
}

TEST(ViewerFactory, CreateDefault) {
    auto viewer = ViewerFactory::createDefault();
    ASSERT_NE(viewer, nullptr);
    // 默认应该是 Coin3D
    EXPECT_EQ(viewer->getBackendType(), Render::BackendType::Coin3D);
}
```

### 集成测试

创建 `test_coin_viewer_integration.py`：

```python
import FreeCAD
import FreeCADGui
import unittest

class TestCoinViewerIntegration(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("Test")
        
    def tearDown(self):
        FreeCAD.closeDocument("Test")
    
    def test_create_view(self):
        """测试创建 3D 视图"""
        view = FreeCADGui.activeDocument().activeView()
        self.assertIsNotNone(view)
    
    def test_add_object(self):
        """测试添加对象到视图"""
        box = self.doc.addObject("Part::Box", "Box")
        self.doc.recompute()
        
        # 视图应该包含对象
        view = FreeCADGui.activeDocument().activeView()
        self.assertIsNotNone(view)
    
    def test_camera_control(self):
        """测试相机控制"""
        view = FreeCADGui.activeDocument().activeView()
        view.viewAxonometric()
        view.viewFront()
        view.viewTop()
        view.viewRight()
        view.viewIsometric()
    
    def test_render_modes(self):
        """测试渲染模式"""
        box = self.doc.addObject("Part::Box", "Box")
        self.doc.recompute()
        
        view = FreeCADGui.activeDocument().activeView()
        # 测试不同渲染模式
        # view.setRenderMode("Wireframe")
        # view.setRenderMode("Shaded")
        # view.setRenderMode("Points")

if __name__ == '__main__':
    unittest.main()
```

## 📈 成功标准

### 编译阶段
- ✅ 所有新文件编译成功
- ✅ 没有链接错误
- ✅ 没有警告（或只有可接受的警告）

### 功能阶段
- ✅ CoinViewer 可以创建
- ✅ ViewerFactory 正确注册 Coin3D 后端
- ✅ 可以通过工厂创建 Coin3D 渲染器
- ✅ 所有 IViewer3D 接口方法可以调用

### 兼容性阶段
- ✅ 现有 3D 视图功能完全正常
- ✅ 所有现有测试通过
- ✅ Python 脚本正常工作
- ✅ 用户界面行为一致

## 🐛 常见问题

### 问题 1：编译错误 - 找不到 View3DInventorViewer

**原因**：头文件包含路径不正确

**解决**：
```cpp
#include <Gui/View3DInventorViewer.h>  // 正确
// 不是 #include "../../View3DInventorViewer.h"
```

### 问题 2：链接错误 - undefined reference

**原因**：CMakeLists.txt 配置不正确

**解决**：确保 View3D 子目录被正确添加到 FreeCADGui 目标

### 问题 3：运行时错误 - 后端未注册

**原因**：全局注册器未执行

**解决**：确保 CoinViewer.cpp 被编译并链接

### 问题 4：视图显示异常

**原因**：CoinViewer 的某个方法实现不正确

**解决**：检查委托调用是否正确，参数转换是否准确

## 📝 下一步

完成第一步后，你将拥有：

1. ✅ 清晰的抽象接口层
2. ✅ 工作的 Coin3D 适配器
3. ✅ 完整的工厂模式
4. ✅ 自动注册机制

**然后可以开始第二步**：实现 OsgVerse 适配器

---

**准备好开始编译测试了吗？** 🚀

需要我帮你：
1. 修改 src/Gui/CMakeLists.txt？
2. 创建测试脚本？
3. 解决编译问题？
