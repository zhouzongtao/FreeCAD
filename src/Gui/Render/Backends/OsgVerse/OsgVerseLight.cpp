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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <algorithm>
#include <cmath>
#endif

#include "OsgVerseLight.h"

using namespace Gui::Render;

//===========================================================================
// OsgVerseLight
//===========================================================================

OsgVerseLight::OsgVerseLight(LightType type)
    : _type(type)
{
    // 根据类型设置默认值 / Set defaults based on type
    switch (_type) {
        case LightType::Directional:
            _name = "DirectionalLight";
            _direction = osg::Vec3(0.0f, -0.707f, -0.707f);  // 45度角向下
            break;
        case LightType::Point:
            _name = "PointLight";
            _position = osg::Vec3(0.0f, 0.0f, 10.0f);
            break;
        case LightType::Spot:
            _name = "SpotLight";
            _position = osg::Vec3(0.0f, 0.0f, 10.0f);
            _direction = osg::Vec3(0.0f, 0.0f, -1.0f);
            break;
    }
}

void OsgVerseLight::setType(LightType type)
{
    _type = type;
}

void OsgVerseLight::setColor(const Color& color)
{
    _color = color;
}

void OsgVerseLight::setIntensity(float intensity)
{
    _intensity = std::max(0.0f, intensity);
}

void OsgVerseLight::setDirection(const osg::Vec3& direction)
{
    _direction = direction;
    _direction.normalize();
}

void OsgVerseLight::setPosition(const osg::Vec3& position)
{
    _position = position;
}

void OsgVerseLight::setAttenuation(float constant, float linear, float quadratic)
{
    _constantAttenuation = std::max(0.0f, constant);
    _linearAttenuation = std::max(0.0f, linear);
    _quadraticAttenuation = std::max(0.0f, quadratic);
}

void OsgVerseLight::setRange(float range)
{
    _range = std::max(0.1f, range);
}

void OsgVerseLight::setInnerConeAngle(float angle)
{
    _innerConeAngle = std::clamp(angle, 0.0f, 90.0f);
}

void OsgVerseLight::setOuterConeAngle(float angle)
{
    _outerConeAngle = std::clamp(angle, 0.0f, 90.0f);
    // 确保外锥角不小于内锥角 / Ensure outer angle is not less than inner angle
    if (_outerConeAngle < _innerConeAngle) {
        _outerConeAngle = _innerConeAngle;
    }
}

void OsgVerseLight::updateUniforms(osg::StateSet* stateSet, int index) const
{
    if (!stateSet || !_enabled) {
        return;
    }

    // 创建 uniform 名称前缀 / Create uniform name prefix
    std::string prefix = "u_lights[" + std::to_string(index) + "].";

    // 光源类型 / Light type
    auto typeUniform = stateSet->getOrCreateUniform(prefix + "type", osg::Uniform::INT);
    typeUniform->set(static_cast<int>(_type));

    // 光源颜色和强度 / Light color and intensity
    osg::Vec3 colorIntensity(_color.r * _intensity, _color.g * _intensity, _color.b * _intensity);
    auto colorUniform = stateSet->getOrCreateUniform(prefix + "color", osg::Uniform::FLOAT_VEC3);
    colorUniform->set(colorIntensity);

    // 根据类型设置不同的参数 / Set different parameters based on type
    switch (_type) {
        case LightType::Directional: {
            auto dirUniform = stateSet->getOrCreateUniform(prefix + "direction", osg::Uniform::FLOAT_VEC3);
            dirUniform->set(_direction);
            break;
        }
        case LightType::Point: {
            auto posUniform = stateSet->getOrCreateUniform(prefix + "position", osg::Uniform::FLOAT_VEC3);
            posUniform->set(_position);

            auto attUniform = stateSet->getOrCreateUniform(prefix + "attenuation", osg::Uniform::FLOAT_VEC3);
            attUniform->set(osg::Vec3(_constantAttenuation, _linearAttenuation, _quadraticAttenuation));

            auto rangeUniform = stateSet->getOrCreateUniform(prefix + "range", osg::Uniform::FLOAT);
            rangeUniform->set(_range);
            break;
        }
        case LightType::Spot: {
            auto posUniform = stateSet->getOrCreateUniform(prefix + "position", osg::Uniform::FLOAT_VEC3);
            posUniform->set(_position);

            auto dirUniform = stateSet->getOrCreateUniform(prefix + "direction", osg::Uniform::FLOAT_VEC3);
            dirUniform->set(_direction);

            auto attUniform = stateSet->getOrCreateUniform(prefix + "attenuation", osg::Uniform::FLOAT_VEC3);
            attUniform->set(osg::Vec3(_constantAttenuation, _linearAttenuation, _quadraticAttenuation));

            auto rangeUniform = stateSet->getOrCreateUniform(prefix + "range", osg::Uniform::FLOAT);
            rangeUniform->set(_range);

            // 转换角度为余弦值 / Convert angles to cosine values
            float innerCos = std::cos(_innerConeAngle * M_PI / 180.0f);
            float outerCos = std::cos(_outerConeAngle * M_PI / 180.0f);
            auto coneUniform = stateSet->getOrCreateUniform(prefix + "coneAngles", osg::Uniform::FLOAT_VEC2);
            coneUniform->set(osg::Vec2(innerCos, outerCos));
            break;
        }
    }
}

//===========================================================================
// OsgVerseLightManager
//===========================================================================

OsgVerseLightManager::OsgVerseLightManager()
{
    _lights.reserve(MAX_LIGHTS);
}

OsgVerseLightManager& OsgVerseLightManager::instance()
{
    static OsgVerseLightManager instance;
    return instance;
}

int OsgVerseLightManager::addLight(std::shared_ptr<OsgVerseLight> light)
{
    if (!light) {
        return -1;
    }

    if (_lights.size() >= MAX_LIGHTS) {
        // 已达到最大光源数量 / Maximum light count reached
        return -1;
    }

    _lights.push_back(light);
    return static_cast<int>(_lights.size() - 1);
}

void OsgVerseLightManager::removeLight(int index)
{
    if (index >= 0 && index < static_cast<int>(_lights.size())) {
        _lights.erase(_lights.begin() + index);
    }
}

void OsgVerseLightManager::removeLight(const std::string& name)
{
    auto it = std::remove_if(_lights.begin(), _lights.end(),
        [&name](const std::shared_ptr<OsgVerseLight>& light) {
            return light && light->getName() == name;
        });
    _lights.erase(it, _lights.end());
}

std::shared_ptr<OsgVerseLight> OsgVerseLightManager::getLight(int index) const
{
    if (index >= 0 && index < static_cast<int>(_lights.size())) {
        return _lights[index];
    }
    return nullptr;
}

std::shared_ptr<OsgVerseLight> OsgVerseLightManager::getLight(const std::string& name) const
{
    auto it = std::find_if(_lights.begin(), _lights.end(),
        [&name](const std::shared_ptr<OsgVerseLight>& light) {
            return light && light->getName() == name;
        });

    if (it != _lights.end()) {
        return *it;
    }
    return nullptr;
}

int OsgVerseLightManager::getLightCount() const
{
    return static_cast<int>(_lights.size());
}

void OsgVerseLightManager::clear()
{
    _lights.clear();
}

void OsgVerseLightManager::updateUniforms(osg::StateSet* stateSet) const
{
    if (!stateSet) {
        return;
    }

    // 设置光源数量 / Set light count
    auto countUniform = stateSet->getOrCreateUniform("u_numLights", osg::Uniform::INT);
    countUniform->set(static_cast<int>(_lights.size()));

    // 设置环境光 / Set ambient light
    auto ambientUniform = stateSet->getOrCreateUniform("u_ambientLight", osg::Uniform::FLOAT_VEC3);
    ambientUniform->set(osg::Vec3(_ambientColor.r, _ambientColor.g, _ambientColor.b));

    // 更新每个光源的 uniforms / Update uniforms for each light
    for (size_t i = 0; i < _lights.size(); ++i) {
        if (_lights[i]) {
            _lights[i]->updateUniforms(stateSet, static_cast<int>(i));
        }
    }
}

void OsgVerseLightManager::createDefaultLights()
{
    // 清除现有光源 / Clear existing lights
    clear();

    // 创建默认方向光（模拟太阳光）/ Create default directional light (simulating sunlight)
    auto sunLight = std::make_shared<OsgVerseLight>(LightType::Directional);
    sunLight->setName("Sun");
    sunLight->setDirection(osg::Vec3(0.3f, -0.5f, -0.8f));  // 从右上方照射
    sunLight->setColor(Color(1.0f, 0.98f, 0.95f, 1.0f));    // 略带暖色的白光
    sunLight->setIntensity(1.0f);
    addLight(sunLight);

    // 设置环境光 / Set ambient light
    setAmbientColor(Color(0.2f, 0.2f, 0.25f, 1.0f));  // 略带蓝色的环境光
}

void OsgVerseLightManager::setAmbientColor(const Color& color)
{
    _ambientColor = color;
}
