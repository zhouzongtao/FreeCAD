# Phase 1 测试指南

## 测试方式

有两种方式测试 Phase 1 的实现：

### 方式 1：FreeCAD GUI 模式（推荐）

这是最直观的测试方式，可以看到实际的渲染效果。

#### 步骤：

1. **启动 FreeCAD GUI**
   ```cmd
   build\bin\FreeCAD.exe
   ```

2. **在 Python 控制台中运行测试**
   
   打开 FreeCAD 的 Python 控制台（View → Panels → Python console），然后逐步执行：

   ```python
   # 导入模块
   import OsgVerseGui
   from FreeCADGui import BackendRegistry
   
   # 检查后端
   print(BackendRegistry.getAvailableBackends())
   # 应该看到: ['Coin3D', 'OsgVerse']
   
   # 获取后端信息
   info = BackendRegistry.getBackendInfo("OsgVerse")
   print(info)
   
   # 创建 viewer
   viewer = BackendRegistry.createViewer("OsgVerse")
   print(f"Viewer: {viewer.getBackendName()}")
   
   # 获取 widget
   widget = viewer.getWidget()
   print(f"Widget: {type(widget).__name__}")
   print(f"Size: {widget.width()}x{widget.height()}")
   
   # 测试基本操作
   from PySide6.QtGui import QColor
   viewer.setBackgroundColor(QColor(50, 50, 80))
   viewer.render()
   viewer.viewAll()
   
   print("✅ Phase 1 基本测试通过!")
   ```

3. **运行完整测试脚本**
   
   在 Python 控制台中：
   ```python
   exec(open('test_phase1_simple.py').read())
   ```

### 方式 2：使用测试脚本文件

1. **创建测试批处理文件**（已创建）
   ```cmd
   run_phase1_test.cmd
   ```

2. **运行测试**
   ```cmd
   run_phase1_test.cmd
   ```

3. **或者直接运行**
   ```cmd
   build\bin\FreeCAD.exe --console test_phase1_simple.py
   ```

## 完整功能测试

在 FreeCAD GUI 中测试完整功能：

### 测试 1：模块加载

```python
import OsgVerseGui
print("✅ 模块加载成功")
```

**预期结果**：无错误，控制台显示成功消息

### 测试 2：后端注册

```python
from FreeCADGui import BackendRegistry
backends = BackendRegistry.getAvailableBackends()
print(f"可用后端: {backends}")
assert "OsgVerse" in backends, "OsgVerse 未注册"
print("✅ 后端注册成功")
```

**预期结果**：显示 `['Coin3D', 'OsgVerse']`

### 测试 3：创建 Viewer

```python
viewer = BackendRegistry.createViewer("OsgVerse")
print(f"Viewer 类型: {type(viewer)}")
print(f"后端名称: {viewer.getBackendName()}")
assert viewer is not None, "Viewer 创建失败"
print("✅ Viewer 创建成功")
```

**预期结果**：Viewer 对象创建，后端名称为 "OsgVerse"

### 测试 4：获取 Widget

```python
widget = viewer.getWidget()
print(f"Widget 类型: {type(widget).__name__}")
print(f"Widget 大小: {widget.width()}x{widget.height()}")
assert widget is not None, "Widget 获取失败"
print("✅ Widget 获取成功")
```

**预期结果**：Widget 类型为 `OsgVerseWidget`，有合理的尺寸

### 测试 5：设置背景色

```python
from PySide6.QtGui import QColor
viewer.setBackgroundColor(QColor(50, 50, 80))  # 深蓝色
print("✅ 背景色设置成功")
```

**预期结果**：无错误

### 测试 6：渲染

```python
viewer.render()
print("✅ 渲染调用成功")
```

**预期结果**：无错误

### 测试 7：ViewAll

```python
viewer.viewAll()
print("✅ ViewAll 调用成功")
```

**预期结果**：无错误

### 测试 8：创建几何体（可选）

```python
import Part
import FreeCAD

# 创建文档
doc = FreeCAD.newDocument("Test")

# 创建 Box
box = doc.addObject("Part::Box", "Box")
box.Length = 10
box.Width = 10
box.Height = 10
doc.recompute()

print(f"✅ 几何体创建成功: {box.Name}")
```

**预期结果**：Box 对象创建成功

## 视觉测试（手动）

这些测试需要在 FreeCAD GUI 中手动验证：

### 测试 9：Widget 显示

1. 切换到 OsgVerse 后端：
   ```python
   BackendRegistry.setDefaultBackend("OsgVerse")
   ```

2. 创建新文档和 3D 视图
   ```python
   import FreeCAD
   import FreeCADGui
   
   doc = FreeCAD.newDocument()
   FreeCADGui.activeDocument().activeView()
   ```

**预期结果**：
- ✅ 3D 视图窗口打开
- ✅ 背景色为深蓝灰色（或你设置的颜色）
- ✅ 窗口可以调整大小
- ✅ 无崩溃或错误

### 测试 10：几何体渲染

```python
import Part
box = Part.makeBox(10, 10, 10)
Part.show(box)
```

**预期结果**：
- ✅ Box 在 3D 视图中可见
- ✅ 几何体正确显示（不是占位符球体）
- ✅ 几何体有正确的形状和大小

### 测试 11：窗口调整

手动调整 3D 视图窗口大小

**预期结果**：
- ✅ 窗口平滑调整
- ✅ 内容正确缩放
- ✅ 无闪烁或撕裂
- ✅ 无崩溃

## 性能测试

### 测试 12：帧率

```python
import time

# 渲染 100 帧并计时
start = time.time()
for i in range(100):
    viewer.render()
end = time.time()

fps = 100 / (end - start)
print(f"平均帧率: {fps:.2f} FPS")

# Phase 1 目标: 能够渲染（帧率在 Phase 2 优化）
assert fps > 10, f"帧率太低: {fps} FPS"
print("✅ 帧率测试通过")
```

**预期结果**：FPS > 10（Phase 1 基本要求）

## 验收标准检查表

根据 `requirements.md` US-1，Phase 1 的验收标准：

- [ ] **OsgVerseViewer 创建 QOpenGLWidget**
  - 测试：检查 `widget` 类型
  - 命令：`print(type(viewer.getWidget()).__name__)`
  - 预期：`OsgVerseWidget`

- [ ] **Widget 集成到 FreeCAD MDI 窗口**
  - 测试：创建 3D 视图，检查是否显示
  - 命令：切换后端，创建文档
  - 预期：窗口正常显示

- [ ] **Widget 处理 resize 事件**
  - 测试：手动调整窗口大小
  - 预期：内容正确缩放，无错误

- [ ] **Widget 有正确的 OpenGL 上下文管理**
  - 测试：多次创建/销毁 viewer
  - 预期：无内存泄漏，无崩溃

- [ ] **渲染流畅（60 FPS 目标）**
  - 测试：运行性能测试
  - 预期：FPS > 10（Phase 1），> 60（最终目标）

## 故障排除

### 问题 1：模块导入失败

**错误**：`ImportError: No module named 'OsgVerseGui'`

**解决**：
1. 检查编译是否成功：`build\Mod\OsgVerseGui\OsgVerseGui.pyd` 是否存在
2. 重新编译：`cmake --build build --target OsgVerseGui --config Release`

### 问题 2：后端未注册

**错误**：`OsgVerse` 不在可用后端列表中

**解决**：
1. 确保导入了模块：`import OsgVerseGui`
2. 检查模块初始化：查看 `AppOsgVerseGui.cpp` 中的注册代码

### 问题 3：Viewer 创建失败

**错误**：`createViewer` 返回 `None`

**解决**：
1. 检查 `OsgVerseBackendFactory::createViewer()` 实现
2. 查看 FreeCAD 控制台的错误消息

### 问题 4：Widget 为空

**错误**：`getWidget()` 返回 `None`

**解决**：
1. 检查 `OsgVerseViewer` 构造函数中 `_widget` 的初始化
2. 确保 `new OsgVerseWidget(parent)` 成功执行

### 问题 5：黑屏

**现象**：Widget 显示但是黑屏

**解决**：
1. 检查 `initializeGL()` 是否被调用
2. 检查 OSG viewer 是否正确初始化
3. 检查场景图是否设置：`viewer->setSceneData(_sceneRoot)`

### 问题 6：崩溃

**现象**：FreeCAD 崩溃

**解决**：
1. 检查 OpenGL 上下文是否在正确的线程
2. 检查 OSG 对象的生命周期管理
3. 使用调试器查看崩溃位置

## 成功标准

Phase 1 测试成功的标志：

✅ **所有自动化测试通过**
- 模块加载
- 后端注册
- Viewer 创建
- Widget 获取
- 基本操作

✅ **所有视觉测试通过**
- Widget 显示
- 几何体渲染
- 窗口调整

✅ **性能测试通过**
- FPS > 10

✅ **无崩溃或内存泄漏**

## 下一步

Phase 1 测试通过后，可以开始 Phase 2：

**Phase 2: Event Handling**
- 实现鼠标事件处理
- 实现键盘事件处理
- 添加 Trackball 相机控制
- 实现交互功能

参考：`.kiro/specs/osgverse-rendering/requirements.md` US-2 和 US-3

---

**测试愉快！** 🚀

如果遇到问题，请查看：
- `Phase1_Qt_Widget_Integration_Complete.md` - 实现细节
- `.kiro/specs/osgverse-rendering/QUICKSTART.md` - 快速开始指南
- FreeCAD 控制台的错误消息
