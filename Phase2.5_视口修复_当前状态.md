# Phase 2.5: 视口修复 - 当前状态

## 📋 修复历史

### 问题描述
- 渲染区域没有覆盖整个窗口
- 有黑色区域（未渲染区域）
- 绿色球体的一部分被裁剪

### 已尝试的方案

#### ✅ 方案 1：强制刷新 GraphicsWindow
- **状态**：已实现（在之前的版本中）
- **效果**：部分改善，但问题仍然存在

#### ✅ 方案 2：devicePixelRatio 修复
- **状态**：刚刚实现并编译
- **修改文件**：`src/Mod/OsgVerseGui/OsgVerseWidget.cpp`
- **修改内容**：
  - 在 `initializeGL()` 中添加 DPR 支持
  - 在 `resizeGL()` 中添加 DPR 支持
  - 使用 `width * devicePixelRatio()` 计算实际像素大小
- **效果**：待测试

#### ⏳ 方案 3：Framebuffer 绑定
- **状态**：准备就绪，如果方案 2 不够则实施
- **修改位置**：`paintGL()` 方法
- **原理**：确保 OSG 渲染到 Qt 的正确 framebuffer

## 🔧 当前实现

### OsgVerseWidget.cpp 的关键代码

```cpp
void OsgVerseWidget::initializeGL()
{
    // ... 其他代码 ...
    
    // 获取实际像素大小（考虑高 DPI）
    qreal dpr = devicePixelRatio();
    int pixelWidth = width() * dpr;
    int pixelHeight = height() * dpr;
    
    Base::Console().warning("OsgVerseWidget: Device pixel ratio: %.2f, pixel size: %d x %d\n",
                           dpr, pixelWidth, pixelHeight);
    
    // 更新视口
    if (_graphicsWindow.valid()) {
        _graphicsWindow->getEventQueue()->windowResize(0, 0, pixelWidth, pixelHeight);
        _graphicsWindow->resized(0, 0, pixelWidth, pixelHeight);
    }
    
    if (_viewer.valid()) {
        osg::Camera* camera = _viewer->getCamera();
        camera->setViewport(0, 0, pixelWidth, pixelHeight);
        
        double aspectRatio = static_cast<double>(pixelWidth) / static_cast<double>(pixelHeight);
        camera->setProjectionMatrixAsPerspective(30.0, aspectRatio, 1.0, 1000.0);
    }
}

void OsgVerseWidget::resizeGL(int width, int height)
{
    // 获取实际像素大小（考虑高 DPI）
    qreal dpr = devicePixelRatio();
    int pixelWidth = width * dpr;
    int pixelHeight = height * dpr;
    
    // 通知 graphics window 调整大小
    if (_graphicsWindow.valid()) {
        _graphicsWindow->getEventQueue()->windowResize(0, 0, pixelWidth, pixelHeight);
        _graphicsWindow->resized(0, 0, pixelWidth, pixelHeight);
    }
    
    // 更新相机视口和投影
    if (_viewer.valid()) {
        osg::Camera* camera = _viewer->getCamera();
        camera->setViewport(0, 0, pixelWidth, pixelHeight);
        
        double aspectRatio = static_cast<double>(pixelWidth) / static_cast<double>(pixelHeight);
        camera->setProjectionMatrixAsPerspective(30.0, aspectRatio, 1.0, 1000.0);
    }
    
    update();
}
```

## 📊 诊断信息

### 预期的控制台输出

```
OsgVerseWidget::initializeGL called, widget size: 400 x 300
OsgVerseWidget: Device pixel ratio: 1.00, pixel size: 400 x 300
OsgVerseWidget: GraphicsWindow resized to 400 x 300
OsgVerseWidget: Camera viewport set to 400 x 300, aspect: 1.33

OsgVerseWidget::resizeGL called: 1373 x 543
OsgVerseWidget: Viewport updated to 1373 x 543 (DPR: 1.00)
```

### 关键指标

1. **Device Pixel Ratio (DPR)**
   - 普通显示器：1.0
   - 高 DPI 显示器：1.5 或 2.0
   - 如果 DPR > 1.0，说明是高 DPI 显示器

2. **Pixel Size**
   - 应该等于 widget size × DPR
   - 这是 OpenGL 实际渲染的像素数量

3. **Viewport Size**
   - 应该等于 pixel size
   - 这是 OSG 相机的视口大小

## 🎯 测试计划

### 测试步骤

1. **启动 FreeCAD**
   ```cmd
   cd E:\Repository\FreeCAD\FreeCAD\build\bin
   FreeCAD.exe
   ```

2. **检查视口**
   - 背景是否填充整个窗口？
   - 是否还有黑色区域？
   - 球体是否完整显示？

3. **测试交互**
   - 拖动鼠标旋转视图
   - 滚轮缩放
   - 调整窗口大小

4. **查看日志**
   - 检查 Report View 中的诊断信息
   - 确认 DPR 和 pixel size 是否正确

### 成功标准

- ✅ 深蓝灰色背景填充整个窗口
- ✅ 没有黑色区域
- ✅ 绿色球体完整显示
- ✅ 鼠标交互流畅
- ✅ 窗口调整大小时视口正确更新

### 如果仍然失败

需要实施方案 3（Framebuffer 绑定）：

```cpp
void OsgVerseWidget::paintGL()
{
    if (_viewer.valid()) {
        // 绑定 Qt 的 framebuffer
        GLuint fbo = defaultFramebufferObject();
        osg::Camera* camera = _viewer->getCamera();
        camera->setRenderTargetImplementation(
            osg::Camera::FRAME_BUFFER_OBJECT, fbo
        );
        
        _viewer->frame();
    }
}
```

## 📝 技术背景

### 为什么需要 devicePixelRatio？

在高 DPI 显示器上：
- Qt widget 的逻辑大小（width/height）可能是 1000x800
- 但实际像素大小可能是 2000x1600（DPR = 2.0）
- OpenGL 需要使用实际像素大小来设置视口

### Qt 和 OSG 的坐标系统

- **Qt**：使用逻辑坐标（考虑 DPI 缩放）
- **OpenGL/OSG**：使用物理像素坐标
- **转换**：物理像素 = 逻辑坐标 × devicePixelRatio

### GraphicsWindowEmbedded 的工作原理

1. 创建一个"嵌入式"图形窗口（不是真正的窗口）
2. 使用 Qt 的 OpenGL 上下文
3. 需要正确设置视口大小以匹配 Qt widget 的实际像素大小

## 🚀 下一步工作

一旦视口问题解决：

### 1. 清理测试代码
- 从 `OsgVerseViewer.cpp` 移除测试球体
- 移除调试日志

### 2. 实现 ViewProvider 自动添加
- 监听文档对象添加事件
- 自动调用 `viewer->addViewProvider()`
- 实现几何体转换

### 3. 测试实际几何体
- 创建 Part::Box
- 创建 Part::Sphere
- 测试多个对象
- 测试对象删除

### 4. 实现完整的相机控制
- viewAll()
- viewFit()
- 标准视图（前、后、左、右、上、下）

## 📚 参考资料

- [Qt QOpenGLWidget 文档](https://doc.qt.io/qt-5/qopenglwidget.html)
- [OSG GraphicsWindowEmbedded](http://www.openscenegraph.org/documentation/OpenSceneGraphReferenceDocs/a00375.html)
- [Qt High DPI 支持](https://doc.qt.io/qt-5/highdpi.html)
- OsgVerse qt_viewer 示例

---

## 📌 当前状态总结

- ✅ 编译成功
- ⏳ 等待测试结果
- 🎯 目标：完全解决视口裁剪问题

**请启动 FreeCAD 并测试，然后报告结果！** 🚀
