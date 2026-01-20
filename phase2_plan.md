# Phase 2 实施计划：真实几何体渲染

## 当前状态总结

### ✅ 已完成（Phase 1 + Phase 1.5）

1. **基础架构** ✅
   - 渲染抽象层（IViewer3D 接口）
   - RenderManager 后端管理
   - 动态后端切换机制

2. **OsgVerse 后端基础** ✅
   - OpenGL 上下文初始化
   - 场景图管理
   - ViewProvider 集成
   - 相机控制（viewAll, resetCamera）
   - 占位符渲染（红色球体）

3. **代码质量** ✅
   - 统一的日志系统
   - 集中的配置管理
   - 清晰的初始化流程
   - 完善的文档注释

4. **测试和文档** ✅
   - 完整的渲染流程图
   - 后端切换指南
   - 测试脚本

### 🎯 Phase 2 目标

**将占位符球体替换为真实的 3D 几何体**

从：所有对象显示为红色球体
到：显示真实的 Box、Cylinder、Sphere 等几何体

## Phase 2 技术挑战

### 1. OCCT 到 OSG 的几何体转换

**挑战**：
- FreeCAD 使用 OCCT (OpenCASCADE) 存储几何数据
- OSG 使用自己的几何体格式
- 需要高效的转换算法

**关键技术**：
- TopoShape 遍历（Face、Edge、Vertex）
- 三角剖分（Tessellation）
- 法线计算
- 纹理坐标生成

### 2. 性能优化

**挑战**：
- 复杂模型可能有数百万个三角形
- 需要高效的内存管理
- 需要合理的细节层次（LOD）

**策略**：
- 使用 OSG 的优化机制
- 缓存转换结果
- 按需加载

### 3. 材质和颜色

**挑战**：
- 从 ViewProvider 获取颜色和材质属性
- 支持透明度
- 支持不同的显示模式（Shaded、Wireframe、Points）

## Phase 2 实施步骤

### Step 1: 几何体转换基础 (2-3 天)

**目标**：实现基本的 TopoShape → OSG Geometry 转换

**任务**：
1. 创建 `GeometryConverter` 类
   ```cpp
   class GeometryConverter {
   public:
       static osg::ref_ptr<osg::Geometry> convertTopoShape(
           const TopoDS_Shape& shape,
           double deflection = 0.1
       );
   };
   ```

2. 实现三角剖分
   - 使用 `BRepMesh_IncrementalMesh` 进行网格化
   - 提取顶点、法线、索引数据
   - 创建 OSG Geometry

3. 测试简单几何体
   - Box
   - Cylinder
   - Sphere

**预期结果**：
- ✓ 简单几何体显示正确
- ✓ 形状准确
- ✓ 性能可接受

### Step 2: 材质和颜色支持 (1-2 天)

**目标**：从 ViewProvider 获取并应用颜色和材质

**任务**：
1. 从 ViewProvider 读取属性
   ```cpp
   // 获取颜色
   App::Color shapeColor = vp->ShapeColor.getValue();
   
   // 获取透明度
   float transparency = vp->Transparency.getValue();
   
   // 获取显示模式
   const char* displayMode = vp->DisplayMode.getValueAsString();
   ```

2. 应用到 OSG 材质
   ```cpp
   osg::ref_ptr<osg::Material> material = new osg::Material();
   material->setDiffuse(osg::Material::FRONT_AND_BACK, 
                       osg::Vec4(color.r, color.g, color.b, 1.0 - transparency));
   ```

3. 支持不同显示模式
   - Shaded（实体）
   - Wireframe（线框）
   - Points（点）

**预期结果**：
- ✓ 对象显示正确的颜色
- ✓ 透明度正常工作
- ✓ 可以切换显示模式

### Step 3: 复杂几何体支持 (2-3 天)

**目标**：支持复杂的 CAD 模型

**任务**：
1. 处理复合形状（Compound）
   - 遍历所有子形状
   - 递归转换

2. 优化大型模型
   - 实现几何体缓存
   - 使用 OSG 的优化器
   - 考虑 LOD（细节层次）

3. 边缘显示
   - 提取边缘
   - 创建线条几何体
   - 支持边缘颜色和宽度

**预期结果**：
- ✓ 复杂模型正确显示
- ✓ 性能良好
- ✓ 边缘清晰可见

### Step 4: 动态更新支持 (1-2 天)

**目标**：支持对象属性变化时的动态更新

**任务**：
1. 监听 ViewProvider 属性变化
   ```cpp
   void OsgVerseViewerImpl::updateViewProvider(ViewProvider* vp) {
       // 重新转换几何体
       // 更新场景图
   }
   ```

2. 实现增量更新
   - 只更新变化的部分
   - 避免完全重建

3. 测试各种更新场景
   - 颜色变化
   - 透明度变化
   - 几何体变化

**预期结果**：
- ✓ 属性变化立即反映
- ✓ 更新流畅
- ✓ 无内存泄漏

### Step 5: 测试和优化 (1-2 天)

**目标**：全面测试和性能优化

**任务**：
1. 创建测试用例
   - 各种几何体类型
   - 不同复杂度的模型
   - 边界情况

2. 性能测试
   - 大型装配体
   - 内存使用
   - 帧率

3. 与 Coin3D 对比
   - 显示质量
   - 性能
   - 功能完整性

**预期结果**：
- ✓ 所有测试通过
- ✓ 性能达标
- ✓ 与 Coin3D 效果相当

## 技术细节

### 几何体转换流程

```
TopoDS_Shape (OCCT)
    ↓
BRepMesh_IncrementalMesh (三角剖分)
    ↓
遍历 Face
    ├─ 提取三角形
    ├─ 计算法线
    └─ 生成纹理坐标
    ↓
创建 OSG Geometry
    ├─ osg::Vec3Array (顶点)
    ├─ osg::Vec3Array (法线)
    ├─ osg::Vec2Array (纹理坐标)
    └─ osg::DrawElementsUInt (索引)
    ↓
应用材质和状态
    ├─ osg::Material (材质)
    ├─ osg::BlendFunc (透明度)
    └─ osg::PolygonMode (显示模式)
    ↓
添加到场景图
```

### 关键代码结构

```cpp
// GeometryConverter.h
class GeometryConverter {
public:
    struct ConversionOptions {
        double deflection = 0.1;      // 三角剖分精度
        bool computeNormals = true;   // 是否计算法线
        bool generateTexCoords = false; // 是否生成纹理坐标
    };
    
    static osg::ref_ptr<osg::Geometry> convertShape(
        const TopoDS_Shape& shape,
        const ConversionOptions& options = ConversionOptions()
    );
    
private:
    static void extractTriangles(
        const TopoDS_Face& face,
        std::vector<osg::Vec3>& vertices,
        std::vector<osg::Vec3>& normals,
        std::vector<unsigned int>& indices
    );
    
    static void computeNormals(
        const std::vector<osg::Vec3>& vertices,
        const std::vector<unsigned int>& indices,
        std::vector<osg::Vec3>& normals
    );
};

// OsgVerseViewerImpl.cpp
void OsgVerseViewerImpl::addViewProvider(ViewProvider* vp) {
    // 获取 TopoShape
    auto* vpDoc = dynamic_cast<ViewProviderDocumentObject*>(vp);
    if (!vpDoc || !vpDoc->getObject()) {
        return;
    }
    
    auto* obj = vpDoc->getObject();
    if (!obj->isDerivedFrom(Part::Feature::getClassTypeId())) {
        return;
    }
    
    auto* partFeature = static_cast<Part::Feature*>(obj);
    const TopoDS_Shape& shape = partFeature->Shape.getValue();
    
    // 转换几何体
    GeometryConverter::ConversionOptions options;
    options.deflection = 0.1;  // 可以从配置读取
    
    osg::ref_ptr<osg::Geometry> geometry = 
        GeometryConverter::convertShape(shape, options);
    
    // 应用材质
    applyMaterial(geometry, vp);
    
    // 添加到场景
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable(geometry.get());
    
    // ... 添加到场景图
}

void OsgVerseViewerImpl::applyMaterial(
    osg::Geometry* geometry,
    ViewProvider* vp
) {
    // 获取颜色
    App::Color color = vp->ShapeColor.getValue();
    float transparency = vp->Transparency.getValue();
    
    // 创建材质
    osg::ref_ptr<osg::Material> material = new osg::Material();
    material->setDiffuse(osg::Material::FRONT_AND_BACK,
                        osg::Vec4(color.r, color.g, color.b, 1.0 - transparency));
    material->setAmbient(osg::Material::FRONT_AND_BACK,
                        osg::Vec4(color.r * 0.5, color.g * 0.5, color.b * 0.5, 1.0));
    material->setSpecular(osg::Material::FRONT_AND_BACK,
                         osg::Vec4(1.0, 1.0, 1.0, 1.0));
    material->setShininess(osg::Material::FRONT_AND_BACK, 64.0);
    
    // 应用到几何体
    osg::StateSet* stateSet = geometry->getOrCreateStateSet();
    stateSet->setAttribute(material.get());
    
    // 处理透明度
    if (transparency > 0.0) {
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    }
}
```

## 风险和缓解措施

### 风险 1：性能问题
**缓解**：
- 实现几何体缓存
- 使用 OSG 优化器
- 分批处理大型模型

### 风险 2：显示质量不如 Coin3D
**缓解**：
- 调整三角剖分参数
- 优化法线计算
- 参考 Coin3D 的实现

### 风险 3：内存占用过高
**缓解**：
- 使用共享顶点
- 实现几何体压缩
- 按需加载

### 风险 4：兼容性问题
**缓解**：
- 全面测试各种几何体类型
- 处理边界情况
- 提供降级方案

## 时间估算

| 步骤 | 预计时间 | 关键里程碑 |
|------|---------|-----------|
| Step 1: 几何体转换基础 | 2-3 天 | 简单几何体显示正确 |
| Step 2: 材质和颜色 | 1-2 天 | 颜色和透明度正常 |
| Step 3: 复杂几何体 | 2-3 天 | 复杂模型正确显示 |
| Step 4: 动态更新 | 1-2 天 | 属性变化实时反映 |
| Step 5: 测试和优化 | 1-2 天 | 性能达标 |
| **总计** | **7-12 天** | **Phase 2 完成** |

## 成功标准

### 功能完整性
- ✓ 所有基本几何体正确显示（Box、Cylinder、Sphere、Cone 等）
- ✓ 复杂 CAD 模型正确显示
- ✓ 颜色和材质正确应用
- ✓ 透明度正常工作
- ✓ 边缘显示清晰
- ✓ 支持不同显示模式

### 性能要求
- ✓ 简单模型（<10K 三角形）：60+ FPS
- ✓ 中等模型（10K-100K 三角形）：30+ FPS
- ✓ 复杂模型（>100K 三角形）：可接受的帧率
- ✓ 内存使用合理（与 Coin3D 相当）

### 质量要求
- ✓ 显示质量与 Coin3D 相当
- ✓ 无明显的视觉瑕疵
- ✓ 边缘清晰
- ✓ 光照和阴影正确

## 下一步行动

### 立即开始
1. 创建 `GeometryConverter` 类框架
2. 实现基本的三角剖分
3. 测试 Box 几何体转换

### 第一个里程碑
**目标**：显示一个真实的 Box（不是红色球体）
**时间**：1-2 天
**验证**：
```python
import FreeCAD
import FreeCADGui

doc = FreeCAD.newDocument()
box = doc.addObject("Part::Box", "Box")
doc.recompute()

FreeCADGui.switchRenderBackend(2)  # OsgVerse
FreeCADGui.SendMsgToActiveView("ViewFit")

# 应该看到一个真实的立方体，而不是红色球体
```

## 参考资源

### OCCT 文档
- BRepMesh_IncrementalMesh
- TopExp_Explorer
- TopoDS_Face, TopoDS_Edge

### OSG 文档
- osg::Geometry
- osg::Material
- osg::StateSet

### FreeCAD 现有实现
- Coin3D 后端的几何体转换
- ViewProviderPartExt
- SoFCSelection

## 总结

Phase 2 是从占位符到真实渲染的关键一步。通过系统的实施计划和清晰的里程碑，我们将逐步实现：

1. **基础转换**：TopoShape → OSG Geometry
2. **材质支持**：颜色、透明度、显示模式
3. **复杂几何体**：装配体、大型模型
4. **动态更新**：实时属性变化
5. **性能优化**：达到生产级别

完成 Phase 2 后，OsgVerse 后端将具备与 Coin3D 相当的基本渲染能力，为后续的高级特性（Phase 3+）打下坚实基础。
