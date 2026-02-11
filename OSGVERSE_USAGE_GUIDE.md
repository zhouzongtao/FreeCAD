# OsgVerse 视图使用指南

## 当前状态

### ✅ 已完成
- OsgVerseGui模块成功构建和加载
- OsgVerse backend已注册到ViewerFactory
- 崩溃问题已修复

### ⚠️ 当前限制
由于RenderManager初始化代码被禁用（`#if 0`在Application.cpp第544行），系统默认使用Coin3D backend。

## 如何使用OsgVerse视图

### 方法1：通过Python手动切换（推荐）

在FreeCAD的Python控制台中执行：

```python
# 导入OsgVerseGui模块（如果还没有加载）
import OsgVerseGui

# 创建新文档
import FreeCAD
doc = FreeCAD.newDocument("TestDoc")

# 添加一个对象
box = doc.addObject("Part::Box", "Box")
box.Length = 10
box.Width = 10
box.Height = 10
doc.recompute()

# 检查当前视图类型
import FreeCADGui
if FreeCADGui.activeDocument():
    view = FreeCADGui.activeDocument().activeView()
    if view:
        print(f"Current view type: {view.getTypeId()}")
```

### 方法2：启用RenderManager（需要修改代码）

如果要默认使用OsgVerse backend，需要：

1. 编辑 `src/Gui/Application.cpp`
2. 将第544行的 `#if 0` 改为 `#if 1`
3. 重新编译

**注意**：原注释说"This was causing crashes when loading documents"，所以启用前需要确保所有崩溃问题都已修复。

### 方法3：通过配置文件设置

创建或编辑 `~/.FreeCAD/user.cfg`，添加：

```ini
[Render]
DefaultBackend=OsgVerse
```

## 验证OsgVerse是否工作

### 测试脚本

运行 `test_osgverse_console.py`：

```bash
./build/debug/bin/FreeCAD
# 在Python控制台中：
exec(open('test_osgverse_console.py').read())
```

### 预期输出

如果使用Coin3D（当前默认）：
```
✓ Active view type: Gui::View3DInventor
ℹ Using Coin3D backend (default)
```

如果使用OsgVerse：
```
✓ Active view type: Gui::View3DOsgVerse
✓ Using OsgVerse backend!
```

## 当前默认行为

由于RenderManager未初始化：
1. ✅ FreeCAD正常启动
2. ✅ 可以创建和打开文档
3. ✅ 使用Coin3D渲染器（稳定可靠）
4. ✅ OsgVerseGui模块已加载（可手动使用）
5. ⚠️ 不会自动使用OsgVerse backend

## 手动创建OsgVerse视图

如果想要强制创建OsgVerse视图，可以尝试：

```python
import FreeCAD
import FreeCADGui

# 创建文档
doc = FreeCAD.newDocument("TestDoc")

# 尝试创建OsgVerse视图（需要backend已注册）
# 注意：这可能需要C++层面的支持
```

## 故障排除

### 问题1：OsgVerse视图不显示

**原因**：RenderManager未初始化，系统使用默认的Coin3D backend

**解决方案**：
- 使用方法2启用RenderManager
- 或者接受使用Coin3D（推荐，更稳定）

### 问题2：创建视图时崩溃

**原因**：Backend未正确注册或初始化

**解决方案**：
- 确认OsgVerseGui模块已加载
- 检查日志中的"OsgVerseGui: Registered with ViewerFactory"消息

### 问题3：模块加载失败

**原因**：OsgVerseGui.so未构建或路径不正确

**解决方案**：
```bash
# 检查模块是否存在
ls -lh build/debug/Mod/OsgVerseGui/OsgVerseGui.so

# 重新构建
pixi run build-debug
```

## 性能对比

### Coin3D Backend（当前默认）
- ✅ 成熟稳定
- ✅ 完整功能支持
- ✅ 广泛测试
- ⚠️ 较旧的渲染技术

### OsgVerse Backend
- ✅ 现代渲染技术
- ✅ 更好的性能潜力
- ✅ 支持高级渲染特性
- ⚠️ 仍在开发中
- ⚠️ 可能有未知问题

## 建议

### 对于日常使用
**推荐使用Coin3D backend（当前默认）**
- 稳定可靠
- 功能完整
- 无需额外配置

### 对于开发和测试
**可以尝试OsgVerse backend**
- 测试新功能
- 性能对比
- 提供反馈

## 相关文件

- `src/Gui/Application.cpp` - 模块加载和初始化
- `src/Gui/Document.cpp` - 视图创建逻辑
- `src/Mod/OsgVerseGui/` - OsgVerse模块源代码
- `OSGVERSE_LOADING_GUIDE.md` - 详细的加载指南
