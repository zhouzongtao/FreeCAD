# Phase 2 方案 A5 失败分析与替代方案

## 方案 A5 失败原因

### 尝试的方法
```cpp
#include <Mod/Part/App/PropertyTopoShape.h>

auto* partShapeProp = dynamic_cast<Part::PropertyPartShape*>(shapeProp);
const TopoDS_Shape& shape = partShapeProp->getValue();
```

### 失败原因
**链接错误**：
```
error LNK2019: unresolved external symbol 
"__declspec(dllimport) public: class TopoDS_Shape const & __cdecl 
Part::PropertyPartShape::getValue(void)const"
```

**根本原因**：
1. `getValue()` **不是内联函数**
2. `getValue()` 的实现在 Part 模块的 DLL 中
3. 需要链接 Part.dll 才能调用
4. Part 模块编译为 Python 扩展（Part.pyd），不是独立的 C++ 库

### 教训
- 包含头文件 ≠ 可以调用函数
- 只有内联函数和模板函数可以不链接
- FreeCAD 的模块系统设计为 Python 扩展，不是 C++ 库

## 当前状态

### ✅ 编译成功
- 时间：2026-01-21 11:58:06
- 使用安全的占位符方案
- 可以检测 Part::PropertyPartShape 类型
- 但暂时无法提取 Shape

### 当前代码行为
```cpp
// 检查属性类型
if (std::string(typeName) == "Part::PropertyPartShape") {
    // 检测到 Part 对象
    OSGVERSE_LOG_INFO("Found Part::PropertyPartShape, but extraction not implemented");
    // 使用占位符球体
}
```

## 可行的替代方案

### 方案 B：创建 Part 接口库

**思路**：
- 创建一个轻量级的 PartInterface 库
- 只包含必要的接口函数
- FreeCADGui 链接这个接口库
- 接口库在运行时加载 Part.pyd

**优点**：
- 类型安全
- 编译时检查
- 性能好

**缺点**：
- 需要重构 Part 模块结构
- 工作量大（1-2天）

### 方案 C：Python API 桥接（推荐）

**思路**：
- 使用 Python C API
- 通过 Python 调用 Part 模块
- 获取 Shape 后传回 C++

**实现步骤**：

#### 步骤 1：添加 Python API 辅助函数

```cpp
// 在 GeometryConverter.h 中添加
class GeometryConverter {
public:
    // 通过 Python API 从 Property 提取 Shape
    static bool extractShapeViaPython(
        App::Property* prop,
        TopoDS_Shape& outShape
    );
};
```

#### 步骤 2：实现 Python 桥接

```cpp
// 在 GeometryConverter.cpp 中实现
#include <Python.h>

bool GeometryConverter::extractShapeViaPython(
    App::Property* prop,
    TopoDS_Shape& outShape
)
{
    if (!prop) {
        return false;
    }
    
    // 确保 Python 已初始化
    if (!Py_IsInitialized()) {
        GEOM_LOG_ERROR("Python not initialized");
        return false;
    }
    
    try {
        // 获取 Property 的 Python 对象
        PyObject* pyProp = prop->getPyObject();
        if (!pyProp) {
            GEOM_LOG_ERROR("Failed to get Python object for property");
            return false;
        }
        
        // 调用 getValue() 方法
        PyObject* pyGetValue = PyObject_GetAttrString(pyProp, "getValue");
        if (!pyGetValue || !PyCallable_Check(pyGetValue)) {
            GEOM_LOG_ERROR("getValue method not found or not callable");
            Py_XDECREF(pyGetValue);
            Py_DECREF(pyProp);
            return false;
        }
        
        // 调用 getValue()
        PyObject* pyShape = PyObject_CallObject(pyGetValue, nullptr);
        Py_DECREF(pyGetValue);
        
        if (!pyShape) {
            GEOM_LOG_ERROR("getValue() call failed");
            Py_DECREF(pyProp);
            return false;
        }
        
        // 从 Python Shape 对象提取 TopoDS_Shape
        // Part 模块的 Python 绑定提供了 getShape() 方法
        PyObject* pyGetShape = PyObject_GetAttrString(pyShape, "getShape");
        if (pyGetShape && PyCallable_Check(pyGetShape)) {
            PyObject* pyTopoShape = PyObject_CallObject(pyGetShape, nullptr);
            Py_DECREF(pyGetShape);
            
            if (pyTopoShape) {
                // 从 Python 对象提取 C++ TopoDS_Shape 指针
                // 这需要使用 PyCapsule 或类似机制
                // 具体实现取决于 Part 模块的 Python 绑定
                
                // 假设有一个 _getShapePointer() 方法
                PyObject* pyPtr = PyObject_CallMethod(pyTopoShape, "_getShapePointer", nullptr);
                if (pyPtr && PyCapsule_CheckExact(pyPtr)) {
                    void* ptr = PyCapsule_GetPointer(pyPtr, nullptr);
                    if (ptr) {
                        outShape = *static_cast<TopoDS_Shape*>(ptr);
                        Py_DECREF(pyPtr);
                        Py_DECREF(pyTopoShape);
                        Py_DECREF(pyShape);
                        Py_DECREF(pyProp);
                        return !outShape.IsNull();
                    }
                }
                Py_XDECREF(pyPtr);
                Py_DECREF(pyTopoShape);
            }
        }
        Py_XDECREF(pyGetShape);
        
        Py_DECREF(pyShape);
        Py_DECREF(pyProp);
        
        GEOM_LOG_ERROR("Failed to extract TopoDS_Shape from Python object");
        return false;
    }
    catch (const std::exception& e) {
        GEOM_LOG_ERROR("Exception in Python bridge: %s", e.what());
        return false;
    }
    catch (...) {
        GEOM_LOG_ERROR("Unknown exception in Python bridge");
        return false;
    }
}
```

#### 步骤 3：在 addViewProvider 中使用

```cpp
if (std::string(typeName) == "Part::PropertyPartShape") {
    TopoDS_Shape shape;
    if (GeometryConverter::extractShapeViaPython(shapeProp, shape)) {
        // 成功提取！
        // 转换几何体...
    }
}
```

**优点**：
- 不需要链接 Part 模块
- 利用现有的 Python 绑定
- 相对安全

**缺点**：
- 依赖 Python 环境
- 性能略低（Python 调用开销）
- 需要了解 Part 模块的 Python API

### 方案 D：延迟加载 Part.pyd

**思路**：
- 在运行时动态加载 Part.pyd
- 获取 getValue() 函数指针
- 直接调用

**实现**：
```cpp
// Windows
HMODULE partModule = LoadLibrary("Part.pyd");
if (partModule) {
    typedef const TopoDS_Shape& (*GetValueFunc)(const void*);
    GetValueFunc getValue = (GetValueFunc)GetProcAddress(partModule, "?getValue@PropertyPartShape@Part@@QEBAAEBVTopoDS_Shape@@XZ");
    if (getValue) {
        const TopoDS_Shape& shape = getValue(shapeProp);
    }
}
```

**优点**：
- 直接调用，性能好
- 不需要 Python

**缺点**：
- 平台相关（Windows/Linux 不同）
- 函数名 mangling 复杂
- 不安全

## 推荐方案

**首选：方案 C - Python API 桥接**

理由：
1. 最符合 FreeCAD 架构
2. 利用现有的 Python 绑定
3. 相对安全和稳定
4. 工作量适中（2-4小时）

**备选：方案 B - 创建接口库**

如果需要更好的性能和类型安全。

## 下一步行动

1. **研究 Part 模块的 Python API**
   - 查看 Part 模块的 Python 绑定代码
   - 了解如何从 Python 对象提取 TopoDS_Shape

2. **实现 Python 桥接**
   - 实现 `extractShapeViaPython()` 函数
   - 测试和调试

3. **集成到 addViewProvider**
   - 调用 Python 桥接函数
   - 转换几何体
   - 测试真实对象渲染

## 当前可测试的功能

虽然真实几何体转换还未实现，但当前版本可以：
- ✅ 正常切换到 OsgVerse 后端
- ✅ 检测 Part 对象
- ✅ 显示占位符球体
- ✅ 应用材质和颜色
- ✅ 稳定运行，不崩溃

## 总结

方案 A5 因为 `getValue()` 不是内联函数而失败。我们需要采用方案 C（Python API 桥接）来实现真实几何体渲染。

当前代码已经编译成功，可以进行基本测试。下一步是实现 Python 桥接来提取 Shape。
