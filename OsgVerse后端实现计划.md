# FreeCAD OsgVerse 后端实现计划

## 📋 项目概述

**项目名称**：FreeCAD 渲染抽象层 - OsgVerse 后端实现
**目标**：为 FreeCAD 的渲染抽象层实现 OsgVerse 后端，提供现代化的 PBR 渲染能力
**OsgVerse 路径**：E:\Repository\OSGVerse\osgverse
**预计工期**：4-6 周
**优先级**：Phase 2（Coin3D 后端已完成）

## 🎯 项目目标

### 核心目标

1. **实现完整的 OsgVerse 后端**
   - 节点包装器（Node, Group, Transform, Switch 等）
   - 材质系统（PBR 材质支持）
   - 几何体管理
   - 渲染引擎集成

2. **保持 API 一致性**
   - 与 Coin3D 后端保持相同的抽象接口
   - 确保可以无缝切换渲染后端
   - 维护向后兼容性

3. **利用 OsgVerse 现代特性**
   - PBR（物理基础渲染）
   - 延迟渲染管线
   - 实时阴影
   - 高级光照系统

4. **性能优化**
   - 几何体合并和批处理
   - GPU 驱动渲染
   - 纹理图集
   - LOD 支持

## 🔍 OsgVerse 代码库分析

### 目录结构

```
E:\Repository\OSGVerse\osgverse\
├── pipeline/          # 现代渲染管线（PBR、延迟着色、阴影）
├── readerwriter/      # 文件 I/O（GLTF、FBX、KTX 等）
├── modeling/          # 几何处理、网格操作
├── animation/         # 物理、角色动画、粒子系统
├── ai/                # 导航网格、AI 工具
├── ui/                # ImGui 集成、场景层次、UI 组件
├── script/            # 脚本支持（Python、JSON）
├── wrappers/          # OSG 序列化包装器
├── applications/      # 查看器应用和示例
├── plugins/           # OSG 风格的读写插件
├── assets/            # 着色器、模型、纹理、天空盒
└── 3rdparty/          # 嵌入式依赖（Eigen、Bullet 等）
```

### 核心模块依赖关系

```
Dependency (base)
├── Modeling (依赖: Dependency)
├── Pipeline (依赖: Dependency, Modeling)
├── Script (依赖: Dependency, Pipeline)
├── AI (依赖: Dependency, Modeling)
├── Animation (依赖: Dependency, Pipeline, Modeling)
├── UI (依赖: Dependency, Modeling, Script)
├── ReaderWriter (依赖: Dependency, Animation, Modeling, Pipeline)
└── Wrappers (依赖: ALL)
```

### Coin3D vs OsgVerse API 映射

| Coin3D 概念 | OsgVerse 等价物 | 位置 | 说明 |
|------------|----------------|------|------|
| SoNode | osg::Node | OSG base | OsgVerse 通过组件扩展 |
| SoGroup | osg::Group | OSG base | 用于场景层次 |
| SoSeparator | osg::Group | OSG base | 通过 StateSet 隔离 |
| SoTransform | osg::MatrixTransform | OSG base | 基于矩阵的变换 |
| SoSwitch | osg::Switch | OSG base | 显示模式切换 |
| SoMaterial | osg::StateSet + Uniforms | pipeline/ | 通过 uniforms 实现 PBR 材质 |
| SoGeometry | osg::Geometry | OSG base | 顶点/索引数据 |
| SoDrawStyle | osg::PolygonMode | OSG base | 渲染样式 |
| SoLightSource | LightDrawable | pipeline/LightDrawable.h | 延迟光照 |
| SoCamera | osg::Camera | OSG base | 渲染相机 |
| SoViewer | StandardPipelineViewer | pipeline/Pipeline.h | 快速查看器设置 |

### 关键头文件

#### Pipeline 模块
- **Pipeline.h** - 核心渲染管线，支持延迟着色
- **Global.h** - 全局定义和组件系统
- **ShaderLibrary.h** - 着色器管理和编译
- **LightModule.h** - 光照系统
- **ShadowModule.h** - 阴影映射系统

#### ReaderWriter 模块
- **LoadSceneGLTF.h** - GLTF/GLB 格式加载
- **MaterialGraph.h** - Blender 材质图导入

#### Modeling 模块
- **GeometryMerger.h** - 几何优化和合并

#### Animation 模块
- **PhysicsEngine.h** - Bullet3 物理集成

## 📐 架构设计

### 整体架构

```
FreeCAD 应用层
    ↓
渲染抽象层 (RenderNode, RenderEngine)
    ↓
┌─────────────────┬─────────────────┐
│  Coin3D 后端    │  OsgVerse 后端  │
│  (已完成)       │  (待实现)       │
└─────────────────┴─────────────────┘
    ↓                   ↓
Coin3D 库          OSG + OsgVerse 库
```

### OsgVerse 后端类层次

```
RenderNode (抽象基类)
    ↓
OsgVerseNode (基础包装器)
    ├── OsgVerseGroup (分组节点)
    │   ├── OsgVerseSeparator (分隔节点)
    │   └── OsgVerseSwitch (切换节点)
    ├── OsgVerseTransform (变换节点)
    ├── OsgVerseMaterial (材质节点)
    ├── OsgVerseGeometry (几何体节点)
    ├── OsgVerseLight (光源节点)
    └── OsgVerseCamera (相机节点)
```

## 📝 实现计划

### Phase 1: 基础设施搭建（第 1-2 周）

#### 1.1 项目配置
- [ ] 创建 OsgVerse 后端目录结构
- [ ] 配置 CMake 构建系统
- [ ] 添加 OsgVerse 依赖检测
- [ ] 设置编译宏和包含路径

**文件清单**：
```
src/Gui/Render/Backends/OsgVerse/
├── CMakeLists.txt
├── OsgVerseNode.h
├── OsgVerseNode.cpp
├── OsgVerseEngine.h
├── OsgVerseEngine.cpp
├── OsgVerseMaterial.h
├── OsgVerseMaterial.cpp
├── OsgVerseGeometry.h
├── OsgVerseGeometry.cpp
├── OsgVerseUtils.h
├── OsgVerseUtils.cpp
└── OsgVerseViewer.h
    OsgVerseViewer.cpp
```

**CMake 配置**：
```cmake
# 检测 OsgVerse
find_package(OpenSceneGraph REQUIRED COMPONENTS osgDB osgUtil osgViewer)
find_path(OSGVERSE_INCLUDE_DIR
    NAMES osgverse/pipeline/Pipeline.h
    PATHS E:/Repository/OSGVerse/osgverse
)

if(OSGVERSE_INCLUDE_DIR)
    option(BUILD_WITH_OSGVERSE "Build with OsgVerse backend" ON)

    if(BUILD_WITH_OSGVERSE)
        target_compile_definitions(FreeCADGui PRIVATE
            RENDER_HAS_OSGVERSE_BACKEND
        )
        target_include_directories(FreeCADGui PRIVATE
            ${OSGVERSE_INCLUDE_DIR}
            ${OPENSCENEGRAPH_INCLUDE_DIRS}
        )
        target_link_libraries(FreeCADGui PRIVATE
            ${OPENSCENEGRAPH_LIBRARIES}
        )
    endif()
endif()
```

#### 1.2 基础节点包装器
- [ ] 实现 OsgVerseNode 基类
- [ ] 实现 OsgVerseGroup 类
- [ ] 实现 OsgVerseSeparator 类
- [ ] 实现 OsgVerseTransform 类

**OsgVerseNode.h 框架**：
```cpp
#ifndef GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSENODE_H
#define GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSENODE_H

#include <memory>
#include <osg/Node>
#include <osg/Group>
#include <osg/MatrixTransform>
#include "../../Core/RenderNode.h"

namespace Gui {
namespace Render {

/**
 * @brief OsgVerse 节点包装器基类
 */
class GuiExport OsgVerseNode : public RenderNode {
public:
    OsgVerseNode(osg::Node* osgNode, bool ownsNode, NodeType type);
    ~OsgVerseNode() override;

    // 获取底层 OSG 节点
    osg::Node* getOsgNode() const { return _osgNode.get(); }

    // RenderNode 接口实现
    void touch() override;

protected:
    void* getBackendNodeImpl() override { return _osgNode.get(); }
    const void* getBackendNodeImpl() const override { return _osgNode.get(); }

    osg::ref_ptr<osg::Node> _osgNode;
    bool _ownsNode;
};

/**
 * @brief OsgVerse Group 包装器
 */
class GuiExport OsgVerseGroup : public OsgVerseNode {
public:
    RENDER_NODE_STATIC_TYPE(NodeType::Group)

    OsgVerseGroup();
    explicit OsgVerseGroup(osg::Group* group, bool ownsNode = false);
    ~OsgVerseGroup() override;

    // 子节点管理
    void addChild(RenderNode::Ptr child);
    void insertChild(size_t index, RenderNode::Ptr child);
    bool removeChild(RenderNode* child);
    void removeAllChildren();

    size_t getNumChildren() const { return _children.size(); }
    RenderNode* getChild(size_t index) const;

    osg::Group* getOsgGroup() const;

protected:
    std::vector<RenderNode::Ptr> _children;
};

// ... 其他类声明

} // namespace Render
} // namespace Gui

#endif
```

#### 1.3 单元测试框架
- [ ] 创建 OsgVerse 后端测试目录
- [ ] 编写基础节点测试
- [ ] 编写场景图构建测试

### Phase 2: 核心功能实现（第 3-4 周）

#### 2.1 材质系统
- [ ] 实现 OsgVerseMaterial 类
- [ ] PBR 材质参数映射
- [ ] 纹理管理
- [ ] StateSet 配置

**材质参数映射**：
```cpp
// Coin3D 材质 -> OsgVerse PBR 材质
struct MaterialMapping {
    // 基础颜色
    Color diffuseColor;      -> baseColor uniform

    // 金属度/粗糙度
    float shininess;         -> roughness uniform (inverse)
    float transparency;      -> alpha channel

    // 纹理
    Texture* diffuseMap;     -> baseColorTexture
    Texture* normalMap;      -> normalTexture
    Texture* specularMap;    -> metallicRoughnessTexture
};
```

#### 2.2 几何体系统
- [ ] 实现 OsgVerseGeometry 类
- [ ] 顶点数据转换
- [ ] 索引缓冲管理
- [ ] 几何体优化（合并、LOD）

**几何体转换**：
```cpp
// FreeCAD 几何 -> OSG 几何
class OsgVerseGeometry {
    // 顶点属性
    void setVertices(const std::vector<Vec3f>& vertices);
    void setNormals(const std::vector<Vec3f>& normals);
    void setTexCoords(const std::vector<Vec2f>& texCoords);
    void setColors(const std::vector<Color>& colors);

    // 索引
    void setIndices(const std::vector<uint32_t>& indices);

    // 图元类型
    void setPrimitiveType(PrimitiveType type);

    // 优化
    void optimize();  // 使用 GeometryMerger
};
```

#### 2.3 渲染引擎
- [ ] 实现 OsgVerseEngine 类
- [ ] 渲染管线初始化
- [ ] 节点工厂方法
- [ ] 全局状态管理

**OsgVerseEngine.h 框架**：
```cpp
class GuiExport OsgVerseEngine : public RenderEngine {
public:
    OsgVerseEngine();
    ~OsgVerseEngine() override;

    // RenderEngine 接口
    std::string getName() const override { return "OsgVerse"; }
    RenderNode::Ptr createNode(RenderNode::NodeType type) override;
    void initialize() override;
    void shutdown() override;

    // OsgVerse 特定功能
    osgVerse::Pipeline* getPipeline() const { return _pipeline.get(); }
    void setupStandardPipeline(osgViewer::View* view);

    // 光照和阴影
    void addLight(const LightParameters& params);
    void enableShadows(bool enable);

private:
    osg::ref_ptr<osgVerse::Pipeline> _pipeline;
    osg::ref_ptr<osgVerse::LightModule> _lightModule;
    osg::ref_ptr<osgVerse::ShadowModule> _shadowModule;
    bool _initialized{false};
};
```

#### 2.4 查看器集成
- [ ] 实现 OsgVerseViewer 类
- [ ] 与 View3DInventorViewer 集成
- [ ] 事件处理
- [ ] 相机控制

### Phase 3: 高级特性（第 5 周）

#### 3.1 PBR 渲染管线
- [ ] 配置延迟渲染管线
- [ ] G-Buffer 设置
- [ ] 光照计算
- [ ] 后处理效果

**管线配置**：
```cpp
void OsgVerseEngine::setupStandardPipeline(osgViewer::View* view) {
    // 创建管线
    _pipeline = new osgVerse::Pipeline(4, 6);  // GL 4.6, GLSL 460

    // 标准管线参数
    osgVerse::StandardPipelineParameters params;
    params.shaderDir = "path/to/shaders";
    params.skyboxFile = "path/to/skybox.hdr";
    params.enablePBR = true;
    params.enableDeferred = true;
    params.enableShadows = true;

    // 设置管线
    osgVerse::setupStandardPipeline(_pipeline.get(), view, params);

    // 添加光照模块
    _lightModule = new osgVerse::LightModule("Lights", _pipeline.get());
    _pipeline->addModule("Lights", _lightModule.get());

    // 添加阴影模块
    _shadowModule = new osgVerse::ShadowModule("Shadows", _pipeline.get(), false);
    _pipeline->addModule("Shadows", _shadowModule.get());
}
```

#### 3.2 光照系统
- [ ] 点光源支持
- [ ] 方向光支持
- [ ] 聚光灯支持
- [ ] 环境光和 IBL

#### 3.3 阴影系统
- [ ] PCF 阴影
- [ ] 方差阴影映射
- [ ] 级联阴影映射
- [ ] 阴影优化

#### 3.4 性能优化
- [ ] 几何体批处理
- [ ] 纹理图集
- [ ] GPU 驱动渲染
- [ ] 视锥剔除优化

### Phase 4: 集成和测试（第 6 周）

#### 4.1 ViewProvider 集成
- [ ] 修改 ViewProvider::getRenderRoot()
- [ ] 支持运行时后端切换
- [ ] 测试双后端共存

**后端切换机制**：
```cpp
// ViewProvider.cpp
Render::RenderNode* ViewProvider::getRenderRoot() const {
    if (!pcRoot) {
        return nullptr;
    }

    // 获取当前渲染后端
    std::string backend = RenderEngine::getCurrentBackend();

    if (backend == "OsgVerse") {
        // 使用 OsgVerse 包装器
        static std::unordered_map<SoSeparator*,
            std::shared_ptr<Gui::Render::OsgVerseSeparator>> osgCache;

        auto it = osgCache.find(pcRoot);
        if (it != osgCache.end()) {
            return it->second.get();
        }

        // 转换 Coin3D 场景图到 OSG
        auto wrapper = convertCoin3DToOsgVerse(pcRoot);
        osgCache[pcRoot] = wrapper;
        return wrapper.get();
    }
    else {
        // 使用 Coin3D 包装器（默认）
        // ... 现有代码
    }
}
```

#### 4.2 场景图转换
- [ ] Coin3D -> OsgVerse 转换器
- [ ] 材质转换
- [ ] 几何体转换
- [ ] 变换转换

**转换器实现**：
```cpp
namespace Gui {
namespace Render {

class Coin3DToOsgVerseConverter {
public:
    static OsgVerseSeparator::Ptr convert(SoSeparator* coinRoot);

private:
    static osg::Node* convertNode(SoNode* coinNode);
    static osg::Group* convertGroup(SoGroup* coinGroup);
    static osg::MatrixTransform* convertTransform(SoTransform* coinTransform);
    static osg::StateSet* convertMaterial(SoMaterial* coinMaterial);
    static osg::Geometry* convertGeometry(SoGeometry* coinGeometry);
};

} // namespace Render
} // namespace Gui
```

#### 4.3 功能测试
- [ ] 基本渲染测试
- [ ] 材质显示测试
- [ ] 光照和阴影测试
- [ ] 性能基准测试

#### 4.4 文档编写
- [ ] API 文档
- [ ] 使用指南
- [ ] 性能对比报告
- [ ] 迁移指南

## 📊 文件清单

### 新增文件（预计 20+ 个）

```
src/Gui/Render/Backends/OsgVerse/
├── CMakeLists.txt                    # 构建配置
├── OsgVerseNode.h                    # 节点包装器声明
├── OsgVerseNode.cpp                  # 节点包装器实现
├── OsgVerseEngine.h                  # 引擎声明
├── OsgVerseEngine.cpp                # 引擎实现
├── OsgVerseMaterial.h                # 材质系统声明
├── OsgVerseMaterial.cpp              # 材质系统实现
├── OsgVerseGeometry.h                # 几何体系统声明
├── OsgVerseGeometry.cpp              # 几何体系统实现
├── OsgVerseUtils.h                   # 工具函数声明
├── OsgVerseUtils.cpp                 # 工具函数实现
├── OsgVerseViewer.h                  # 查看器包装声明
├── OsgVerseViewer.cpp                # 查看器包装实现
├── Coin3DToOsgVerseConverter.h       # 场景图转换器声明
├── Coin3DToOsgVerseConverter.cpp     # 场景图转换器实现
└── tests/
    ├── CMakeLists.txt
    ├── test_osgverse_node.cpp
    ├── test_osgverse_material.cpp
    └── test_osgverse_converter.cpp
```

### 修改文件

```
src/Gui/
├── CMakeLists.txt                    # 添加 OsgVerse 选项
├── ViewProvider.cpp                  # 支持 OsgVerse 后端
└── Render/
    ├── CMakeLists.txt                # 添加 OsgVerse 源文件
    └── Core/
        └── RenderEngine.cpp          # 注册 OsgVerse 引擎
```

## 🔧 技术挑战和解决方案

### 挑战 1: Coin3D 与 OSG 场景图差异

**问题**：
- Coin3D 使用引用计数（ref/unref）
- OSG 使用智能指针（osg::ref_ptr）
- 场景图结构略有不同

**解决方案**：
- 使用适配器模式包装 OSG 节点
- 维护双向映射表（Coin3D <-> OSG）
- 实现延迟转换（按需转换）

### 挑战 2: 材质系统差异

**问题**：
- Coin3D 使用固定管线材质
- OsgVerse 使用 PBR 材质和 uniforms

**解决方案**：
- 创建材质参数映射表
- 实现自动材质转换
- 提供手动材质调整接口

### 挑战 3: 性能优化

**问题**：
- 场景图转换开销
- 双后端内存占用

**解决方案**：
- 使用缓存减少重复转换
- 实现增量更新机制
- 提供后端独占模式

### 挑战 4: 向后兼容性

**问题**：
- 现有代码直接访问 Coin3D 节点
- 需要保持 API 稳定性

**解决方案**：
- 保持 Coin3D 后端为默认
- 提供渐进式迁移路径
- 双后端共存支持

## 📈 性能目标

### 渲染性能

| 指标 | Coin3D 基准 | OsgVerse 目标 | 提升 |
|------|------------|--------------|------|
| FPS (复杂场景) | 30 | 60+ | 2x |
| 绘制调用数 | 1000+ | 100- | 10x |
| 内存占用 | 基准 | 1.2x | 可接受 |
| 启动时间 | 基准 | 1.5x | 可接受 |

### 质量目标

- ✅ PBR 材质渲染
- ✅ 实时阴影
- ✅ 环境光遮蔽
- ✅ 抗锯齿（MSAA/FXAA）
- ✅ 后处理效果

## 🧪 测试策略

### 单元测试
- 节点创建和销毁
- 场景图操作
- 材质参数设置
- 几何体转换

### 集成测试
- ViewProvider 集成
- 后端切换
- 场景图转换
- 渲染输出验证

### 性能测试
- FPS 基准测试
- 内存占用测试
- 启动时间测试
- 大场景压力测试

### 兼容性测试
- 现有 FreeCAD 文档加载
- 各模块功能验证
- 跨平台测试（Windows/Linux/macOS）

## 📚 依赖项

### 必需依赖
- OpenSceneGraph 3.6+
- OsgVerse (E:\Repository\OSGVerse\osgverse)
- C++17 编译器
- CMake 3.16+

### 可选依赖
- Bullet3（物理引擎）
- ImGui（调试 UI）
- GLTF 加载器

## 🚀 里程碑

### Milestone 1: 基础设施（第 2 周末）
- ✅ 项目配置完成
- ✅ 基础节点包装器实现
- ✅ 单元测试框架搭建

### Milestone 2: 核心功能（第 4 周末）
- ✅ 材质系统实现
- ✅ 几何体系统实现
- ✅ 渲染引擎实现
- ✅ 查看器集成

### Milestone 3: 高级特性（第 5 周末）
- ✅ PBR 渲染管线
- ✅ 光照和阴影系统
- ✅ 性能优化

### Milestone 4: 集成和发布（第 6 周末）
- ✅ ViewProvider 集成
- ✅ 场景图转换
- ✅ 完整测试
- ✅ 文档完成

## 📖 参考资料

### OsgVerse 文档
- OsgVerse GitHub: https://github.com/xarray/osgverse
- Pipeline 文档: E:\Repository\OSGVerse\osgverse\pipeline\README.md
- 示例代码: E:\Repository\OSGVerse\osgverse\applications\

### OpenSceneGraph 文档
- OSG 官网: http://www.openscenegraph.org/
- OSG 教程: https://github.com/openscenegraSceneGraph
- OSG Quick Start Guide

### FreeCAD 文档
- 渲染抽象层设计: FreeCAD_Architecture.html
- Coin3D 后端实现: 渲染抽象层实现总结.md
- ViewProvider API: src/Gui/ViewProvider.h

## 🎯 成功标准

### 功能完整性
- ✅ 所有 RenderNode 接口实现
- ✅ 材质系统完整
- ✅ 几何体渲染正确
- ✅ 光照和阴影工作

### 性能达标
- ✅ FPS 提升 2x 以上
- ✅ 绘制调用减少 10x
- ✅ 内存增加不超过 20%

### 质量保证
- ✅ 所有单元测试通过
- ✅ 集成测试通过
- ✅ 无内存泄漏
- ✅ 无崩溃和错误

### 文档完善
- ✅ API 文档完整
- ✅ 使用指南清晰
- ✅ 示例代码可运行

## 🔄 后续工作

### Phase 5: 优化和完善
- 进一步性能优化
- 更多后处理效果
- 高级材质特性
- VR/AR 支持

### Phase 6: 生态系统
- 材质编辑器
- 光照编辑器
- 场景导出工具
- 渲染设置面板

---

**文档版本**：1.0
**创建日期**：2026年1月18日
**作者**：FreeCAD 开发团队
**状态**：📋 计划阶段
