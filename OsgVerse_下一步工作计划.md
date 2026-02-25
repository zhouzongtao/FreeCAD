# OsgVerse 下一步工作计划

## 当前状态

✅ **Phase 1 完成：** OsgVerse 后端编译错误修复  
✅ **Phase 2 完成：** 启动失败问题诊断和修复  
✅ **Phase 3 完成：** GraphicsWindow 集成实现  

🔄 **当前阶段：** 准备进入 Phase 4 运行时测试

## Phase 4: 运行时测试和调试

### 4.1 基础启动测试

**目标：** 确认 FreeCAD 可以正常启动，OsgVerse 后端可以加载

**步骤：**

1. **启动 FreeCAD**
   ```cmd
   cd E:\Repository\FreeCAD\FreeCAD\build\bin
   FreeCAD.exe
   ```

2. **检查控制台输出**
   - 查找 "OsgVerseEngine" 相关日志
   - 确认无错误或异常
   - 记录任何警告信息

3. **检查当前后端**
   - 打开 FreeCAD Python 控制台
   - 运行：`python check_current_backend.py`
   - 确认当前使用的后端（预期：Coin3D）

**预期结果：**
- FreeCAD 正常启动
- 无崩溃或错误
- Coin3D 后端正常工作

### 4.2 OsgVerse 后端激活测试

**目标：** 尝试激活 OsgVerse 后端

**方法 1：修改默认后端（推荐）**

编辑 `src/Gui/Render/Core/RenderEngine.h`：

```cpp
// 修改前
BackendType _defaultType{BackendType::Coin3D};

// 修改后
BackendType _defaultType{BackendType::OsgVerse};
```

然后重新编译：
```cmd
cmake --build build --config Release --target FreeCADGui
```

**方法 2：通过 Python API（需要先完成 Phase 5）**

```python
import FreeCADGui
# 假设 Python API 已启用
FreeCADGui.switchRenderBackend("OsgVerse")
```

**预期结果：**
- OsgVerse 后端成功激活
- 3D 视图窗口正常显示
- 无崩溃或错误

### 4.3 基础渲染测试

**目标：** 验证 OsgVerse 后端可以正确渲染 3D 场景

**测试步骤：**

1. **创建简单对象**
   ```python
   import FreeCAD
   import Part
   
   # 创建文档
   doc = FreeCAD.newDocument()
   
   # 创建立方体
   box = doc.addObject("Part::Box", "Box")
   box.Length = 10
   box.Width = 10
   box.Height = 10
   
   # 刷新视图
   doc.recompute()
   FreeCADGui.SendMsgToActiveView("ViewFit")
   ```

2. **检查渲染结果**
   - 立方体是否可见？
   - 颜色是否正确？
   - 边缘是否清晰？

3. **测试多个对象**
   ```python
   # 添加圆柱体
   cylinder = doc.addObject("Part::Cylinder", "Cylinder")
   cylinder.Radius = 5
   cylinder.Height = 15
   cylinder.Placement.Base = FreeCAD.Vector(20, 0, 0)
   
   # 添加球体
   sphere = doc.addObject("Part::Sphere", "Sphere")
   sphere.Radius = 7
   sphere.Placement.Base = FreeCAD.Vector(-20, 0, 0)
   
   doc.recompute()
   ```

**预期结果：**
- 所有对象正确渲染
- 无渲染错误或黑屏
- 性能可接受

### 4.4 相机操作测试

**目标：** 验证相机操纵器工作正常

**测试项目：**

1. **鼠标左键拖动** - 旋转视图
   - 测试各个方向的旋转
   - 检查旋转是否流畅
   - 确认旋转中心正确

2. **鼠标中键拖动** - 平移视图
   - 测试上下左右平移
   - 检查平移是否跟随鼠标

3. **鼠标滚轮** - 缩放视图
   - 测试放大和缩小
   - 检查缩放中心是否正确

4. **鼠标右键** - 上下文菜单
   - 确认菜单正常弹出
   - 测试菜单功能

5. **键盘快捷键**
   - 测试常用快捷键（如 V 键切换视图）
   - 确认键盘输入正常

**预期结果：**
- 所有交互正常工作
- 响应流畅，无延迟
- 与 Coin3D 后端行为一致

### 4.5 性能测试

**目标：** 评估 OsgVerse 后端的性能

**测试方法：**

1. **创建复杂场景**
   ```python
   import FreeCAD
   import Part
   
   doc = FreeCAD.newDocument()
   
   # 创建 100 个对象
   for i in range(10):
       for j in range(10):
           box = doc.addObject("Part::Box", f"Box_{i}_{j}")
           box.Length = 5
           box.Width = 5
           box.Height = 5
           box.Placement.Base = FreeCAD.Vector(i*10, j*10, 0)
   
   doc.recompute()
   ```

2. **测试帧率**
   - 旋转视图，观察流畅度
   - 使用 FPS 计数器（如果可用）
   - 对比 Coin3D 后端的性能

3. **内存使用**
   - 监控内存占用
   - 检查是否有内存泄漏

**预期结果：**
- 帧率 > 30 FPS
- 内存使用合理
- 无明显性能问题

### 4.6 稳定性测试

**目标：** 验证 OsgVerse 后端的稳定性

**测试项目：**

1. **长时间运行**
   - 运行 FreeCAD 30 分钟以上
   - 持续操作（旋转、缩放、创建对象）
   - 检查是否有崩溃或内存泄漏

2. **压力测试**
   - 创建大量对象（1000+）
   - 快速切换视图
   - 频繁刷新场景

3. **异常处理**
   - 测试错误场景（如无效对象）
   - 确认错误处理正确
   - 验证不会崩溃

**预期结果：**
- 无崩溃
- 无内存泄漏
- 错误处理正确

## Phase 5: Python API 集成

### 5.1 添加 Python 绑定到构建系统

**目标：** 使 RenderManagerPy.cpp 可用

**步骤：**

1. **修改 CMakeLists.txt**
   
   编辑 `src/Gui/Core/CMakeLists.txt`，添加：
   ```cmake
   set(FreeCADGui_Core_SRCS
       ...
       RenderManagerPy.cpp
       ...
   )
   ```

2. **注册 Python 模块**
   
   可能需要在 `src/Gui/Application.cpp` 中注册模块。

3. **重新编译**
   ```cmd
   cmake --build build --config Release --target FreeCADGui
   ```

### 5.2 测试 Python API

**测试脚本：**

```python
import FreeCAD
import FreeCADGui

# 测试 1: 获取当前后端
current = FreeCADGui.getCurrentRenderBackend()
print(f"当前后端: {current}")

# 测试 2: 检查后端可用性
coin3d_available = FreeCADGui.isRenderBackendAvailable("Coin3D")
osgverse_available = FreeCADGui.isRenderBackendAvailable("OsgVerse")
print(f"Coin3D 可用: {coin3d_available}")
print(f"OsgVerse 可用: {osgverse_available}")

# 测试 3: 切换到 OsgVerse
if osgverse_available:
    success = FreeCADGui.switchRenderBackend("OsgVerse")
    print(f"切换到 OsgVerse: {'成功' if success else '失败'}")
    
    # 验证切换
    current = FreeCADGui.getCurrentRenderBackend()
    print(f"当前后端: {current}")

# 测试 4: 获取渲染器信息
info = FreeCADGui.getRendererInfo()
print(f"渲染器信息: {info}")

# 测试 5: 获取统计信息
stats = FreeCADGui.getRenderStats()
print(f"渲染统计: {stats}")
```

### 5.3 更新切换脚本

更新 `switch_to_osgverse.py` 使用新的 Python API。

## Phase 6: 文档和发布

### 6.1 用户文档

- 创建用户指南
- 说明如何切换后端
- 列出已知限制

### 6.2 开发者文档

- 更新架构文档
- 添加 API 参考
- 创建贡献指南

### 6.3 示例和教程

- 创建示例脚本
- 编写教程
- 制作演示视频

## 问题排查指南

### 问题 1: FreeCAD 启动失败

**可能原因：**
- 缺少 OSG DLL 文件
- DLL 版本不匹配
- 初始化错误

**排查步骤：**
1. 检查 `build\bin` 目录中的 OSG DLL
2. 运行 `check_dll_dependencies.py`
3. 查看控制台错误信息
4. 检查 OsgVerseEngine 初始化日志

### 问题 2: 黑屏或无渲染

**可能原因：**
- GraphicsWindow 未正确初始化
- OpenGL 上下文问题
- 场景图为空

**排查步骤：**
1. 检查 `initializeGL()` 是否被调用
2. 验证 OpenGL 上下文是否有效
3. 检查场景根节点是否设置
4. 查看 OSG 日志输出

### 问题 3: 事件不响应

**可能原因：**
- 事件队列未正确设置
- 事件转换错误
- 相机操纵器未设置

**排查步骤：**
1. 检查 `getEventQueue()` 是否返回有效队列
2. 验证事件转换代码
3. 确认相机操纵器已设置
4. 测试简单的事件（如键盘）

### 问题 4: 性能问题

**可能原因：**
- 渲染管线配置不当
- 场景图结构问题
- OpenGL 状态管理

**排查步骤：**
1. 使用 OSG 性能分析工具
2. 检查渲染统计信息
3. 优化场景图结构
4. 调整渲染设置

## 成功标准

Phase 4 成功标准：
- ✅ FreeCAD 正常启动
- ✅ OsgVerse 后端可以激活
- ✅ 基础 3D 对象正确渲染
- ✅ 相机操作正常工作
- ✅ 性能可接受（> 30 FPS）
- ✅ 无崩溃或严重错误

Phase 5 成功标准：
- ✅ Python API 可用
- ✅ 后端切换功能正常
- ✅ 所有 Python 函数工作正常

## 时间估计

- **Phase 4:** 2-4 小时（测试和调试）
- **Phase 5:** 1-2 小时（Python API 集成）
- **Phase 6:** 2-3 小时（文档编写）

**总计：** 5-9 小时

## 联系和支持

如果遇到问题：
1. 检查控制台日志
2. 查看本文档的问题排查指南
3. 参考已创建的诊断脚本
4. 查看 OsgVerse 和 OSG 官方文档

---

**准备就绪！** 可以开始 Phase 4 运行时测试了。
