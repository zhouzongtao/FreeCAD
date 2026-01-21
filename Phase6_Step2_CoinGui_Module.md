# Phase 6 Step 2: CoinGui 模块创建

## 执行摘要

已完成 Phase 2 的基础工作 - 创建了 CoinGui 独立模块。这是后端模块化的关键一步，将 Coin3D 从 FreeCADGui 中分离出来。

## 已完成的工作

### 1. 创建模块结构

```
src/Mod/CoinGui/
├── CMakeLists.txt           # 构建配置
├── PreCompiled.h/cpp        # 预编译头
├── AppCoinGui.cpp           # 模块初始化
├── CoinBackendFactory.h/cpp # 后端工厂实现
├── CoinViewer.h/cpp         # Coin3D 视图实现
└── Resources/               # 资源目录
```

### 2. 核心组件

#### CoinBackendFactory
- 实现 `IBackendFactory` 接口
- 提供 Coin3D 后端的创建和销毁
- 返回 Coin3D 版本信息
- 优先级设置为 10（最高，作为默认后端）

**特性**：
- `getName()` → "Coin3D"
- `getDescription()` → "Coin3D rendering backend (default, stable)"
- `createViewer()` → 创建 CoinViewer 实例
- `isAvailable()` → 始终返回 true
- `getVersion()` → 返回 Coin3D 版本
- `getPriority()` → 返回 10（最高优先级）

#### CoinViewer
- 实现 `IViewer3D` 接口
- 封装 `SoQtExaminerViewer`
- 管理场景图（SoSeparator）
- 提供完整的 3D 视图功能

**实现的方法**：
- ✅ 场景管理（add/remove/update ViewProvider）
- ✅ 渲染控制（render, setBackgroundColor）
- ✅ 相机控制（viewAll, set/getCamera）
- ⚠️ 选择管理（TODO：需要集成现有选择系统）
- ⚠️ 导航样式（TODO：需要集成现有导航系统）
- ✅ 能力查询（supportsFeature）

#### AppCoinGui
- 模块初始化入口
- 注册 Coin3D 后端到 BackendRegistry
- 设置为默认后端（向后兼容）
- Python 模块导出

### 3. 构建系统

#### CMakeLists.txt 特性
- 链接 FreeCADGui
- **链接 Part 模块** ✅（这是关键！）
- 链接 Coin3D 库
- 支持预编译头
- 正确的安装配置

#### 添加到主构建
- 更新了 `src/Mod/CMakeLists.txt`
- 在 `BUILD_GUI` 条件下编译
- 位于 Assembly 和 Cloud 之间

### 4. 当前状态

**已实现**：
- ✅ 基础架构
- ✅ 后端注册机制
- ✅ 简单的场景管理
- ✅ 基本的渲染控制
- ✅ 相机控制

**待实现**（Phase 2 后续步骤）：
- ⚠️ 完整的 ViewProvider 集成
- ⚠️ 选择系统集成
- ⚠️ 导航系统集成
- ⚠️ 从 FreeCADGui 移动现有 Coin3D 代码
- ⚠️ 更新 FreeCADGui 以使用 CoinGui

## 架构对比

### 之前（Phase 1）
```
FreeCADGui
├── View3D/Interfaces/  ← Phase 1
└── 所有 Coin3D 代码直接在 FreeCADGui 中
```

### 现在（Phase 2 开始）
```
FreeCADGui
└── View3D/Interfaces/  ← Phase 1

CoinGui (新模块)  ← Phase 2
├── CoinBackendFactory
├── CoinViewer
└── 可以链接 Part 模块！✅
```

### 目标（Phase 2 完成后）
```
FreeCADGui
└── View3D/Interfaces/  ← 只有接口

CoinGui (独立模块)
├── 所有 Coin3D 代码
├── 链接 Part 模块 ✅
└── 可以调用 Part::Feature::getTopoShape() ✅

OsgVerseGui (Phase 3)
├── 所有 OsgVerse 代码
├── 链接 Part 模块 ✅
└── 可以调用 Part::Feature::getTopoShape() ✅
```

## 关键优势

### 1. 可以链接 Part 模块
```cmake
# src/Mod/CoinGui/CMakeLists.txt
set(CoinGui_LIBS
    FreeCADGui
    Part  # ← 可以链接！
)
```

这意味着 CoinGui 可以：
```cpp
// 直接访问 Part 模块
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PropertyPartShape.h>

// 直接调用
TopoDS_Shape shape = Part::Feature::getTopoShape(obj, options);
const TopoDS_Shape& shape2 = partShapeProp->getValue();
```

### 2. 模块化架构
- CoinGui 是独立的共享库（.dll/.so）
- 可以独立编译和测试
- 可以动态加载/卸载
- 不影响 FreeCADGui 核心

### 3. 对称性
- CoinGui 和 OsgVerseGui 将完全对称
- 都是独立模块
- 都可以链接 Part
- 都实现相同的接口

## 下一步工作

### Phase 2 剩余任务

#### 步骤 2.1：编译测试
```bash
cmake --build build --config Release --target CoinGui
```

#### 步骤 2.2：完善 CoinViewer
- 集成现有的 ViewProvider 系统
- 实现选择管理
- 实现导航样式切换
- 添加事件处理

#### 步骤 2.3：移动现有 Coin3D 代码
从 `src/Gui/` 移动到 `src/Mod/CoinGui/`：
- `View3DInventor.h/cpp`
- `View3DInventorViewer.h/cpp`
- `View3DInventorSelection.h/cpp`
- `SplitView3DInventor.h/cpp`
- `Inventor/` 目录
- `Quarter/` 目录
- 所有 Coin3D 相关的 ViewProvider

#### 步骤 2.4：更新 FreeCADGui
- 移除 Coin3D 特定代码
- 只保留抽象接口
- 更新 CMakeLists.txt
- 更新 Application.cpp 以加载 CoinGui

#### 步骤 2.5：测试
- 编译整个项目
- 测试 Coin3D 后端
- 验证向后兼容性
- 测试 Part 对象显示

## 风险和注意事项

### 风险

1. **大量代码移动**
   - 需要移动数千行代码
   - 可能破坏现有功能
   - 缓解：逐步移动，每步测试

2. **依赖关系复杂**
   - Coin3D 代码与 FreeCADGui 紧密耦合
   - 可能有隐藏的依赖
   - 缓解：仔细分析依赖，逐个解决

3. **向后兼容性**
   - 现有代码可能依赖 Coin3D 在 FreeCADGui 中
   - Python 脚本可能受影响
   - 缓解：保持 API 兼容，添加兼容层

### 注意事项

1. **编译顺序**
   - CoinGui 依赖 FreeCADGui
   - 必须先编译 FreeCADGui
   - CMake 会自动处理

2. **模块加载**
   - CoinGui 需要在启动时加载
   - 需要更新 Application::initApplication()
   - 确保在创建视图前加载

3. **Python 绑定**
   - 可能需要导出 Python 接口
   - 保持与现有 API 兼容
   - 考虑添加新的 Python API

## 测试计划

### 单元测试
1. **CoinBackendFactory 测试**
   - 创建/销毁 viewer
   - 版本信息
   - 可用性检查

2. **CoinViewer 测试**
   - 场景管理
   - 渲染控制
   - 相机控制

### 集成测试
1. **后端注册测试**
   - 注册成功
   - 设置为默认
   - 创建 viewer

2. **ViewProvider 测试**
   - 添加 Part 对象
   - 显示几何体
   - 材质和颜色

3. **兼容性测试**
   - 打开旧文件
   - 运行现有脚本
   - 测试所有工作台

## 编译命令

```bash
# 重新配置（如果需要）
cmake -S . -B build

# 编译 CoinGui 模块
cmake --build build --config Release --target CoinGui -j 4

# 编译整个项目
cmake --build build --config Release -j 4
```

## 总结

Phase 2 的基础工作已完成：
- ✅ 创建了 CoinGui 模块结构
- ✅ 实现了基本的后端工厂和视图
- ✅ 配置了构建系统
- ✅ 可以链接 Part 模块
- ✅ **编译成功！**（2026-01-21 16:10）

**关键发现**：
- FreeCAD 使用 Quarter（不是 SoQt）
- Quarter 已经在 FreeCADGui 中
- CoinViewer 使用 QuarterWidget

**下一步**：
1. ✅ 编译测试 CoinGui 模块 - **完成！**
2. 测试 CoinGui 模块加载
3. 完善 CoinViewer 实现
4. 开始移动现有 Coin3D 代码

---

**时间**: 2026-01-21  
**状态**: ✅ Phase 2 基础完成，编译成功  
**下一步**: 测试 CoinGui 模块加载
