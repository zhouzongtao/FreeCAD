# OsgVerse Phase 1 - 球体不可见问题诊断

## 当前状态

### ✅ 已完成的工作
1. **占位符球体创建** - 半径 5.0 的大红色球体
2. **ViewProvider 集成** - 新对象会自动添加到 OsgVerse 视图
3. **场景图构建** - 节点正确添加到场景容器
4. **边界框计算** - 正确计算并更新
5. **相机调整** - viewAll() 正确计算相机位置
6. **渲染循环** - paintGL() 和 frame() 正常执行
7. **OpenGL 初始化** - initializeGL() 被调用，viewer 已 realized

### ❌ 当前问题
**球体在 3D 视图中不可见**

### 📊 诊断日志（全部正常）
```
20:24:31 OsgVerseViewerImpl: Adding ViewProvider (TestBox)
20:24:31 OsgVerseViewerImpl: Created LARGE RED sphere with radius 5.0 at origin
20:24:31 OsgVerseViewerImpl: Added node to container (total children: 1)
20:24:31 OsgVerseViewerImpl: ViewProvider added successfully
20:24:31 OsgVerseViewerImpl: View all - center(0.00, 0.00, 0.00) radius=8.66
```

### 🔍 可能的原因

1. **渲染状态问题**
   - OpenGL 状态可能没有正确设置
   - 深度测试、背面剔除等可能有问题

2. **场景图遍历问题**
   - OSG 可能没有正确遍历场景图
   - 节点可能被剔除（culling）

3. **GraphicsWindowEmbedded 问题**
   - 可能需要特殊的设置才能在 Qt widget 中正确渲染

4. **相机或投影矩阵问题**
   - 虽然日志显示正常，但实际可能有问题

## 下一步调试计划

### 方案 A：简化测试
创建最简单的 OSG 场景（不通过 FreeCAD 的 ViewProvider 系统）：
- 直接在 initializeGL 中创建一个球体
- 直接添加到 _sceneRoot
- 看是否能渲染

### 方案 B：检查 OSG 渲染状态
- 添加 OSG 的调试输出
- 检查 CullVisitor 和 DrawVisitor 是否正常工作
- 查看是否有 OpenGL 错误

### 方案 C：参考 Coin3D 实现
- 对比 Coin3D 后端的渲染设置
- 看是否缺少某些关键的初始化步骤

## 测试脚本

### 创建对象并查看日志
```python
exec(open('E:\\Repository\\FreeCAD\\FreeCAD\\create_test_object.py', encoding='utf-8').read())
```

### 详细场景诊断
```python
exec(open('E:\\Repository\\FreeCAD\\FreeCAD\\diagnose_scene_detailed.py', encoding='utf-8').read())
```

## 技术细节

### 球体创建代码
```cpp
osg::ref_ptr<osg::Sphere> sphere = new osg::Sphere(osg::Vec3(0, 0, 0), 5.0f);
osg::ref_ptr<osg::ShapeDrawable> drawable = new osg::ShapeDrawable(sphere.get());
drawable->setColor(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));  // 纯红色
```

### 场景图结构
```
_sceneRoot (Group)
  ├─ LightSource
  └─ _vpContainerNode (Group)
      └─ vpNode (Group) - "TestBox"
          └─ geode (Geode)
              └─ ShapeDrawable (Sphere, radius=5.0)
```

### 相机设置
- FOV: 45度
- Near: 0.1
- Far: 1000.0
- 位置：根据边界框自动计算

## 参考资料
- OSG GraphicsWindowEmbedded 文档
- Qt QOpenGLWidget 集成指南
- FreeCAD Coin3D 后端实现
