# Phase 6 Step 3: 创建 OsgVerseGui 独立模块

## 目标

创建 OsgVerseGui 作为独立的共享库模块，与 CoinGui 对称。这样 OsgVerseGui 就可以链接 Part 模块，直接访问 `Part::Feature::getTopoShape()`。

## 架构对比

### 当前架构（Phase 2 完成）

```
FreeCADGui (核心库)
└── View3D/Interfaces/  ← 抽象接口层

CoinGui (独立模块) ✅
├── 可以链接 Part 模块
└── 实现 IViewer3D 接口

OsgVerse (编译到 FreeCADGui) ❌
├── 不能链接 Part 模块
└── 需要 Python API 桥接
```

### 目标架构（Phase 3）

```
FreeCADGui (核心库)
└── View3D/Interfaces/  ← 抽象接口层

CoinGui (独立模块) ✅
├── 可以链接 Part 模块
└── 实现 IViewer3D 接口

OsgVerseGui (独立模块) ← 新增
├── 可以链接 Part 模块 ✅
├── 实现 IViewer3D 接口
└── 直接调用 Part::Feature::getTopoShape() ✅
```

## 模块结构

```
src/Mod/OsgVerseGui/
├── CMakeLists.txt              # 构建配置
├── PreCompiled.h/cpp           # 预编译头
├── AppOsgVerseGui.cpp          # 模块初始化
├── OsgVerseBackendFactory.h/cpp # 后端工厂
├── OsgVerseViewer.h/cpp        # OsgVerse 视图
└── GeometryConverter.h/cpp     # 几何体转换（从 FreeCADGui 移动）
```

## 实施步骤

### Step 1: 创建模块目录和基础文件

1. **创建目录**
   ```
   src/Mod/OsgVerseGui/
   ```

2. **创建 CMakeLists.txt**
   - 参考 CoinGui 的配置
   - 链接 FreeCADGui, Part, OSG, OCCT
   - 配置预编译头

3. **创建 PreCompiled.h/cpp**
   - 包含常用头文件
   - OSG 头文件
   - OCCT 头文件
   - FreeCAD 头文件

### Step 2: 实现核心组件

1. **OsgVerseBackendFactory**
   - 实现 `IBackendFactory` 接口
   - 创建和销毁 OsgVerseViewer
   - 优先级 5（低于 Coin3D）
   - 版本信息："OsgVerse + OSG 3.6+"

2. **OsgVerseViewer**
   - 实现 `IViewer3D` 接口
   - 管理 OSG 场景图
   - 集成 GeometryConverter
   - **直接访问 Part::Feature::getTopoShape()** ✅

3. **GeometryConverter**
   - 从 `src/Gui/View3D/Backends/OsgVerse/` 移动
   - 保持现有实现不变
   - 已经完整实现了 OCCT → OSG 转换

### Step 3: 模块初始化

1. **AppOsgVerseGui.cpp**
   - 初始化 OSG
   - 注册 OsgVerse 后端
   - Python 模块导出

### Step 4: 更新构建系统

1. **src/Mod/CMakeLists.txt**
   - 添加 OsgVerseGui 子目录

2. **src/Gui/View3D/CMakeLists.txt**
   - 移除 OsgVerse 后端代码
   - 只保留接口层

## 关键优势

### 1. 可以链接 Part 模块 ✅

```cpp
// OsgVerseViewer.cpp
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PropertyPartShape.h>

void OsgVerseViewer::addViewProvider(Gui::ViewProvider* vp)
{
    // 直接调用 Part::Feature::getTopoShape()
    TopoDS_Shape shape = Part::Feature::getTopoShape(
        obj,
        Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
    );
    
    // 转换为 OSG Geometry
    osg::ref_ptr<osg::Geode> geode = GeometryConverter::convertShape(shape);
    
    // 添加到场景
    _sceneRoot->addChild(geode);
}
```

### 2. 与 CoinGui 对称 ✅

- 两个后端都是独立模块
- 都可以链接 Part 模块
- 都实现相同的接口
- 架构完全一致

### 3. 真实几何体渲染 ✅

- 不再需要 Python API 桥接
- 直接访问 TopoDS_Shape
- 使用已实现的 GeometryConverter
- 显示真实的 Part 对象

### 4. 模块化架构 ✅

- OsgVerseGui 是独立的共享库
- 可以独立编译和测试
- 可以动态加载/卸载
- 不影响 FreeCADGui 核心

## 文件清单

### 新增文件

- `src/Mod/OsgVerseGui/CMakeLists.txt`
- `src/Mod/OsgVerseGui/PreCompiled.h`
- `src/Mod/OsgVerseGui/PreCompiled.cpp`
- `src/Mod/OsgVerseGui/AppOsgVerseGui.cpp`
- `src/Mod/OsgVerseGui/OsgVerseBackendFactory.h`
- `src/Mod/OsgVerseGui/OsgVerseBackendFactory.cpp`
- `src/Mod/OsgVerseGui/OsgVerseViewer.h`
- `src/Mod/OsgVerseGui/OsgVerseViewer.cpp`

### 移动文件

从 `src/Gui/View3D/Backends/OsgVerse/` 移动到 `src/Mod/OsgVerseGui/`：
- `GeometryConverter.h`
- `GeometryConverter.cpp`

### 修改文件

- `src/Mod/CMakeLists.txt` - 添加 OsgVerseGui 子目录
- `src/Gui/View3D/CMakeLists.txt` - 移除 OsgVerse 后端代码

## 下一步

1. 创建 OsgVerseGui 模块结构
2. 实现核心组件
3. 编译测试
4. 测试真实几何体渲染

---

**时间**: 2026-01-21
**状态**: 📝 规划完成
**下一步**: 开始实施
