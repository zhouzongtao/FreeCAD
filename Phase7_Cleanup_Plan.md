# Phase 7: 清理旧代码

## 目标

清理 FreeCADGui 中的旧 OsgVerse 实现，因为现在 OsgVerse 已经作为独立的 OsgVerseGui 模块实现。

## 需要清理的内容

### 1. 旧的 OsgVerse 后端文件（在 FreeCADGui 中）

**目录**: `src/Gui/View3D/Backends/OsgVerse/`

需要删除的文件：
- `OsgVerseViewerImpl.h`
- `OsgVerseViewerImpl.cpp`
- `GeometryConverter.h` (已移动到 OsgVerseGui)
- `GeometryConverter.cpp` (已移动到 OsgVerseGui)
- `PreCompiled.h`
- `PreCompiled.cpp`

**注意**: 这些文件的功能已经在 `src/Mod/OsgVerseGui/` 中重新实现。

### 2. View3DOsgVerse 类

**文件**: 
- `src/Gui/View3DOsgVerse.h`
- `src/Gui/View3DOsgVerse.cpp`

这个类是旧的 OsgVerse 视图实现，现在应该使用新的后端架构。

### 3. CMakeLists.txt 中的 OsgVerse 配置

**文件**: `src/Gui/View3D/CMakeLists.txt`

需要移除：
```cmake
if(BUILD_WITH_OSGVERSE)
    # ... OsgVerse 相关配置
endif()
```

### 4. Application.cpp 中的引用

**文件**: `src/Gui/Application.cpp`

需要移除：
- `#include "View3DOsgVerse.h"`
- `Gui::View3DOsgVerse::init();`

### 5. Document.cpp 中的引用

**文件**: `src/Gui/Document.cpp`

需要移除：
- `#include "View3DOsgVerse.h"`
- `#include "View3D/Backends/OsgVerse/OsgVerseViewerImpl.h"`
- 所有 `View3DOsgVerse` 相关的代码

## 实施步骤

### Step 1: 更新 CMakeLists.txt
移除 `src/Gui/View3D/CMakeLists.txt` 中的 OsgVerse 配置块

### Step 2: 更新 Application.cpp
移除 View3DOsgVerse 的引用和初始化

### Step 3: 更新 Document.cpp
移除 View3DOsgVerse 相关的代码

### Step 4: 删除旧文件
- 删除 `src/Gui/View3DOsgVerse.h`
- 删除 `src/Gui/View3DOsgVerse.cpp`
- 删除 `src/Gui/View3D/Backends/OsgVerse/` 目录

### Step 5: 编译测试
确保清理后代码可以正常编译

## 保留的内容

以下内容需要保留：
- `src/Gui/View3D/Interfaces/` - 新的抽象接口层
- `src/Gui/View3D/Backends/Coin/` - Coin3D 后端（仍在 FreeCADGui 中）
- `src/Mod/CoinGui/` - 独立的 CoinGui 模块
- `src/Mod/OsgVerseGui/` - 独立的 OsgVerseGui 模块

## 注意事项

1. **不要删除 Coin 后端**: `src/Gui/View3D/Backends/Coin/` 仍然需要保留，因为它是 FreeCADGui 的一部分
2. **保留接口层**: `src/Gui/View3D/Interfaces/` 是新架构的核心，必须保留
3. **检查依赖**: 确保没有其他代码依赖被删除的文件

## 预期结果

清理后：
- FreeCADGui 不再包含 OsgVerse 的具体实现
- OsgVerse 功能完全由独立的 OsgVerseGui 模块提供
- 代码更清晰，职责分离更明确
- 编译成功，测试通过
