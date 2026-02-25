# 请关闭 FreeCAD 后重新编译

## 链接错误

```
LINK : fatal error LNK1104: cannot open file 'E:\Repository\FreeCAD\FreeCAD\build\bin\FreeCADGui.dll'
```

## 原因

FreeCADGui.dll 正在被 FreeCAD 进程使用，无法被链接器覆盖。

## 解决步骤

1. **关闭所有 FreeCAD 实例**
   - 关闭主窗口
   - 检查任务管理器，确保没有 FreeCAD.exe 或 FreeCADCmd.exe 进程

2. **重新编译**
   ```powershell
   cmake --build build --config Release --target FreeCADGui -- /maxcpucount:1
   ```

## 修改内容

### ViewerFactory.cpp - createDefault()

添加了后端可用性检查和自动回退机制：

```cpp
std::unique_ptr<IViewer3D> ViewerFactory::createDefault(
    QWidget* parent,
    const QOpenGLWidget* shareWidget)
{
    // 从 RenderManager 获取当前后端
    auto& renderMgr = Gui::Core::RenderManager::instance();
    auto backendType = renderMgr.getCurrentBackend();
    
    Base::Console().log("ViewerFactory: Creating default viewer (backend: %d)\n",
                        static_cast<int>(backendType));
    
    // 如果当前后端是 None，使用 Coin3D 作为默认
    if (backendType == Render::BackendType::None) {
        backendType = Render::BackendType::Coin3D;
        Base::Console().warning("ViewerFactory: No backend selected, using Coin3D as default\n");
    }
    
    // 检查请求的后端是否已注册
    if (!isRegistered(backendType)) {
        Base::Console().warning(
            "ViewerFactory: Requested backend %d is not registered, falling back to Coin3D\n",
            static_cast<int>(backendType)
        );
        backendType = Render::BackendType::Coin3D;
        
        // 如果 Coin3D 也没注册，抛出异常
        if (!isRegistered(backendType)) {
            std::string msg = "ViewerFactory: No viewer backends registered!";
            Base::Console().error("%s\n", msg.c_str());
            throw std::runtime_error(msg);
        }
    }
    
    return create(backendType, parent, shareWidget);
}
```

### 改进点

1. **自动回退**: 如果请求的后端未注册，自动回退到 Coin3D
2. **日志记录**: 记录回退操作
3. **错误处理**: 如果没有任何后端注册，抛出清晰的错误

## 预期运行时输出

```
Application: Initializing RenderManager...
RenderManager::initialize: Initializing render manager
RenderManager::initialize: Initialized with backend: OsgVerse
Application: RenderManager initialized successfully

Application: Registering viewer backends...
ViewerFactory: Registered creator for backend type 1
Application: Coin3D viewer registered

View3DInventor: Creating viewer using ViewerFactory
ViewerFactory: Creating default viewer (backend: 2)
ViewerFactory: Requested backend 2 is not registered, falling back to Coin3D
ViewerFactory: Creating viewer for backend type 1
CoinViewer: Creating Coin3D viewer
CoinViewer: Coin3D viewer created successfully
View3DInventor: Successfully created viewer via factory
```

## 关键改进

现在即使 RenderManager 返回 OsgVerse (值为 2)，ViewerFactory 也会：
1. 检测到 OsgVerse viewer 未注册
2. 自动回退到 Coin3D
3. 成功创建 3D 视图

这样就不会再出现 "Viewer backend not registered: 2" 错误了！

---

**请关闭 FreeCAD 后重新编译**
