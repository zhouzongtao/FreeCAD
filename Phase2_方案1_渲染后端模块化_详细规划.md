# 方案 1：渲染后端模块化 - 详细实施规划

## 执行摘要

本方案将 FreeCAD 的渲染架构重构为模块化设计，使 OsgVerse 和 Coin3D 成为平等的、可互换的渲染后端模块。这是最彻底的解决方案，将完全消除当前的架构限制。

## 目标

1. **统一架构**：让 OsgVerse 和 Coin3D 的机制完全一样
2. **消除循环依赖**：通过抽象接口层打破 `FreeCADGui  Part  FreeCADGui` 的循环
3. **直接访问 Shape**：两个后端都可以直接调用 `Part::Feature::getTopoShape()`
4. **动态切换**：运行时选择渲染后端，无需重新编译

## 当前架构 vs 目标架构

### 当前架构（问题）

```
FreeCADGui (包含 OsgVerse)
 Coin3D 代码（直接集成）
 OsgVerse 代码（编译到 FreeCADGui）
 不能链接 Part 模块（循环依赖）

PartGui (独立模块)
 链接 Part 模块 
 可以调用 Part::Feature::getTopoShape() 
```

**问题**：
- OsgVerse 在 FreeCADGui 中，不能链接 Part
- Coin3D 代码直接集成到 FreeCADGui，无法移除
- 架构不对称，OsgVerse 和 Coin3D 地位不平等

### 目标架构（方案 1）

```
FreeCADGui (核心 GUI 库)
 只包含抽象接口
    IViewer3D (接口)
    IViewProvider (接口)
    ViewerFactory (工厂)
 不包含具体渲染实现

CoinGui (独立模块)
 Coin3D 渲染实现
 链接 Part 模块 
 可以调用 Part::Feature::getTopoShape() 

OsgVerseGui (独立模块)
 OsgVerse 渲染实现
 链接 Part 模块 
 可以调用 Part::Feature::getTopoShape() 
```

**优势**：
- 两个后端完全平等
- 都可以直接访问 Part 模块
- FreeCADGui 只包含抽象接口
- 可以动态加载/卸载后端

