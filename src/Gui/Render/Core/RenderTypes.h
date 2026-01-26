/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Development Team                             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef GUI_RENDER_RENDERTYPES_H
#define GUI_RENDER_RENDERTYPES_H

#include <cstdint>
#include <string>
#include <unordered_map>

#include <Base/Vector3D.h>
#include <Base/Matrix.h>
#include <Base/BoundBox.h>
#include <Base/Color.h>
#include <App/Material.h>
#include <FCGlobal.h>

namespace Gui {
namespace Render {

// 前向声明
class RenderNode;

/**
 * @brief 渲染后端类型枚举 / Rendering backend type enumeration
 *
 * 支持的渲染引擎后端类型。
 * Supported rendering engine backend types.
 */
enum class BackendType : uint8_t {
    None = 0,       ///< 无后端 / No backend
    Coin3D = 1,     ///< Coin3D (Open Inventor) 后端 / Coin3D backend
    OsgVerse = 2    ///< OsgVerse (OpenSceneGraph) 后端 / OsgVerse backend
};

/**
 * @brief 节点类型枚举 / Node type enumeration
 *
 * 场景图中节点的类型分类。
 * Classification of nodes in the scene graph.
 */
enum class NodeType : uint8_t {
    Node = 0,           ///< 基础节点 / Base node
    Group = 1,          ///< 分组节点 / Group node
    Transform = 2,      ///< 变换节点 / Transform node
    Switch = 3,         ///< 切换节点 / Switch node
    Separator = 4,      ///< 分隔节点 / Separator node
    Geometry = 5,       ///< 几何节点 / Geometry node
    Material = 6,       ///< 材质节点 / Material node
    Camera = 7,         ///< 相机节点 / Camera node
    Light = 8,          ///< 灯光节点 / Light node
    Coordinate = 9,     ///< 坐标节点 / Coordinate node
    Selection = 10      ///< 选择节点 / Selection node
};

/**
 * @brief 渲染模式 / Rendering mode
 */
enum class RenderMode : uint8_t {
    Default = 0,        ///< 默认模式 / Default mode
    Wireframe = 1,      ///< 线框模式 / Wireframe mode
    Shaded = 2,         ///< 着色模式 / Shaded mode
    Flat = 3,           ///< 平面着色 / Flat shading
    Gouraud = 4,        ///< Gouraud 着色 / Gouraud shading
    Phong = 5,          ///< Phong 着色 / Phong shading
    Points = 6,         ///< 点模式 / Points mode
    BoundingBox = 7     ///< 边界框 / Bounding box
};

/**
 * @brief 材质着色模型 / Material shading model
 */
enum class ShadeModel : uint8_t {
    Flat = 0,           ///< 平面着色 / Flat shading
    Gouraud = 1,        ///< Gouraud 着色 / Gouraud shading
    Phong = 2,          ///< Phong 着色 / Phong shading
    PBR = 3             ///< 基于物理的渲染 / Physically based rendering
};

/**
 * @brief 二维向量类型 / 2D vector type
 *
 * 用于纹理坐标等二维数据。
 * For 2D data like texture coordinates.
 */
struct Vec2f {
    float x, y;

    Vec2f() : x(0.0f), y(0.0f) {}
    Vec2f(float x_, float y_) : x(x_), y(y_) {}

    bool operator==(const Vec2f& other) const {
        return x == other.x && y == other.y;
    }
    bool operator!=(const Vec2f& other) const {
        return !(*this == other);
    }
};

/**
 * @brief 三维向量类型 / 3D vector type
 *
 * 使用 FreeCAD 现有的 Base::Vector3f 作为基础类型。
 * Uses FreeCAD's existing Base::Vector3f as the base type.
 */
using Vec3f = Base::Vector3f;
using Vec3d = Base::Vector3d;

/**
 * @brief Simple 3D vector for render abstraction layer
 *
 * This is a lightweight struct to avoid depending on Base::Vector3 in headers
 * where simple point/direction data is needed.
 */
struct Vector3 {
    float x{0.0f}, y{0.0f}, z{0.0f};

    Vector3() = default;
    Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    // Conversion from Base::Vector3f
    Vector3(const Base::Vector3f& v) : x(v.x), y(v.y), z(v.z) {}

    // Conversion to Base::Vector3f
    operator Base::Vector3f() const { return Base::Vector3f(x, y, z); }

    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }
    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }
    Vector3 operator*(float s) const {
        return Vector3(x * s, y * s, z * s);
    }
};

/**
 * @brief 颜色类型 / Color type
 *
 * 使用 FreeCAD 现有的 Base::Color 作为基础类型。
 * Uses FreeCAD's existing Base::Color as the base type.
 */
using Color = ::Base::Color;

/**
 * @brief 四元数表示旋转 / Quaternion for rotation representation
 */
struct Quaternion {
    float x, y, z, w;

    Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
    Quaternion(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

    bool operator==(const Quaternion& other) const {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }
    bool operator!=(const Quaternion& other) const {
        return !(*this == other);
    }

    /**
     * @brief 转换为轴角表示 / Convert to axis-angle representation
     * @param axis 旋转轴 / Rotation axis
     * @param angle 旋转角度（弧度）/ Rotation angle in radians
     */
    void getValue(Vec3f& axis, float& angle) const {
        angle = 2.0f * std::acos(std::clamp(w, -1.0f, 1.0f));
        float s = std::sqrt(1.0f - w * w);
        if (s < 0.001f) {
            // 接近零旋转，任意轴 / Near zero rotation, arbitrary axis
            axis = Vec3f(1.0f, 0.0f, 0.0f);
        } else {
            axis = Vec3f(x / s, y / s, z / s);
        }
    }

    /**
     * @brief 获取四元数分量 / Get quaternion components
     * @param x_ X 分量 / X component
     * @param y_ Y 分量 / Y component
     * @param z_ Z 分量 / Z component
     * @param w_ W 分量 / W component
     */
    void getValue(double& x_, double& y_, double& z_, double& w_) const {
        x_ = static_cast<double>(x);
        y_ = static_cast<double>(y);
        z_ = static_cast<double>(z);
        w_ = static_cast<double>(w);
    }

    /**
     * @brief 从轴角设置四元数 / Set quaternion from axis-angle
     * @param axis 旋转轴（应该是单位向量）/ Rotation axis (should be normalized)
     * @param angle 旋转角度（弧度）/ Rotation angle in radians
     */
    void setValue(const Vec3f& axis, float angle) {
        float halfAngle = angle * 0.5f;
        float s = std::sin(halfAngle);
        x = axis.x * s;
        y = axis.y * s;
        z = axis.z * s;
        w = std::cos(halfAngle);
    }
};

/**
 * @brief 4x4 矩阵 / 4x4 matrix
 *
 * 使用 FreeCAD 现有的 Base::Matrix4D 作为基础类型。
 * Uses FreeCAD's existing Base::Matrix4D as the base type.
 */
using Matrix4 = Base::Matrix4D;

/**
 * @brief 轴对齐边界框 / Axis-aligned bounding box
 *
 * 使用 FreeCAD 现有的 Base::BoundBox3d 作为基础类型。
 * Uses FreeCAD's existing Base::BoundBox3d as the base type.
 */
using BoundingBox = Base::BoundBox3d;

/**
 * @brief 材质属性 / Material properties
 *
 * 基础材质属性，支持传统着色和 PBR。
 * Basic material properties supporting both traditional shading and PBR.
 */
struct Material {
    float ambientIntensity{0.2f};      ///< 环境光强度 / Ambient intensity
    Color ambientColor{0.2f, 0.2f, 0.2f, 1.0f};  ///< 环境光颜色 / Ambient color
    Color diffuseColor{0.8f, 0.8f, 0.8f, 1.0f};  ///< 漫反射颜色 / Diffuse color
    Color specularColor{0.0f, 0.0f, 0.0f, 1.0f}; ///< 镜面反射颜色 / Specular color
    Color emissiveColor{0.0f, 0.0f, 0.0f, 1.0f}; ///< 自发光颜色 / Emissive color
    float shininess{0.2f};               ///< 镜面反射指数 / Specular exponent [0-1]
    float transparency{0.0f};            ///< 透明度 / Transparency [0-1]

    // PBR 材质属性 / PBR material properties
    float metallic{0.0f};                ///< 金属度 / Metallic [0-1]
    float roughness{0.5f};               ///< 粗糙度 / Roughness [0-1]
    float ao{1.0f};                      ///< 环境光遮蔽 / Ambient occlusion [0-1]
    float normalScale{1.0f};             ///< 法线贴图强度 / Normal map scale

    // 纹理路径 / Texture paths
    std::string albedoTexture;           ///< 反照率纹理 / Albedo texture
    std::string normalTexture;           ///< 法线纹理 / Normal texture
    std::string metallicTexture;         ///< 金属度纹理 / Metallic texture
    std::string roughnessTexture;        ///< 粗糙度纹理 / Roughness texture
    std::string aoTexture;               ///< AO 纹理 / AO texture
    std::string emissiveTexture;         ///< 自发光纹理 / Emissive texture

    ShadeModel shadeModel{ShadeModel::Phong};  ///< 着色模型 / Shading model

    bool isPBR() const { return shadeModel == ShadeModel::PBR; }
};

/**
 * @brief 相机参数 / Camera parameters
 */
struct CameraParams {
    Vec3f position{0.0f, 0.0f, 10.0f};        ///< 相机位置 / Camera position
    Vec3f target{0.0f, 0.0f, 0.0f};           ///< 目标点 / Target point
    Vec3f direction{0.0f, 0.0f, -1.0f};       ///< 观察方向 / View direction
    Vec3f up{0.0f, 1.0f, 0.0f};               ///< 上方向 / Up direction
    Vec3f upVector{0.0f, 1.0f, 0.0f};         ///< 上向量（别名）/ Up vector (alias)
    float fieldOfView{45.0f};                  ///< 视野角度 / Field of view (degrees)
    float nearDistance{0.1f};                  ///< 近裁剪面 / Near clipping distance
    float nearPlane{0.1f};                     ///< 近裁剪面（别名）/ Near plane (alias)
    float farDistance{1000.0f};                ///< 远裁剪面 / Far clipping distance
    float farPlane{1000.0f};                   ///< 远裁剪面（别名）/ Far plane (alias)
    float aspectRatio{1.0f};                   ///< 宽高比 / Aspect ratio
    bool orthographic{false};                  ///< 是否正交投影 / Orthographic projection
    float height{10.0f};                       ///< 正交投影高度 / Orthographic height
};

/**
 * @brief 灯光类型 / Light type
 */
enum class LightType : uint8_t {
    Directional = 0,    ///< 方向光 / Directional light
    Point = 1,          ///< 点光源 / Point light
    Spot = 2,           ///< 聚光灯 / Spot light
    Ambient = 3         ///< 环境光 / Ambient light
};

/**
 * @brief 灯光参数 / Light parameters
 */
struct LightParams {
    LightType type{LightType::Directional};
    Color color{1.0f, 1.0f, 1.0f, 1.0f};
    float intensity{1.0f};

    Vec3f position{0.0f, 0.0f, 10.0f};        ///< 位置 (Point/Spot) / Position
    Vec3f direction{0.0f, 0.0f, -1.0f};       ///< 方向 (Directional/Spot) / Direction

    float constantAttenuation{1.0f};          ///< 常数衰减 / Constant attenuation
    float linearAttenuation{0.0f};            ///< 线性衰减 / Linear attenuation
    float quadraticAttenuation{0.0f};         ///< 二次衰减 / Quadratic attenuation

    float spotCutOff{180.0f};                 ///< 聚光灯截止角 / Spot cutoff (degrees)
    float spotExponent{0.0f};                 ///< 聚光灯指数 / Spot exponent

    bool on{true};                            ///< 开关状态 / On/off state
};

// Note: PickResult and related picking types are defined in InteractionTypes.h
// 注意：PickResult 和相关拾取类型在 InteractionTypes.h 中定义

/**
 * @brief 后端信息 / Backend information
 */
struct BackendInfo {
    BackendType type{BackendType::None};
    std::string name;
    std::string version;
    std::string description;
    bool supportsPBR{false};
    bool supportsDeferredRendering{false};
    bool supportsRaytracing{false};
};

/**
 * @brief 渲染统计信息 / Rendering statistics
 */
struct RenderStats {
    uint32_t frameCount{0};
    uint32_t drawCalls{0};
    uint32_t triangleCount{0};
    uint32_t vertexCount{0};
    double frameTime{0.0};                    ///< 毫秒 / Milliseconds
    double fps{0.0};
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_RENDERTYPES_H
