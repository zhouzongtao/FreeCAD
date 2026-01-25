# Phase 2 接口统一 - 快速参考

## 编译

```powershell
cmake --build build --target OsgVerseGui --config Release -j 8
```

## 测试

### 1. 快速测试脚本

```python
# 在 FreeCAD Python 控制台中运行
exec(open(r'E:\Repository\FreeCAD\FreeCAD\test_interface_unified.py', encoding='utf-8').read())
```

或者使用命令行：
```powershell
FreeCADCmd.exe -c "exec(open(r'E:\Repository\FreeCAD\FreeCAD\test_interface_unified.py', encoding='utf-8').read())"
```

### 2. 手动测试

```python
# 启动 FreeCAD
import FreeCAD
import FreeCADGui
import OsgVerseGui

# 检查后端可用性
print("Coin3D available:", FreeCADGui.isRenderBackendAvailable(1))
print("OsgVerse available:", FreeCADGui.isRenderBackendAvailable(2))

# 获取当前后端
current = FreeCADGui.getCurrentRenderBackend()
print("Current backend:", current)  # 0=None, 1=Coin3D, 2=OsgVerse

# 切换到 OsgVerse
success = FreeCADGui.switchRenderBackend(2)
print("Switch success:", success)

# 获取渲染器信息
print("Renderer:", FreeCADGui.getRendererInfo())

# 创建文档和视图
doc = FreeCAD.newDocument()
view = FreeCADGui.activeDocument().activeView()

# 创建几何体
import Part
box = doc.addObject("Part::Box", "Box")
doc.recompute()
FreeCADGui.SendMsgToActiveView("ViewFit")

# 获取渲染统计
stats = FreeCADGui.getRenderStats()
print("FPS:", stats['fps'])
print("Triangles:", stats['triangleCount'])
```

## 架构变化

### 旧系统 (已废弃)
```
BackendRegistry
    ↓
IBackendFactory (OsgVerseBackendFactory)
    ↓
Gui::IViewer3D (旧接口)
```

### 新系统 (当前)
```
ViewerFactory
    ↓
Lambda creator
    ↓
Gui::View3D::IViewer3D (新接口)
```

## 关键文件

### 已移除 (从构建中)
- `OsgVerseBackendFactory.h`
- `OsgVerseBackendFactory.cpp`

### 核心文件
- `OsgVerseViewer.h/cpp` - 实现新接口
- `AppOsgVerseGui.cpp` - ViewerFactory 注册
- `OsgVerseWidget.h/cpp` - Qt OpenGL widget
- `GeometryConverter.h/cpp` - OCCT to OSG

## Python API

### 后端管理函数

所有函数都通过 `FreeCADGui` 模块直接访问：

```python
import FreeCADGui

# 检查后端可用性
FreeCADGui.isRenderBackendAvailable(type)  # type: 0=None, 1=Coin3D, 2=OsgVerse

# 获取当前后端
FreeCADGui.getCurrentRenderBackend()  # 返回: 0, 1, 或 2

# 切换后端
FreeCADGui.switchRenderBackend(type)  # 返回: bool

# 获取渲染器信息
FreeCADGui.getRendererInfo()  # 返回: str

# 获取渲染统计
FreeCADGui.getRenderStats()  # 返回: dict

# 重置统计
FreeCADGui.resetRenderStats()
```

## 故障排除

### 编译错误: 类型不匹配
**原因**: OsgVerseBackendFactory 仍在构建中
**解决**: 从 CMakeLists.txt 中移除

### ViewerFactory 未注册
**原因**: 模块未加载
**解决**: 确保 Init.py 存在并正确安装

### 视图创建失败
**原因**: OSG 初始化问题
**解决**: 检查 OSG DLL 路径和版本

## 下一步

1. 测试完整功能
2. 实现待办功能 (pick, 渲染模式等)
3. 性能优化
4. 文档完善

## 参考文档

- `Phase2_接口统一完成.md` - 详细说明
- `Phase2_Implementation_Complete.md` - Phase 2 完成报告
- `Phase2_快速参考.md` - Phase 2 快速参考
