# Phase 3: ViewProvider 集成计划

## 🎯 目标

实现 ViewProvider 的自动添加和实际几何体的渲染，使 OsgVerse 后端能够显示 FreeCAD 文档中的对象。

## 📋 当前状态

### ✅ 已完成（Phase 2.5）
- View3DOsgVerse 类创建
- 渲染管线工作
- 鼠标交互工作
- 视口正确显示
- 测试球体已移除

### ⏳ 待实现
- ViewProvider 自动添加
- 实际几何体渲染
- 对象更新处理
- 对象删除处理

## 🔧 实施步骤

### Step 1: 监听文档对象事件

在 `View3DOsgVerse` 中监听文档对象的添加/删除事件。

#### 修改文件
- `src/Gui/View3DOsgVerse.h`
- `src/Gui/View3DOsgVerse.cpp`

#### 实现内容

```cpp
// View3DOsgVerse.h
class GuiExport View3DOsgVerse : public MDIView
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
    
public:
    // ... 现有代码 ...
    
    // 文档事件处理
    void onObjectAdded(const App::DocumentObject& obj);
    void onObjectRemoved(const App::DocumentObject& obj);
    void onObjectChanged(const App::DocumentObject& obj, const App::Property& prop);
    
private:
    // 连接到文档信号
    void connectToDocument();
    void disconnectFromDocument();
    
    // 信号连接
    boost::signals2::scoped_connection _connObjectAdded;
    boost::signals2::scoped_connection _connObjectRemoved;
    boost::signals2::scoped_connection _connObjectChanged;
};
```

```cpp
// View3DOsgVerse.cpp
void View3DOsgVerse::connectToDocument()
{
    if (!_document) return;
    
    App::Document* doc = _document->getDocument();
    if (!doc) return;
    
    // 连接到文档信号
    _connObjectAdded = doc->signalNewObject.connect(
        boost::bind(&View3DOsgVerse::onObjectAdded, this, _1)
    );
    
    _connObjectRemoved = doc->signalDeletedObject.connect(
        boost::bind(&View3DOsgVerse::onObjectRemoved, this, _1)
    );
    
    _connObjectChanged = doc->signalChangedObject.connect(
        boost::bind(&View3DOsgVerse::onObjectChanged, this, _1, _2)
    );
}

void View3DOsgVerse::onObjectAdded(const App::DocumentObject& obj)
{
    // 获取 ViewProvider
    Gui::ViewProvider* vp = _document->getViewProvider(&obj);
    if (!vp) return;
    
    // 添加到 viewer
    if (_viewer) {
        _viewer->addViewProvider(vp);
    }
}

void View3DOsgVerse::onObjectRemoved(const App::DocumentObject& obj)
{
    // 获取 ViewProvider
    Gui::ViewProvider* vp = _document->getViewProvider(&obj);
    if (!vp) return;
    
    // 从 viewer 移除
    if (_viewer) {
        _viewer->removeViewProvider(vp);
    }
}

void View3DOsgVerse::onObjectChanged(const App::DocumentObject& obj, const App::Property& prop)
{
    // 获取 ViewProvider
    Gui::ViewProvider* vp = _document->getViewProvider(&obj);
    if (!vp) return;
    
    // 更新 viewer 中的对象
    if (_viewer && _viewer->hasViewProvider(vp)) {
        _viewer->removeViewProvider(vp);
        _viewer->addViewProvider(vp);
    }
}
```

### Step 2: 改进几何体转换

当前 `GeometryConverter` 已经实现，但需要测试和优化。

#### 测试内容
1. Part::Box - 简单立方体
2. Part::Sphere - 球体
3. Part::Cylinder - 圆柱体
4. Part::Cone - 圆锥体
5. 复杂模型 - 导入的 STEP 文件

#### 可能需要的改进
- 调整网格细分参数
- 优化法线计算
- 添加边缘线渲染
- 材质和颜色处理

### Step 3: 实现 viewAll() 功能

让相机自动适配场景中的所有对象。

#### 修改文件
- `src/Mod/OsgVerseGui/OsgVerseViewer.cpp`

#### 实现内容

```cpp
void OsgVerseViewer::viewAll()
{
    if (!_widget) return;
    
    osgViewer::Viewer* viewer = _widget->getViewer();
    if (!viewer) return;
    
    // 计算场景边界
    osg::BoundingSphere bs = _sceneRoot->getBound();
    
    if (bs.valid()) {
        // 使用 OSG 的 home() 功能
        // 它会自动计算合适的相机位置
        viewer->home();
    }
    
    render();
}
```

### Step 4: 处理现有对象

当视图创建时，文档中可能已经有对象了，需要添加它们。

#### 修改文件
- `src/Gui/View3DOsgVerse.cpp`

#### 实现内容

```cpp
View3DOsgVerse::View3DOsgVerse(Gui::Document* doc, QWidget* parent)
    : MDIView(doc, parent, Qt::WindowFlags())
{
    // ... 现有代码 ...
    
    // 连接到文档事件
    connectToDocument();
    
    // 添加现有对象
    addExistingObjects();
}

void View3DOsgVerse::addExistingObjects()
{
    if (!_document || !_viewer) return;
    
    App::Document* doc = _document->getDocument();
    if (!doc) return;
    
    // 遍历所有对象
    std::vector<App::DocumentObject*> objects = doc->getObjects();
    for (App::DocumentObject* obj : objects) {
        Gui::ViewProvider* vp = _document->getViewProvider(obj);
        if (vp) {
            _viewer->addViewProvider(vp);
        }
    }
    
    // 适配视图
    if (!objects.empty()) {
        _viewer->viewAll();
    }
}
```

## 🧪 测试计划

### Test 1: 简单立方体

```python
import FreeCAD
import FreeCADGui

# 确保使用 OsgVerse 后端
FreeCADGui.setRenderBackend(2)

# 创建文档
doc = FreeCAD.newDocument("Test1")

# 创建立方体
box = doc.addObject("Part::Box", "Box")
box.Length = 10
box.Width = 10
box.Height = 10
doc.recompute()

# 应该自动显示在视图中
FreeCADGui.activeView().viewAll()
```

### Test 2: 多个对象

```python
import FreeCAD
import FreeCADGui

FreeCADGui.setRenderBackend(2)
doc = FreeCAD.newDocument("Test2")

# 创建多个对象
box = doc.addObject("Part::Box", "Box")
box.Placement.Base = FreeCAD.Vector(0, 0, 0)

sphere = doc.addObject("Part::Sphere", "Sphere")
sphere.Placement.Base = FreeCAD.Vector(20, 0, 0)

cylinder = doc.addObject("Part::Cylinder", "Cylinder")
cylinder.Placement.Base = FreeCAD.Vector(40, 0, 0)

doc.recompute()
FreeCADGui.activeView().viewAll()
```

### Test 3: 对象删除

```python
# 继续 Test 2
doc.removeObject("Sphere")
# 球体应该从视图中消失
```

### Test 4: 对象修改

```python
# 继续 Test 2
box.Length = 20
doc.recompute()
# 立方体应该更新
```

### Test 5: 现有文档

```python
import FreeCAD
import FreeCADGui

# 先创建对象
doc = FreeCAD.newDocument("Test5")
box = doc.addObject("Part::Box", "Box")
sphere = doc.addObject("Part::Sphere", "Sphere")
sphere.Placement.Base = FreeCAD.Vector(20, 0, 0)
doc.recompute()

# 然后切换到 OsgVerse
FreeCADGui.setRenderBackend(2)

# 创建新视图
FreeCADGui.activeDocument().activeView().viewAll()
# 应该看到现有的对象
```

## 📊 预期结果

### 成功标准

1. **自动添加**
   - ✅ 创建对象后自动显示在视图中
   - ✅ 不需要手动调用 addViewProvider

2. **几何体渲染**
   - ✅ Part::Box 正确显示
   - ✅ Part::Sphere 正确显示
   - ✅ Part::Cylinder 正确显示
   - ✅ 复杂模型正确显示

3. **对象更新**
   - ✅ 修改对象属性后自动更新
   - ✅ 删除对象后从视图中移除

4. **相机控制**
   - ✅ viewAll() 正确适配所有对象
   - ✅ 相机位置合理

## 🚨 可能的问题

### 问题 1: ViewProvider 时机

**问题**：对象创建时 ViewProvider 可能还没有创建。

**解决方案**：
- 在 `onObjectAdded` 中检查 ViewProvider 是否存在
- 如果不存在，延迟添加或监听 ViewProvider 创建事件

### 问题 2: 几何体转换失败

**问题**：某些复杂几何体可能转换失败。

**解决方案**：
- 添加错误处理
- 显示占位符（如之前的球体）
- 记录错误日志

### 问题 3: 性能问题

**问题**：大量对象或复杂模型可能导致性能下降。

**解决方案**：
- 实现 LOD (Level of Detail)
- 使用 OSG 的优化功能
- 考虑异步加载

### 问题 4: 材质和颜色

**问题**：ViewProvider 的颜色和材质可能不正确。

**解决方案**：
- 从 ViewProvider 读取颜色属性
- 应用到 OSG 材质
- 支持透明度

## 📝 实施顺序

1. **Step 1**：实现文档事件监听（1-2 小时）
2. **Step 2**：测试几何体转换（1 小时）
3. **Step 3**：实现 viewAll()（30 分钟）
4. **Step 4**：处理现有对象（30 分钟）
5. **测试**：运行所有测试用例（1 小时）
6. **调试**：修复发现的问题（1-2 小时）

**总计**：5-7 小时

## 🎯 Phase 3 完成标准

- ✅ 创建对象自动显示
- ✅ 删除对象自动移除
- ✅ 修改对象自动更新
- ✅ viewAll() 正确工作
- ✅ 至少 3 种基本几何体正确渲染
- ✅ 所有测试用例通过

---

**准备好开始 Phase 3 了吗？** 🚀
