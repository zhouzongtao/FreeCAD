# Phase 4 测试指南

## 当前状态

✅ **编译完成** - OsgVerse 后端已设置为默认后端并重新编译  
🔄 **准备测试** - 需要手动启动 FreeCAD 进行测试

## 测试准备

### 1. 确认编译成功

已完成：
- ✅ 修改 `src/Gui/Render/Core/RenderEngine.h` 中的默认后端为 OsgVerse
- ✅ 重新编译 FreeCADGui.dll
- ✅ 编译成功，无错误

### 2. 测试文件准备

已创建以下测试文件：
- `TestOsgVerseBackend.FCMacro` - FreeCAD 宏文件（推荐使用）
- `test_osgverse_active.py` - Python 测试脚本
- `test_freecad_startup.cmd` - 批处理启动脚本

## 测试步骤

### 方法 1：使用 FreeCAD 宏（推荐）

这是最简单和最可靠的测试方法。

**步骤：**

1. **启动 FreeCAD**
   ```cmd
   cd E:\Repository\FreeCAD\FreeCAD\build\bin
   FreeCAD.exe
   ```

2. **观察启动过程**
   - 查看控制台输出
   - 查找 "OsgVerseEngine" 相关日志
   - 确认无错误或崩溃

3. **运行测试宏**
   - 打开菜单：Macro → Macros...
   - 点击 "User macros" 标签
   - 浏览到项目根目录
   - 选择 `TestOsgVerseBackend.FCMacro`
   - 点击 "Execute" 运行

4. **观察测试结果**
   - 查看 Python 控制台输出
   - 检查是否创建了三个 3D 对象
   - 验证对象是否正确渲染

### 方法 2：使用 Python 控制台

如果宏方法不可用，可以直接在 Python 控制台中运行测试代码。

**步骤：**

1. **启动 FreeCAD**
   ```cmd
   cd E:\Repository\FreeCAD\FreeCAD\build\bin
   FreeCAD.exe
   ```

2. **打开 Python 控制台**
   - 菜单：View → Panels → Python console

3. **复制并运行测试代码**
   
   打开 `TestOsgVerseBackend.FCMacro` 文件，复制所有代码到 Python 控制台。

### 方法 3：手动测试

如果自动化测试不可用，可以手动创建对象进行测试。

**步骤：**

1. **启动 FreeCAD**

2. **创建新文档**
   - File → New

3. **创建测试对象**
   - 切换到 Part 工作台
   - 创建立方体：Part → Primitives → Cube
   - 创建圆柱体：Part → Primitives → Cylinder
   - 创建球体：Part → Primitives → Sphere

4. **测试交互**
   - 用鼠标旋转视图
   - 用滚轮缩放
   - 用中键平移

## 测试检查清单

### 4.1 基础启动测试

- [ ] FreeCAD 正常启动，无崩溃
- [ ] 控制台显示 OsgVerse 相关日志
- [ ] 无严重错误或警告
- [ ] 3D 视图窗口正常显示

**预期日志：**
```
OsgVerseEngine: Constructor called
OsgVerseEngine::initialize: Initializing...
OsgVerseGraphicsWindow: Created
```

### 4.2 渲染测试

- [ ] 3D 对象正确显示
- [ ] 对象边缘清晰
- [ ] 颜色正确
- [ ] 无黑屏或闪烁
- [ ] 视图刷新正常

### 4.3 交互测试

**鼠标操作：**
- [ ] 左键拖动 - 旋转视图
- [ ] 中键拖动 - 平移视图
- [ ] 滚轮 - 缩放视图
- [ ] 右键 - 上下文菜单

**键盘操作：**
- [ ] V 键 - 切换视图模式
- [ ] 数字键 - 标准视图
- [ ] 其他快捷键正常

### 4.4 性能测试

- [ ] 旋转流畅（目测 > 30 FPS）
- [ ] 无明显卡顿
- [ ] 响应及时
- [ ] 内存使用正常

### 4.5 稳定性测试

- [ ] 持续操作 5 分钟无崩溃
- [ ] 创建多个对象无问题
- [ ] 切换视图无问题
- [ ] 关闭文档无问题

## 问题排查

### 问题 1: FreeCAD 启动失败

**症状：** FreeCAD.exe 启动后立即崩溃或无响应

**可能原因：**
- OsgVerse 初始化失败
- 缺少 OSG DLL 文件
- OpenGL 驱动问题

**排查步骤：**
1. 检查控制台错误信息
2. 运行 `check_dll_dependencies.py` 检查 DLL
3. 查看 Windows 事件查看器
4. 尝试切换回 Coin3D 后端

### 问题 2: 黑屏或无渲染

**症状：** FreeCAD 启动正常，但 3D 视图是黑屏

**可能原因：**
- GraphicsWindow 初始化失败
- OpenGL 上下文问题
- 场景图未正确设置

**排查步骤：**
1. 检查控制台是否有 "OsgVerseGraphicsWindow" 日志
2. 查看是否有 OpenGL 错误
3. 尝试创建简单对象
4. 检查 OpenGL 版本是否支持

### 问题 3: 鼠标事件不响应

**症状：** 无法用鼠标旋转或缩放视图

**可能原因：**
- 事件队列未正确设置
- 事件转换错误
- 相机操纵器未设置

**排查步骤：**
1. 检查是否有事件相关的错误日志
2. 尝试键盘快捷键是否工作
3. 检查 ViewerWidget 是否正确初始化

### 问题 4: 性能问题

**症状：** 旋转卡顿，帧率低

**可能原因：**
- 渲染管线配置不当
- 场景图结构问题
- OpenGL 状态管理问题

**排查步骤：**
1. 检查是否启用了垂直同步
2. 查看 GPU 使用率
3. 简化场景测试
4. 对比 Coin3D 后端性能

## 测试结果记录

### 测试环境

- **操作系统：** Windows
- **FreeCAD 版本：** [待填写]
- **OpenGL 版本：** [待填写]
- **显卡：** [待填写]
- **驱动版本：** [待填写]

### 测试结果

#### 4.1 基础启动测试
- **状态：** [ ] 通过 / [ ] 失败
- **备注：**

#### 4.2 渲染测试
- **状态：** [ ] 通过 / [ ] 失败
- **备注：**

#### 4.3 交互测试
- **状态：** [ ] 通过 / [ ] 失败
- **备注：**

#### 4.4 性能测试
- **状态：** [ ] 通过 / [ ] 失败
- **帧率：** [待填写] FPS
- **备注：**

#### 4.5 稳定性测试
- **状态：** [ ] 通过 / [ ] 失败
- **备注：**

### 控制台日志

```
[粘贴关键日志]
```

### 截图

[如果可能，添加截图]

## 下一步

### 如果测试成功

恭喜！OsgVerse 后端工作正常。下一步：

1. **Phase 5：** 集成 Python API
   - 添加 RenderManagerPy.cpp 到构建系统
   - 测试后端切换功能
   - 更新文档

2. **优化和完善：**
   - 性能优化
   - 添加更多渲染特性
   - 完善错误处理

### 如果测试失败

不要担心，这是正常的开发过程。请：

1. **记录详细信息：**
   - 错误消息
   - 控制台日志
   - 复现步骤

2. **尝试回退：**
   - 切换回 Coin3D 后端
   - 验证 Coin3D 是否正常

3. **调试：**
   - 使用 Visual Studio 调试器
   - 添加更多日志输出
   - 逐步排查问题

## 联系和支持

如果遇到问题：
1. 查看本文档的问题排查部分
2. 检查控制台日志
3. 参考已创建的诊断脚本
4. 查看 OsgVerse 和 OSG 官方文档

---

**准备就绪！** 现在可以启动 FreeCAD 进行测试了。

**启动命令：**
```cmd
cd E:\Repository\FreeCAD\FreeCAD\build\bin
FreeCAD.exe
```

祝测试顺利！🚀
