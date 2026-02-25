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

#ifndef GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSELIGHT_H
#define GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSELIGHT_H

#include <memory>
#include <vector>
#include <string>

#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Uniform>
#include <osg/StateSet>

#include <FCGlobal.h>
#include "../../Core/RenderTypes.h"

namespace Gui {
namespace Render {

/**
 * @brief OsgVerse 光源 / OsgVerse light
 *
 * 支持三种光源类型：方向光、点光源、聚光灯。
 * Supports three light types: directional, point, and spot lights.
 *
 * 设计原则 / Design Principles:
 * 1. 支持物理基础的光照计算
 * 2. 兼容 PBR 材质系统
 * 3. 高效的 uniform 更新
 * 4. 支持阴影（预留接口）
 */
class GuiExport OsgVerseLight {
public:
    /**
     * @brief 构造函数 / Constructor
     * @param type 光源类型 / Light type
     */
    explicit OsgVerseLight(LightType type = LightType::Directional);

    /**
     * @brief 析构函数 / Destructor
     */
    ~OsgVerseLight() = default;

    // 禁止拷贝，允许移动 / Disable copy, allow move
    OsgVerseLight(const OsgVerseLight&) = delete;
    OsgVerseLight& operator=(const OsgVerseLight&) = delete;
    OsgVerseLight(OsgVerseLight&&) = default;
    OsgVerseLight& operator=(OsgVerseLight&&) = default;

    //-----------------------------------------------------------------------
    // 基本属性 / Basic Properties
    //-----------------------------------------------------------------------

    /**
     * @brief 设置光源类型 / Set light type
     */
    void setType(LightType type);
    LightType getType() const { return _type; }

    /**
     * @brief 设置光源名称 / Set light name
     */
    void setName(const std::string& name) { _name = name; }
    std::string getName() const { return _name; }

    /**
     * @brief 启用/禁用光源 / Enable/disable light
     */
    void setEnabled(bool enabled) { _enabled = enabled; }
    bool isEnabled() const { return _enabled; }

    //-----------------------------------------------------------------------
    // 光照参数 / Lighting Parameters
    //-----------------------------------------------------------------------

    /**
     * @brief 设置光源颜色 / Set light color
     * @param color RGB颜色值 / RGB color value
     */
    void setColor(const Color& color);
    Color getColor() const { return _color; }

    /**
     * @brief 设置光源强度 / Set light intensity
     * @param intensity 强度值（通常0-10）/ Intensity value (typically 0-10)
     */
    void setIntensity(float intensity);
    float getIntensity() const { return _intensity; }

    //-----------------------------------------------------------------------
    // 方向光参数 / Directional Light Parameters
    //-----------------------------------------------------------------------

    /**
     * @brief 设置光源方向（用于方向光和聚光灯）/ Set light direction (for directional and spot lights)
     * @param direction 方向向量（会自动归一化）/ Direction vector (will be normalized)
     */
    void setDirection(const osg::Vec3& direction);
    osg::Vec3 getDirection() const { return _direction; }

    //-----------------------------------------------------------------------
    // 点光源和聚光灯参数 / Point and Spot Light Parameters
    //-----------------------------------------------------------------------

    /**
     * @brief 设置光源位置（用于点光源和聚光灯）/ Set light position (for point and spot lights)
     */
    void setPosition(const osg::Vec3& position);
    osg::Vec3 getPosition() const { return _position; }

    /**
     * @brief 设置衰减参数 / Set attenuation parameters
     * @param constant 常数衰减 / Constant attenuation
     * @param linear 线性衰减 / Linear attenuation
     * @param quadratic 二次衰减 / Quadratic attenuation
     */
    void setAttenuation(float constant, float linear, float quadratic);
    float getConstantAttenuation() const { return _constantAttenuation; }
    float getLinearAttenuation() const { return _linearAttenuation; }
    float getQuadraticAttenuation() const { return _quadraticAttenuation; }

    /**
     * @brief 设置光源范围 / Set light range
     * @param range 最大影响距离 / Maximum influence distance
     */
    void setRange(float range);
    float getRange() const { return _range; }

    //-----------------------------------------------------------------------
    // 聚光灯参数 / Spot Light Parameters
    //-----------------------------------------------------------------------

    /**
     * @brief 设置聚光灯内锥角 / Set spot light inner cone angle
     * @param angle 角度（度）/ Angle in degrees
     */
    void setInnerConeAngle(float angle);
    float getInnerConeAngle() const { return _innerConeAngle; }

    /**
     * @brief 设置聚光灯外锥角 / Set spot light outer cone angle
     * @param angle 角度（度）/ Angle in degrees
     */
    void setOuterConeAngle(float angle);
    float getOuterConeAngle() const { return _outerConeAngle; }

    //-----------------------------------------------------------------------
    // 阴影参数（预留）/ Shadow Parameters (Reserved)
    //-----------------------------------------------------------------------

    /**
     * @brief 启用/禁用阴影投射 / Enable/disable shadow casting
     */
    void setCastShadow(bool cast) { _castShadow = cast; }
    bool getCastShadow() const { return _castShadow; }

    /**
     * @brief 设置阴影偏移 / Set shadow bias
     */
    void setShadowBias(float bias) { _shadowBias = bias; }
    float getShadowBias() const { return _shadowBias; }

    //-----------------------------------------------------------------------
    // Uniform 更新 / Uniform Update
    //-----------------------------------------------------------------------

    /**
     * @brief 更新 uniform 到 StateSet / Update uniforms to StateSet
     * @param stateSet 目标状态集 / Target state set
     * @param index 光源索引 / Light index
     */
    void updateUniforms(osg::StateSet* stateSet, int index) const;

private:
    // 基本属性 / Basic properties
    LightType _type;
    std::string _name;
    bool _enabled{true};

    // 光照参数 / Lighting parameters
    Color _color{1.0f, 1.0f, 1.0f, 1.0f};
    float _intensity{1.0f};

    // 方向光参数 / Directional light parameters
    osg::Vec3 _direction{0.0f, 0.0f, -1.0f};

    // 点光源和聚光灯参数 / Point and spot light parameters
    osg::Vec3 _position{0.0f, 0.0f, 0.0f};
    float _constantAttenuation{1.0f};
    float _linearAttenuation{0.09f};
    float _quadraticAttenuation{0.032f};
    float _range{100.0f};

    // 聚光灯参数 / Spot light parameters
    float _innerConeAngle{12.5f};  // degrees
    float _outerConeAngle{17.5f};  // degrees

    // 阴影参数 / Shadow parameters
    bool _castShadow{false};
    float _shadowBias{0.005f};
};

/**
 * @brief 光源管理器 / Light manager
 *
 * 管理场景中的所有光源，并将光源数据传递给着色器。
 * Manages all lights in the scene and passes light data to shaders.
 *
 * 设计原则 / Design Principles:
 * 1. 单例模式，全局唯一
 * 2. 支持最多 MAX_LIGHTS 个光源
 * 3. 自动更新所有材质的光源 uniforms
 * 4. 高效的批量更新
 */
class GuiExport OsgVerseLightManager {
public:
    /**
     * @brief 最大光源数量 / Maximum number of lights
     */
    static constexpr int MAX_LIGHTS = 8;

    /**
     * @brief 获取单例实例 / Get singleton instance
     */
    static OsgVerseLightManager& instance();

    /**
     * @brief 添加光源 / Add light
     * @param light 光源对象 / Light object
     * @return 光源索引，-1表示失败 / Light index, -1 for failure
     */
    int addLight(std::shared_ptr<OsgVerseLight> light);

    /**
     * @brief 移除光源 / Remove light
     * @param index 光源索引 / Light index
     */
    void removeLight(int index);

    /**
     * @brief 移除光源（按名称）/ Remove light (by name)
     */
    void removeLight(const std::string& name);

    /**
     * @brief 获取光源 / Get light
     * @param index 光源索引 / Light index
     */
    std::shared_ptr<OsgVerseLight> getLight(int index) const;

    /**
     * @brief 获取光源（按名称）/ Get light (by name)
     */
    std::shared_ptr<OsgVerseLight> getLight(const std::string& name) const;

    /**
     * @brief 获取光源数量 / Get light count
     */
    int getLightCount() const;

    /**
     * @brief 清除所有光源 / Clear all lights
     */
    void clear();

    /**
     * @brief 更新所有光源的 uniforms / Update uniforms for all lights
     * @param stateSet 目标状态集 / Target state set
     */
    void updateUniforms(osg::StateSet* stateSet) const;

    /**
     * @brief 创建默认光源 / Create default lights
     *
     * 创建一个默认的方向光（模拟太阳光）。
     * Creates a default directional light (simulating sunlight).
     */
    void createDefaultLights();

    /**
     * @brief 设置环境光颜色 / Set ambient light color
     */
    void setAmbientColor(const Color& color);
    Color getAmbientColor() const { return _ambientColor; }

private:
    OsgVerseLightManager();
    ~OsgVerseLightManager() = default;

    // 禁止拷贝和移动 / Disable copy and move
    OsgVerseLightManager(const OsgVerseLightManager&) = delete;
    OsgVerseLightManager& operator=(const OsgVerseLightManager&) = delete;

    std::vector<std::shared_ptr<OsgVerseLight>> _lights;
    Color _ambientColor{0.2f, 0.2f, 0.2f, 1.0f};
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSELIGHT_H
