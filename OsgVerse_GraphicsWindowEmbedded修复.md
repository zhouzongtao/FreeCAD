# OsgVerse GraphicsWindowEmbedded 修复

## 🔍 问题分析

程序仍然无法启动。通过参考 OsgVerse 官方的 qt_viewer 示例，发现我们的实现与官方示例有关键差异。

## 📚 参考实现

**OsgVerse qt_viewer 示例路径**:
`E:\Repository\OSGVerse\osgverse\applications\qt_viewer`

**关键文件**:
- `qt_header.h` - 头文件定义
- `qt_widget.cpp` - Widget 实现
- `viewer_main.cpp` - 主程序

## 🔑 关键发现

### 1. 使用 GraphicsWindowEmbedded

OsgVerse 官方示例使用的是 `osgViewer::GraphicsWindowEmbedded`，而不是自定义的 GraphicsWindow：

```cpp
// OsgVerse qt_viewer 示例
_graphicsWindow = new osgViewer::GraphicsWindowEmbedded(
    this->x(), this->y(), this->width(), this->height());
```

**优点**:
- 更简单
- 更可靠
- OSG 官方支持
- 无需实现复杂的 GraphicsWindow 接口

### 2. 设置默认 FBO

在 `paintGL()` 的第一帧必须设置默认 FBO：

```cpp
void OsgSceneWidget::paintGL()
{
    if (_viewer.valid())
    {
        if (_firstFrame)
        {
            GLuint defaultFboId = this->defaultFramebufferObject();
            _graphicsWindow->setDefaultFboId(defaultFboId);  // 关键！
            _firstFrame = false;
        }
        _viewer->frame();
    }
}
```

**原因**:
- QOpenGLWidget 使用 FBO 进行渲染
- OSG 需要知道 Qt 的默认 FBO ID
- 否则渲染会失败

### 3. 正确的 resize 处理

```cpp
void OsgSceneWidget::resizeGL(int width, int height)
{
    _graphicsWindow->getEventQueue()->windowResize(this->x(), this->y(), width, height);
    _graphicsWindow->resized(this->x(), this->y(), width, height);
    paintGL();  // 立即绘制一次，避免闪烁
}
```

### 4. OpenGL 格式设置

```cpp
QSurfaceFormat format;
format.setRenderableType(QSurfaceFormat::OpenGL);
format.setProfile(QSurfaceFormat::CompatibilityProfile);
format.setSamples(4);
setFormat(format);
```

## ✅ 实施的修复

### 修改 1: 头文件 (OsgVerseViewer.h)

**改动**:
1. 移除 `OsgVerseGraphicsWindow.h` 包含
2. 添加 `osgViewer::GraphicsWindowEmbedded` 前向声明
3. 修改 `_graphicsWindow` 类型为 `GraphicsWindowEmbedded`
4. 添加 `_firstFrame` 标志
5. 修改 ViewerWidget 构造函数参数

**关键代码**:
```cpp
// 前向声明
namespace osgViewer {
    class Viewer;
    class GraphicsWindowEmbedded;
}

// 成员变量
osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> _graphicsWindow;
bool _firstFrame{true};

// ViewerWidget 构造函数
ViewerWidget(osgViewer::Viewer* viewer, 
             osgViewer::GraphicsWindowEmbedded* graphicsWindow,
             QWidget* parent = nullptr);
```

### 修改 2: 实现文件 (OsgVerseViewer.cpp)

#### 2.1 包含文件
```cpp
#include <osgViewer/GraphicsWindow>  // 添加
#include <QSurfaceFormat>            // 添加
// 移除 #include "OsgVerseGraphicsWindow.h"
```

#### 2.2 initializeViewer()
```cpp
void OsgVerseViewer::initializeViewer()
{
    // 创建 GraphicsWindowEmbedded（像 OsgVerse qt_viewer 示例）
    _graphicsWindow = new osgViewer::GraphicsWindowEmbedded(0, 0, 800, 600);
    
    // 创建 viewer
    _viewer = new osgViewer::Viewer();
    _viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    
    // 设置相机
    osg::Camera* camera = _viewer->getCamera();
    camera->setGraphicsContext(_graphicsWindow.get());
    camera->setViewport(0, 0, 800, 600);
    camera->setClearColor(...);
    
    // 设置相机操纵器
    _viewer->setCameraManipulator(new osgGA::TrackballManipulator());
    _viewer->setKeyEventSetsDone(0);
}
```

#### 2.3 ViewerWidget 构造函数
```cpp
OsgVerseViewer::ViewerWidget::ViewerWidget(
    osgViewer::Viewer* viewer, 
    osgViewer::GraphicsWindowEmbedded* graphicsWindow,
    QWidget* parent)
    : QOpenGLWidget(parent)
    , _viewer(viewer)
    , _graphicsWindow(graphicsWindow)
    , _firstFrame(true)
{
    // 设置 OpenGL 格式（像 OsgVerse qt_viewer 示例）
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setSamples(4);
    setFormat(format);

    // 启用鼠标跟踪和键盘焦点
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(320, 240);
}
```

#### 2.4 paintGL()
```cpp
void OsgVerseViewer::ViewerWidget::paintGL()
{
    if (_viewer) {
        // 第一帧设置默认 FBO（像 OsgVerse qt_viewer 示例）
        if (_firstFrame) {
            GLuint defaultFboId = this->defaultFramebufferObject();
            _graphicsWindow->setDefaultFboId(defaultFboId);
            _firstFrame = false;
        }
        _viewer->frame();
    }
}
```

#### 2.5 resizeGL()
```cpp
void OsgVerseViewer::ViewerWidget::resizeGL(int width, int height)
{
    if (_graphicsWindow.valid()) {
        _graphicsWindow->getEventQueue()->windowResize(this->x(), this->y(), width, height);
        _graphicsWindow->resized(this->x(), this->y(), width, height);
    }
    
    // 立即绘制一次，避免闪烁（像 OsgVerse qt_viewer 示例）
    paintGL();
}
```

#### 2.6 事件处理
所有鼠标和键盘事件都添加 `update()` 调用：

```cpp
void OsgVerseViewer::ViewerWidget::mousePressEvent(QMouseEvent* event)
{
    // ... 处理事件 ...
    update();  // 触发重绘
}
```

## 🎯 预期效果

### 修复的问题
1. ✅ 使用官方推荐的 GraphicsWindowEmbedded
2. ✅ 正确设置默认 FBO
3. ✅ 正确处理 resize 事件
4. ✅ 正确的 OpenGL 格式设置
5. ✅ 事件处理后触发重绘

### 预期结果
- FreeCAD 应该能够正常启动
- 3D 视图应该能够正常显示
- 鼠标交互应该正常工作

## 📝 技术要点

### GraphicsWindowEmbedded vs 自定义 GraphicsWindow

**GraphicsWindowEmbedded**:
- ✅ 简单易用
- ✅ OSG 官方支持
- ✅ 专为嵌入式场景设计
- ✅ 无需实现复杂接口

**自定义 GraphicsWindow**:
- ❌ 实现复杂
- ❌ 容易出错
- ❌ 需要深入了解 OSG 内部机制
- ✅ 更灵活（但我们不需要）

### FBO 的重要性

QOpenGLWidget 使用 FBO (Framebuffer Object) 进行渲染：
1. Qt 创建一个 FBO
2. Widget 渲染到这个 FBO
3. Qt 将 FBO 内容合成到窗口

OSG 需要知道这个 FBO 的 ID，否则会渲染到错误的目标。

### 单线程模式

```cpp
_viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
```

在 Qt 集成中必须使用单线程模式，因为：
- Qt 的 OpenGL 上下文不是线程安全的
- 多线程模式会导致上下文冲突

## 🔄 下一步

1. **编译修改**
   ```cmd
   cmake --build build --config Release --target FreeCADGui -- /maxcpucount:1
   cmake --build build --config Release --target FreeCADMain -- /maxcpucount:1
   ```

2. **测试启动**
   ```cmd
   cd build\bin
   FreeCAD.exe --console
   ```

3. **观察日志**
   - 应该看到 "OsgVerseViewer: Starting lazy initialization..."
   - 应该看到 "OsgVerseViewer::ViewerWidget: Constructor completed"
   - 应该看到 "Set default FBO ID: ..."

## 💡 经验教训

1. **参考官方示例**
   - 官方示例是最可靠的参考
   - 不要过度设计
   - 简单的方案往往更好

2. **理解 Qt + OSG 集成**
   - FBO 是关键
   - 单线程模式是必须的
   - 事件处理需要触发重绘

3. **逐步调试**
   - 从最简单的实现开始
   - 逐步添加功能
   - 每一步都验证

---

**状态**: 修改完成，准备编译测试

**关键改进**: 使用 GraphicsWindowEmbedded 替代自定义 GraphicsWindow
