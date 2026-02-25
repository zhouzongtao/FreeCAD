# Phase 6 Step 5: 完善 View3DOsgVerse 实现

## 当前状态分析

### ✅ 已完成
1. **View3DOsgVerse 基础框架**
   - 继承自 View3DBase
   - 实现了基本的生命周期方法
   - 集成了 OsgVerseViewerImpl
   - 实现了 IViewer3D 接口

2. **OsgVerseViewerImpl 基础实现**
   - 实现了 IViewer3D 接口
   - 创建了 ViewerWidget (QOpenGLWidget)
   - 基本的 OSG viewer 初始化
   - GraphicsWindowEmbedded 集成

3. **基本功能**
   - 窗口创建和显示
   - 基本的鼠标/键盘事件处理
   - viewAll() 功能

### 🚧 需要完善的功能

#### 1. ViewProvider 管理（高优先级）
**当前状态**: 空实现  
**需要实现**:
- `addViewProvider()` - 添加 ViewProvider 到场景
- `removeViewProvider()` - 从场景移除 ViewProvider
- `hasViewProvider()` - 检查 ViewProvider 是否存在
- `getViewProviders()` - 获取所有 ViewProvider

**实现方案**:
```cpp
class OsgVerseViewerImpl {
private:
    std::vector<ViewProvider*> _viewProviders;
    std::map<ViewProvider*, osg::ref_ptr<osg::Node>> _vpNodeMap;
    
public:
    void addViewProvider(ViewProvider* vp) override {
        if (!vp || hasViewProvider(vp)) return;
        
        _viewProviders.push_back(vp);
        
        // 创建 OSG 节点（需要转换 Coin3D 场景图）
        // TODO: 实现 Coin3D -> OSG 转换
        
        updateScene();
    }
    
    void removeViewProvider(ViewProvider* vp) override {
        auto it = std::find(_viewProviders.begin(), _viewProviders.end(), vp);
        if (it != _viewProviders.end()) {
            _viewProviders.erase(it);
            
            // 从场景中移除节点
            auto nodeIt = _vpNodeMap.find(vp);
            if (nodeIt != _vpNodeMap.end()) {
                _sceneRoot->removeChild(nodeIt->second);
                _vpNodeMap.erase(nodeIt);
            }
            
            updateScene();
        }
    }
};
```

#### 2. 场景图转换（高优先级）
**问题**: FreeCAD 使用 Coin3D 场景图，OsgVerse 使用 OSG 场景图  
**需要**: Coin3D -> OSG 转换器

**实现方案**:
- **选项 A**: 完整转换器（复杂，长期）
  - 遍历 Coin3D 场景图
  - 转换每个节点到对应的 OSG 节点
  - 处理材质、纹理、几何体等

- **选项 B**: 最小可行转换（推荐，短期）
  - 只转换基本几何体
  - 使用简单材质
  - 后续逐步完善

**建议**: 先实现选项 B，让基本渲染工作，然后逐步完善

#### 3. 相机同步（中优先级）
**当前状态**: 基本实现  
**需要完善**:
- 相机参数的完整转换
- 与 View3DInventor 的相机参数兼容
- 保存/恢复相机状态

#### 4. 拾取和选择（中优先级）
**当前状态**: 返回空结果  
**需要实现**:
- OSG 拾取系统
- 将拾取结果转换为 FreeCAD 对象
- 选择高亮显示

#### 5. Python 绑定（低优先级）
**当前状态**: 返回 None  
**需要实现**:
- View3DOsgVersePy 类
- Python 接口导出
- 与现有 Python API 兼容

## 实施策略

### Phase 1: 最小可行渲染（本次实施）
**目标**: 让 View3DOsgVerse 能够显示基本的 3D 场景

**任务**:
1. ✅ 完善 ViewProvider 管理基础设施
2. ✅ 实现简单的场景图转换（仅支持基本几何体）
3. ✅ 测试基本渲染功能
4. ✅ 确保不会崩溃

**预期结果**:
- 可以创建 View3DOsgVerse 视图
- 可以显示简单的几何体（立方体、球体等）
- 相机控制基本工作
- 不会影响 Coin3D 后端

### Phase 2: 完整功能（后续）
**任务**:
1. 完整的场景图转换
2. 拾取和选择
3. 高级渲染特性
4. Python 绑定
5. 性能优化

## 实施步骤

### Step 1: 完善 ViewProvider 管理
**文件**: `src/Gui/View3D/Backends/OsgVerse/OsgVerseViewerImpl.cpp`

```cpp
// 添加成员变量
std::vector<ViewProvider*> _viewProviders;
std::map<ViewProvider*, osg::ref_ptr<osg::Node>> _vpNodeMap;

// 实现方法
void OsgVerseViewerImpl::addViewProvider(ViewProvider* vp) {
    if (!vp || hasViewProvider(vp)) {
        return;
    }
    
    Base::Console().log("OsgVerseViewerImpl: Adding ViewProvider %p\n", vp);
    _viewProviders.push_back(vp);
    
    // 创建占位符节点（暂时）
    osg::ref_ptr<osg::Group> vpNode = new osg::Group();
    vpNode->setName(vp->getObject()->getNameInDocument());
    
    _vpNodeMap[vp] = vpNode;
    _sceneRoot->addChild(vpNode);
    
    updateScene();
}

void OsgVerseViewerImpl::removeViewProvider(ViewProvider* vp) {
    auto it = std::find(_viewProviders.begin(), _viewProviders.end(), vp);
    if (it != _viewProviders.end()) {
        Base::Console().log("OsgVerseViewerImpl: Removing ViewProvider %p\n", vp);
        _viewProviders.erase(it);
        
        auto nodeIt = _vpNodeMap.find(vp);
        if (nodeIt != _vpNodeMap.end()) {
            _sceneRoot->removeChild(nodeIt->second);
            _vpNodeMap.erase(nodeIt);
        }
        
        updateScene();
    }
}

bool OsgVerseViewerImpl::hasViewProvider(ViewProvider* vp) const {
    return std::find(_viewProviders.begin(), _viewProviders.end(), vp) 
           != _viewProviders.end();
}

std::vector<ViewProvider*> OsgVerseViewerImpl::getViewProviders() const {
    return _viewProviders;
}
```

### Step 2: 实现基本场景图转换
**新文件**: `src/Gui/View3D/Backends/OsgVerse/SceneGraphConverter.h/cpp`

```cpp
// SceneGraphConverter.h
class SceneGraphConverter {
public:
    // 将 Coin3D SoNode 转换为 OSG Node
    static osg::ref_ptr<osg::Node> convertNode(SoNode* coinNode);
    
    // 转换基本几何体
    static osg::ref_ptr<osg::Geode> convertGeometry(SoNode* coinNode);
    
    // 转换材质
    static osg::ref_ptr<osg::StateSet> convertMaterial(SoMaterial* coinMaterial);
};
```

**实现策略**:
- 第一阶段：只转换基本形状（立方体、球体、圆柱体）
- 使用简单的默认材质
- 忽略复杂的 Coin3D 特性

### Step 3: 更新 View3DOsgVerse
**文件**: `src/Gui/View3DOsgVerse.cpp`

需要实现的方法：
```cpp
bool View3DOsgVerse::containsViewProvider(const ViewProvider* vp) const {
    if (_viewer) {
        return _viewer->hasViewProvider(const_cast<ViewProvider*>(vp));
    }
    return false;
}
```

### Step 4: 测试
创建测试脚本验证基本功能：
```python
import FreeCAD
import FreeCADGui
from Gui import RenderManager

# 切换到 OsgVerse 后端
RenderManager.switchRenderBackend("OsgVerse")

# 创建新文档
doc = FreeCAD.newDocument("OsgVerseTest")

# 添加一些对象
box = doc.addObject("Part::Box", "Box")
sphere = doc.addObject("Part::Sphere", "Sphere")
sphere.Placement.Base = FreeCAD.Vector(50, 0, 0)

doc.recompute()

# 检查视图
view = FreeCADGui.activeDocument().activeView()
print(f"View type: {type(view).__name__}")
print(f"Backend: {view.getBackendType()}")

# 测试 viewAll
view.viewAll()
```

## 风险和缓解

### 风险 1: 场景图转换复杂度
**风险**: Coin3D 到 OSG 的完整转换非常复杂  
**缓解**: 
- 采用渐进式实现
- 先支持基本几何体
- 使用占位符表示不支持的特性

### 风险 2: 性能问题
**风险**: 实时转换可能影响性能  
**缓解**:
- 实现缓存机制
- 只在必要时转换
- 后续优化

### 风险 3: 兼容性问题
**风险**: 可能破坏现有 Coin3D 功能  
**缓解**:
- 保持两个后端完全独立
- 充分测试 Coin3D 后端
- 使用特性开关

## 成功标准

### Phase 1 成功标准
- ✅ 可以创建 View3DOsgVerse 视图
- ✅ 可以添加/移除 ViewProvider
- ✅ 可以显示简单的占位符几何体
- ✅ 相机控制工作
- ✅ 不会崩溃
- ✅ Coin3D 后端不受影响

### Phase 2 成功标准（后续）
- 可以正确显示复杂模型
- 拾取和选择工作
- 性能可接受
- 所有测试通过

## 时间估算

- **Step 1**: ViewProvider 管理 - 1-2 小时
- **Step 2**: 基本场景图转换 - 2-3 小时
- **Step 3**: 更新 View3DOsgVerse - 1 小时
- **Step 4**: 测试和调试 - 1-2 小时

**总计**: 5-8 小时（Phase 1）

## 下一步

1. 实现 ViewProvider 管理
2. 创建简单的场景图转换
3. 测试基本渲染
4. 提交 Phase 1 实现
5. 规划 Phase 2 工作
