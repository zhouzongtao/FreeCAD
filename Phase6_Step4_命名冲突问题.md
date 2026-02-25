# Phase 6 Step 4 - 命名冲突问题

## 问题描述

在实施 View3D 抽象基类时遇到命名冲突：

```
error C2869: 'Gui::View3D': has already been defined to be a namespace
```

## 根本原因

`View3D` 这个名字已经被用作命名空间：
- `src/Gui/View3D/` 目录下的所有内容都在 `namespace Gui::View3D` 中
- 包括 `IViewer3D.h`, `ViewerFactory.h`, `CoinViewer.h`, `OsgVerseViewerImpl.h` 等

现在试图创建一个类 `class Gui::View3D`，与命名空间冲突。

## 解决方案

需要重命名抽象基类，有以下选项：

### 选项 A: View3DBase
```cpp
class GuiExport View3DBase : public MDIView
{
    // ...
};

class GuiExport View3DInventor : public View3DBase { };
class GuiExport View3DOsgVerse : public View3DBase { };
```

### 选项 B: AbstractView3D
```cpp
class GuiExport AbstractView3D : public MDIView
{
    // ...
};

class GuiExport View3DInventor : public AbstractView3D { };
class GuiExport View3DOsgVerse : public AbstractView3D { };
```

### 选项 C: View3DWindow
```cpp
class GuiExport View3DWindow : public MDIView
{
    // ...
};

class GuiExport View3DInventor : public View3DWindow { };
class GuiExport View3DOsgVerse : public View3DWindow { };
```

## 推荐方案

**选项 A: View3DBase** 最合适，因为：
1. 清晰表明这是基类
2. 与现有命名风格一致
3. 简洁明了

## 需要修改的文件

1. `src/Gui/View3D.h` → `src/Gui/View3DBase.h`
2. `src/Gui/View3D.cpp` → `src/Gui/View3DBase.cpp`
3. `src/Gui/View3DInventor.h` - 修改继承
4. `src/Gui/View3DInventor.cpp` - 修改继承
5. `src/Gui/View3DOsgVerse.h` - 修改继承
6. `src/Gui/View3DOsgVerse.cpp` - 修改继承
7. `src/Gui/Document.cpp` - 修改类型检查
8. `src/Gui/CMakeLists.txt` - 修改文件名

## 下一步

重命名所有相关文件和引用，然后重新编译。
