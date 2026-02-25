# Phase 6 Step 3 Phase 2 - 基础渲染实施说明

## 实施时间
2026-01-20

## 实施目标

实现 OsgVerse viewer 的基础渲染功能，让视图能够显示 3D 内容。

## 实施内容

### 1. OSG Graphics Window 集成

#### ViewerWidget::initializeGL()
```cpp
void OsgVerseViewerImpl::ViewerWidget::initializeGL()
{
    // 创建 GraphicsWindow::Traits
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits();
    traits->x = 0;
    traits->y = 0;
    traits->width = width();
    traits->height = height();
    traits->windowDecoration = false;
    traits->doubleBuffer = true;
    traits->sharedContext = nullptr;
    traits->vsync = true;
    
    // 创建 GraphicsWindowEmbedded
    _graphicsWindow = new osgViewer::GraphicsWindowEmbedded(traits.get());
    
    // 设置 viewer 的 graphics context
    _viewer->getCamera()->setGraphicsContext(_graphicsWindow.get());
    
    // 设置视口
    _viewer->getCamera()->setViewport(new osg::Viewport(0, 0, width(), height()));
}
```

**功能**:
- 创建 OSG GraphicsWindowEmbedded
- 将其与 Qt OpenGL widget 集成
- 设置视口

#### ViewerWidget::paintGL()
```cpp
void OsgVerseViewerImpl::ViewerWidget::paintGL()
{
    if (_viewer && _graphicsWindow) {
        // 渲染一帧
        _viewer->frame();
    }
}
```

**功能**:
- 调用 OSG viewer 的 frame() 方法渲染一帧
- 由 Qt 的渲染循环自动调用

#### ViewerWidget::resizeGL()
```cpp
void OsgVerseViewerImpl::ViewerWidget::resizeGL(int width, int height)
{
    if (_graphicsWindow) {
        _graphicsWindow->resized(0, 0, width, height);
    }
    
    if (_viewer && _viewer->getCamera()) {
        _viewer->getCamera()->setViewport(0, 0, width, height);
        
        // 更新投影矩阵
        double aspectRatio = static_cast<double>(width) / static_cast<double>(height);
        _viewer->getCamera()->setProjectionMatrixAsPerspective(45.0, aspectRatio, 0.1, 1000.0);
    }
}
```

**功能**:
- 通知 OSG graphics window 尺寸变化
- 更新相机视口
- 更新投影矩阵的宽高比

### 2. 相机系统

#### setCamera()
```cpp
void OsgVerseViewerImpl::setCamera(const CameraParams& params)
{
    osg::Camera* camera = _viewer->getCamera();
    
    // 设置视图矩阵
    osg::Vec3d eye(params.position.x, params.position.y, params.position.z);
    osg::Vec3d center(params.target.x, params.target.y, params.target.z);
    osg::Vec3d up(params.upVector.x, params.upVector.y, params.upVector.z);
    camera->setViewMatrixAsLookAt(eye, center, up);
    
    // 设置投影矩阵
    if (params.orthographic) {
        camera->setProjectionMatrixAsOrtho(...);
    } else {
        camera->setProjectionMatrixAsPerspective(...);
    }
}
```

**功能**:
- 设置相机位置、目标点和上向量
- 支持透视和正交投影
- 设置近远裁剪面

#### getCamera()
```cpp
CameraParams OsgVerseViewerImpl::getCamera() const
{
    CameraParams params;
    osg::Camera* camera = _viewer->getCamera();
    
    // 获取视图矩阵
    osg::Vec3d eye, center, up;
    camera->getViewMatrixAsLookAt(eye, center, up);
    
    // 转换为 FreeCAD 类型
    params.position = Base::Vector3d(eye.x(), eye.y(), eye.z());
    // ...
    
    return params;
}
```

**功能**:
- 从 OSG 相机获取参数
- 转换为 FreeCAD 的 CameraParams 结构

#### viewAll()
```cpp
void OsgVerseViewerImpl::viewAll()
{
    // 计算场景包围盒
    osg::ComputeBoundsVisitor cbv;
    _sceneRoot->accept(cbv);
    osg::BoundingBox bb = cbv.getBoundingBox();
    
    // 计算包围球
    osg::BoundingSphere bs;
    bs.expandBy(bb);
    
    // 设置相机位置
    osg::Vec3d center = bs.center();
    double radius = bs.radius();
    double distance = radius / std::tan(osg::DegreesToRadians(45.0 / 2.0));
    
    osg::Vec3d eye = center + osg::Vec3d(0.0, -distance * 1.5, radius * 0.5);
    _viewer->getCamera()->setViewMatrixAsLookAt(eye, center, osg::Vec3d(0, 0, 1));
}
```

**功能**:
- 计算场景的包围盒和包围球
- 自动调整相机位置以查看所有对象
- 类似 Coin3D 的 viewAll() 功能

#### resetCamera()
```cpp
void OsgVerseViewerImpl::resetCamera()
{
    // 重置到默认相机位置
    osg::Vec3d eye(0.0, -10.0, 5.0);
    osg::Vec3d center(0.0, 0.0, 0.0);
    osg::Vec3d up(0.0, 0.0, 1.0);
    
    _viewer->getCamera()->setViewMatrixAsLookAt(eye, center, up);
}
```

**功能**:
- 重置相机到默认位置
- 默认位置：eye(0, -10, 5), center(0, 0, 0), up(0, 0, 1)

#### setCameraType()
```cpp
void OsgVerseViewerImpl::setCameraType(bool orthographic)
{
    _orthographic = orthographic;
    
    if (orthographic) {
        // 切换到正交投影
        camera->setProjectionMatrixAsOrtho(-halfWidth, halfWidth, -halfHeight, halfHeight, 0.1, 1000.0);
    } else {
        // 切换到透视投影
        camera->setProjectionMatrixAsPerspective(45.0, aspectRatio, 0.1, 1000.0);
    }
}
```

**功能**:
- 在正交和透视投影之间切换
- 保持相同的视图中心

### 3. 场景管理

#### setBackgroundColor()
```cpp
void OsgVerseViewerImpl::setBackgroundColor(const Base::Color& color)
{
    _backgroundColor = color;
    
    if (_viewer && _viewer->getCamera()) {
        osg::Vec4 clearColor(color.r, color.g, color.b, color.a);
        _viewer->getCamera()->setClearColor(clearColor);
    }
}
```

**功能**:
- 设置背景颜色
- 立即应用到 OSG 相机

### 4. 初始化方法

#### initializeViewer()
```cpp
void OsgVerseViewerImpl::initializeViewer()
{
    // 设置线程模型
    _viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    
    // 设置默认相机
    setupDefaultCamera();
    
    // 设置默认光照
    setupDefaultLighting();
    
    // 设置背景颜色
    _viewer->getCamera()->setClearColor(osg::Vec4(_backgroundColor.r, _backgroundColor.g, 
                                                   _backgroundColor.b, _backgroundColor.a));
    
    // 禁用默认的事件处理器
    _viewer->setKeyEventSetsDone(0);
    
    _initialized = true;
}
```

**功能**:
- 设置单线程模型（与 Qt 集成）
- 初始化相机和光照
- 设置背景颜色
- 禁用默认的退出键

#### setupDefaultCamera()
```cpp
void OsgVerseViewerImpl::setupDefaultCamera()
{
    osg::Camera* camera = _viewer->getCamera();
    
    // 设置默认视图矩阵
    osg::Vec3d eye(0.0, -10.0, 5.0);
    osg::Vec3d center(0.0, 0.0, 0.0);
    osg::Vec3d up(0.0, 0.0, 1.0);
    camera->setViewMatrixAsLookAt(eye, center, up);
    
    // 设置默认投影矩阵（透视）
    camera->setProjectionMatrixAsPerspective(45.0, aspectRatio, 0.1, 1000.0);
    
    // 设置清除掩码
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
```

**功能**:
- 设置默认相机位置和方向
- 设置透视投影
- 配置清除缓冲区

#### setupDefaultLighting()
```cpp
void OsgVerseViewerImpl::setupDefaultLighting()
{
    // 创建光源
    osg::ref_ptr<osg::Light> light = new osg::Light();
    light->setLightNum(0);
    light->setPosition(osg::Vec4(0.0, 0.0, 10.0, 1.0));  // 位置光
    light->setAmbient(osg::Vec4(0.2, 0.2, 0.2, 1.0));
    light->setDiffuse(osg::Vec4(0.8, 0.8, 0.8, 1.0));
    light->setSpecular(osg::Vec4(1.0, 1.0, 1.0, 1.0));
    
    osg::ref_ptr<osg::LightSource> lightSource = new osg::LightSource();
    lightSource->setLight(light.get());
    
    // 添加到场景根节点
    _sceneRoot->addChild(lightSource.get());
    
    // 启用光照
    _sceneRoot->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);
}
```

**功能**:
- 创建默认光源（位置光）
- 设置环境光、漫反射光和镜面光
- 添加到场景根节点
- 启用 OpenGL 光照

## 技术要点

### 1. GraphicsWindowEmbedded
- OSG 提供的嵌入式图形窗口
- 不创建自己的窗口，使用外部 OpenGL 上下文
- 适合与 Qt 等 GUI 框架集成

### 2. 单线程模型
```cpp
_viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
```
- 与 Qt 的渲染循环集成
- 避免多线程同步问题
- 由 Qt 的 paintGL() 驱动渲染

### 3. 相机坐标系
- OSG 使用右手坐标系
- FreeCAD 也使用右手坐标系（Z 轴向上）
- 默认相机位置：eye(0, -10, 5), center(0, 0, 0), up(0, 0, 1)

### 4. 包围盒计算
```cpp
osg::ComputeBoundsVisitor cbv;
_sceneRoot->accept(cbv);
osg::BoundingBox bb = cbv.getBoundingBox();
```
- 使用访问者模式遍历场景图
- 计算所有节点的包围盒
- 用于 viewAll() 功能

### 5. 光照模型
- 使用 OpenGL 固定管线光照
- 一个位置光源
- 环境光 + 漫反射光 + 镜面光

## 预期效果

### 启动时
```
OsgVerseViewerImpl: Creating OsgVerse viewer (Phase 2 - Basic Rendering)
OsgVerseViewerImpl: Initializing viewer...
OsgVerseViewerImpl: Default camera setup complete
OsgVerseViewerImpl: Default lighting setup complete
OsgVerseViewerImpl: Viewer initialized successfully
OsgVerseViewerImpl: OsgVerse viewer created successfully
```

### 创建视图时
```
OsgVerseViewerImpl::ViewerWidget: Creating widget
OsgVerseViewerImpl::ViewerWidget: Initializing OpenGL
OsgVerseViewerImpl::ViewerWidget: OpenGL initialized
```

### 调整大小时
```
OsgVerseViewerImpl::ViewerWidget: Resize to 800x600
```

### 使用相机功能时
```
OsgVerseViewerImpl: View all - center(0.00, 0.00, 0.00) radius=5.00
OsgVerseViewerImpl: Camera reset to default position
OsgVerseViewerImpl: Switched to orthographic projection
OsgVerseViewerImpl: Background color set to (0.20, 0.20, 0.30, 1.00)
```

## 测试方法

### 1. 基础渲染测试
```python
import FreeCAD
import FreeCADGui as Gui

# 切换到 OsgVerse
Gui.switchRenderBackend(2)

# 创建文档和对象
doc = FreeCAD.newDocument()
box = doc.addObject("Part::Box", "Box")
box.Length = 10
box.Width = 10
box.Height = 10
doc.recompute()

# 打开 3D 视图
Gui.activeDocument().activeView()
```

**预期结果**:
- ✅ 视图窗口显示
- ✅ 可以看到蓝灰色背景
- ⚠️ 可能看不到 Box（需要 ViewProvider 支持，Phase 3 实现）

### 2. 相机控制测试
```python
import FreeCADGui as Gui

view = Gui.activeDocument().activeView()

# 测试 viewAll
view.viewAll()

# 测试 resetCamera
# view.resetCamera()  # 如果接口可用

# 测试切换投影
# view.setCameraType(True)  # 正交
# view.setCameraType(False)  # 透视
```

### 3. 背景颜色测试
```python
import FreeCADGui as Gui

# 设置背景颜色
# Gui.activeDocument().activeView().setBackgroundColor(...)
```

## 限制和已知问题

### Phase 2 限制
1. **ViewProvider 支持**: 还不能显示 FreeCAD 对象（需要 Phase 3）
2. **事件处理**: 鼠标/键盘交互未实现（Phase 3）
3. **拾取**: 对象选择未实现（Phase 3）

### 可能的问题
1. **空白视图**: 如果没有添加任何几何体，视图只显示背景色
2. **性能**: 单线程模型可能在复杂场景中性能较低
3. **光照**: 默认光照可能不适合所有场景

## 下一步工作

### Phase 3: 完整功能
1. **事件处理** (~200 行)
   - 鼠标事件转换
   - 键盘事件处理
   - 相机操纵器集成

2. **拾取和选择** (~150 行)
   - 实现 pick() 方法
   - 射线求交
   - 高亮显示

3. **ViewProvider 管理** (~150 行)
   - addViewProvider/removeViewProvider
   - ViewProvider 到 OSG 节点转换
   - 场景图同步

## 参考资料

- OSG 官方文档: http://www.openscenegraph.org/
- OSG Quick Start Guide
- FreeCAD View3DInventor 实现
- Qt OpenGL 集成文档
