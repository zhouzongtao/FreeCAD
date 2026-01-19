# FreeCAD 切换到 OsgVerse 渲染引擎完整指南

## 目录
1. [快速开始](#快速开始)
2. [切换方法详解](#切换方法详解)
3. [验证切换结果](#验证切换结果)
4. [常见问题](#常见问题)
5. [性能对比](#性能对比)
6. [回退到 Coin3D](#回退到-coin3d)

---

## 快速开始

### 最简单的方法（Python 控制台）

1. 打开 FreeCAD
2. 打开 Python 控制台（视图 -> 面板 -> Python 控制台）
3. 执行以下代码：

```python
# 方法 A: 如果 Python 绑定可用
import FreeCADGui as Gui

# 检查 OsgVerse 是否可用
if hasattr(Gui, 'isRenderBackendAvailable'):
    if Gui.isRenderBackendAvailable(2):
        Gui.switchRenderBackend(2)
        print("已切换到 OsgVerse")
    else:
        print("OsgVerse 不可用")
else:
    # 方法 B: 使用参数设置（需要重启）
    import FreeCAD as App
    param = App.ParamGet("User parameter:BaseApp/Preferences/View")
    param.SetInt("RenderBackend", 2)
    print("已设置参数，请重启 FreeCAD")
```

### 使用提供的脚本

1. 将 `switch_to_osgverse.py` 复制到 FreeCAD 宏目录
2. 在 FreeCAD 中：工具 -> 宏 -> 宏...
3. 选择 `switch_to_osgverse.py` 并执行

---

## 切换方法详解

### 方法 1: Python API（运行时切换，无需重启）

**前提条件**：需要编译并启用 RenderManager Python 绑定

```python
import FreeCADGui as Gui

# 1. 检查当前后端
current = Gui.getCurrentRenderBackend()
print(f"当前后端: {current}")  # 0=None, 1=Coin3D, 2=OsgVerse

# 2. 检查 OsgVerse 是否可用
if Gui.isRenderBackendAvailable(2):
    print("OsgVerse 可用")
    
    # 3. 切换到 OsgVerse
    success = Gui.switchRenderBackend(2)
    
    if success:
        print("切换成功")
        
        # 4. 查看渲染器信息
        info = Gui.getRendererInfo()
        print(f"渲染器: {info}")
        
        # 5. 查看渲染统计
        stats = Gui.getRenderStats()
        print(f"FPS: {stats['fps']}")
        print(f"三角形数: {stats['triangleCount']}")
    else:
        print("切换失败")
else:
    print("OsgVerse 不可用")
```

**优点**：
- ✅ 立即生效，无需重启
- ✅ 可以动态切换进行对比
- ✅ 可以获取详细的渲染统计信息

**缺点**：
- ❌ 需要 Python 绑定支持
- ❌ 可能需要重新加载场景

### 方法 2: 参数设置（持久化，需要重启）

```python
import FreeCAD as App

# 获取视图参数组
param = App.ParamGet("User parameter:BaseApp/Preferences/View")

# 设置渲染后端
# 0 = None
# 1 = Coin3D (默认)
# 2 = OsgVerse
param.SetInt("RenderBackend", 2)

# 保存参数
App.saveParameter()

print("参数已设置，请重启 FreeCAD")
```

**优点**：
- ✅ 设置持久化，重启后仍然有效
- ✅ 不需要 Python 绑定
- ✅ 简单可靠

**缺点**：
- ❌ 需要重启 FreeCAD
- ❌ 无法动态切换

### 方法 3: 配置文件（高级用户）

编辑 FreeCAD 配置文件：

**Windows**: `%APPDATA%\FreeCAD\user.cfg`
**Linux**: `~/.config/FreeCAD/user.cfg`
**macOS**: `~/Library/Preferences/FreeCAD/user.cfg`

添加或修改：

```ini
[View]
RenderBackend=2
```

保存后重启 FreeCAD。

### 方法 4: 命令行参数（临时测试）

```bash
# Windows
FreeCAD.exe --render-backend=osgverse

# Linux/macOS
freecad --render-backend=osgverse
```

**注意**：此功能需要在 Application 中实现命令行参数解析。

---

## 验证切换结果

### 1. 通过 Python 控制台验证

```python
import FreeCADGui as Gui

# 检查当前后端
if hasattr(Gui, 'getCurrentRenderBackend'):
    backend = Gui.getCurrentRenderBackend()
    backend_names = {0: "None", 1: "Coin3D", 2: "OsgVerse"}
    print(f"当前渲染后端: {backend_names[backend]}")
    
    # 获取详细信息
    info = Gui.getRendererInfo()
    print(f"渲染器信息: {info}")
else:
    # 通过参数检查
    import FreeCAD as App
    param = App.ParamGet("User parameter:BaseApp/Preferences/View")
    backend = param.GetInt("RenderBackend", 1)
    backend_names = {0: "None", 1: "Coin3D", 2: "OsgVerse"}
    print(f"参数设置: {backend_names[backend]}")
```

### 2. 通过日志验证

查看 FreeCAD 控制台输出，应该看到类似：

```
RenderManager::switchBackend: Switching from 1 to 2
RenderManager::switchBackend: Successfully switched to OsgVerse
```

### 3. 视觉验证

OsgVerse 后端的特征：
- ✨ 更好的抗锯齿效果
- ✨ 支持 PBR 材质（金属度、粗糙度）
- ✨ 更真实的阴影
- ✨ HDR 渲染和 Bloom 效果
- ✨ 更流畅的动画

---

## 常见问题

### Q1: 提示 "OsgVerse 不可用"

**原因**：
1. FreeCAD 编译时未包含 OsgVerse 支持
2. 缺少 OpenSceneGraph 依赖库
3. 系统不支持 OpenGL 3.3+

**解决方法**：
```python
# 检查是否编译了 OsgVerse
import FreeCADGui as Gui
if hasattr(Gui, 'isRenderBackendAvailable'):
    available = Gui.isRenderBackendAvailable(2)
    print(f"OsgVerse 可用: {available}")
else:
    print("RenderManager Python 绑定不可用")
```

如果不可用，需要重新编译 FreeCAD 并启用 OsgVerse：

```bash
cmake -DBUILD_OSGVERSE=ON ..
```

### Q2: 切换后界面显示异常

**原因**：场景图需要重新构建

**解决方法**：
```python
import FreeCADGui as Gui

# 刷新所有视图
for doc in App.listDocuments().values():
    for obj in doc.Objects:
        if hasattr(obj, 'ViewObject'):
            obj.ViewObject.touch()

# 或者重新加载文档
Gui.activeDocument().recompute()
```

### Q3: 性能下降

**原因**：OsgVerse 默认启用了高质量渲染特性

**解决方法**：调整渲染质量
```python
# 注意：需要访问 OsgVerseEngine 实例
# 这需要扩展 Python 绑定

# 禁用一些高级特性
# engine.setShadowsEnabled(False)
# engine.setAntiAliasingSamples(2)  # 降低抗锯齿采样
```

### Q4: 如何查看渲染统计信息

```python
import FreeCADGui as Gui

if hasattr(Gui, 'getRenderStats'):
    stats = Gui.getRenderStats()
    
    print("渲染统计:")
    print(f"  帧数: {stats['frameCount']}")
    print(f"  绘制调用: {stats['drawCalls']}")
    print(f"  三角形数: {stats['triangleCount']}")
    print(f"  顶点数: {stats['vertexCount']}")
    print(f"  帧时间: {stats['frameTime']:.2f} ms")
    print(f"  FPS: {stats['fps']:.1f}")
    
    # 重置统计
    Gui.resetRenderStats()
```

---

## 性能对比

### Coin3D vs OsgVerse

| 特性 | Coin3D | OsgVerse |
|------|--------|----------|
| 渲染性能 | 中等 | 高 |
| 大场景支持 | 一般 | 优秀 |
| PBR 材质 | ❌ | ✅ |
| HDR 渲染 | ❌ | ✅ |
| 实时阴影 | 基础 | 高质量 |
| 抗锯齿 | MSAA | MSAA + TAA |
| 后处理效果 | ❌ | ✅ (SSAO, Bloom) |
| 多线程渲染 | ❌ | ✅ |
| 内存占用 | 低 | 中等 |
| 稳定性 | 非常稳定 | 实验性 |

### 性能测试脚本

```python
import FreeCADGui as Gui
import time

def benchmark_backend(backend_id, backend_name):
    """测试渲染后端性能"""
    print(f"\n测试 {backend_name} 后端...")
    
    # 切换后端
    if hasattr(Gui, 'switchRenderBackend'):
        Gui.switchRenderBackend(backend_id)
        time.sleep(1)  # 等待切换完成
        
        # 重置统计
        Gui.resetRenderStats()
        
        # 旋转视图触发渲染
        for i in range(100):
            Gui.activeDocument().activeView().viewRotateLeft()
            time.sleep(0.01)
        
        # 获取统计
        stats = Gui.getRenderStats()
        print(f"  平均 FPS: {stats['fps']:.1f}")
        print(f"  平均帧时间: {stats['frameTime']:.2f} ms")
        print(f"  绘制调用: {stats['drawCalls']}")
        
        return stats
    else:
        print("  Python API 不可用")
        return None

# 运行测试
print("=" * 60)
print("渲染后端性能测试")
print("=" * 60)

coin3d_stats = benchmark_backend(1, "Coin3D")
osgverse_stats = benchmark_backend(2, "OsgVerse")

if coin3d_stats and osgverse_stats:
    print("\n性能对比:")
    fps_improvement = (osgverse_stats['fps'] / coin3d_stats['fps'] - 1) * 100
    print(f"  FPS 提升: {fps_improvement:+.1f}%")
```

---

## 回退到 Coin3D

如果 OsgVerse 出现问题，可以随时切换回 Coin3D：

### 方法 1: Python API

```python
import FreeCADGui as Gui

# 切换回 Coin3D
if hasattr(Gui, 'switchRenderBackend'):
    Gui.switchRenderBackend(1)
    print("已切换回 Coin3D")
```

### 方法 2: 参数设置

```python
import FreeCAD as App

param = App.ParamGet("User parameter:BaseApp/Preferences/View")
param.SetInt("RenderBackend", 1)
print("已设置为 Coin3D，请重启 FreeCAD")
```

### 紧急回退

如果 FreeCAD 无法启动：

1. 删除配置文件中的 RenderBackend 设置
2. 或者手动编辑配置文件，将 `RenderBackend=1`

---

## 开发者信息

### 添加 Python 绑定到构建系统

如果 `RenderManagerPy.cpp` 还未集成到构建系统，需要修改 `src/Gui/CMakeLists.txt`:

```cmake
# 在 Gui 源文件列表中添加
SET(FreeCADGui_SRCS
    ...
    Core/RenderManager.cpp
    Core/RenderManagerPy.cpp  # 添加这一行
    ...
)
```

### 在 Application 初始化时注册

在 `src/Gui/Application.cpp` 中添加：

```cpp
#include "Core/RenderManagerPy.h"

void Application::initApplication()
{
    // ... 现有代码 ...
    
    // 初始化 RenderManager Python 绑定
    Gui::Core::initRenderManagerPy();
    
    // ... 现有代码 ...
}
```

---

## 总结

切换到 OsgVerse 渲染引擎可以获得：
- ✅ 更好的渲染质量
- ✅ 更高的性能
- ✅ 现代渲染特性（PBR、HDR、阴影）
- ✅ 更好的大场景支持

但需要注意：
- ⚠️ 仍处于实验阶段
- ⚠️ 可能存在兼容性问题
- ⚠️ 需要较新的 OpenGL 支持

建议在测试环境中先试用，确认稳定后再在生产环境使用。
