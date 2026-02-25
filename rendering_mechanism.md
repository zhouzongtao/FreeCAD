# OsgVerse 渲染机制完整流程图

## 1. 系统初始化流程

```
用户启动 FreeCAD
    ↓
Application::init()
    ↓
加载 FreeCADGui 模块
    ↓
RenderManager::init()
    ├─ 注册 Coin3D 后端 (默认)
    └─ 注册 OsgVerse 后端
    ↓
读取用户配置
    ↓
选择渲染后端 (Coin3D / OsgVerse)
```

## 2. 文档和视图创建流程

```
用户创建/打开文档
    ↓
FreeCAD::Document 创建
    ↓
Gui::Document 创建
    ↓
Document::createView("View3DOsgVerse")
    ↓
创建 View3DOsgVerse 对象
    ↓
View3DOsgVerse::View3DOsgVerse()
    ├─ 创建 OsgVerseViewerImpl
    │   ↓
    │   OsgVerseViewerImpl::OsgVerseViewerImpl()
    │   ├─ 创建 osgViewer::Viewer
    │   ├─ 创建场景根节点 (_sceneRoot)
    │   ├─ 创建 ViewProvider 容器 (_vpContainerNode)
    │   ├─ 创建 ViewerWidget (Qt widget)
    │   └─ initializeViewer()
    │       ├─ 设置线程模型 (SingleThreaded)
    │       ├─ setupDefaultCamera()
    │       │   ├─ 设置相机位置 (0, -20, 10)
    │       │   ├─ 设置视角 FOV 45°
    │       │   └─ 设置裁剪面 (0.01 - 10000.0)
    │       ├─ setupDefaultLighting()
    │       │   ├─ 创建光源 (位置光)
    │       │   └─ 启用光照和深度测试
    │       └─ 设置背景颜色
    └─ 将 widget 添加到 MDI 窗口
```

## 3. OpenGL 上下文初始化流程

```
Qt 创建 QOpenGLWidget
    ↓
ViewerWidget::initializeGL()
    ↓
ensureInitialized()
    ├─ 检查 _initialized 标志
    ├─ createGraphicsWindow()
    │   ├─ 创建 GraphicsContext::Traits
    │   │   ├─ 设置窗口大小
    │   │   ├─ doubleBuffer = true
    │   │   └─ vsync = true
    │   └─ 创建 GraphicsWindowEmbedded
    ├─ initializeViewerContext()
    │   ├─ 设置 graphics context
    │   ├─ 设置 viewport
    │   ├─ 设置投影矩阵
    │   └─ viewer->realize()
    └─ _initialized = true
```

## 4. 对象添加和渲染流程

```
用户创建对象 (如 Part::Box)
    ↓
App::DocumentObject 创建
    ↓
Document::addObject()
    ↓
触发 Document::slotNewObject()
    ↓
创建 ViewProvider
    ↓
ViewProviderDocumentObject::attach()
    ↓
Document::addViewProvider()
    ↓
遍历所有视图，调用 addViewProvider()
    ↓
View3DOsgVerse::addViewProvider()
    ↓
OsgVerseViewerImpl::addViewProvider(vp)
    ├─ 获取对象名称
    ├─ 创建 OSG 节点树
    │   ├─ 创建 Group 节点 (vpNode)
    │   ├─ 创建 Geode 节点
    │   ├─ 创建占位符球体
    │   │   ├─ Sphere (半径 5.0)
    │   │   ├─ ShapeDrawable
    │   │   ├─ 设置颜色 (红色)
    │   │   └─ 设置材质
    │   │       ├─ Diffuse (红色)
    │   │       ├─ Ambient (暗红)
    │   │       ├─ Specular (白色)
    │   │       ├─ Shininess (64.0)
    │   │       └─ Emission (微红)
    │   └─ 计算边界框
    ├─ 保存 VP -> Node 映射
    ├─ 添加到场景容器
    │   └─ _vpContainerNode->addChild(vpNode)
    ├─ 更新边界框
    └─ updateScene()
```

## 5. 渲染循环流程

```
Qt 事件循环
    ↓
ViewerWidget::paintGL()
    ├─ ensureInitialized() (保险检查)
    └─ _viewer->frame()
        ├─ OSG 场景图遍历
        │   ├─ Cull 阶段 (视锥体裁剪)
        │   ├─ Draw 阶段 (渲染)
        │   │   └─ 遍历场景树
        │   │       ├─ _sceneRoot
        │   │       │   ├─ LightSource (光源)
        │   │       │   └─ _vpContainerNode
        │   │       │       └─ vpNode (每个对象)
        │   │       │           └─ Geode
        │   │       │               └─ ShapeDrawable (球体)
        │   │       │                   ├─ 应用材质
        │   │       │                   ├─ 应用光照
        │   │       │                   └─ 绘制几何体
        │   └─ Swap buffers
        └─ 返回
```

## 6. 窗口调整流程

```
用户调整窗口大小
    ↓
ViewerWidget::resizeGL(width, height)
    ├─ _graphicsWindow->resized(0, 0, width, height)
    ├─ 更新 viewport
    └─ 更新投影矩阵
        └─ 重新计算 aspect ratio
```

## 7. 相机控制流程

### 7.1 ViewAll (适应视图)

```
用户点击 "View Fit" 或调用 viewAll()
    ↓
OsgVerseViewerImpl::viewAll()
    ├─ 计算场景包围盒
    │   └─ ComputeBoundsVisitor 遍历场景
    ├─ 计算包围球
    │   ├─ center (中心点)
    │   └─ radius (半径)
    ├─ 计算相机位置
    │   ├─ distance = radius / tan(FOV/2)
    │   ├─ eye = center + (0, -distance * 2.5, radius * 0.8)
    │   └─ up = (0, 0, 1)
    ├─ 设置相机矩阵
    │   └─ setViewMatrixAsLookAt(eye, center, up)
    └─ updateScene()
```

### 7.2 ResetCamera (重置相机)

```
用户调用 resetCamera()
    ↓
OsgVerseViewerImpl::resetCamera()
    ├─ 使用默认相机位置
    │   ├─ eye = CAMERA_DEFAULT_EYE (0, -20, 10)
    │   ├─ center = CAMERA_DEFAULT_CENTER (0, 0, 0)
    │   └─ up = CAMERA_DEFAULT_UP (0, 0, 1)
    └─ setViewMatrixAsLookAt(eye, center, up)
```

## 8. ViewProvider 移除流程

```
用户删除对象
    ↓
Document::removeObject()
    ↓
Document::removeViewProvider()
    ↓
View3DOsgVerse::removeViewProvider()
    ↓
OsgVerseViewerImpl::removeViewProvider(vp)
    ├─ 从 _viewProviders 列表移除
    ├─ 从 _vpNodeMap 查找对应节点
    ├─ 从场景容器移除节点
    │   └─ _vpContainerNode->removeChild(node)
    ├─ 清理映射
    └─ updateScene()
```

## 9. 场景图结构

```
_sceneRoot (osg::Group)
    ├─ LightSource (光源节点)
    │   └─ Light (osg::Light)
    │       ├─ Position: (0, 0, 10, 1)
    │       ├─ Ambient: (0.2, 0.2, 0.2, 1)
    │       ├─ Diffuse: (0.8, 0.8, 0.8, 1)
    │       └─ Specular: (1, 1, 1, 1)
    │
    └─ _vpContainerNode (osg::Group) "ViewProviders"
        ├─ vpNode1 (osg::Group) "TestBox"
        │   └─ Geode
        │       └─ ShapeDrawable (Sphere)
        │           ├─ Geometry: Sphere(radius=5.0)
        │           ├─ Color: (1, 0, 0, 1) 红色
        │           └─ Material
        │               ├─ Diffuse: (1, 0, 0, 1)
        │               ├─ Ambient: (0.5, 0, 0, 1)
        │               ├─ Specular: (1, 1, 1, 1)
        │               ├─ Shininess: 64.0
        │               └─ Emission: (0.2, 0, 0, 1)
        │
        ├─ vpNode2 (osg::Group) "Cylinder"
        │   └─ Geode
        │       └─ ShapeDrawable (Sphere)
        │           └─ ... (同样的占位符球体)
        │
        └─ ... (更多对象)
```

## 10. 数据流向图

```
FreeCAD 核心层
    ↓
App::DocumentObject (几何数据)
    ├─ TopoShape (OCCT 几何)
    ├─ Properties (属性)
    └─ Label (名称)
    ↓
Gui::ViewProvider (显示属性)
    ├─ Visibility
    ├─ DisplayMode
    ├─ ShapeColor
    └─ Transparency
    ↓
View3D 抽象层
    ↓
IViewer3D 接口
    ↓
OsgVerseViewerImpl (后端实现)
    ↓
OSG 场景图
    ├─ osg::Group (节点组织)
    ├─ osg::Geode (几何容器)
    ├─ osg::Drawable (可绘制对象)
    └─ osg::StateSet (渲染状态)
    ↓
OpenGL 渲染管线
    ├─ Vertex Shader
    ├─ Fragment Shader
    └─ Frame Buffer
    ↓
显示器输出
```

## 11. 关键配置参数

### 相机配置
```
FOV: 45.0°
Near Plane: 0.01
Far Plane: 10000.0
Default Eye: (0, -20, 10)
Default Center: (0, 0, 0)
Default Up: (0, 0, 1)
```

### ViewAll 配置
```
Distance Factor: 2.5
Height Factor: 0.8
```

### 占位符配置
```
Sphere Radius: 5.0
Color: (1.0, 0.0, 0.0, 1.0) 红色
```

### 材质配置
```
Ambient: (0.5, 0.0, 0.0, 1.0)
Diffuse: (1.0, 0.0, 0.0, 1.0)
Specular: (1.0, 1.0, 1.0, 1.0)
Shininess: 64.0
Emission: (0.2, 0.0, 0.0, 1.0)
```

### 光照配置
```
Position: (0, 0, 10, 1) 位置光
Ambient: (0.2, 0.2, 0.2, 1.0)
Diffuse: (0.8, 0.8, 0.8, 1.0)
Specular: (1.0, 1.0, 1.0, 1.0)
```

## 12. Phase 1 vs Phase 2 对比

### Phase 1 (当前实现)
```
App::DocumentObject
    ↓
ViewProvider
    ↓
OsgVerseViewerImpl::addViewProvider()
    ↓
创建占位符球体 (固定红色，半径 5.0)
    ↓
添加到场景图
    ↓
渲染 (所有对象都是红色球体)
```

### Phase 2 (计划实现)
```
App::DocumentObject
    ↓
ViewProvider
    ↓
OsgVerseViewerImpl::addViewProvider()
    ↓
获取 TopoShape (OCCT 几何)
    ↓
TopoShape -> OSG Geometry 转换
    ├─ 顶点数据提取
    ├─ 法线计算
    ├─ 纹理坐标生成
    └─ 索引数据组织
    ↓
创建 OSG Geometry
    ├─ osg::Geometry
    ├─ osg::Vec3Array (顶点)
    ├─ osg::Vec3Array (法线)
    └─ osg::DrawElements (索引)
    ↓
应用材质和颜色
    ├─ 从 ViewProvider 获取颜色
    ├─ 从 ViewProvider 获取透明度
    └─ 从 ViewProvider 获取显示模式
    ↓
添加到场景图
    ↓
渲染 (显示真实几何体)
```

## 13. 线程模型

```
主线程 (Qt GUI Thread)
    ├─ Qt 事件循环
    ├─ ViewerWidget::paintGL()
    ├─ OSG Viewer (SingleThreaded 模式)
    │   ├─ Cull 遍历
    │   ├─ Draw 遍历
    │   └─ OpenGL 调用
    └─ 返回 Qt 事件循环

注意：当前使用 SingleThreaded 模式，所有操作在主线程
未来可以考虑使用 ThreadPerContext 或 DrawThreadPerContext
```

## 14. 内存管理

```
OSG 智能指针 (osg::ref_ptr)
    ├─ 自动引用计数
    ├─ 自动内存释放
    └─ 线程安全

FreeCAD 对象
    ├─ App::DocumentObject (由 Document 管理)
    └─ Gui::ViewProvider (由 Document 管理)

映射关系
    └─ _vpNodeMap (ViewProvider* -> osg::ref_ptr<osg::Node>)
        ├─ ViewProvider 删除时清理
        └─ 节点自动释放 (ref_ptr)
```

## 15. 错误处理流程

```
初始化失败
    ├─ _viewer 为 null
    │   └─ OSGVERSE_LOG_ERROR("Viewer is null")
    ├─ GraphicsWindow 创建失败
    │   └─ 在 paintGL 中重试
    └─ realize() 失败
        └─ 记录错误日志

渲染失败
    ├─ 场景为空
    │   └─ 使用默认相机位置
    ├─ 边界框无效
    │   └─ 使用默认相机位置
    └─ OpenGL 错误
        └─ OSG 内部处理
```

## 总结

当前 OsgVerse 渲染机制的特点：

1. **清晰的分层架构**：App 层 → Gui 层 → View3D 抽象层 → 后端实现
2. **完整的初始化流程**：从应用启动到 OpenGL 上下文创建
3. **简单的占位符渲染**：Phase 1 使用红色球体验证管线
4. **良好的扩展性**：为 Phase 2 的真实几何体转换预留接口
5. **统一的配置管理**：所有参数集中定义，易于调整
6. **完善的日志系统**：便于调试和问题追踪

下一步 (Phase 2) 将实现真实的几何体转换，将 OCCT TopoShape 转换为 OSG Geometry，实现真实的 3D 模型渲染。
