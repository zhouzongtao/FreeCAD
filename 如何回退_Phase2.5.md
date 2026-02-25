# 如何回退 Phase 2.5 改动

如果需要回退到 Phase 2.5 之前的状态，按以下步骤操作：

## 回退步骤

### 1. 从 CMakeLists.txt 移除 View3DOsgVerse

编辑 `src/Gui/CMakeLists.txt`，移除：
```cmake
View3DOsgVerse.cpp
View3DOsgVerse.h
```

### 2. 恢复 Document.cpp

编辑 `src/Gui/Document.cpp`：

移除包含：
```cpp
#include "View3DOsgVerse.h"
```

恢复 createView 方法到原始状态（移除 View3DOsgVerse 相关代码）

### 3. 恢复 Application.cpp

编辑 `src/Gui/Application.cpp`：

移除包含：
```cpp
#include "View3DOsgVerse.h"
```

移除 init 调用：
```cpp
Gui::View3DOsgVerse::init();
```

### 4. 重新编译

```powershell
cmake --build build --target FreeCADGui --config Release -j 8
```

## 保留的部分（这些是有价值的）

✅ **保留**：
- `src/Mod/OsgVerseGui/OsgVerseViewer.h/cpp` - IViewer3D 接口实现
- `src/Mod/OsgVerseGui/OsgVerseWidget.h/cpp` - OpenGL widget
- `src/Mod/OsgVerseGui/GeometryConverter.h/cpp` - 几何转换
- ViewerFactory 注册（在 AppOsgVerseGui.cpp 中）

这些是核心功能，已经正确实现。

## 可选：删除 Phase 2.5 文件

如果想完全清理：
```powershell
# 删除源文件
Remove-Item src/Gui/View3DOsgVerse.h
Remove-Item src/Gui/View3DOsgVerse.cpp

# 删除文档（可选）
Remove-Item Phase2.5_*.md
Remove-Item test_phase2.5_*.py
Remove-Item diagnose_view3dosgverse.py
```

## 回退后的状态

回退后，你将回到：
- ✅ OsgVerseViewer 实现了 IViewer3D 接口
- ✅ ViewerFactory 可以创建 OsgVerseViewer
- ✅ Python API 正常工作
- ⚠️ 但 View3DInventor 无法使用 OsgVerseViewer（会回退到 Coin3D）

这是 Phase 2 完成后的状态。

## 替代方案

如果不想完全回退，可以考虑：

### 方案 1: 暂时禁用 View3DOsgVerse
在 Document.cpp 中注释掉 OsgVerse 分支：
```cpp
if (backend == Gui::Render::BackendType::OsgVerse) {
    // 暂时禁用
    // return createView(View3DOsgVerse::getClassTypeId(), mode);
}
```

这样会继续使用 Coin3D，但代码保留。

### 方案 2: 添加调试开关
```cpp
bool useView3DOsgVerse = false;  // 调试开关
if (backend == Gui::Render::BackendType::OsgVerse && useView3DOsgVerse) {
    return createView(View3DOsgVerse::getClassTypeId(), mode);
}
```

## 记住

Phase 2.5 的工作不是白费的：
- 学习了类型系统
- 理解了视图创建流程
- 知道了 ViewerFactory 的工作原理
- 这些知识对找到正确方案很有帮助

休息一下，明天重新开始！
