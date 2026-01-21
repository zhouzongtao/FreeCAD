# Phase 6: 简化实现方案

## 🎯 目标

实现一个**最小可行版本**，让 OsgVerse 能够真正渲染 3D 场景，而不是完整的功能对等。

## 📊 简化策略

### 不做的事情（暂时）
1. ❌ 运行时切换渲染后端
2. ❌ 完整的场景图转换
3. ❌ 所有 Coin3D 功能的对等实现
4. ❌ 复杂的事件处理

### 要做的事情（MVP）
1. ✅ 在启动时选择渲染后端
2. ✅ 基本的几何体渲染
3. ✅ 基本的相机控制（旋转、缩放、平移）
4. ✅ 简单的场景图支持

## 🚀 实现方案

### 方案：在 View3DInventorViewer 中添加后端选择

**核心思路**：
- View3DInventorViewer 在构造时检查 RenderManager 的当前后端
- 根据后端类型初始化不同的渲染器
- 保持现有 API 不变

**优点**：
- 最小化代码修改
- 不破坏现有功能
- 可以逐步完善

**实现步骤**：

### 步骤 1: 修改 View3DInventorViewer 构造函数

```cpp
View3DInventorViewer::View3DInventorViewer(...)
{
    // 检查当前渲染后端
    auto& renderMgr = Gui::Core::RenderManager::instance();
    auto backendType = renderMgr.getCurrentBackend();
    
    if (backendType == Render::BackendType::OsgVerse) {
        // 使用 OsgVerse 渲染
        initializeOsgVerse();
    } else {
        // 使用 Coin3D 渲染（默认）
        initializeCoin3D();
    }
}
```

### 步骤 2: 添加 OsgVerse 初始化

```cpp
void View3DInventorViewer::initializeOsgVerse()
{
    // 创建 OSG viewer
    _osgViewer = new osgViewer::Viewer();
    
    // 设置 graphics context
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> gw = 
        new osgViewer::GraphicsWindowEmbedded(0, 0, width(), height());
    
    _osgViewer->getCamera()->setGraphicsContext(gw);
    _osgViewer->getCamera()->setViewport(0, 0, width(), height());
    
    // 创建场景
    _osgSceneRoot = new osg::Group();
    _osgViewer->setSceneData(_osgSceneRoot);
    
    _useOsgVerse = true;
}
```

### 步骤 3: 修改 paintGL

```cpp
void View3DInventorViewer::paintGL()
{
    if (_useOsgVerse) {
        // OsgVerse 渲染
        if (_osgViewer) {
            _osgViewer->frame();
        }
    } else {
        // Coin3D 渲染（现有代码）
        getSoRenderManager()->render();
    }
}
```

### 步骤 4: 添加简单的场景图同步

```cpp
void View3DInventorViewer::setSceneGraph(SoNode* root)
{
    if (_useOsgVerse) {
        // 转换 Coin3D 场景图到 OSG
        // 简化版：只支持基本几何体
        convertAndSetOsgScene(root);
    } else {
        // Coin3D（现有代码）
        inherited::setSceneGraph(root);
    }
}
```

## 📝 具体实现

### 文件修改清单

1. **src/Gui/View3DInventorViewer.h**
   - 添加 OsgVerse 成员变量
   - 添加初始化方法声明

2. **src/Gui/View3DInventorViewer.cpp**
   - 修改构造函数
   - 添加 OsgVerse 初始化
   - 修改 paintGL
   - 添加场景图转换

3. **src/Gui/CMakeLists.txt**
   - 添加 OSG 库链接

## ⚠️ 限制和已知问题

### 当前限制
1. **不支持运行时切换** - 必须重启 FreeCAD
2. **有限的场景图支持** - 只支持基本几何体
3. **简化的事件处理** - 可能有些交互不工作
4. **性能未优化** - 场景图转换可能较慢

### 已知问题
1. 复杂场景可能无法正确显示
2. 某些 Coin3D 特性不可用
3. 可能有渲染错误

## 🎯 成功标准（MVP）

- [ ] FreeCAD 启动时可以选择 OsgVerse 后端
- [ ] 可以显示简单的立方体
- [ ] 可以旋转视图
- [ ] 可以缩放视图
- [ ] 可以平移视图
- [ ] 没有崩溃

## 📊 工作量估算（简化版）

- **代码行数**: ~500 行
- **修改文件**: 2-3 个
- **预计时间**: 1-2 天

## 🚀 立即开始

让我们从最简单的开始：

1. **现在**: 修改 View3DInventorViewer.h 添加成员变量
2. **接下来**: 修改构造函数添加后端检查
3. **然后**: 实现基本的 OsgVerse 渲染
4. **最后**: 测试基本功能

---

**当前阶段**: Phase 6 - 简化实现  
**方法**: 最小可行产品 (MVP)  
**目标**: 让 OsgVerse 能够渲染简单场景
