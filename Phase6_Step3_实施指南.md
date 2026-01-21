# Phase 6 Step 3 实施指南：OsgVerseGui 模块

## 概述

本指南详细说明如何编译、测试和使用 OsgVerseGui 模块。

## 前提条件

### 已完成的工作
- ✅ Phase 1: 抽象接口层（`src/Gui/View3D/Interfaces/`）
- ✅ Phase 2: CoinGui 模块（`src/Mod/CoinGui/`）
- ✅ Phase 3: OsgVerseGui 模块代码（`src/Mod/OsgVerseGui/`）

### 系统要求
- FreeCAD 源代码
- CMake 3.16+
- C++17 编译器
- Qt 5.12+
- OSG 3.6+
- OsgVerse 库
- OCCT 7.x

## 编译步骤

### 步骤 1: 配置 CMake

```bash
# 确保启用 OsgVerse 支持
cmake -B build \
    -DBUILD_GUI=ON \
    -DBUILD_WITH_OSGVERSE=ON \
    -DCMAKE_BUILD_TYPE=Debug

# Windows (PowerShell)
cmake -B build `
    -DBUILD_GUI=ON `
    -DBUILD_WITH_OSGVERSE=ON `
    -DCMAKE_BUILD_TYPE=Debug
```

**关键选项**：
- `BUILD_GUI=ON` - 启用 GUI 构建
- `BUILD_WITH_OSGVERSE=ON` - 启用 OsgVerse 支持
- `CMAKE_BUILD_TYPE=Debug` - 调试模式（可选）

### 步骤 2: 编译依赖模块

```bash
# 1. 编译 FreeCADGui（包含接口层）
cmake --build build --target FreeCADGui

# 2. 编译 Part 模块（OsgVerseGui 依赖）
cmake --build build --target Part

# 3. 编译 CoinGui 模块
cmake --build build --target CoinGui
```

### 步骤 3: 编译 OsgVerseGui

```bash
# 编译 OsgVerseGui 模块
cmake --build build --target OsgVerseGui

# 查看详细输出（如果需要）
cmake --build build --target OsgVerseGui -- -v
```

**预期输出**：
```
[ 50%] Building CXX object src/Mod/OsgVerseGui/CMakeFiles/OsgVerseGui.dir/PreCompiled.cpp.obj
[ 60%] Building CXX object src/Mod/OsgVerseGui/CMakeFiles/OsgVerseGui.dir/AppOsgVerseGui.cpp.obj
[ 70%] Building CXX object src/Mod/OsgVerseGui/CMakeFiles/OsgVerseGui.dir/OsgVerseBackendFactory.cpp.obj
[ 80%] Building CXX object src/Mod/OsgVerseGui/CMakeFiles/OsgVerseGui.dir/OsgVerseViewer.cpp.obj
[ 90%] Building CXX object src/Mod/OsgVerseGui/CMakeFiles/OsgVerseGui.dir/GeometryConverter.cpp.obj
[100%] Linking CXX shared library ../../../bin/OsgVerseGui.pyd
[100%] Built target OsgVerseGui
```

**生成文件**：
- Windows: `build/bin/OsgVerseGui.pyd`
- Linux: `build/lib/OsgVerseGui.so`

### 步骤 4: 完整构建（可选）

```bash
# 编译所有目标
cmake --build build

# 或者使用并行编译
cmake --build build -j 8
```

## 测试步骤

### 测试 1: 模块加载

```bash
# 启动 FreeCAD
build/bin/FreeCAD.exe  # Windows
build/bin/FreeCAD      # Linux
```

在 Python 控制台中：

```python
# 测试模块导入
import OsgVerseGui
print("✅ OsgVerseGui 模块加载成功")
```

### 测试 2: 后端注册

```python
# 检查后端注册
from Gui import BackendRegistry

# 列出所有可用后端
backends = BackendRegistry.getAvailableBackends()
print(f"可用后端: {backends}")
# 预期输出: ['Coin3D', 'OsgVerse']

# 检查 OsgVerse 是否可用
if BackendRegistry.isBackendAvailable("OsgVerse"):
    print("✅ OsgVerse 后端可用")
else:
    print("❌ OsgVerse 后端不可用")
```

### 测试 3: 后端信息

```python
# 获取后端信息
info = BackendRegistry.getBackendInfo("OsgVerse")
print("OsgVerse 后端信息:")
for key, value in info.items():
    print(f"  {key}: {value}")

# 预期输出:
# name: OsgVerse
# description: OsgVerse rendering backend using OpenSceneGraph
# version: OsgVerse + OSG 3.6+
# priority: 5
```

### 测试 4: 视图创建

```python
# 创建 OsgVerse 视图
viewer = BackendRegistry.createViewer("OsgVerse")

if viewer:
    print(f"✅ 视图创建成功")
    print(f"  后端名称: {viewer.getBackendName()}")
    print(f"  版本: {viewer.getVersion()}")
else:
    print("❌ 视图创建失败")
```

### 测试 5: 几何体渲染

```python
# 创建一个简单的 Part 对象
import Part
box = Part.makeBox(10, 10, 10)

# 显示对象
Part.show(box)

# 切换到 OsgVerse 后端
BackendRegistry.setDefaultBackend("OsgVerse")

# 注意：需要重新创建视图才能看到效果
# 这需要在 Phase 5 中实现视图切换机制
```

### 测试 6: 自动化测试脚本

```bash
# 运行测试脚本
build/bin/FreeCAD.exe -c test_osgversegui_module.py

# 或者在 FreeCAD Python 控制台中
exec(open('test_osgversegui_module.py').read())
```

## 常见问题

### 问题 1: 模块找不到

**症状**：
```python
>>> import OsgVerseGui
ImportError: No module named 'OsgVerseGui'
```

**解决方案**：
1. 检查编译是否成功
2. 检查 `build/bin/OsgVerseGui.pyd` 是否存在
3. 检查 Python 路径：
   ```python
   import sys
   print(sys.path)
   ```

### 问题 2: 后端未注册

**症状**：
```python
>>> BackendRegistry.getAvailableBackends()
['Coin3D']  # 没有 OsgVerse
```

**解决方案**：
1. 检查模块初始化日志：
   ```
   OsgVerseGui: Initializing module
   OsgVerseGui: Backend registered successfully
   ```
2. 如果没有日志，检查模块是否正确加载
3. 检查 `AppOsgVerseGui.cpp` 中的 `initOsgVerseGui()` 函数

### 问题 3: 视图创建失败

**症状**：
```python
>>> viewer = BackendRegistry.createViewer("OsgVerse")
>>> viewer is None
True
```

**解决方案**：
1. 检查错误日志
2. 检查 OSG 库是否正确链接
3. 检查 `OsgVerseViewer` 构造函数

### 问题 4: 几何体不显示

**症状**：
- 视图创建成功
- 但是看不到几何体

**解决方案**：
1. 检查 `createNodeForViewProvider()` 日志
2. 检查 `Part::Feature::getTopoShape()` 是否成功
3. 检查 `GeometryConverter::convertShape()` 是否成功
4. 检查是否显示占位符球体（红色）

### 问题 5: 编译错误

#### 错误 5.1: Part 模块找不到

**症状**：
```
fatal error: Mod/Part/App/PartFeature.h: No such file or directory
```

**解决方案**：
1. 确保 Part 模块已编译
2. 检查包含目录配置
3. 检查 `target_include_directories` 设置

#### 错误 5.2: OSG 库找不到

**症状**：
```
Could not find a package configuration file provided by "osgVerse"
```

**解决方案**：
1. 检查 OSG 安装
2. 设置 `OSG_DIR` 环境变量
3. 检查 `find_package(osgVerse REQUIRED)`

#### 错误 5.3: 链接错误

**症状**：
```
undefined reference to `Part::Feature::getTopoShape(...)`
```

**解决方案**：
1. 确保 Part 模块在链接列表中：
   ```cmake
   set(OsgVerseGui_LIBS
       FreeCADGui
       Part  # ← 必须有这个
       ...
   )
   ```
2. 检查链接顺序

## 调试技巧

### 启用详细日志

在 `OsgVerseViewer.cpp` 中添加更多日志：

```cpp
Base::Console().message("OsgVerseViewer: [DEBUG] ...\n");
```

### 使用调试器

```bash
# Windows (Visual Studio)
# 1. 打开 build/FreeCAD.sln
# 2. 设置断点
# 3. F5 启动调试

# Linux (GDB)
gdb build/bin/FreeCAD
(gdb) break OsgVerseViewer::addViewProvider
(gdb) run
```

### 检查符号导出

```bash
# Windows
dumpbin /EXPORTS build/bin/OsgVerseGui.pyd

# Linux
nm -D build/lib/OsgVerseGui.so | grep PyInit
```

## 性能优化

### 编译优化

```bash
# Release 模式
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target OsgVerseGui
```

### 几何体转换优化

调整 `GeometryConverter::ConversionOptions`：

```cpp
GeometryConverter::ConversionOptions options;
options.deflection = 0.05;  // 更精细（更慢）
options.deflection = 0.2;   // 更粗糙（更快）
```

## 下一步

### Phase 4: 清理旧代码

1. 删除 `src/Gui/View3D/Backends/OsgVerse/` 目录
2. 更新 `src/Gui/View3D/CMakeLists.txt`
3. 移除 FreeCADGui 中的 OsgVerse 相关代码

### Phase 5: 完善功能

1. **Qt 集成**
   - 创建 Qt widget 用于嵌入 OSG viewer
   - 处理鼠标和键盘事件

2. **选择系统**
   - 实现对象选择
   - 高亮显示

3. **导航样式**
   - Trackball
   - Inventor
   - CAD

4. **材质和光照**
   - 完善材质系统
   - 光照设置
   - 阴影支持

### Phase 6: 视图切换

实现运行时切换渲染后端：

```python
# 切换到 OsgVerse
BackendRegistry.setDefaultBackend("OsgVerse")

# 重新创建当前视图
# ... (需要实现)
```

## 参考资料

### 文档
- `Phase6_Step3_OsgVerseGui_Module.md` - 模块设计文档
- `Phase6_Step3_完成报告.md` - 完成报告
- `Phase6_Step1_Interface_And_Coin_Adapter.md` - 接口层文档
- `Phase6_Step2_完成报告.md` - CoinGui 完成报告

### 代码
- `src/Mod/OsgVerseGui/` - OsgVerseGui 模块源代码
- `src/Mod/CoinGui/` - CoinGui 模块（参考）
- `src/Gui/View3D/Interfaces/` - 抽象接口层

### 测试
- `test_osgversegui_module.py` - 自动化测试脚本

## 总结

完成以上步骤后，你应该能够：

1. ✅ 成功编译 OsgVerseGui 模块
2. ✅ 在 FreeCAD 中加载模块
3. ✅ 创建 OsgVerse 视图
4. ✅ 显示真实的 Part 几何体（不是占位符）
5. ✅ 在 Coin3D 和 OsgVerse 之间切换

**关键成就**：
- OsgVerseGui 可以链接 Part 模块
- 直接调用 `Part::Feature::getTopoShape()`
- 使用 GeometryConverter 转换几何体
- 真实几何体渲染

---

**时间**: 2026-01-21
**状态**: 📝 实施指南完成
**下一步**: 开始编译和测试
