# OsgVerse Shader System

## 概述 / Overview

OsgVerse后端的shader系统提供了现代化的渲染能力，包括PBR（物理基础渲染）和传统的Phong着色。

The OsgVerse backend shader system provides modern rendering capabilities, including PBR (Physically Based Rendering) and traditional Phong shading.

## Shader类型 / Shader Types

### 1. PBR Shader (默认 / Default)
- **用途**: 物理基础渲染，提供真实感的材质表现
- **特性**:
  - 金属/粗糙度工作流 (Metallic/Roughness workflow)
  - 法线贴图支持 (Normal mapping)
  - 多种纹理支持 (Multiple texture support)
  - HDR色调映射 (HDR tone mapping)
  - Gamma校正 (Gamma correction)

### 2. Standard Phong Shader
- **用途**: 传统的Phong光照模型
- **特性**:
  - 环境光、漫反射、镜面反射 (Ambient, Diffuse, Specular)
  - 与Coin3D材质兼容 (Compatible with Coin3D materials)
  - 性能优化 (Performance optimized)

### 3. Wireframe Shader
- **用途**: 线框渲染
- **特性**: 简单的线框显示

### 4. Flat Shader
- **用途**: 平面着色
- **特性**: 每个面使用单一颜色

### 5. Unlit Shader
- **用途**: 无光照渲染
- **特性**: 直接显示材质颜色，不受光照影响

## 使用方法 / Usage

### C++ API

```cpp
#include "OsgVerseShaderManager.h"
#include "OsgVerseMaterial.h"

// 获取shader管理器
auto& shaderMgr = OsgVerseShaderManager::instance();

// 创建材质并设置shader类型
auto material = std::make_shared<OsgVerseMaterial>();

// 使用PBR shader (默认)
material->setShaderType(ShaderType::PBR);
material->setBaseColor(Color{0.8f, 0.2f, 0.2f, 1.0f});
material->setMetallic(0.5f);
material->setRoughness(0.3f);

// 或使用标准Phong shader
material->setShaderType(ShaderType::Standard);
material->setDiffuseColor(Color{0.8f, 0.2f, 0.2f, 1.0f});
material->setShininess(32.0f);

// 应用到节点
material->applyToNode(myNode);
```

### 材质参数 / Material Parameters

#### PBR材质参数:
- `baseColor`: 基础颜色 (vec4)
- `metallic`: 金属度 (0.0 - 1.0)
- `roughness`: 粗糙度 (0.0 - 1.0)
- `emissive`: 自发光颜色 (vec4)
- `ambientOcclusion`: 环境光遮蔽 (0.0 - 1.0)

#### 纹理支持:
- `baseColorTexture`: 基础颜色贴图
- `normalTexture`: 法线贴图
- `metallicRoughnessTexture`: 金属度/粗糙度贴图
- `occlusionTexture`: 环境光遮蔽贴图
- `emissiveTexture`: 自发光贴图

#### 标准Phong材质参数:
- `ambientColor`: 环境光颜色 (vec4)
- `diffuseColor`: 漫反射颜色 (vec4)
- `specularColor`: 镜面反射颜色 (vec4)
- `emissiveColor`: 自发光颜色 (vec4)
- `shininess`: 光泽度 (0.0 - 128.0)
- `opacity`: 不透明度 (0.0 - 1.0)

## Shader源码 / Shader Source

所有shader都是内置在OsgVerseShaderManager中的，使用GLSL 3.30 Core Profile编写。

All shaders are built into OsgVerseShaderManager, written in GLSL 3.30 Core Profile.

### 自定义Shader / Custom Shaders

如果需要自定义shader，可以使用以下方法：

```cpp
auto& shaderMgr = OsgVerseShaderManager::instance();

// 从源码创建
auto program = shaderMgr.createProgram("MyShader", vertexSource, fragmentSource);

// 或从文件加载
auto program = shaderMgr.loadProgramFromFiles("MyShader", "my_vertex.glsl", "my_fragment.glsl");

// 应用到StateSet
stateSet->setAttributeAndModes(program, osg::StateAttribute::ON);
```

## 性能优化 / Performance Optimization

1. **Shader缓存**: 所有shader程序都会被缓存，避免重复编译
2. **延迟编译**: Shader在首次使用时才编译
3. **Uniform优化**: 只更新变化的uniform值

## 调试 / Debugging

### 检查shader编译状态:
```cpp
auto& shaderMgr = OsgVerseShaderManager::instance();

if (!shaderMgr.isCompiled(ShaderType::PBR)) {
    std::string error = shaderMgr.getCompileError(ShaderType::PBR);
    Base::Console().error("Shader compile error: %s\n", error.c_str());
}
```

### 热重载shader (开发模式):
```cpp
shaderMgr.reloadAll();
```

## 技术细节 / Technical Details

### PBR实现
- **BRDF**: Cook-Torrance微表面模型
- **NDF**: GGX/Trowbridge-Reitz分布
- **几何函数**: Smith几何遮蔽函数
- **Fresnel**: Schlick近似

### 光照模型
- 支持方向光、点光源、聚光灯
- 多光源支持（未来版本）
- 阴影映射（未来版本）

## 未来计划 / Future Plans

- [ ] 多光源支持
- [ ] 阴影映射
- [ ] IBL（基于图像的光照）
- [ ] 后处理效果（SSAO, Bloom, TAA）
- [ ] 延迟渲染管线
- [ ] 计算着色器支持

## 参考资料 / References

- [Learn OpenGL - PBR Theory](https://learnopengl.com/PBR/Theory)
- [Physically Based Rendering in Filament](https://google.github.io/filament/Filament.html)
- [Real Shading in Unreal Engine 4](https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)
