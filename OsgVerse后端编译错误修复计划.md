# OsgVerse 后端编译错误修复计划

## 当前状态

OsgVerse 渲染后端已实现基本框架，但存在大量编译错误，暂时禁用（`BUILD_WITH_OSGVERSE=OFF`）。

## 主要编译错误分类

### 1. OsgVerseNode.cpp 错误

#### 错误 1: `Quaternion::getValue()` 不存在
```cpp
// 错误代码 (第 400 行)
rotation.getValue(axis, angle);

// 问题：Gui::Render::Quaternion 没有 getValue() 方法
```

**修复方案**：
```cpp
// 方案 A: 添加 getValue() 方法到 Quaternion 结构体
struct Quaternion {
    float x, y, z, w;
    
    void getValue(Vec3f& axis, float& angle) const {
        // 四元数转轴角
        angle = 2.0f * std::acos(w);
        float s = std::sqrt(1.0f - w * w);
        if (s < 0.001f) {
            axis = Vec3f{1.0f, 0.0f, 0.0f};
        } else {
            axis = Vec3f{x / s, y / s, z / s};
        }
    }
};

// 方案 B: 直接在 OsgVerseNode.cpp 中转换
osg::Quat osgQuat(rotation.x, rotation.y, rotation.z, rotation.w);
double angle;
osg::Vec3d axis;
osgQuat.getRotate(angle, axis);
```

#### 错误 2: `osg::Matrixd::setScale()` 不存在
```cpp
// 错误代码 (第 426 行)
matrix.setScale(scale.x, scale.y, scale.z);

// 问题：OSG 的 Matrix 没有 setScale() 方法
```

**修复方案**：
```cpp
// 正确的 OSG API
matrix = osg::Matrixd::scale(scale.x, scale.y, scale.z) *
         osg::Matrixd::rotate(osgQuat) *
         osg::Matrixd::translate(translation.x, translation.y, translation.z);
```

### 2. OsgVerseGeometry.cpp 错误

#### 错误 1: `Base::Vector3f` 不支持初始化列表赋值
```cpp
// 错误代码 (多处)
normal = {0.0f, 0.0f, 1.0f};  // ❌ 错误

// 问题：Base::Vector3f 的构造函数是 explicit，不能用初始化列表赋值
```

**修复方案**：
```cpp
// 方案 A: 使用构造函数
normal = Vec3f(0.0f, 0.0f, 1.0f);

// 方案 B: 使用 Set 方法
normal.Set(0.0f, 0.0f, 1.0f);

// 方案 C: 分别赋值
normal.x = 0.0f;
normal.y = 0.0f;
normal.z = 1.0f;
```

#### 错误 2: `BoundingBox::min/max` 不存在
```cpp
// 错误代码 (第 399-400 行)
bbox.min = osg::Vec3d(...);
bbox.max = osg::Vec3d(...);

// 问题：Base::BoundBox3d 使用 MinX, MaxX 等字段
```

**修复方案**：
```cpp
// 正确的 API
bbox.MinX = minPoint.x();
bbox.MinY = minPoint.y();
bbox.MinZ = minPoint.z();
bbox.MaxX = maxPoint.x();
bbox.MaxY = maxPoint.y();
bbox.MaxZ = maxPoint.z();
```

#### 错误 3: `std::make_unique` 参数错误
```cpp
// 错误代码 (多处)
return std::make_unique<OsgVerseGeometry>(geom, false);

// 问题：OsgVerseGeometry 构造函数签名不匹配
```

**修复方案**：
```cpp
// 检查 OsgVerseGeometry 构造函数
explicit OsgVerseGeometry(osg::Geometry* geometry, bool ownsNode = false);

// 修复：使用 new + unique_ptr
auto result = std::unique_ptr<OsgVerseGeometry>(
    new OsgVerseGeometry(geom, false)
);
return result;
```

### 3. OsgVerseMaterial.cpp 错误

#### 错误 1: `Material` 结构体字段名错误
```cpp
// 错误代码
material.ambient
material.diffuse
material.specular
material.emissive

// 问题：RenderTypes.h 中的字段名不同
```

**修复方案**：
查看 `src/Gui/Render/Core/RenderTypes.h` 中 `Material` 结构体的实际字段名：
```cpp
struct Material {
    float ambientIntensity{0.2f};
    Color ambientColor{...};
    Color diffuseColor{...};
    Color specularColor{...};
    Color emissiveColor{...};
    // ...
};

// 修复代码
osgMaterial->setAmbient(osg::Material::FRONT_AND_BACK, 
    osg::Vec4(material.ambientColor.r, material.ambientColor.g, 
              material.ambientColor.b, material.ambientColor.a));
```

#### 错误 2: 未定义的标识符 `ive`
```cpp
// 错误代码 (第 104 行)
ive

// 问题：可能是拼写错误或未完成的代码
```

**修复方案**：删除或注释掉该行。

#### 错误 3: `Base::Console().Warning` 不存在
```cpp
// 错误代码 (第 172 行)
Base::Console().Warning("...");

// 问题：应该是 Warning() 而不是 Warning
```

**修复方案**：
```cpp
Base::Console().Warning("...\n");
```

#### 错误 4: `clone()` 返回类型不匹配
```cpp
// 错误代码
std::unique_ptr<OsgVerseMaterial> clone() const override;

// 问题：基类返回 RenderNode::Ptr (shared_ptr)
```

**修复方案**：
```cpp
// 在头文件中
RenderNode::Ptr clone() const override;

// 在实现中
RenderNode::Ptr OsgVerseMaterial::clone() const {
    auto cloned = std::make_shared<OsgVerseMaterial>();
    // 复制数据...
    return cloned;
}
```

### 4. OsgVerseViewer.cpp 错误

#### 错误: 缺少 `osgQt/GraphicsWindowQt` 头文件
```cpp
// 错误代码 (第 35 行)
#include <osgQt/GraphicsWindowQt>

// 问题：osgQt 库未安装或未链接
```

**修复方案**：
```cmake
# 在 CMakeLists.txt 中添加
find_package(osgQt REQUIRED)
target_link_libraries(FreeCADGui PRIVATE ${OSGQT_LIBRARIES})
target_include_directories(FreeCADGui PRIVATE ${OSGQT_INCLUDE_DIRS})
```

或者使用 Qt 原生窗口：
```cpp
// 使用 QOpenGLWidget 代替 osgQt
#include <QOpenGLWidget>
class OsgVerseWidget : public QOpenGLWidget {
    // 自定义实现
};
```

## 修复优先级

### 高优先级（阻塞编译）
1. ✅ 禁用 OsgVerse 后端（已完成）
2. ⬜ 修复 `Base::Vector3f` 初始化列表问题
3. ⬜ 修复 `Material` 结构体字段名
4. ⬜ 修复 `BoundingBox` API
5. ⬜ 修复 `clone()` 返回类型

### 中优先级（功能完整性）
6. ⬜ 修复 `Quaternion::getValue()`
7. ⬜ 修复 `osg::Matrixd` 变换
8. ⬜ 修复 `std::make_unique` 调用
9. ⬜ 添加 osgQt 依赖或替代方案

### 低优先级（优化和清理）
10. ⬜ 清理未使用的代码
11. ⬜ 添加单元测试
12. ⬜ 性能优化

## 修复步骤

### 第一阶段：修复基础类型问题
1. 在 `RenderTypes.h` 中为 `Quaternion` 添加 `getValue()` 方法
2. 创建辅助函数处理 `Base::Vector3f` 初始化
3. 统一 `Material` 结构体字段访问

### 第二阶段：修复 OsgVerse 特定问题
1. 修复所有 OSG API 调用
2. 实现正确的 `clone()` 方法
3. 处理 osgQt 依赖

### 第三阶段：测试和验证
1. 启用 `BUILD_WITH_OSGVERSE=ON`
2. 编译并修复剩余错误
3. 运行基本功能测试
4. 性能测试和优化

## 启用 OsgVerse 的步骤

当所有错误修复后：

```bash
# 1. 重新配置 CMake
cmake -S . -B build -DBUILD_WITH_OSGVERSE=ON

# 2. 编译
cmake --build build --config Release --target FreeCADGui

# 3. 测试
# 在 FreeCAD Python 控制台中
import FreeCADGui as Gui
if Gui.isRenderBackendAvailable(2):
    Gui.switchRenderBackend(2)
    print("OsgVerse enabled!")
```

## 当前配置

- **BUILD_WITH_OSGVERSE**: OFF (默认禁用)
- **默认后端**: Coin3D
- **状态**: 开发中，不建议生产使用

## 相关文件

- `src/Gui/Render/Backends/OsgVerse/CMakeLists.txt` - 构建配置
- `src/Gui/Render/Core/RenderTypes.h` - 类型定义
- `src/Gui/Render/Core/RenderEngine.h` - 引擎接口
- `切换OsgVerse渲染引擎指南.md` - 使用指南

## 注意事项

1. **不要在生产环境启用** - OsgVerse 后端仍在开发中
2. **Coin3D 是默认且稳定的** - 推荐使用
3. **修复需要时间** - 预计需要几天到几周完成所有修复
4. **需要 OSG 和 osgQt** - 确保系统已安装这些依赖

## 贡献

如果你想帮助修复这些错误：
1. 选择一个错误类别
2. 创建分支进行修复
3. 提交 Pull Request
4. 确保编译通过并添加测试

---

最后更新：2026-01-19
状态：OsgVerse 后端暂时禁用，等待修复
