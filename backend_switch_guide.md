# 渲染后端切换使用指南

## 概述

FreeCAD 现在支持两个渲染后端：
- **Coin3D**：默认后端，成熟稳定，显示真实几何体
- **OsgVerse**：实验性后端，现代特性，当前 Phase 1 显示占位符球体

可以在运行时动态切换这两个后端，无需重启 FreeCAD。

## 后端类型常量

```python
BACKEND_NONE = 0      # 无后端
BACKEND_COIN3D = 1    # Coin3D 后端（默认）
BACKEND_OSGVERSE = 2  # OsgVerse 后端（实验性）
```

## Python API

### 1. 获取当前后端

```python
import FreeCADGui

current = FreeCADGui.getCurrentRenderBackend()
# 返回: 0=None, 1=Coin3D, 2=OsgVerse

backend_names = {0: "None", 1: "Coin3D", 2: "OsgVerse"}
print(f"当前后端: {backend_names[current]}")
```

### 2. 检查后端是否可用

```python
# 检查 Coin3D
if FreeCADGui.isRenderBackendAvailable(1):
    print("Coin3D 可用")

# 检查 OsgVerse
if FreeCADGui.isRenderBackendAvailable(2):
    print("OsgVerse 可用")
```

### 3. 切换后端

```python
# 切换到 Coin3D
success = FreeCADGui.switchRenderBackend(1)
if success:
    print("成功切换到 Coin3D")

# 切换到 OsgVerse
success = FreeCADGui.switchRenderBackend(2)
if success:
    print("成功切换到 OsgVerse")
```

### 4. 获取渲染器信息

```python
info = FreeCADGui.getRendererInfo()
print(f"渲染器: {info}")
# 例如: "Coin3D 4.0.0" 或 "OSG 3.6.5"
```

### 5. 获取渲染统计

```python
stats = FreeCADGui.getRenderStats()
print(f"帧数: {stats['frameCount']}")
print(f"FPS: {stats['fps']}")
print(f"三角形数: {stats['triangleCount']}")
```

## 快速切换脚本

### 方法 1：使用提供的脚本

#### 切换到 Coin3D
```python
exec(open('E:\\Repository\\FreeCAD\\FreeCAD\\switch_to_coin3d.py', encoding='utf-8').read())
```

#### 切换到 OsgVerse
```python
exec(open('E:\\Repository\\FreeCAD\\FreeCAD\\switch_to_osgverse.py', encoding='utf-8').read())
```

#### 完整测试（包含来回切换）
```python
exec(open('E:\\Repository\\FreeCAD\\FreeCAD\\test_backend_switch.py', encoding='utf-8').read())
```

### 方法 2：直接在 Python 控制台输入

#### 切换到 Coin3D
```python
import FreeCADGui
FreeCADGui.switchRenderBackend(1)  # 1 = Coin3D
FreeCADGui.SendMsgToActiveView("ViewFit")
```

#### 切换到 OsgVerse
```python
import FreeCADGui
FreeCADGui.switchRenderBackend(2)  # 2 = OsgVerse
FreeCADGui.SendMsgToActiveView("ViewFit")
```

## 测试步骤

### 完整测试流程

1. **启动 FreeCAD**
   - 确保 FreeCAD 完全启动
   - 打开或创建一个文档

2. **创建测试对象**
   ```python
   import FreeCAD
   doc = FreeCAD.ActiveDocument or FreeCAD.newDocument()
   
   # 创建几个不同的对象
   box = doc.addObject("Part::Box", "TestBox")
   box.Length = 10
   
   cylinder = doc.addObject("Part::Cylinder", "TestCylinder")
   cylinder.Radius = 5
   cylinder.Placement.Base = FreeCAD.Vector(20, 0, 0)
   
   sphere = doc.addObject("Part::Sphere", "TestSphere")
   sphere.Radius = 6
   sphere.Placement.Base = FreeCAD.Vector(-20, 0, 0)
   
   doc.recompute()
   ```

3. **测试 Coin3D（默认）**
   ```python
   exec(open('switch_to_coin3d.py', encoding='utf-8').read())
   ```
   
   **预期效果**：
   - ✓ 对象显示为真实几何体（Box、Cylinder、Sphere）
   - ✓ 对象有正确的颜色和材质
   - ✓ 可以正常旋转、缩放、平移视图
   - ✓ 所有 Coin3D 功能正常

4. **切换到 OsgVerse**
   ```python
   exec(open('switch_to_osgverse.py', encoding='utf-8').read())
   ```
   
   **预期效果（Phase 1）**：
   - ✓ 所有对象显示为红色球体占位符
   - ✓ 球体半径 5.0，完整可见
   - ✓ Report View 中有 `[OsgVerse]` 前缀的日志
   - ✓ 可以正常旋转、缩放、平移视图
   - ✓ ViewFit 功能正常

5. **切换回 Coin3D**
   ```python
   exec(open('switch_to_coin3d.py', encoding='utf-8').read())
   ```
   
   **预期效果**：
   - ✓ 对象恢复为真实几何体
   - ✓ 所有功能正常
   - ✓ 没有遗留问题

6. **多次切换测试**
   - 重复步骤 4 和 5 多次
   - 确认每次切换都正常
   - 确认没有内存泄漏或崩溃

## 预期行为对比

### Coin3D 后端
| 特性 | 行为 |
|------|------|
| 几何体显示 | 真实的 3D 模型（Box、Cylinder、Sphere 等） |
| 颜色 | 根据 ViewProvider 设置的颜色 |
| 材质 | 完整的材质系统 |
| 透明度 | 支持 |
| 边缘显示 | 支持 |
| 选择高亮 | 支持 |
| 日志前缀 | 无特殊前缀 |

### OsgVerse 后端（Phase 1）
| 特性 | 行为 |
|------|------|
| 几何体显示 | 红色球体占位符（所有对象相同） |
| 颜色 | 固定红色 (1.0, 0.0, 0.0) |
| 材质 | 固定材质（红色漫反射 + 白色高光） |
| 透明度 | 不支持（Phase 2） |
| 边缘显示 | 不支持（Phase 2） |
| 选择高亮 | 不支持（Phase 3） |
| 日志前缀 | `[OsgVerse]` |

## 常见问题

### Q1: 切换后端失败怎么办？

**检查项**：
1. 确保 FreeCAD 已完全启动
2. 确保有活动文档
3. 检查 Report View 中的错误信息
4. 确认目标后端已编译和安装

**解决方法**：
```python
# 检查后端是否可用
import FreeCADGui
print(f"Coin3D 可用: {FreeCADGui.isRenderBackendAvailable(1)}")
print(f"OsgVerse 可用: {FreeCADGui.isRenderBackendAvailable(2)}")
```

### Q2: 切换后视图是黑屏怎么办？

**解决方法**：
```python
# 重新适应视图
FreeCADGui.SendMsgToActiveView("ViewFit")

# 或者重置相机
FreeCADGui.ActiveDocument.ActiveView.viewAxonometric()
```

### Q3: OsgVerse 显示的球体被裁剪了？

这不应该发生（已在 Phase 1 中修复）。如果出现：
1. 检查是否使用了最新编译的版本
2. 查看 Report View 中的日志
3. 尝试手动调整视图

### Q4: 切换后性能下降？

**正常情况**：
- Coin3D：成熟优化，性能稳定
- OsgVerse Phase 1：占位符渲染，性能应该很好
- OsgVerse Phase 2+：性能取决于几何体复杂度

**检查方法**：
```python
import FreeCADGui
stats = FreeCADGui.getRenderStats()
print(f"FPS: {stats['fps']}")
print(f"帧时间: {stats['frameTime']} ms")
```

### Q5: 可以在启动时指定后端吗？

目前需要在启动后通过 Python 切换。未来可能添加配置文件支持。

**临时方案**：创建启动宏
```python
# 在 FreeCAD 启动后自动执行
import FreeCADGui
FreeCADGui.switchRenderBackend(2)  # 自动切换到 OsgVerse
```

## 开发者信息

### 后端切换流程

```
用户调用 switchRenderBackend(type)
    ↓
RenderManager::switchBackend(type)
    ├─ 保存旧后端
    ├─ 创建新后端引擎
    ├─ 初始化新引擎
    ├─ 遍历所有文档和视图
    │   └─ 重新创建视图（使用新后端）
    ├─ 触发后端切换事件
    └─ 返回成功/失败
```

### 关键代码位置

- **Python 绑定**: `src/Gui/Core/RenderManagerPy.cpp`
- **后端管理**: `src/Gui/Core/RenderManager.cpp`
- **Coin3D 实现**: `src/Gui/View3DInventor.cpp`
- **OsgVerse 实现**: `src/Gui/View3D/Backends/OsgVerse/OsgVerseViewerImpl.cpp`

### 添加新的切换功能

如果需要添加新的后端切换相关功能：

1. 在 `RenderManager.h` 中添加方法
2. 在 `RenderManager.cpp` 中实现
3. 在 `RenderManagerPy.cpp` 中添加 Python 绑定
4. 更新文档

## 总结

动态后端切换功能让你可以：
- ✓ 在运行时切换渲染后端
- ✓ 对比不同后端的效果
- ✓ 测试新功能而不影响稳定版本
- ✓ 根据需求选择最合适的后端

当前 OsgVerse 处于 Phase 1（占位符渲染），主要用于验证架构和基础功能。Phase 2 将实现真实几何体渲染，届时 OsgVerse 将提供与 Coin3D 相当的显示效果。
