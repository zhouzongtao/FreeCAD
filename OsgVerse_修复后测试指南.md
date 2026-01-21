# OsgVerse 修复后测试指南

## ✅ 编译完成

**时间：** 2026-01-19 23:03:49

**状态：**
- ✅ FreeCADGui.dll 已重新编译
- ✅ 包含 OsgVerse 修复（硬编码版本号）
- ✅ 当前默认后端：Coin3D

## 🧪 测试方案

### 方案 1：先测试 Coin3D（推荐）

**目的：** 确保修复没有破坏 Coin3D 后端

**步骤：**
```cmd
cd E:\Repository\FreeCAD\FreeCAD\build\bin
FreeCAD.exe
```

**预期结果：**
- ✅ FreeCAD 正常启动
- ✅ 所有功能正常
- ✅ 无错误

### 方案 2：测试 OsgVerse

**目的：** 验证修复是否解决了 OsgVerse 的启动问题

#### 步骤 1：切换到 OsgVerse

修改 `src/Gui/Render/Core/RenderEngine.h`（约第 360 行）：

```cpp
// 修改前
BackendType _defaultType{BackendType::Coin3D};

// 修改后
BackendType _defaultType{BackendType::OsgVerse};
```

#### 步骤 2：重新编译

```cmd
cmake --build build --config Release --target FreeCADGui -- /maxcpucount:1
cmake --build build --config Release --target FreeCADMain -- /maxcpucount:1
```

#### 步骤 3：测试启动

```cmd
cd E:\Repository\FreeCAD\FreeCAD\build\bin
FreeCAD.exe
```

#### 预期结果

**如果修复成功：**
- ✅ FreeCAD 正常启动
- ✅ 控制台显示 OsgVerse 相关日志
- ✅ 3D 视图正常（当打开时）
- ✅ 无崩溃

**控制台日志示例：**
```
OsgVerseEngine: Constructor called
(启动时不会立即初始化)
...
(当打开 3D 视图时)
OsgVerseViewer: Constructor called (lazy initialization mode)
OsgVerseViewer: Starting lazy initialization...
OsgVerseEngine::initialize: Starting initialization...
OsgVerseEngine::initialize: Initialization complete
```

**如果仍然失败：**
- 记录错误信息
- 查看控制台日志
- 告诉我具体情况

## 🔍 关键修复点

### 修复内容

**文件：** `src/Gui/Render/Backends/OsgVerse/OsgVerseEngine.cpp`

**修改：**
```cpp
// 修改前
std::string OsgVerseEngine::getVersion() const
{
    return osgGetVersion();  // ❌ 触发 OSG 初始化
}

// 修改后
std::string OsgVerseEngine::getVersion() const
{
    return "3.6.5";  // ✅ 硬编码版本号
}
```

### 为什么这样修复

1. **避免过早调用 OSG 函数**
   - `osgGetVersion()` 会触发 OSG 库初始化
   - 在程序启动早期调用会失败

2. **延迟真正的初始化**
   - OSG 的真正初始化在 `initialize()` 方法中
   - 只在需要时才调用
   - 此时所有依赖都已就绪

## 📊 测试检查清单

### Coin3D 测试（当前状态）
- [ ] FreeCAD 正常启动
- [ ] 可以创建新文档
- [ ] 3D 视图正常显示
- [ ] 鼠标交互正常

### OsgVerse 测试（需要切换）
- [ ] FreeCAD 正常启动（关键！）
- [ ] 控制台显示 OsgVerse 日志
- [ ] 3D 视图可以打开
- [ ] 延迟初始化日志正确
- [ ] 无崩溃或错误

## 💡 调试提示

### 如果 Coin3D 失败
说明修复破坏了某些东西，需要回退。

### 如果 OsgVerse 仍然失败

**可能的原因：**
1. 还有其他 OSG 函数被过早调用
2. 静态初始化的其他问题
3. DLL 依赖问题

**调试步骤：**
1. 查看详细的错误日志
2. 使用调试器查看崩溃点
3. 检查是否有其他 OSG 函数调用

## 🎯 我的建议

### 立即测试

**先测试 Coin3D：**
```cmd
cd build\bin
FreeCAD.exe
```

如果 Coin3D 正常，再考虑测试 OsgVerse。

### 如果想测试 OsgVerse

告诉我，我可以帮您：
1. 修改 RenderEngine.h
2. 重新编译
3. 准备测试

---

**准备测试！请告诉我结果！** 🚀

**当前状态：**
- ✅ FreeCADGui.dll 已编译
- ✅ 包含 OsgVerse 修复
- 🔄 等待测试
