# Phase 2 方案 A：通过属性系统动态访问 Shape

## 概述

方案 A 利用 FreeCAD 的属性系统和反射机制，在运行时动态访问 Part 对象的 Shape 属性，而不需要在编译时链接 Part 模块。

## 核心思想

FreeCAD 的架构设计：
- **App 层**：核心对象模型，包括 DocumentObject、Property 等
- **Mod 层**：各个模块（Part、Mesh、Draft 等），作为 Python 扩展
- **Gui 层**：GUI 相关代码，包括 ViewProvider

**关键点**：
- Gui 层可以访问 App 层的对象和属性
- 不需要直接依赖 Mod 层的具体类型
- 通过属性名称和类型信息进行动态访问

## 技术原理

### 1. FreeCAD 属性系统

FreeCAD 的每个对象都有属性（Property），可以通过名称访问：

```cpp
App::DocumentObject* obj = ...;
App::Property* prop = obj->getPropertyByName("Shape");
```

### 2. 类型信息系统

FreeCAD 使用自己的类型系统（Base::Type）：

```cpp
// 获取属性的类型信息
Base::Type propType = prop->getTypeId();
const char* typeName = propType.getName();

// 检查类型
if (propType.isDerivedFrom(某个基类::getClassTypeId())) {
    // 类型匹配
}
```

### 3. PropertyPartShape 结构

Part 模块定义了 `PropertyPartShape`，它存储 `TopoDS_Shape`：

```cpp
// Part 模块中的定义（我们不能直接使用）
class PropertyPartShape : public App::Property {
public:
    const TopoDS_Shape& getValue() const;
    void setValue(const TopoDS_Shape& shape);
};
```

## 实现方案

### 方案 A1：通过 void* 指针访问（推荐）

**原理**：
- PropertyPartShape 的内存布局是固定的
- getValue() 方法返回 TopoDS_Shape 的引用
- 我们可以通过 void* 指针和偏移量访问

**实现步骤**：

#### 步骤 1：定义接口函数

```cpp
// 在 GeometryConverter.h 中添加
class GeometryConverter {
public:
    // 从 Property 中提取 TopoDS_Shape
    static bool extractShapeFromProperty(
        App::Property* prop,
        TopoDS_Shape& outShape
    );
};
```

#### 步骤 2：实现提取逻辑

```cpp
// 在 GeometryConverter.cpp 中实现
bool GeometryConverter::extractShapeFromProperty(
    App::Property* prop,
    TopoDS_Shape& outShape
)
{
    if (!prop) {
        return false;
    }
    
    // 检查类型名称
    const char* typeName = prop->getTypeId().getName();
    std::string typeStr(typeName);
    
    // 检查是否是 Shape 相关的属性
    if (typeStr.find("PropertyPartShape") == std::string::npos &&
        typeStr.find("PropertyShape") == std::string::npos) {
        return false;
    }
    
    try {
        // 方法 1：通过虚函数表调用 getValue()
        // PropertyPartShape 有一个虚函数 getValue()
        // 我们可以通过函数指针调用它
        
        // 定义函数指针类型
        typedef const TopoDS_Shape& (*GetValueFunc)(const void*);
        
        // 获取虚函数表
        void** vtable = *(void***)prop;
        
        // getValue() 通常在虚函数表的特定位置
        // 这需要通过调试确定具体偏移
        // 假设 getValue() 在偏移 X
        GetValueFunc getValue = (GetValueFunc)vtable[X];
        
        // 调用 getValue()
        outShape = getValue(prop);
        
        return !outShape.IsNull();
    }
    catch (...) {
        return false;
    }
}
```

**问题**：这种方法依赖虚函数表布局，不够稳定。

### 方案 A2：通过反射和动态类型转换（更安全）

**原理**：
- 使用 FreeCAD 的类型系统
- 通过 dynamic_cast 或类型检查
- 利用 Property 的通用接口

**实现步骤**：

#### 步骤 1：检查 Property 基类

FreeCAD 的 Property 系统有一些通用方法：

```cpp
class Property {
public:
    virtual void Save(Base::Writer& writer) const;
    virtual void Restore(Base::XMLReader& reader);
    virtual Property* Copy() const;
    // ...
};
```

#### 步骤 2：使用 Python API 桥接

```cpp
bool GeometryConverter::extractShapeFromProperty(
    App::Property* prop,
    TopoDS_Shape& outShape
)
{
    if (!prop) {
        return false;
    }
    
    // 检查类型
    const char* typeName = prop->getTypeId().getName();
    if (std::string(typeName).find("Shape") == std::string::npos) {
        return false;
    }
    
    try {
        // 使用 Python API 获取 Shape
        // 这需要 Python 环境已初始化
        
        // 获取 Property 的 Python 对象
        PyObject* pyProp = prop->getPyObject();
        if (!pyProp) {
            return false;
        }
        
        // 调用 Python 的 getValue() 方法
        PyObject* pyShape = PyObject_CallMethod(pyProp, "getValue", nullptr);
        if (!pyShape) {
            Py_DECREF(pyProp);
            return false;
        }
        
        // 从 Python Shape 对象提取 TopoDS_Shape
        // 这需要使用 Part 模块的 Python API
        // 具体实现取决于 Part 模块的 Python 绑定
        
        // 清理
        Py_DECREF(pyShape);
        Py_DECREF(pyProp);
        
        return !outShape.IsNull();
    }
    catch (...) {
        return false;
    }
}
```

### 方案 A3：通过内存布局直接访问（最简单但不推荐）

**原理**：
- PropertyPartShape 内部存储 TopoDS_Shape
- 我们知道内存布局
- 直接读取内存

```cpp
bool GeometryConverter::extractShapeFromProperty(
    App::Property* prop,
    TopoDS_Shape& outShape
)
{
    if (!prop) {
        return false;
    }
    
    // 检查类型
    const char* typeName = prop->getTypeId().getName();
    if (std::string(typeName).find("PropertyPartShape") == std::string::npos) {
        return false;
    }
    
    try {
        // PropertyPartShape 的内存布局（需要通过调试确定）:
        // [虚函数表指针] [Property 基类成员] [TopoDS_Shape _Shape]
        
        // 假设 TopoDS_Shape 在偏移 X 字节
        const size_t SHAPE_OFFSET = sizeof(void*) + sizeof(App::Property);
        
        // 直接读取
        const char* propBytes = reinterpret_cast<const char*>(prop);
        const TopoDS_Shape* shapePtr = reinterpret_cast<const TopoDS_Shape*>(
            propBytes + SHAPE_OFFSET
        );
        
        outShape = *shapePtr;
        
        return !outShape.IsNull();
    }
    catch (...) {
        return false;
    }
}
```

**问题**：
- 依赖内存布局，不同编译器可能不同
- 不同版本的 FreeCAD 可能改变布局
- 非常不安全

## 推荐方案：A4 - 混合方法

结合多种方法，按优先级尝试：

```cpp
bool GeometryConverter::extractShapeFromProperty(
    App::Property* prop,
    TopoDS_Shape& outShape
)
{
    if (!prop) {
        GEOM_LOG_ERROR("Property is null");
        return false;
    }
    
    // 检查类型名称
    const char* typeName = prop->getTypeId().getName();
    std::string typeStr(typeName);
    
    GEOM_LOG_DEBUG("Property type: %s", typeName);
    
    // 必须是 Shape 相关的属性
    if (typeStr.find("Shape") == std::string::npos) {
        GEOM_LOG_DEBUG("Not a Shape property");
        return false;
    }
    
    // 方法 1：尝试通过 Python API（最安全）
    if (tryExtractViaPython(prop, outShape)) {
        GEOM_LOG_DEBUG("Extracted via Python API");
        return true;
    }
    
    // 方法 2：尝试通过已知的接口
    if (tryExtractViaInterface(prop, outShape)) {
        GEOM_LOG_DEBUG("Extracted via interface");
        return true;
    }
    
    // 方法 3：尝试通过内存布局（最后手段）
    if (tryExtractViaMemoryLayout(prop, outShape)) {
        GEOM_LOG_DEBUG("Extracted via memory layout");
        return true;
    }
    
    GEOM_LOG_ERROR("Failed to extract Shape from property");
    return false;
}

// 辅助方法 1：Python API
bool GeometryConverter::tryExtractViaPython(
    App::Property* prop,
    TopoDS_Shape& outShape
)
{
    // TODO: 实现 Python API 访问
    return false;
}

// 辅助方法 2：接口访问
bool GeometryConverter::tryExtractViaInterface(
    App::Property* prop,
    TopoDS_Shape& outShape
)
{
    // TODO: 实现接口访问
    return false;
}

// 辅助方法 3：内存布局
bool GeometryConverter::tryExtractViaMemoryLayout(
    App::Property* prop,
    TopoDS_Shape& outShape
)
{
    // TODO: 实现内存布局访问
    return false;
}
```

## 实际可行的简化方案：A5 - 使用 Part 模块的头文件但不链接

**关键发现**：
- 我们可以包含 Part 模块的头文件
- 只要不调用需要链接的函数
- 可以使用类型定义和内联函数

```cpp
// 在 OsgVerseViewerImpl.cpp 中
#include <Mod/Part/App/PartFeature.h>  // 只包含头文件
#include <Mod/Part/App/PropertyPartShape.h>

void OsgVerseViewerImpl::addViewProvider(ViewProvider* vp)
{
    // ... 前面的代码 ...
    
    // 检查是否有 Shape 属性
    if (vpDoc && vpDoc->getObject()) {
        auto* obj = vpDoc->getObject();
        
        // 通过属性名称获取
        App::Property* shapeProp = obj->getPropertyByName("Shape");
        if (shapeProp) {
            // 尝试转换为 PropertyPartShape
            // 使用 dynamic_cast（需要 RTTI）
            auto* partShapeProp = dynamic_cast<Part::PropertyPartShape*>(shapeProp);
            
            if (partShapeProp) {
                // 成功！现在可以访问 Shape
                const TopoDS_Shape& shape = partShapeProp->getValue();
                
                if (!shape.IsNull()) {
                    OSGVERSE_LOG_INFO("Converting real geometry for %s", objName);
                    
                    // 使用 GeometryConverter 转换
                    GeometryConverter::ConversionOptions options;
                    options.deflection = 0.1;
                    
                    GeometryConverter::ConversionStats stats;
                    geode = GeometryConverter::convertShape(shape, options, &stats);
                    
                    if (geode) {
                        useRealGeometry = true;
                        OSGVERSE_LOG_INFO("Geometry converted: %d vertices, %d triangles",
                                         stats.vertexCount, stats.triangleCount);
                    }
                }
            }
        }
    }
    
    // ... 后面的代码 ...
}
```

**为什么这样可行**：
1. `Part::PropertyPartShape` 的头文件只包含类定义
2. `getValue()` 可能是内联函数或虚函数
3. `dynamic_cast` 是编译器内置的，不需要链接
4. 只要不调用需要链接的函数（如构造函数、静态方法），就不会有链接错误

**需要验证**：
- Part 模块是否启用了 RTTI
- getValue() 是否是内联函数
- 是否有其他依赖

## 实施步骤

### 步骤 1：验证 RTTI 和头文件

```cpp
// 测试代码
#include <Mod/Part/App/PropertyPartShape.h>

void test() {
    App::Property* prop = ...;
    
    // 测试 dynamic_cast
    auto* partProp = dynamic_cast<Part::PropertyPartShape*>(prop);
    if (partProp) {
        // 成功！
    }
}
```

### 步骤 2：实现提取逻辑

在 `OsgVerseViewerImpl::addViewProvider()` 中添加 Shape 提取代码。

### 步骤 3：测试

创建 Part 对象，验证是否能正确提取和转换 Shape。

### 步骤 4：错误处理

添加完善的错误处理和降级方案。

## 优缺点分析

### 优点
✅ 不需要链接 Part 模块  
✅ 利用 FreeCAD 现有的架构  
✅ 运行时灵活  
✅ 可以支持其他模块的 Shape 属性  

### 缺点
⚠ 依赖 RTTI（运行时类型信息）  
⚠ 需要包含 Part 模块的头文件  
⚠ 可能在不同 FreeCAD 版本间有兼容性问题  

### 风险
🔴 如果 Part 模块没有启用 RTTI，dynamic_cast 会失败  
🔴 如果 getValue() 不是内联函数，会有链接错误  

## 替代方案对比

| 方案 | 复杂度 | 安全性 | 兼容性 | 推荐度 |
|------|--------|--------|--------|--------|
| A1: 虚函数表 | 高 | 低 | 低 | ❌ |
| A2: Python API | 中 | 高 | 高 | ⭐⭐⭐ |
| A3: 内存布局 | 低 | 极低 | 极低 | ❌ |
| A4: 混合方法 | 高 | 中 | 中 | ⭐⭐ |
| A5: 头文件+RTTI | 低 | 高 | 高 | ⭐⭐⭐⭐⭐ |

## 推荐实施方案

**首选：A5 - 使用头文件 + dynamic_cast**

理由：
1. 最简单直接
2. 利用 C++ 标准特性（RTTI）
3. 不需要复杂的反射或内存操作
4. 如果失败，可以降级到占位符

**备选：A2 - Python API 桥接**

如果 A5 不可行（RTTI 未启用或链接错误），使用 Python API。

## 下一步行动

1. **验证 A5 方案**：
   - 添加 Part 头文件包含
   - 尝试 dynamic_cast
   - 编译测试

2. **如果 A5 失败**：
   - 实施 A2（Python API）
   - 或者考虑方案 B/C

3. **测试和优化**：
   - 创建测试用例
   - 性能测试
   - 错误处理

你想先尝试方案 A5 吗？这是最简单也最有可能成功的方案。
