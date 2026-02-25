# FreeCAD 渲染抽象层设计方案

## 版本信息

| 项目 | 信息 |
|------|------|
| 文档版本 | 1.0 |
| 创建日期 | 2026-01-17 |
| 状态 | 设计草案 |

---

## 1. 概述

### 1.1 设计目标

1. **渲染引擎无关性**：允许在 **Coin3D** 和 **OsgVerse** 等渲染引擎间切换
2. **最小化破坏**：保持现有API兼容，渐进式迁移
3. **高性能**：抽象层开销尽可能小
4. **可扩展性**：便于添加新的渲染引擎后端

### 1.2 OsgVerse 简介

**OsgVerse** 是基于 OpenSceneGraph (OSG) 的现代渲染引擎，提供：

- **PBR（基于物理的渲染）**材质系统
- **延迟渲染**和**前向渲染**管线
- **实时阴影**（PCSS、CSM）
- **后处理效果**（SSAO、Bloom、Tone Mapping）
- **GPU Instancing**和**Culling**优化

与 Coin3D 相比的优势：

- 现代OpenGL特性支持（OpenGL 3.0+ / Vulkan）
- 更���的视觉效果和光照模型
- 更强的性能优化能力

---

## 2. 分层架构设计

```
┌─────────────────────────────────────────────────────────────┐
│                     Application Layer (App)                  │
│              DocumentObject, Feature, Geometry...            │
└──────────────────────────┬──────────────────────────────────┘
                           │ 信号/槽 + 属性更新
                           ▼
┌─────────────────────────────────────────────────────────────┐
│                      GUI Layer (Gui)                         │
│   ViewProvider, ViewProviderDocumentObject, Document...      │
└──────────────────────────┬──────────────────────────────────┘
                           │ 场景图抽象接口
                           ▼
┌─────────────────────────────────────────────────────────────┐
│              Rendering Abstraction Layer (NEW)               │
│  ┌──────────────────┐  ┌──────────────┐  ┌────────────────┐  │
│  │  SceneGraph     │  │   Renderer   │  │ RenderEngine   │  │
│  │  Abstraction    │  │   Abstraction│  │   Factory      │  │
│  └──────────────────┘  └──────────────┘  └────────────────┘  │
└──────────────────────────┬──────────────────────────────────┘
                           │
         ┌─────────────────┼─────────────────┐
         ▼                 ▼                 ▼
┌──────────────────┐ ┌──────────────┐ ┌──────────────┐
│   Coin3D Backend │ │  OsgVerse    │ │   Future...  │
│   (当前)         │ │  Backend     │ │   Backend    │
│                  │ │              │ │              │
│  - Open Inventor │ │ - OpenSceneGraph│  - Vulkan    │
│  - 场景图        │ │ - PBR渲染    │ │  - DirectX   │
│  - 固定功能管线  │ │ - 延迟渲染   │ │  - WebGPU    │
└──────────────────┘ └──────────────┘ └──────────────┘
         │                 │                 │
         └─────────────────┴─────────────────┘
                           ▼
              ┌────────────────────────┐
              │   Graphics Hardware    │
              │   (OpenGL / Vulkan)    │
              └────────────────────────┘
```

---

## 3. 核心抽象接口设计

### 3.1 场景图抽象 (RenderSceneNode.h)

```cpp
namespace Render {

/**
 * 场景图节点类型枚举
 */
enum class NodeType {
    Root,           // 根节点
    Group,          // 分组节点
    Transform,      // 变换节点
    Switch,         // 条件切换节点
    Separator,      // 状态隔离节点
    Geometry,       // 几何节点
    Material,       // 材质节点
    Camera,         // 相机节点
    Light,          // 灯光节点
    Custom          // 自定义节点
};

/**
 * 场景图节点基类 - 替代SoNode
 */
class RenderNode {
public:
    virtual ~RenderNode() = default;

    virtual NodeType getType() const = 0;
    virtual const std::string& getName() const = 0;
    virtual void setName(const std::string& name) = 0;

    // 引用计数（兼容Coin3D的引用机制）
    virtual void ref() const = 0;
    virtual void unref() const = 0;
    virtual int getRefCount() const = 0;

    // 父子关系
    virtual RenderNode* getParent() const = 0;
    virtual size_t getChildCount() const = 0;
    virtual RenderNode* getChild(size_t index) const = 0;

    // 可见性
    virtual void setVisible(bool visible) = 0;
    virtual bool isVisible() const = 0;

    // 用户数据（用于关联ViewProvider）
    virtual void setUserData(void* data) = 0;
    virtual void* getUserData() const = 0;
};

/**
 * 分组节点 - 替代SoGroup/SoSeparator
 */
class RenderGroup : public RenderNode {
public:
    virtual void addChild(RenderNode* child) = 0;
    virtual void insertChild(size_t index, RenderNode* child) = 0;
    virtual void removeChild(RenderNode* child) = 0;
    virtual void removeAllChildren() = 0;
    virtual RenderNode* findChild(const std::string& name) const = 0;
};

/**
 * 变换节点 - 替代SoTransform
 */
class RenderTransform : public RenderNode {
public:
    // 变换操作
    virtual void setTranslation(const Base::Vector3d& t) = 0;
    virtual void setRotation(const Base::Rotation& r) = 0;
    virtual void setScale(const Base::Vector3d& s) = 0;
    virtual void setScaleFactor(double s) = 0;

    virtual Base::Vector3d getTranslation() const = 0;
    virtual Base::Rotation getRotation() const = 0;
    virtual Base::Vector3d getScale() const = 0;

    // 矩阵操作
    virtual void setMatrix(const Base::Matrix4D& m) = 0;
    virtual Base::Matrix4D getMatrix() const = 0;

    // 累积变换
    virtual void preMultiply(const Base::Matrix4D& m) = 0;
    virtual void postMultiply(const Base::Matrix4D& m) = 0;
};

/**
 * 切换节点 - 替代SoSwitch
 */
class RenderSwitch : public RenderNode {
public:
    static const int SO_SWITCH_NONE = -1;
    static const int SO_SWITCH_ALL = -2;
    static const int SO_SWITCH_INHERIT = -3;

    virtual void setWhichChild(int index) = 0;
    virtual int getWhichChild() const = 0;
    virtual void addChild(RenderNode* child, const std::string& modeName) = 0;
    virtual int getChildIndex(const std::string& modeName) const = 0;
    virtual std::string getModeName(int index) const = 0;
};

/**
 * 相机节点 - 替代SoCamera
 */
class RenderCamera : public RenderNode {
public:
    enum Type { Perspective, Orthographic };

    virtual Type getType() const = 0;

    // 位置和方向
    virtual void setPosition(const Base::Vector3d& pos) = 0;
    virtual void setOrientation(const Base::Rotation& rot) = 0;
    virtual Base::Vector3d getPosition() const = 0;
    virtual Base::Rotation getOrientation() const = 0;

    // 视图目标
    virtual void setTarget(const Base::Vector3d& target) = 0;
    virtual Base::Vector3d getTarget() const = 0;

    // 投影参数
    virtual void setNearFar(double nearPlane, double farPlane) = 0;
    virtual void getNearFar(double& nearPlane, double& farPlane) const = 0;
    virtual void setAspectRatio(double ratio) = 0;
    virtual double getAspectRatio() const = 0;

    // 特定类型参数
    virtual void setHeight(double height) = 0;     // orthographic
    virtual void setFovyAngle(double angle) = 0;   // perspective
    virtual void setViewDistance(float distance) = 0;

    // 视图操作
    virtual void viewAll(const RenderGroup* root) = 0;
    virtual void fitBoundingBox(const Base::BoundBox3d& bbox) = 0;
};

/**
 * 灯光节点 - 替代SoLight
 */
class RenderLight : public RenderNode {
public:
    enum Type { Directional, Point, Spot, Ambient, Hemisphere };

    virtual Type getType() const = 0;

    // 基本属性
    virtual void setColor(const App::Color& color) = 0;
    virtual App::Color getColor() const = 0;
    virtual void setIntensity(float intensity) = 0;
    virtual float getIntensity() const = 0;
    virtual void setOn(bool on) = 0;
    virtual bool isOn() const = 0;

    // 位置和方向（Point/Spot）
    virtual void setPosition(const Base::Vector3d& pos) = 0;
    virtual void setDirection(const Base::Vector3d& dir) = 0;

    // 聚光灯参数
    virtual void setSpotCutoff(float angle) = 0;
    virtual void setSpotExponent(float exponent) = 0;

    // 衰减（Point Light）
    virtual void setAttenuation(float constant, float linear, float quadratic) = 0;

    // 阴影
    virtual void setCastShadow(bool enable) = 0;
    virtual bool getCastShadow() const = 0;
};

/**
 * 几何节点 - 替代SoIndexedFaceSet等
 */
class RenderGeometry : public RenderNode {
public:
    // 顶点数据
    virtual void setCoordinates(const std::vector<Base::Vector3f>& points) = 0;
    virtual void setNormals(const std::vector<Base::Vector3f>& normals) = 0;
    virtual void setTextureCoords(const std::vector<Base::Vector2f>& coords) = 0;
    virtual void setTangents(const std::vector<Base::Vector4f>& tangents) = 0;  // PBR

    // 拓扑结构
    virtual void setCoordIndex(const std::vector<int32_t>& indices) = 0;
    virtual void setNormalIndex(const std::vector<int32_t>& indices) = 0;
    virtual void setTexCoordIndex(const std::vector<int32_t>& indices) = 0;

    // 绑定方式
    enum Binding { PerVertex, PerFace, Overall };
    virtual void setNormalBinding(Binding binding) = 0;
    virtual void setMaterialBinding(Binding binding) = 0;

    // 边渲染（线框模式）
    virtual void setEdgeVisibility(bool showEdges) = 0;
    virtual void setEdgeColor(const App::Color& color) = 0;
    virtual void setEdgeWidth(float width) = 0;

    // 包围盒
    virtual Base::BoundBox3d getBoundingBox() const = 0;
    virtual void setBoundingBox(const Base::BoundBox3d& bbox) = 0;
};

/**
 * 材质节点 - 替代SoMaterial
 */
class RenderMaterial : public RenderNode {
public:
    // 传统材质属性
    virtual void setAmbientColor(const App::Color& color) = 0;
    virtual void setDiffuseColor(const App::Color& color) = 0;
    virtual void setSpecularColor(const App::Color& color) = 0;
    virtual void setEmissiveColor(const App::Color& color) = 0;
    virtual void setShininess(float shininess) = 0;
    virtual void setTransparency(float transparency) = 0;

    // PBR材质属性（OsgVerse支持）
    virtual void setAlbedo(const App::Color& color) = 0;
    virtual void setMetallic(float metallic) = 0;
    virtual void setRoughness(float roughness) = 0;
    virtual void setAo(float ao) = 0;  // Ambient Occlusion
    virtual void setNormalScale(float scale) = 0;

    // 纹理支持
    virtual void setAlbedoTexture(const std::string& filename) = 0;
    virtual void setNormalTexture(const std::string& filename) = 0;
    virtual void setRoughnessTexture(const std::string& filename) = 0;
    virtual void setMetallicTexture(const std::string& filename) = 0;
    virtual void setAoTexture(const std::string& filename) = 0;

    // 材质模型
    virtual bool isPBR() const = 0;
    virtual void setPBR(bool enable) = 0;
};

/**
 * 绘制样式节点 - 替代SoDrawStyle
 */
class RenderDrawStyle : public RenderNode {
public:
    enum Style { Filled, Wireframe, Points, Lines, HiddenLine };

    virtual void setStyle(Style style) = 0;
    virtual Style getStyle() const = 0;
    virtual void setLineWidth(float width) = 0;
    virtual void setPointSize(float size) = 0;
};

} // namespace Render
```

### 3.2 渲染引擎抽象 (RenderEngine.h)

```cpp
namespace Render {

/**
 * 渲染引擎后端标识
 */
enum class Backend {
    Coin3D,      // Open Inventor (当前)
    OsgVerse,    // OsgVerse引擎
    Vulkan,      // 未来可能的Vulkan后端
    Direct3D,    // 未来可能的Direct3D后端
    WebGPU       // 未来可能的WebGPU后端
};

/**
 * 渲染特性支持
 */
struct RenderCapabilities {
    bool pbr = false;                // 基于物理的渲染
    bool deferredRendering = false;  // 延迟渲染
    bool realtimeShadow = false;     // 实时阴影
    bool postProcessing = false;     // 后处理效果
    bool instancing = false;         // GPU实例化
    bool tessellation = false;       // 曲面细分
    bool rayTracing = false;         // 光线追踪
    bool computeShader = false;      // 计算着色器
    int maxTextureSize = 4096;       // 最大纹理尺寸
    int maxSamples = 4;              // 最大MSAA采样数
};

/**
 * 渲染模式
 */
enum class RenderMode {
    Forward,           // 前向渲染
    Deferred,          // 延迟渲染
    Wireframe,         // 线框模式
    Shaded,            // 着色模式
    Flat,              // 平面着色
};

/**
 * 渲染上下文 - 管理OpenGL/Vulkan上下文和状态
 */
class RenderContext {
public:
    virtual ~RenderContext() = default;

    virtual void makeCurrent() = 0;
    virtual void doneCurrent() = 0;
    virtual void swapBuffers() = 0;
    virtual void setViewport(int x, int y, int width, int height) = 0;
    virtual void getViewport(int& x, int& y, int& width, int& height) const = 0;

    // 状态查询
    virtual bool isValid() const = 0;
    virtual bool isCurrent() const = 0;
};

/**
 * 渲染动作 - 封装渲染操作
 */
class RenderAction {
public:
    virtual ~RenderAction() = default;

    virtual void apply(RenderNode* root) = 0;
    virtual void setViewport(const SbVec2s& origin, const SbVec2s& size) = 0;

    // 渲染模式
    virtual void setRenderMode(RenderMode mode) = 0;
    virtual RenderMode getRenderMode() const = 0;

    // 拾取操作
    struct PickedPoint {
        Base::Vector3d point;           // 拾取点坐标（世界空间）
        Base::Vector3d normal;          // 法向量
        RenderNode* node;               // 命中的节点
        std::string detail;             // 详细信息（子元素）
        float depth;                    // 深度值
    };
    virtual std::vector<PickedPoint> pickRay(const Base::Vector3d& rayOrigin,
                                             const Base::Vector3d& rayDir) const = 0;
    virtual std::vector<PickedPoint> pickArea(const SbVec2s& pos,
                                              float radius = 0.0f) const = 0;
    virtual PickedPoint* pickSingle(const SbVec2s& pos) const = 0;
};

/**
 * 渲染引擎接口 - 核心抽象
 */
class RenderEngine {
public:
    virtual ~RenderEngine() = default;

    // 引擎信息
    virtual Backend getBackendType() const = 0;
    virtual const char* getBackendName() const = 0;
    virtual const char* getBackendVersion() const = 0;
    virtual RenderCapabilities getCapabilities() const = 0;

    // 场景图创建
    virtual RenderGroup* createRoot() = 0;
    virtual RenderGroup* createGroup() = 0;
    virtual RenderTransform* createTransform() = 0;
    virtual RenderSwitch* createSwitch() = 0;
    virtual RenderSeparator* createSeparator() = 0;
    virtual RenderGeometry* createGeometry() = 0;
    virtual RenderMaterial* createMaterial() = 0;
    virtual RenderDrawStyle* createDrawStyle() = 0;
    virtual RenderCamera* createCamera(RenderCamera::Type type) = 0;
    virtual RenderLight* createLight(RenderLight::Type type) = 0;

    // 渲染动作
    virtual RenderAction* createRenderAction() = 0;
    virtual RenderAction* createPickAction() = 0;

    // 事件处理
    class EventHandler {
    public:
        virtual ~EventHandler() = default;
        virtual bool handleEvent(const class Event& event) = 0;
    };
    virtual std::unique_ptr<EventHandler> createEventHandler() = 0;

    // 初始化和清理
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;
};

/**
 * 渲染引擎工厂
 */
class RenderEngineFactory {
public:
    static RenderEngineFactory& instance();

    // 注册引擎
    void registerEngine(Backend type,
                       std::function<std::unique_ptr<RenderEngine>()> creator);

    // 创建引擎
    std::unique_ptr<RenderEngine> create(Backend type);

    // 获取默认引擎
    std::unique_ptr<RenderEngine> createDefault();

    // 列出可用引擎
    std::vector<Backend> availableEngines() const;
    std::string getEngineName(Backend type) const;

private:
    RenderEngineFactory() = default;
    std::map<Backend, std::function<std::unique_ptr<RenderEngine>()>> _creators;
    std::map<Backend, std::string> _engineNames;
};

} // namespace Render
```

### 3.3 3D查看器抽象 (RenderViewer.h)

```cpp
namespace Render {

/**
 * 相机导航模式
 */
enum class NavigationMode {
    Orbit,          // 轨道旋转
    Pan,            // 平移
    Zoom,           // 缩放
    Fly,            // 飞行模式
    Walk,           // 行走模式
    Examine,        // 检查模式
    FirstPerson     // 第一人称模式
};

/**
 * 3D查看器抽象 - 替代View3DInventorViewer
 */
class RenderViewer {
public:
    virtual ~RenderViewer() = default;

    // 渲染引擎
    virtual void setRenderEngine(std::unique_ptr<RenderEngine> engine) = 0;
    virtual RenderEngine* getRenderEngine() const = 0;

    // 场景管理
    virtual RenderGroup* getSceneRoot() const = 0;
    virtual void setSceneRoot(RenderGroup* root) = 0;

    // 相机
    virtual RenderCamera* getCamera() const = 0;
    virtual void setCamera(RenderCamera* camera) = 0;

    // 渲染
    virtual void render() = 0;
    virtual void scheduleRedraw() = 0;
    bool isRedrawScheduled() const = 0;

    // 导航
    virtual void setNavigationMode(NavigationMode mode) = 0;
    virtual NavigationMode getNavigationMode() const = 0;
    virtual void viewAll() = 0;
    virtual void fitSelection() = 0;
    virtual void setAxoView() = 0;
    virtual void setFrontView() = 0;
    virtual void setTopView() = 0;
    virtual void setRightView() = 0;
    virtual void setBottomView() = 0;
    virtual void setLeftView() = 0;
    virtual void setRearView() = 0;

    // 视口
    virtual void setViewport(int x, int y, int width, int height) = 0;
    virtual void getViewport(int& x, int& y, int& width, int& height) const = 0;

    // 背景设置
    virtual void setBackgroundColor(const App::Color& color) = 0;
    virtual void setGradientBackground(bool enable,
                                      const App::Color& color1,
                                      const App::Color& color2) = 0;
    virtual void setBackgroundImage(const std::string& filename) = 0;

    // 网格/地面
    virtual void setGridVisible(bool visible) = 0;
    virtual void setGridSize(float size) = 0;
    virtual void setGridSpacing(float spacing) = 0;

    // 坐标轴
    virtual void setAxisCrossVisible(bool visible) = 0;

    // 光照
    virtual void setHeadlightEnabled(bool enabled) = 0;
    virtual void setAmbientLight(const App::Color& color) = 0;

    // 渲染模式
    virtual void setRenderMode(RenderMode mode) = 0;
    virtual RenderMode getRenderMode() const = 0;

    // 后处理效果（OsgVerse支持）
    virtual void setAntiAliasing(int samples) = 0;
    virtual void setBloomEnabled(bool enable, float threshold = 1.0f) = 0;
    virtual void setSSAOEnabled(bool enable) = 0;
    virtual void setShadowEnabled(bool enable) = 0;
    virtual void setShadowQuality(int quality) = 0;  // 0-3

    // ViewProvider管理
    virtual void addViewProvider(class ViewProvider* vp) = 0;
    virtual void removeViewProvider(class ViewProvider* vp) = 0;
    virtual bool hasViewProvider(class ViewProvider* vp) const = 0;
    virtual std::vector<class ViewProvider*> getViewProviders() const = 0;

    // 渲染统计
    struct RenderStats {
        int frameCount;
        float fps;
        int triangleCount;
        int drawCallCount;
        float gpuTime;
    };
    virtual RenderStats getRenderStats() const = 0;
    virtual void setStatsVisible(bool visible) = 0;

    // 信号
    fastsignals::signal<void()> renderRequested;
    fastsignals::signal<void(const RenderStats&)> statsUpdated;
};

/**
 * 查看器创建工厂
 */
class RenderViewerFactory {
public:
    static std::unique_ptr<RenderViewer> create(QWidget* parent = nullptr);
    static std::unique_ptr<RenderViewer> create(Backend backend,
                                               QWidget* parent = nullptr);
};

} // namespace Render
```

---

## 4. 后端实现

### 4.1 Coin3D后端 (Coin3DBackend.h)

```cpp
namespace Render {
namespace Backend {

/**
 * Coin3D渲染引擎实现（包装现有代码）
 */
class Coin3DEngine : public RenderEngine {
public:
    Coin3DEngine();
    ~Coin3DEngine() override;

    Backend getBackendType() const override { return Backend::Coin3D; }
    const char* getBackendName() const override { return "Coin3D"; }
    const char* getBackendVersion() const override;
    RenderCapabilities getCapabilities() const override;

    bool initialize() override;
    void shutdown() override;
    bool isInitialized() const override { return true; }

    // 场景图创建 - 包装现有SoNode
    RenderGroup* createRoot() override;
    RenderGroup* createGroup() override;
    RenderTransform* createTransform() override;
    RenderSwitch* createSwitch() override;
    RenderSeparator* createSeparator() override;
    RenderGeometry* createGeometry() override;
    RenderMaterial* createMaterial() override;
    RenderDrawStyle* createDrawStyle() override;
    RenderCamera* createCamera(RenderCamera::Type type) override;
    RenderLight* createLight(RenderLight::Type type) override;

    RenderAction* createRenderAction() override;
    RenderAction* createPickAction() override;

    std::unique_ptr<EventHandler> createEventHandler() override;

    // 直接访问底层Coin3D场景图（用于兼容）
    SoNode* getSoNode(RenderNode* node) const;
    RenderNode* getRenderNode(SoNode* node) const;
};

} // namespace Backend
} // namespace Render
```

### 4.2 OsgVerse后端 (OsgVerseBackend.h)

```cpp
namespace Render {
namespace Backend {

/**
 * OsgVerse渲染引擎实现
 */
class OsgVerseEngine : public RenderEngine {
public:
    OsgVerseEngine();
    ~OsgVerseEngine() override;

    Backend getBackendType() const override { return Backend::OsgVerse; }
    const char* getBackendName() const override { return "OsgVerse"; }
    const char* getBackendVersion() const override;
    RenderCapabilities getCapabilities() const override;

    bool initialize() override;
    void shutdown() override;
    bool isInitialized() const override { return _initialized; }

    // 场景图创建
    RenderGroup* createRoot() override;
    RenderGroup* createGroup() override;
    RenderTransform* createTransform() override;
    RenderSwitch* createSwitch() override;
    RenderSeparator* createSeparator() override;
    RenderGeometry* createGeometry() override;
    RenderMaterial* createMaterial() override;
    RenderDrawStyle* createDrawStyle() override;
    RenderCamera* createCamera(RenderCamera::Type type) override;
    RenderLight* createLight(RenderLight::Type type) override;

    RenderAction* createRenderAction() override;
    RenderAction* createPickAction() override;

    std::unique_ptr<EventHandler> createEventHandler() override;

    // OsgVerse 特定功能
    void setDeferredRendering(bool enable);
    void setPBREnabled(bool enable);
    void setShadowTechnique(int technique);  // 0=None, 1=ShadowMap, 2=PCSS, 3=PCF

private:
    bool _initialized = false;
    osg::ref_ptr<osgVerse::Pipeline> _pipeline;  // OsgVerse渲染管线
};

} // namespace Backend
} // namespace Render
```

---

## 5. ViewProvider适配层

```cpp
/**
 * ViewProvider渲染代理 - 连接ViewProvider与抽象渲染层
 */
class ViewProviderRenderProxy {
public:
    ViewProviderRenderProxy(ViewProvider* vp);
    ~ViewProviderRenderProxy();

    // 获取场景图根节点（抽象接口）
    Render::RenderGroup* getRootNode() const;
    Render::RenderGroup* getFrontRoot() const;
    Render::RenderGroup* getBackRoot() const;
    Render::RenderGroup* getChildRoot() const;
    Render::RenderTransform* getTransform() const;
    Render::RenderSwitch* getModeSwitch() const;

    // 显示模式
    void setDisplayMode(const char* modeName);
    const char* getDisplayMode() const;
    std::vector<std::string> getDisplayModes() const;

    // 可见性
    void setVisible(bool visible);
    bool isVisible() const;
    void setOnTopWhenSelected(int onTop);

    // 选择
    void setSelectable(bool selectable);
    bool isSelectable() const;
    bool isSelected() const;
    void setSelectionColor(const App::Color& color);

    // 编辑模式
    bool startEdit(int ModNum, Render::RenderViewer* viewer);
    void finishEdit();

    // 材质（用于几何对象）
    void setMaterial(const App::Material& material);
    void setTransparency(float transparency);

    // 通知更新
    void scheduleUpdate();
    void updateGeometry();

private:
    ViewProvider* _viewProvider;

    // 抽象场景图节点
    std::unique_ptr<Render::RenderGroup> _rootNode;
    std::unique_ptr<Render::RenderGroup> _frontRoot;
    std::unique_ptr<Render::RenderGroup> _backRoot;
    std::unique_ptr<Render::RenderGroup> _childRoot;
    std::unique_ptr<Render::RenderTransform> _transform;
    std::unique_ptr<Render::RenderSwitch> _modeSwitch;
    std::unique_ptr<Render::RenderSelectionNode> _selectionNode;

    // 兼容层：旧版SoNode指针
    SoSeparator* _legacyRoot = nullptr;
};
```

---

## 6. 目录结构

```
src/Gui/
├── Render/                           # 新增：渲染抽象层
│   ├── CMakeLists.txt
│   │
│   ├── Core/                         # 核心抽象接口
│   │   ├── RenderSceneNode.h
│   │   ├── RenderSceneNode.cpp
│   │   ├── RenderEngine.h
│   │   ├── RenderEngine.cpp
│   │   ├── RenderViewer.h
│   │   ├── RenderViewer.cpp
│   │   ├── RenderSelection.h
│   │   ├── RenderSelection.cpp
│   │   ├── RenderAction.h
│   │   └── RenderAction.cpp
│   │
│   ├── Backends/                     # 后端实现
│   │   ├── CMakeLists.txt
│   │   │
│   │   ├── Coin3D/                   # Coin3D后端
│   │   │   ├── Coin3DEngine.h
│   │   │   ├── Coin3DEngine.cpp
│   │   │   ├── Coin3DNodes.h
│   │   │   ├── Coin3DNodes.cpp
│   │   │   ├── Coin3DAction.h
│   │   │   └── Coin3DAction.cpp
│   │   │
│   │   └── OsgVerse/                 # OsgVerse后端
│   │       ├── OsgVerseEngine.h
│   │       ├── OsgVerseEngine.cpp
│   │       ├── OsgVerseNodes.h
│   │       ├── OsgVerseNodes.cpp
│   │       ├── OsgVerseAction.h
│   │       ├── OsgVerseAction.cpp
│   │       ├── OsgVerseMaterial.h    # PBR材质
│   │       └── OsgVerseMaterial.cpp
│   │
│   └── Utils/                        # 辅助工具
│       ├── RenderUtils.h
│       ├── RenderUtils.cpp
│       ├── GeometryConverter.h       # 几何数据转换
│       └── MaterialConverter.h       # 材质转换
│
├── ViewProviderRenderProxy.h          # ViewProvider适配层
├── ViewProviderRenderProxy.cpp
│
├── View3DInventor.h                   # 保持（逐渐迁移）
├── View3DInventor.cpp
├── View3DInventorViewer.h             # 保持（逐渐迁移）
├── View3DInventorViewer.cpp
│
└── ... 现有文件保持不变 ...
```

---

## 7. CMake配置

```cmake
# ===== 渲染抽象层配置 =====
option(FREECAD_USE_RENDER_ABSTRACTION "Enable rendering abstraction layer" ON)
set(FREECAD_DEFAULT_RENDER_BACKEND "Coin3D" CACHE STRING "Default rendering backend")
set_property(CACHE FREECAD_DEFAULT_RENDER_BACKEND PROPERTY STRINGS "Coin3D" "OsgVerse")

option(FREECAD_ENABLE_OSGVERSE "Enable OsgVerse rendering backend" OFF)

# 定义后端宏
if(FREECAD_ENABLE_OSGVERSE)
    add_compile_definitions(FREECAD_RENDER_BACKEND_OSGVERSE=1)
    find_package(osgVerse REQUIRED)
    find_package(OpenSceneGraph REQUIRED COMPONENTS osg osgGA osgViewer osgDB)
endif()

if(TARGET Coin::Coin)
    add_compile_definitions(FREECAD_RENDER_BACKEND_COIN3D=1)
endif()
```

---

## 8. 迁移策略

```
┌─────────────────────────────────────────────────────────────┐
│                    阶段0：准备期                              │
│  • 创建抽象接口定义                                           │
│  • 实现Coin3D后端（包装现有代码）                             │
│  • 实现OsgVerse后端基础                                       │
│  • 添加编译选项控制新渲染层                                   │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    阶段1：并行运行期                          │
│  • ViewProvider支持新旧两种模式                              │
│  • 运行时切换：FREECAD_RENDER_ENGINE=coin3d|osgverse        │
│  • 基础几何渲染迁移完成                                      │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    阶段2：功能迁移期                          │
│  • 迁移选择系统                                              │
│  • 迁移编辑模式                                              │
│  • 迁移拖拽操作                                              │
│  • 添加OsgVerse高级特性（PBR、阴影、后处理）                  │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    阶段3：稳定期                              │
│  • OsgVerse作为默认选项（可选）                              │
│  • Coin3D继续作为经典后端                                    │
│  • 用户可自由选择渲染后端                                    │
└─────────────────────────────────────────────────────────────┘
```

---

## 9. Coin3D vs OsgVerse 特性对比

| 特性 | Coin3D | OsgVerse |
|------|--------|----------|
| **架构** | Open Inventor场景图 | OpenSceneGraph + 现代渲染管线 |
| **OpenGL版本** | OpenGL 1.x/2.x（固定管线） | OpenGL 3.0+/Vulkan |
| **材质系统** | 传统Phong | PBR（Metallic/Roughness） |
| **渲染方式** | 前向渲染 | 延迟渲染 + 前向渲染 |
| **阴影** | 基础阴影贴图 | PCSS、PCF、CSM |
| **后处理** | 无 | SSAO、Bloom、Tone Mapping |
| **性能优化** | 基本剔除 | GPU Instancing、Frustum Culling |
| **光线追踪** | 无 | 可选（Vulkan后端） |
| **稳定性** | 非常成熟 | 较新，活跃开发 |

---

## 10. 实现优先级

**P0 - 核心接口（必须实现）**
- RenderNode基类和基本节点类型
- RenderEngine工厂接口
- Coin3D后端基础包装
- ViewProviderRenderProxy基础功能

**P1 - 基础功能（第一阶段实现）**
- 几何节点渲染
- 变换节点
- 相机控制
- 基本选择系统

**P2 - 高级功能（第二阶段实现）**
- 高级选择（子元素选择）
- 编辑模式支持
- 拖拽操作
- OsgVerse PBR渲染

**P3 - 扩展功能（后续实现）**
- OsgVerse延迟渲染
- 后处理效果
- 阴影系统
- VR/AR支持
