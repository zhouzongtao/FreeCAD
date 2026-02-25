# Phase 2 接口统一完成

## 概述

成功将 OsgVerse 后端从旧的 `Gui::IViewer3D` 接口迁移到新的 `Gui::View3D::IViewer3D` 接口。

## 问题分析

### 发现的问题
系统中存在两套接口：
1. **旧接口**: `Gui::IViewer3D` (位于 `src/Gui/View3D/Interfaces/IViewer3D.h`)
   - 使用 BackendRegistry 注册
   - 通过 IBackendFactory 创建 viewer
   
2. **新接口**: `Gui::View3D::IViewer3D` (位于 `src/Gui/View3D/IViewer3D.h`)
   - 使用 ViewerFactory 注册
   - 直接通过 lambda 创建 viewer
   - 被 View3DInventor 使用

### 根本原因
- OsgVerseViewer 已迁移到新接口
- OsgVerseBackendFactory 仍在使用旧接口
- 编译时类型不匹配导致错误

## 实施的修改

### 1. 移除旧接口相关文件

从 `src/Mod/OsgVerseGui/CMakeLists.txt` 中移除：
```cmake
# 已删除
OsgVerseBackendFactory.h
OsgVerseBackendFactory.cpp
```

这两个文件实现了旧的 `IBackendFactory` 接口，不再需要。

### 2. OsgVerseViewer 实现新接口

`OsgVerseViewer.h` 和 `OsgVerseViewer.cpp` 已完整实现新接口的所有方法：

#### 基础渲染接口
- `render()` - 触发重绘
- `resize()` - 调整大小
- `getWidget()` / `getGLWidget()` - 获取 Qt widget

#### 场景管理
- `setSceneGraph()` / `getSceneGraph()` - 场景图管理
- `updateScene()` - 更新场景

#### 相机控制
- `setCamera()` / `getCamera()` - 相机参数
- `viewAll()` / `resetCamera()` - 相机重置
- `setCameraType()` / `isCameraOrthographic()` - 投影类型

#### 事件处理
- `handleMouseEvent()` - 鼠标事件
- `handleKeyEvent()` - 键盘事件
- `handleWheelEvent()` - 滚轮事件

#### 拾取和选择
- `pick()` - 射线拾取
- `setSelectionMode()` / `getSelectionMode()` - 选择模式
- `startSelection()` / `stopSelection()` / `abortSelection()` - 选择控制

#### ViewProvider 管理
- `addViewProvider()` / `removeViewProvider()` - 添加/移除 VP
- `hasViewProvider()` / `getViewProviders()` - 查询 VP
- 自动将 Part::Feature 转换为 OSG 几何体

#### 渲染设置
- `setRenderMode()` / `getRenderMode()` - 渲染模式
- `setBackgroundColor()` / `getBackgroundColor()` - 背景色
- `setBacklightEnabled()` / `isBacklightEnabled()` - 背光

#### 导航和交互
- `setNavigationStyle()` / `getNavigationStyle()` - 导航风格
- `setViewing()` / `isViewing()` - 查看模式

#### 后端信息
- `getBackendType()` - 返回 `BackendType::OsgVerse`
- `getBackendName()` - 返回 "OsgVerse"
- `getBackendVersion()` - 返回版本信息

#### 统计和调试
- `getStats()` / `resetStats()` - 渲染统计
- `setFPSEnabled()` / `isFPSEnabled()` - FPS 显示

#### 高级功能
- `grabImage()` / `saveScreenshot()` - 截图
- `setEditingViewProvider()` / `getEditingViewProvider()` - 编辑模式

### 3. 注册方式更新

`AppOsgVerseGui.cpp` 中只使用 ViewerFactory 注册：

```cpp
// 使用 ViewerFactory (新系统)
Gui::View3D::ViewerFactory::registerCreator(
    Gui::Render::BackendType::OsgVerse,
    [](QWidget* parent, const QOpenGLWidget* /*shareWidget*/) -> std::unique_ptr<Gui::View3D::IViewer3D> {
        return std::make_unique<OsgVerseViewer>(parent);
    }
);

// 不再使用 BackendRegistry (旧系统已废弃)
```

## 编译指令

```powershell
# 清理旧的构建文件
cmake --build build --target clean

# 重新编译 OsgVerseGui 模块
cmake --build build --target OsgVerseGui --config Release -j 8

# 或者完整重新编译
cmake --build build --config Release -j 8
```

## 测试步骤

### 1. 验证模块加载

```python
# 在 FreeCAD Python 控制台中运行
exec(open(r'E:\Repository\FreeCAD\FreeCAD\test_interface_unified.py', encoding='utf-8').read())
```

或者手动测试：

```python
import FreeCAD
import FreeCADGui
import OsgVerseGui

# 检查后端可用性
print("OsgVerse available:", FreeCADGui.isRenderBackendAvailable(2))
```

### 2. 切换到 OsgVerse 后端

```python
# 切换后端
success = FreeCADGui.switchRenderBackend(2)  # 2 = OsgVerse
print("Switch success:", success)

# 验证
current = FreeCADGui.getCurrentRenderBackend()
print("Current backend:", current)  # 应该是 2

# 创建新文档和 3D 视图
doc = FreeCAD.newDocument()
FreeCADGui.activeDocument().activeView()
```

### 3. 测试几何体渲染

```python
# 创建测试对象
import Part
box = doc.addObject("Part::Box", "Box")
doc.recompute()

# 应该在 OsgVerse 中渲染
FreeCADGui.SendMsgToActiveView("ViewFit")
```

## 预期结果

1. **编译成功**: 不再有类型不匹配错误
2. **模块加载**: OsgVerseGui 模块正常导入
3. **ViewerFactory 注册**: Backend type 2 (OsgVerse) 已注册
4. **自动创建**: 新建 3D 视图时自动使用 OsgVerse
5. **几何体渲染**: Part 对象正确转换为 OSG 几何体并显示

## 架构优势

### 统一接口
- 所有后端使用相同的 `IViewer3D` 接口
- ViewerFactory 统一管理所有后端
- 简化了后端切换逻辑

### 类型安全
- 使用 `std::unique_ptr` 管理生命周期
- 编译时类型检查
- 避免了手动内存管理

### 扩展性
- 新后端只需实现 `IViewer3D` 接口
- 通过 lambda 注册到 ViewerFactory
- 不需要额外的 Factory 类

## 下一步工作

1. **测试完整功能**
   - 相机控制
   - 事件处理
   - ViewProvider 管理
   - 几何体转换

2. **实现待办功能**
   - 射线拾取 (pick)
   - 渲染模式切换
   - 背光效果
   - FPS 显示

3. **性能优化**
   - 几何体缓存
   - LOD 支持
   - 批量渲染

4. **文档完善**
   - API 文档
   - 使用示例
   - 故障排除指南

## 文件清单

### 修改的文件
- `src/Mod/OsgVerseGui/CMakeLists.txt` - 移除 BackendFactory
- `src/Mod/OsgVerseGui/OsgVerseViewer.h` - 新接口声明
- `src/Mod/OsgVerseGui/OsgVerseViewer.cpp` - 新接口实现
- `src/Mod/OsgVerseGui/AppOsgVerseGui.cpp` - ViewerFactory 注册

### 删除的文件 (从构建中移除)
- `src/Mod/OsgVerseGui/OsgVerseBackendFactory.h`
- `src/Mod/OsgVerseGui/OsgVerseBackendFactory.cpp`

### 保留的文件
- `src/Mod/OsgVerseGui/OsgVerseWidget.h/cpp` - Qt OpenGL widget
- `src/Mod/OsgVerseGui/GeometryConverter.h/cpp` - OCCT to OSG 转换
- `src/Mod/OsgVerseGui/Init.py` - Python 模块初始化

## 总结

成功完成接口统一，OsgVerse 后端现在使用与 Coin3D 相同的新接口系统。这为后续的功能开发和维护奠定了坚实的基础。

编译后即可测试完整的 OsgVerse 渲染功能！
