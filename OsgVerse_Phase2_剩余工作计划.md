# FreeCAD OsgVerse 后端 - Phase 2 剩余工作实施计划

## 📋 概述

**当前状态**：Phase 2 核心组件 60% 完成
**剩余工作**：3个主要组件 + 测试
**预计时间**：3-5 天
**优先级**：高

## ✅ 已完成工作回顾

### 完成的组件
1. ✅ **OsgVerseNode** - 基础节点包装器（Phase 1）
2. ✅ **OsgVerseMaterial** - 完整的 PBR 材质系统
3. ✅ **OsgVerseGeometry.h** - 几何体系统头文件

### 代码统计
- 已完成文件：8 个
- 已完成代码：~3,370 行
- 完成度：60%

## 🚧 剩余工作清单

### 1. OsgVerseGeometry.cpp 实现 ⏳

**优先级**：高
**预计时间**：1-2 天
**代码量**：~1,000 行

#### 需要实现的功能

##### 1.1 基础构造和析构
```cpp
OsgVerseGeometry::OsgVerseGeometry()
    : OsgVerseNode(new osg::Geometry(), true, NodeType::Geometry)
{
}

OsgVerseGeometry::~OsgVerseGeometry() = default;

osg::Geometry* OsgVerseGeometry::getOsgGeometry() const
{
    return dynamic_cast<osg::Geometry*>(_osgNode.get());
}
```

##### 1.2 顶点数据管理
```cpp
void OsgVerseGeometry::setVertices(const std::vector<Vec3f>& vertices)
{
    auto* geom = getOsgGeometry();
    if (!geom) return;

    auto vertexArray = new osg::Vec3Array(vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        (*vertexArray)[i].set(vertices[i].x, vertices[i].y, vertices[i].z);
    }
    geom->setVertexArray(vertexArray);
}

void OsgVerseGeometry::setNormals(const std::vector<Vec3f>& normals)
{
    auto* geom = getOsgGeometry();
    if (!geom) return;

    auto normalArray = new osg::Vec3Array(normals.size());
    for (size_t i = 0; i < normals.size(); ++i) {
        (*normalArray)[i].set(normals[i].x, normals[i].y, normals[i].z);
    }
    geom->setNormalArray(normalArray);
    geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
}

void OsgVerseGeometry::setTexCoords(const std::vector<Vec2f>& texCoords, unsigned int unit)
{
    auto* geom = getOsgGeometry();
    if (!geom) return;

    auto texCoordArray = new osg::Vec2Array(texCoords.size());
    for (size_t i = 0; i < texCoords.size(); ++i) {
        (*texCoordArray)[i].set(texCoords[i].x, texCoords[i].y);
    }
    geom->setTexCoordArray(unit, texCoordArray);
}

void OsgVerseGeometry::setColors(const std::vector<Color>& colors)
{
    auto* geom = getOsgGeometry();
    if (!geom) return;

    auto colorArray = new osg::Vec4Array(colors.size());
    for (size_t i = 0; i < colors.size(); ++i) {
        (*colorArray)[i].set(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
    }
    geom->setColorArray(colorArray);
    geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
}
```

##### 1.3 索引管理
```cpp
void OsgVerseGeometry::setIndices(const std::vector<uint32_t>& indices)
{
    auto* geom = getOsgGeometry();
    if (!geom) return;

    auto indexArray = new osg::DrawElementsUInt(
        primitiveTypeToGL(_primitiveType),
        indices.size()
    );

    for (size_t i = 0; i < indices.size(); ++i) {
        (*indexArray)[i] = indices[i];
    }

    geom->addPrimitiveSet(indexArray);
}
```

##### 1.4 几何体操作
```cpp
void OsgVerseGeometry::computeNormals()
{
    auto* geom = getOsgGeometry();
    if (!geom) return;

    osgUtil::SmoothingVisitor sv;
    geom->accept(sv);
}

void OsgVerseGeometry::computeTangents()
{
    auto* geom = getOsgGeometry();
    if (!geom) return;

    osgUtil::TangentSpaceGenerator tsg;
    tsg.generate(geom);

    geom->setVertexAttribArray(6, tsg.getTangentArray());
    geom->setVertexAttribBinding(6, osg::Geometry::BIND_PER_VERTEX);

    geom->setVertexAttribArray(7, tsg.getBinormalArray());
    geom->setVertexAttribBinding(7, osg::Geometry::BIND_PER_VERTEX);
}

void OsgVerseGeometry::optimize()
{
    auto* geom = getOsgGeometry();
    if (!geom) return;

    // 使用 OSG 的几何体优化器
    osgUtil::Optimizer optimizer;
    optimizer.optimize(geom,
        osgUtil::Optimizer::VERTEX_PRETRANSFORM |
        osgUtil::Optimizer::VERTEX_POSTTRANSFORM |
        osgUtil::Optimizer::INDEX_MESH);
}
```

##### 1.5 几何体构建器
```cpp
std::unique_ptr<OsgVerseGeometry> OsgVerseGeometryBuilder::createCube(float size)
{
    auto geometry = std::make_unique<OsgVerseGeometry>();

    float hs = size * 0.5f; // half size

    // 24 个顶点（每个面 4 个）
    std::vector<Vec3f> vertices = {
        // Front face
        {-hs, -hs,  hs}, { hs, -hs,  hs}, { hs,  hs,  hs}, {-hs,  hs,  hs},
        // Back face
        {-hs, -hs, -hs}, {-hs,  hs, -hs}, { hs,  hs, -hs}, { hs, -hs, -hs},
        // Top face
        {-hs,  hs, -hs}, {-hs,  hs,  hs}, { hs,  hs,  hs}, { hs,  hs, -hs},
        // Bottom face
        {-hs, -hs, -hs}, { hs, -hs, -hs}, { hs, -hs,  hs}, {-hs, -hs,  hs},
        // Right face
        { hs, -hs, -hs}, { hs,  hs, -hs}, { hs,  hs,  hs}, { hs, -hs,  hs},
        // Left face
        {-hs, -hs, -hs}, {-hs, -hs,  hs}, {-hs,  hs,  hs}, {-hs,  hs, -hs}
    };

    // 法线
    std::vector<Vec3f> normals = {
        // Front
        {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1},
        // Back
        {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},
        // Top
        {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0},
        // Bottom
        {0,0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0},
        // Right
        {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0},
        // Left
        {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}
    };

    // 索引（36 个索引，12 个三角形）
    std::vector<uint32_t> indices = {
        0, 1, 2,  0, 2, 3,    // Front
        4, 5, 6,  4, 6, 7,    // Back
        8, 9, 10, 8, 10, 11,  // Top
        12, 13, 14, 12, 14, 15, // Bottom
        16, 17, 18, 16, 18, 19, // Right
        20, 21, 22, 20, 22, 23  // Left
    };

    geometry->setVertices(vertices);
    geometry->setNormals(normals);
    geometry->setIndices(indices);
    geometry->setPrimitiveType(PrimitiveType::Triangles);

    return geometry;
}

std::unique_ptr<OsgVerseGeometry> OsgVerseGeometryBuilder::createSphere(
    float radius, unsigned int segments)
{
    auto geometry = std::make_unique<OsgVerseGeometry>();

    std::vector<Vec3f> vertices;
    std::vector<Vec3f> normals;
    std::vector<Vec2f> texCoords;
    std::vector<uint32_t> indices;

    // 生成球体顶点
    for (unsigned int lat = 0; lat <= segments; ++lat) {
        float theta = lat * M_PI / segments;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        for (unsigned int lon = 0; lon <= segments; ++lon) {
            float phi = lon * 2 * M_PI / segments;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            Vec3f normal{cosPhi * sinTheta, cosTheta, sinPhi * sinTheta};
            Vec3f vertex{radius * normal.x, radius * normal.y, radius * normal.z};
            Vec2f texCoord{(float)lon / segments, (float)lat / segments};

            vertices.push_back(vertex);
            normals.push_back(normal);
            texCoords.push_back(texCoord);
        }
    }

    // 生成索引
    for (unsigned int lat = 0; lat < segments; ++lat) {
        for (unsigned int lon = 0; lon < segments; ++lon) {
            unsigned int first = lat * (segments + 1) + lon;
            unsigned int second = first + segments + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    geometry->setVertices(vertices);
    geometry->setNormals(normals);
    geometry->setTexCoords(texCoords);
    geometry->setIndices(indices);
    geometry->setPrimitiveType(PrimitiveType::Triangles);

    return geometry;
}
```

### 2. OsgVerseEngine 实现 ⏳

**优先级**：高
**预计时间**：2-3 天
**代码量**：~800 行

#### 需要实现的功能

##### 2.1 引擎头文件框架
```cpp
// OsgVerseEngine.h
class GuiExport OsgVerseEngine : public RenderEngine {
public:
    OsgVerseEngine();
    ~OsgVerseEngine() override;

    // RenderEngine 接口
    std::string getName() const override { return "OsgVerse"; }
    RenderNode::Ptr createNode(NodeType type) override;
    void initialize() override;
    void shutdown() override;

    // OsgVerse 特定功能
    osgVerse::Pipeline* getPipeline() const { return _pipeline.get(); }
    void setupStandardPipeline(osgViewer::View* view);

    // 光照管理
    void addLight(const LightParameters& params);
    void removeLight(const std::string& name);
    void setMainLight(const std::string& name);

    // 阴影管理
    void enableShadows(bool enable);
    void setShadowQuality(ShadowQuality quality);

    // 全局状态
    void setAmbientLight(const Color& color);
    void setSkybox(const std::string& filename);

private:
    osg::ref_ptr<osgVerse::Pipeline> _pipeline;
    osg::ref_ptr<osgVerse::LightModule> _lightModule;
    osg::ref_ptr<osgVerse::ShadowModule> _shadowModule;
    bool _initialized{false};
};
```

##### 2.2 节点工厂实现
```cpp
RenderNode::Ptr OsgVerseEngine::createNode(NodeType type)
{
    switch (type) {
        case NodeType::Group:
            return std::make_shared<OsgVerseGroup>();

        case NodeType::Separator:
            return std::make_shared<OsgVerseSeparator>();

        case NodeType::Transform:
            return std::make_shared<OsgVerseTransform>();

        case NodeType::Switch:
            return std::make_shared<OsgVerseSwitch>();

        case NodeType::Geometry:
            return std::make_shared<OsgVerseGeometry>();

        default:
            Base::Console().Warning("OsgVerseEngine: Unknown node type\n");
            return nullptr;
    }
}
```

##### 2.3 Pipeline 初始化
```cpp
void OsgVerseEngine::initialize()
{
    if (_initialized) return;

    // 创建 Pipeline
    _pipeline = new osgVerse::Pipeline(4, 6); // GL 4.6, GLSL 460

    // 初始化光照模块
    _lightModule = new osgVerse::LightModule("Lights", _pipeline.get());
    _pipeline->addModule("Lights", _lightModule.get());

    // 初始化阴影模块
    _shadowModule = new osgVerse::ShadowModule("Shadows", _pipeline.get(), false);
    _pipeline->addModule("Shadows", _shadowModule.get());

    _initialized = true;
}

void OsgVerseEngine::setupStandardPipeline(osgViewer::View* view)
{
    if (!_initialized) {
        initialize();
    }

    // 标准 Pipeline 参数
    osgVerse::StandardPipelineParameters params;
    params.shaderDir = "path/to/shaders";
    params.enablePBR = true;
    params.enableDeferred = true;
    params.enableShadows = true;

    // 设置 Pipeline
    osgVerse::setupStandardPipeline(_pipeline.get(), view, params);
}
```

### 3. OsgVerseViewer 实现 ⏳

**优先级**：中
**预计时间**：2-3 天
**代码量**：~600 行

#### 需要实现的功能

##### 3.1 查看器头文件框架
```cpp
// OsgVerseViewer.h
class GuiExport OsgVerseViewer {
public:
    OsgVerseViewer();
    ~OsgVerseViewer();

    // 视图管理
    void setSceneData(osg::Node* scene);
    osg::Node* getSceneData() const;

    // 相机控制
    void setCameraManipulator(osgGA::CameraManipulator* manipulator);
    osgGA::CameraManipulator* getCameraManipulator() const;

    // 事件处理
    bool handleEvent(const osgGA::GUIEventAdapter& ea,
                     osgGA::GUIActionAdapter& aa);

    // 渲染
    void frame();
    void setFrameStamp(osg::FrameStamp* fs);

    // 与 FreeCAD 集成
    void setView3DInventorViewer(View3DInventorViewer* viewer);
    View3DInventorViewer* getView3DInventorViewer() const;

private:
    osg::ref_ptr<osgViewer::Viewer> _viewer;
    osg::ref_ptr<osgGA::CameraManipulator> _manipulator;
    View3DInventorViewer* _fcViewer{nullptr};
};
```

##### 3.2 基础实现
```cpp
OsgVerseViewer::OsgVerseViewer()
    : _viewer(new osgViewer::Viewer())
{
    // 设置默认相机操作器
    _manipulator = new osgGA::TrackballManipulator();
    _viewer->setCameraManipulator(_manipulator.get());
}

void OsgVerseViewer::setSceneData(osg::Node* scene)
{
    _viewer->setSceneData(scene);
}

void OsgVerseViewer::frame()
{
    _viewer->frame();
}

bool OsgVerseViewer::handleEvent(const osgGA::GUIEventAdapter& ea,
                                  osgGA::GUIActionAdapter& aa)
{
    // 转发事件到相机操作器
    if (_manipulator) {
        return _manipulator->handle(ea, aa);
    }
    return false;
}
```

### 4. 单元测试 ⏳

**优先级**：中
**预计时间**：1-2 天
**代码量**：~500 行

#### 测试文件结构
```
tests/
├── test_osgverse_material.cpp  (材质系统测试)
├── test_osgverse_geometry.cpp  (几何体系统测试)
├── test_osgverse_engine.cpp    (渲染引擎测试)
└── test_osgverse_integration.cpp (集成测试)
```

#### 测试用例示例
```cpp
// test_osgverse_material.cpp
TEST(OsgVerseMaterialTest, SetPBRParameters) {
    OsgVerseMaterial material;

    material.setBaseColor(Color{0.8f, 0.2f, 0.2f, 1.0f});
    material.setMetallic(0.5f);
    material.setRoughness(0.3f);

    EXPECT_EQ(material.getBaseColor().r, 0.8f);
    EXPECT_EQ(material.getMetallic(), 0.5f);
    EXPECT_EQ(material.getRoughness(), 0.3f);
}

TEST(OsgVerseMaterialTest, Coin3DConversion) {
    OsgVerseMaterial material;

    Material coinMat;
    coinMat.diffuse = Color{0.8f, 0.2f, 0.2f, 1.0f};
    coinMat.shininess = 64.0f;
    coinMat.transparency = 0.0f;

    material.setFromCoin3DMaterial(coinMat);

    EXPECT_EQ(material.getBaseColor().r, 0.8f);
    EXPECT_FLOAT_EQ(material.getRoughness(), 0.5f); // 1.0 - 64/128
}

// test_osgverse_geometry.cpp
TEST(OsgVerseGeometryTest, SetVertexData) {
    OsgVerseGeometry geometry;

    std::vector<Vec3f> vertices = {
        {0, 0, 0}, {1, 0, 0}, {0, 1, 0}
    };

    geometry.setVertices(vertices);

    EXPECT_EQ(geometry.getVertexCount(), 3);
}

TEST(OsgVerseGeometryTest, CreateCube) {
    auto cube = OsgVerseGeometryBuilder::createCube(2.0f);

    EXPECT_NE(cube, nullptr);
    EXPECT_EQ(cube->getVertexCount(), 24);
    EXPECT_EQ(cube->getIndexCount(), 36);
}
```

## 📊 工作量估算

### 代码量预估

| 组件 | 头文件 | 实现文件 | 测试文件 | 总计 |
|------|--------|----------|----------|------|
| OsgVerseGeometry | 完成 | ~1,000 | ~200 | ~1,200 |
| OsgVerseEngine | ~400 | ~800 | ~150 | ~1,350 |
| OsgVerseViewer | ~300 | ~600 | ~150 | ~1,050 |
| **总计** | **~700** | **~2,400** | **~500** | **~3,600** |

### 时间估算

| 任务 | 预计时间 | 优先级 |
|------|----------|--------|
| OsgVerseGeometry.cpp | 1-2 天 | 高 |
| OsgVerseEngine | 2-3 天 | 高 |
| OsgVerseViewer | 2-3 天 | 中 |
| 单元测试 | 1-2 天 | 中 |
| **总计** | **6-10 天** | - |

## 🎯 实施策略

### 第一周（Day 1-3）

**目标**：完成核心功能实现

1. **Day 1**: OsgVerseGeometry.cpp
   - 顶点数据管理
   - 索引管理
   - 基础几何体操作

2. **Day 2**: OsgVerseGeometry.cpp + 几何体构建器
   - 完成几何体操作
   - 实现 6 种基本形状构建器
   - 基础测试

3. **Day 3**: OsgVerseEngine 开始
   - 引擎头文件
   - 节点工厂
   - 基础初始化

### 第二周（Day 4-7）

**目标**：完成引擎和查看器

4. **Day 4-5**: OsgVerseEngine 完成
   - Pipeline 集成
   - 光照模块
   - 阴影模块

5. **Day 6-7**: OsgVerseViewer
   - 基础查看器
   - 事件处理
   - FreeCAD 集成

### 第三周（Day 8-10）

**目标**：测试和优化

6. **Day 8-9**: 单元测试
   - 材质系统测试
   - 几何体系统测试
   - 引擎测试

7. **Day 10**: 集成测试和文档
   - 集成测试
   - 文档完善
   - Bug 修复

## 📋 检查清单

### OsgVerseGeometry.cpp
- [ ] 构造和析构函数
- [ ] 顶点位置管理
- [ ] 法线管理
- [ ] 纹理坐标管理
- [ ] 顶点颜色管理
- [ ] 切线和副切线管理
- [ ] 索引管理
- [ ] 图元类型设置
- [ ] 计算法线
- [ ] 计算切线
- [ ] 计算边界框
- [ ] 几何体优化
- [ ] 反转法线/面
- [ ] Coin3D 转换
- [ ] 几何体构建器（6种形状）

### OsgVerseEngine
- [ ] 引擎初始化
- [ ] 节点工厂
- [ ] Pipeline 创建
- [ ] Pipeline 配置
- [ ] 光照模块集成
- [ ] 阴影模块集成
- [ ] 全局状态管理
- [ ] 资源管理

### OsgVerseViewer
- [ ] 查看器创建
- [ ] 场景数据设置
- [ ] 相机操作器
- [ ] 事件处理
- [ ] 渲染循环
- [ ] FreeCAD 集成接口

### 单元测试
- [ ] 材质系统测试（10+ 测试）
- [ ] 几何体系统测试（10+ 测试）
- [ ] 引擎测试（5+ 测试）
- [ ] 集成测试（5+ 测试）

## 🎉 完成标准

### Phase 2 完成条件

1. **代码完整性** ✅
   - 所有头文件和实现文件完成
   - 代码编译无错误
   - 无明显的内存泄漏

2. **功能完整性** ✅
   - 所有计划功能实现
   - API 接口完整
   - Coin3D 兼容性验证

3. **测试覆盖** ✅
   - 单元测试覆盖率 > 70%
   - 所有测试通过
   - 集成测试验证

4. **文档完善** ✅
   - API 文档完整
   - 使用示例清晰
   - 技术文档更新

## 📚 参考资料

### 代码参考
- OSG Geometry: http://www.openscenegraph.org/documentation/
- OsgVerse Pipeline: E:\Repository\OSGVerse\osgverse\pipeline\
- OsgVerse Examples: E:\Repository\OSGVerse\osgverse\applications\

### 技术文档
- OpenSceneGraph Quick Start Guide
- OsgVerse Pipeline Documentation
- PBR Rendering Theory

---

**文档版本**：1.0
**创建日期**：2026年1月18日
**作者**：FreeCAD 开发团队
**状态**：📋 实施计划 - 准备开始
