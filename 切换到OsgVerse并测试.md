# 切换到 OsgVerse 并测试

## 📋 当前状态

- ✅ FreeCADGui.dll 已编译（包含 OsgVerse 修复）
- ✅ FreeCAD 可以正常启动（使用 Coin3D）
- ✅ Python 绑定已添加到 CMakeLists.txt
- 🔄 准备切换到 OsgVerse 进行测试

## 🎯 测试计划

### 步骤 1: 切换默认后端到 OsgVerse

修改 `src/Gui/Render/Core/RenderEngine.h`（约第 360 行）：

```cpp
// 修改前
BackendType _defaultType{BackendType::Coin3D};

// 修改后
BackendType _defaultType{BackendType::OsgVerse};
```

### 步骤 2: 重新编译

```cmd
cmake --build build --config Release --target FreeCADGui -- /maxcpucount:1
cmake --build build --config Release --target FreeCADMain -- /maxcpucount:1
```

### 步骤 3: 测试启动

```cmd
cd build\bin
FreeCAD.exe --console
```

## 🔍 预期结果

### 如果修复成功 ✅

**启动时的日志**:
```
OsgVerseEngine: Constructor called
(不会立即初始化 - 延迟初始化)
```

**打开 3D 视图时**:
```
OsgVerseViewer: Constructor called (lazy initialization mode)
OsgVerseViewer: Starting lazy initialization...
OsgVerseEngine::initialize: Starting initialization...
OsgVerseEngine::initialize: OSG Registry initialized
OsgVerseEngine::initialize: Scene root created
OsgVerseEngine::initialize: Rendering pipeline initialized
OsgVerseEngine::initialize: Initialization complete
```

**行为**:
- ✅ FreeCAD 正常启动
- ✅ 无崩溃
- ✅ 3D 视图可以打开
- ✅ 渲染正常

### 如果仍然失败 ❌

**可能的原因**:
1. 还有其他 OSG 函数在静态初始化时被调用
2. OsgVerseViewer 的延迟初始化有问题
3. OSG DLL 依赖问题

**调试步骤**:
1. 查看详细错误日志
2. 使用调试器定位崩溃点
3. 检查是否有其他 OSG 函数调用

## 💡 关键修复回顾

### 问题根源
```cpp
// 问题代码（已修复）
std::string OsgVerseEngine::getVersion() const
{
    return osgGetVersion();  // ❌ 触发 OSG 初始化
}
```

### 修复方案
```cpp
// 修复后
std::string OsgVerseEngine::getVersion() const
{
    return "3.6.5";  // ✅ 硬编码版本号
}
```

### 为什么有效
1. **避免过早调用 OSG 函数**
   - `osgGetVersion()` 会触发 OSG 库初始化
   - 在静态注册阶段调用会失败

2. **延迟真正的初始化**
   - OSG 的真正初始化在 `initialize()` 方法中
   - 只在需要时才调用（打开 3D 视图时）
   - 此时 Qt 和 OpenGL 都已就绪

## 📊 测试检查清单

### 基础测试
- [ ] FreeCAD 启动成功
- [ ] 无崩溃或错误对话框
- [ ] 控制台无错误信息

### OsgVerse 功能测试
- [ ] 3D 视图可以打开
- [ ] 可以创建简单几何体
- [ ] 鼠标交互正常
- [ ] 渲染正常

### 延迟初始化验证
- [ ] 启动时不立即初始化
- [ ] 打开 3D 视图时才初始化
- [ ] 日志顺序正确

## 🚀 开始测试

准备好了吗？让我们开始测试！

**选项 1: 我来帮你修改和编译**
- 我会修改 RenderEngine.h
- 然后重新编译
- 你只需要测试启动

**选项 2: 你自己修改**
- 手动修改 RenderEngine.h
- 运行编译命令
- 测试启动

请告诉我你想选择哪个选项！
