# 请关闭 FreeCAD 后重新编译

## 当前状态
编译失败，因为 FreeCADGui.dll 被锁定（FreeCAD 正在运行）。

## 操作步骤

1. **关闭所有 FreeCAD 实例**
   - 关闭 FreeCAD GUI
   - 确保没有 FreeCADCmd.exe 进程在运行
   - 可以在任务管理器中检查

2. **重新编译**
   ```cmd
   cmake --build build --config Release --target FreeCADGui -- /maxcpucount:1
   ```

3. **启动 FreeCAD GUI 测试**
   ```cmd
   E:\Repository\FreeCAD\FreeCAD\build\bin\FreeCAD.exe
   ```

4. **在 Python 控制台中测试**
   ```python
   import FreeCADGui
   print(dir(FreeCADGui))
   print(hasattr(FreeCADGui, 'getCurrentRenderBackend'))
   ```

5. **检查 Report View**
   应该看到以下日志：
   - "Application: Initializing RenderManager..."
   - "RenderManager::initialize: Initializing render manager"
   - "RenderManager::initialize: Registering OsgVerse engine..."
   - "RenderManager::initialize: Initializing Python bindings..."
   - "initRenderManagerPy: START"
   - "initRenderManagerPy: Adding function 'getCurrentRenderBackend'..."

## 关键修改

### 修改 1: 延迟 Python 绑定初始化
将 `initRenderManagerPy()` 的调用从 `Application` 构造函数移到了 `RenderManager::initialize()` 中。

**原因**: 
- Application 构造函数在 Python 模块初始化时被调用
- 那时 FreeCADGui 模块可能还没有完全设置好
- 延迟到 RenderManager 初始化时，GUI 已经完全启动

### 修改 2: 使用字典直接添加
使用 `PyDict_SetItemString()` 直接添加函数到模块字典，而不是 `PyModule_AddObject()`。

**原因**:
- 更可靠的方法
- 不依赖模块的可变性

## 预期结果

如果成功，您应该能够：
1. 在 Python 控制台中看到 RenderManager 函数
2. 调用 `FreeCADGui.getCurrentRenderBackend()` 获取当前后端
3. 调用 `FreeCADGui.switchRenderBackend(2)` 切换到 OsgVerse
4. 看到 Report View 中的详细日志

## 如果仍然失败

如果 Python 绑定仍然没有被添加，我们需要采用更根本的方法：
- 将 RenderManager 方法直接添加到 ApplicationPy::Methods 数组
- 在模块创建时就包含所有方法
- 这需要修改 ApplicationPy.cpp

---

**下一步**: 请关闭 FreeCAD 并重新编译
