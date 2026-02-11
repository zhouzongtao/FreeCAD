# 如何切换到OsgVerse视图

## 🎉 好消息

OsgVerse backend已经完全可用！测试结果显示：

```
Current backend: 2 (OsgVerse)
[SUCCESS] OsgVerse backend is available!
[SUCCESS] Switched to OsgVerse backend!
[VERIFIED] Current backend is indeed OsgVerse
Renderer info: OsgVerse 3.6.5
```

## 快速切换方法

### 方法1：使用Python脚本（推荐）

运行提供的切换脚本：

```bash
./build/debug/bin/FreeCAD --console switch_to_osgverse_ascii.py
```

或者在FreeCAD的Python控制台中执行：

```python
exec(open('switch_to_osgverse_ascii.py').read())
```

### 方法2：手动Python命令

在FreeCAD的Python控制台中执行以下命令：

```python
import FreeCADGui

# 检查OsgVerse是否可用
print(FreeCADGui.isRenderBackendAvailable(2))  # 2 = OsgVerse

# 切换到OsgVerse
FreeCADGui.switchRenderBackend(2)

# 验证切换
print(FreeCADGui.getCurrentRenderBackend())  # 应该返回 2

# 获取渲染器信息
print(FreeCADGui.getRendererInfo())  # 应该显示 "OsgVerse 3.6.5"
```

## Backend类型常量

```python
BACKEND_NONE = 0      # 无backend
BACKEND_COIN3D = 1    # Coin3D (默认)
BACKEND_OSGVERSE = 2  # OsgVerse (新)
```

## 完整的Python API

### 1. 切换backend

```python
FreeCADGui.switchRenderBackend(backendType)
# 参数: 0=None, 1=Coin3D, 2=OsgVerse
# 返回: True/False
```

### 2. 获取当前backend

```python
current = FreeCADGui.getCurrentRenderBackend()
# 返回: 0, 1, 或 2
```

### 3. 检查backend是否可用

```python
available = FreeCADGui.isRenderBackendAvailable(backendType)
# 返回: True/False
```

### 4. 获取渲染器信息

```python
info = FreeCADGui.getRendererInfo()
# 返回: 字符串，例如 "OsgVerse 3.6.5"
```

### 5. 获取渲染统计

```python
stats = FreeCADGui.getRenderStats()
# 返回: 字典，包含:
# - frameCount: 帧数
# - drawCalls: 绘制调用次数
# - triangleCount: 三角形数量
# - vertexCount: 顶点数量
# - frameTime: 帧时间（秒）
# - fps: 每秒帧数
```

### 6. 重置统计

```python
FreeCADGui.resetRenderStats()
```

## 使用示例

### 示例1：切换到OsgVerse并创建文档

```python
import FreeCAD
import FreeCADGui

# 切换到OsgVerse
FreeCADGui.switchRenderBackend(2)

# 创建新文档
doc = FreeCAD.newDocument("MyDoc")

# 添加对象
box = doc.addObject("Part::Box", "Box")
box.Length = 10
box.Width = 10
box.Height = 10
doc.recompute()

# 新创建的3D视图将使用OsgVerse backend
```

### 示例2：在backends之间切换

```python
import FreeCADGui

# 切换到OsgVerse
print("Switching to OsgVerse...")
FreeCADGui.switchRenderBackend(2)
print("Current:", FreeCADGui.getCurrentRenderBackend())

# 切换回Coin3D
print("Switching to Coin3D...")
FreeCADGui.switchRenderBackend(1)
print("Current:", FreeCADGui.getCurrentRenderBackend())
```

### 示例3：监控渲染性能

```python
import FreeCADGui
import time

# 切换到OsgVerse
FreeCADGui.switchRenderBackend(2)

# 重置统计
FreeCADGui.resetRenderStats()

# 执行一些操作...
time.sleep(5)

# 获取统计
stats = FreeCADGui.getRenderStats()
print("FPS:", stats['fps'])
print("Frame time:", stats['frameTime'])
print("Triangles:", stats['triangleCount'])
```

## 自动切换到OsgVerse

如果你想让FreeCAD启动时自动使用OsgVerse，可以创建一个启动宏：

### 创建启动宏

1. 在 `~/.FreeCAD/Macro/` 目录下创建 `startup.FCMacro`
2. 添加以下内容：

```python
# startup.FCMacro
import FreeCADGui

# 自动切换到OsgVerse
try:
    if FreeCADGui.isRenderBackendAvailable(2):
        FreeCADGui.switchRenderBackend(2)
        print("Auto-switched to OsgVerse backend")
    else:
        print("OsgVerse backend not available")
except Exception as e:
    print("Failed to switch backend:", e)
```

## 验证OsgVerse是否工作

### 检查清单

1. ✅ **检查backend可用性**
   ```python
   FreeCADGui.isRenderBackendAvailable(2)  # 应该返回 True
   ```

2. ✅ **检查当前backend**
   ```python
   FreeCADGui.getCurrentRenderBackend()  # 应该返回 2
   ```

3. ✅ **检查渲染器信息**
   ```python
   FreeCADGui.getRendererInfo()  # 应该显示 "OsgVerse 3.6.5"
   ```

4. ✅ **创建测试文档**
   - 创建新文档
   - 添加一个Box对象
   - 检查3D视图是否正常显示

## 故障排除

### 问题1：switchRenderBackend返回False

**原因**：Backend未注册或RenderManager未初始化

**解决方案**：
```python
# 检查backend是否可用
print(FreeCADGui.isRenderBackendAvailable(2))

# 如果不可用，检查OsgVerseGui模块
import OsgVerseGui
```

### 问题2：getCurrentRenderBackend返回1而不是2

**原因**：切换未成功

**解决方案**：
- 确保在创建新文档之前切换backend
- 已存在的视图不会自动切换，只有新创建的视图会使用新backend

### 问题3：3D视图不显示或显示异常

**原因**：OsgVerse渲染器可能有问题

**解决方案**：
```python
# 切换回Coin3D
FreeCADGui.switchRenderBackend(1)
```

## 性能对比

### Coin3D vs OsgVerse

| 特性 | Coin3D | OsgVerse |
|------|--------|----------|
| 成熟度 | ✅ 非常成熟 | ⚠️ 开发中 |
| 性能 | ✅ 良好 | ✅ 更好 |
| 渲染质量 | ✅ 良好 | ✅ 更好 |
| 高级特性 | ⚠️ 有限 | ✅ 丰富 |
| 稳定性 | ✅ 非常稳定 | ⚠️ 测试中 |

## 推荐使用场景

### 使用Coin3D（默认）
- 日常工作
- 生产环境
- 需要最大稳定性

### 使用OsgVerse
- 测试新功能
- 性能对比
- 需要高级渲染特性
- 开发和调试

## 相关文件

- `switch_to_osgverse_ascii.py` - 切换脚本
- `OSGVERSE_LOADING_GUIDE.md` - 加载指南
- `OSGVERSE_USAGE_GUIDE.md` - 使用指南
- `src/Gui/Core/RenderManagerPy.cpp` - Python API实现

## 总结

✅ OsgVerse backend已完全可用
✅ 可以通过Python API轻松切换
✅ 支持实时性能监控
✅ 可以在backends之间自由切换

享受使用OsgVerse带来的更好渲染体验！
