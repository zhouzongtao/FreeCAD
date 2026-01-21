# OsgVerse 快速参考

## 当前状态

✅ **编译完成** - OsgVerse 后端已成功编译  
⏸️ **未测试** - 尚未进行运行时测试  
🔄 **默认后端** - Coin3D（需要手动切换到 OsgVerse）

## 快速启动

### 1. 启动 FreeCAD

```cmd
cd E:\Repository\FreeCAD\FreeCAD\build\bin
FreeCAD.exe
```

### 2. 检查当前后端

```cmd
python check_current_backend.py
```

### 3. 切换到 OsgVerse（方法 1：修改代码）

编辑 `src/Gui/Render/Core/RenderEngine.h`：

```cpp
BackendType _defaultType{BackendType::OsgVerse};  // 改为 OsgVerse
```

重新编译：
```cmd
cmake --build build --config Release --target FreeCADGui
```

### 4. 切换到 OsgVerse（方法 2：Python API - 需要 Phase 5）

```python
import FreeCADGui
FreeCADGui.switchRenderBackend("OsgVerse")
```

## 关键文件位置

### 源代码
```
src/Gui/Render/
├── Core/
│   ├── RenderEngine.h          # 默认后端设置
│   ├── RenderManager.h         # 渲染管理器
│   └── RenderManagerPy.cpp     # Python 绑定（未启用）
└── Backends/
    ├── Coin3D/                 # Coin3D 后端
    └── OsgVerse/               # OsgVerse 后端
        ├── OsgVerseEngine.cpp
        ├── OsgVerseViewer.cpp
        ├── OsgVerseGraphicsWindow.cpp
        └── ...
```

### 构建输出
```
build/
├── bin/
│   ├── FreeCAD.exe
│   ├── FreeCADGui.dll          # 包含 OsgVerse 后端
│   └── osg161-*.dll            # OSG 运行时库
└── src/Gui/FreeCADGui.vcxproj
```

### 工具脚本
```
├── switch_to_osgverse.py       # 切换脚本（需要 Python API）
├── check_current_backend.py    # 检查当前后端
├── test_osgverse_startup.py    # 启动测试
└── diagnose_freecad_startup.ps1 # 诊断脚本
```

### 文档
```
├── OsgVerse_Phase1_完成报告.md
├── OsgVerse_Phase2_完成总结.md
├── OsgVerse_Phase3_完成总结.md
├── OsgVerse_下一步工作计划.md
└── 切换OsgVerse渲染引擎指南.md
```

## 常用命令

### 编译 FreeCADGui

```cmd
cmake --build build --config Release --target FreeCADGui
```

### 完整重新编译

```cmd
cmake --build build --config Release --target clean
cmake --build build --config Release --target FreeCADGui
```

### 检查 DLL 依赖

```cmd
python check_dll_dependencies.py
```

### 查看构建日志

```cmd
type build_freecadgui.log
```

## 调试技巧

### 1. 查看 OsgVerse 日志

启动 FreeCAD 后，在控制台查找：
```
OsgVerseEngine: Constructor called
OsgVerseEngine::initialize: Initializing...
OsgVerseGraphicsWindow: Created
```

### 2. 检查 OpenGL 上下文

在 FreeCAD Python 控制台：
```python
import FreeCADGui
# 检查 3D 视图是否存在
view = FreeCADGui.ActiveDocument.ActiveView
print(view)
```

### 3. 测试简单渲染

```python
import FreeCAD
import Part

doc = FreeCAD.newDocument()
box = doc.addObject("Part::Box", "Box")
doc.recompute()
FreeCADGui.SendMsgToActiveView("ViewFit")
```

## 已知问题

### 1. Python API 未启用
- **状态：** RenderManagerPy.cpp 已创建但未添加到构建系统
- **影响：** 无法通过 Python 切换后端
- **解决：** Phase 5 将解决此问题

### 2. 默认后端是 Coin3D
- **状态：** OsgVerse 已编译但不是默认后端
- **影响：** 需要手动切换
- **解决：** 修改 RenderEngine.h 或使用 Python API

### 3. 未经过实际测试
- **状态：** 编译成功但未验证渲染功能
- **影响：** 可能存在运行时问题
- **解决：** Phase 4 将进行全面测试

## 架构概览

```
应用层 (FreeCAD)
    │
    ▼
渲染抽象层 (RenderManager, RenderViewer, RenderNode)
    │
    ├─────────────┬─────────────┐
    ▼             ▼             ▼
Coin3D 后端   OsgVerse 后端   未来后端
    │             │
    ▼             ▼
Coin3D 库     OSG + OsgVerse 库
```

## 性能指标（预期）

- **帧率：** > 30 FPS（简单场景）
- **启动时间：** < 5 秒
- **内存占用：** 与 Coin3D 相当
- **渲染质量：** 支持 PBR、HDR 等现代特性

## 下一步

1. ✅ **Phase 3 完成** - GraphicsWindow 集成
2. 🔄 **Phase 4 进行中** - 运行时测试
3. ⏳ **Phase 5 待开始** - Python API 集成
4. ⏳ **Phase 6 待开始** - 文档和发布

## 快速链接

- **OSG 文档：** http://www.openscenegraph.org/
- **OsgVerse 仓库：** https://github.com/xarray/osgverse
- **FreeCAD 论坛：** https://forum.freecadweb.org/

## 联系信息

如有问题，请参考：
1. `OsgVerse_下一步工作计划.md` - 详细的测试计划
2. `OsgVerse_Phase3_完成总结.md` - Phase 3 完成情况
3. 控制台日志 - 运行时错误信息

---

**最后更新：** 2026-01-19  
**版本：** Phase 3 完成
