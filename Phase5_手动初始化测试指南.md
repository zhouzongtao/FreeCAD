# Phase 5: 手动初始化测试指南

## 🎯 目标

测试新添加的 `initializeRenderManager()` 函数，手动初始化 RenderManager 以启用 OsgVerse 后端。

## ✅ 已完成的工作

1. **添加了 `initializeRenderManager()` Python 函数**
   - 位置：`src/Gui/Core/RenderManagerPy.cpp`
   - 功能：手动初始化 RenderManager，注册所有可用的渲染后端（包括 OsgVerse）
   - 返回值：`True` 表示成功，`False` 表示失败

2. **重新编译了 FreeCADGui**
   - 编译成功，没有错误
   - `initializeRenderManager()` 函数已包含在 FreeCADGui.dll 中

## 🧪 测试方法

### 方法 1: 使用自动化测试脚本（推荐）

1. **运行测试脚本**：
   ```cmd
   test_manual_init.cmd
   ```

2. **查看输出**：
   - 脚本会自动测试所有功能
   - 检查 OsgVerse 是否在初始化后可用
   - 如果可用，尝试切换到 OsgVerse

### 方法 2: 在 FreeCAD GUI 中手动测试

1. **启动 FreeCAD**：
   ```cmd
   E:\Repository\FreeCAD\FreeCAD\build\bin\FreeCAD.exe
   ```

2. **打开 Python 控制台**（View → Panels → Python console）

3. **运行以下命令**：
   ```python
   import FreeCADGui
   
   # 检查函数是否存在
   print("Has initializeRenderManager:", hasattr(FreeCADGui, 'initializeRenderManager'))
   
   # 检查 OsgVerse 初始化前的状态
   print("OsgVerse available (before):", FreeCADGui.isRenderBackendAvailable(2))
   
   # 手动初始化 RenderManager
   result = FreeCADGui.initializeRenderManager()
   print("Initialization result:", result)
   
   # 检查 OsgVerse 初始化后的状态
   print("OsgVerse available (after):", FreeCADGui.isRenderBackendAvailable(2))
   
   # 如果可用，切换到 OsgVerse
   if FreeCADGui.isRenderBackendAvailable(2):
       print("Switching to OsgVerse...")
       switch_result = FreeCADGui.switchRenderBackend(2)
       print("Switch result:", switch_result)
       print("Current backend:", FreeCADGui.getCurrentRenderBackend())
       print("Renderer info:", FreeCADGui.getRendererInfo())
   ```

### 方法 3: 使用 FreeCAD 宏

1. **创建宏文件** `InitOsgVerse.FCMacro`：
   ```python
   import FreeCADGui
   
   # 初始化 RenderManager
   if FreeCADGui.initializeRenderManager():
       print("RenderManager initialized successfully")
       
       # 检查 OsgVerse 是否可用
       if FreeCADGui.isRenderBackendAvailable(2):
           print("OsgVerse is available!")
           
           # 切换到 OsgVerse
           if FreeCADGui.switchRenderBackend(2):
               print("Successfully switched to OsgVerse")
               print("Renderer:", FreeCADGui.getRendererInfo())
           else:
               print("Failed to switch to OsgVerse")
       else:
           print("OsgVerse is not available")
   else:
       print("Failed to initialize RenderManager")
   ```

2. **在 FreeCAD 中运行宏**（Macro → Macros → Run）

## 📊 预期结果

### 成功的情况

```
Has initializeRenderManager: True
OsgVerse available (before): False
Initialization result: True
OsgVerse available (after): True
Switching to OsgVerse...
Switch result: True
Current backend: 2
Renderer info: OsgVerse OSG 3.6.5
```

### 失败的情况

如果 `OsgVerse available (after)` 仍然是 `False`，可能的原因：

1. **OsgVerse DLL 未找到**
   - 检查 OSG DLL 是否在 `build/bin` 目录中
   - 运行 `copy_osg_runtime.ps1` 复制 DLL

2. **OsgVerse 引擎注册失败**
   - 检查 `BUILD_WITH_OSGVERSE` 是否在编译时启用
   - 查看 Report View 中的错误消息

3. **初始化异常**
   - 查看 Report View 中的详细错误信息
   - 检查 `registerOsgVerseEngine()` 是否被调用

## 🔍 调试信息

### 检查编译选项

```python
import FreeCAD
print("Build info:", FreeCAD.ConfigGet("BuildVersionMajor"))
```

### 检查 DLL 依赖

运行 `check_dll_dependencies.py` 检查 FreeCADGui.dll 的依赖项。

### 查看日志

在 FreeCAD 中：
1. 打开 Report View（View → Panels → Report view）
2. 设置日志级别为 "Log"
3. 运行初始化命令
4. 查看详细的日志输出

## 📝 测试检查清单

- [ ] `initializeRenderManager()` 函数存在
- [ ] 调用 `initializeRenderManager()` 返回 `True`
- [ ] 初始化后 `isRenderBackendAvailable(2)` 返回 `True`
- [ ] 可以成功切换到 OsgVerse（`switchRenderBackend(2)` 返回 `True`）
- [ ] 切换后 `getCurrentRenderBackend()` 返回 `2`
- [ ] `getRendererInfo()` 显示 OsgVerse 信息
- [ ] 没有崩溃或异常

## 🎯 下一步

### 如果测试成功

1. **创建自动初始化机制**
   - 在 Application 构造函数中自动调用 `initializeRenderManager()`
   - 或在 FreeCAD 启动脚本中调用

2. **添加用户界面**
   - 在 Preferences 中添加后端选择选项
   - 添加工具栏按钮切换后端

3. **测试 OsgVerse 功能**
   - 创建 3D 对象
   - 测试渲染性能
   - 验证所有功能正常工作

### 如果测试失败

1. **收集诊断信息**
   - Report View 中的错误消息
   - DLL 依赖检查结果
   - 编译配置信息

2. **检查 OsgVerse 引擎**
   - 验证 `registerOsgVerseEngine()` 函数
   - 检查 OsgVerseEngine 的实现
   - 确认 OSG 库版本兼容性

3. **调试初始化流程**
   - 在 `RenderManager::initialize()` 中添加更多日志
   - 检查异常处理
   - 验证引擎注册逻辑

## 💡 提示

1. **首次运行**：建议使用自动化测试脚本（`test_manual_init.cmd`），它会提供详细的输出。

2. **日常使用**：如果测试成功，可以将初始化命令添加到 FreeCAD 的启动脚本中：
   ```python
   # ~/.FreeCAD/Macro/start.py
   import FreeCADGui
   FreeCADGui.initializeRenderManager()
   ```

3. **性能**：`initializeRenderManager()` 只需要调用一次，之后 OsgVerse 就会一直可用。

## 📞 支持

如果遇到问题，请提供：
1. 测试脚本的完整输出
2. Report View 中的错误消息
3. FreeCAD 版本和编译配置
4. OSG 库版本

---

**状态**: 等待测试
**优先级**: 高
**预计时间**: 5-10 分钟
