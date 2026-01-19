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
# include <osg/StateSet>
# include <osg/Material>
# include <osg/Texture2D>
# include <osg/Image>
# include <osg/PolygonMode>
# include <osg/CullFace>
# include <osg/BlendFunc>
# include <osg/Depth>
# include <osg/Uniform>
# include <osgDB/ReadFile>
#endif

#include "OsgVerseMaterial.h"
#include <Base/Console.h>

using namespace Gui::Render;

//===========================================================================
// OsgVerseMaterial Implementation
//===========================================================================

OsgVerseMaterial::OsgVerseMaterial()
    : RenderNode(NodeType::Material)
    , _stateSet(new osg::StateSet())
{
    // 初始化默认状态 / Initialize default state
    updateStateSet();
}

OsgVerseMaterial::~OsgVerseMaterial() = default;

//-----------------------------------------------------------------------
// PBR 材质参数 / PBR Material Parameters
//-----------------------------------------------------------------------

void OsgVerseMaterial::setBaseColor(const Color& color)
{
    _baseColor = color;

    // 设置 uniform / Set uniform
    auto uniform = new osg::Uniform("baseColor",
        osg::Vec4f(color.r, color.g, color.b, color.a));
    _stateSet->addUniform(uniform);
}

Color OsgVerseMaterial::getBaseColor() const
{
    return _baseColor;
}

void OsgVerseMaterial::setMetallic(float metallic)
{
    _metallic = std::clamp(metallic, 0.0f, 1.0f);

    auto uniform = new osg::Uniform("metallic", _metallic);
    _stateSet->addUniform(uniform);
}

float OsgVerseMaterial::getMetallic() const
{
    return _metallic;
}

void OsgVerseMaterial::setRoughness(float roughness)
{
    _roughness = std::clamp(roughness, 0.0f, 1.0f);

    auto uniform = new osg::Uniform("roughness", _roughness);
    _stateSet->addUniform(uniform);
}

float OsgVerseMaterial::getRoughness() const
{
    return _roughness;
}

void OsgVerseMaterial::setEmissive(const Color& color)
{
 ive = color;

    auto uniform = new osg::Uniform("emissive",
        osg::Vec4f(color.r, color.g, color.b, color.a));
    _stateSet->addUniform(uniform);
}

Color OsgVerseMaterial::getEmissive() const
{
    return _emissive;
}

void OsgVerseMaterial::setOpacity(float opacity)
{
    _opacity = std::clamp(opacity, 0.0f, 1.0f);

    // 更新基础颜色的 alpha 通道 / Update base color alpha channel
    _baseColor.a = _opacity;
    setBaseColor(_baseColor);

    // 如果透明度 < 1.0，启用混合 / Enable blending if opacity < 1.0
    if (_opacity < 1.0f) {
        setBlendMode(BlendMode::Transparent);
    }
}

float OsgVerseMaterial::getOpacity() const
{
    return _opacity;
}

void OsgVerseMaterial::setAmbientOcclusion(float ao)
{
    _ambientOcclusion = std::clamp(ao, 0.0f, 1.0f);

    auto uniform = new osg::Uniform("ambientOcclusion", _ambientOcclusion);
    _stateSet->addUniform(uniform);
}

float OsgVerseMaterial::getAmbientOcclusion() const
{
    return _ambientOcclusion;
}

//-----------------------------------------------------------------------
// 纹理贴图 / Texture Maps
//-----------------------------------------------------------------------

int OsgVerseMaterial::getTextureUnit(TextureType type) const
{
    // 纹理单元映射 / Texture unit mapping
    switch (type) {
        case TextureType::BaseColor:         return 0;
        case TextureType::Normal:            return 1;
        case TextureType::MetallicRoughness: return 2;
        case TextureType::Occlusion:         return 3;
        case TextureType::Emissive:          return 4;
        case TextureType::Height:            return 5;
        case TextureType::Opacity:           return 6;
        default:                             return 0;
    }
}

bool OsgVerseMaterial::setTexture(TextureType type, const std::string& filename)
{
    // 加载图像 / Load image
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile(filename);
    if (!image) {
        Base::Console().Warning("OsgVerseMaterial: Failed to load texture: %s\n",
                                filename.c_str());
        return false;
    }

    return setTexture(type, image.get());
}

bool OsgVerseMaterial::setTexture(TextureType type, osg::Image* image)
{
    if (!image) {
        return false;
    }

    // 创建纹理 / Create texture
    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D(image);
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
    texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

    // 获取纹理单元 / Get texture unit
    int unit = getTextureUnit(type);

    // 设置到 StateSet / Set to StateSet
    _stateSet->setTextureAttributeAndModes(unit, texture.get(),
                                           osg::StateAttribute::ON);

    // 保存引用 / Save reference
    _textures[type] = texture;

    // 设置 uniform 标志 / Set uniform flag
    std::string uniformName;
    switch (type) {
        case TextureType::BaseColor:
            uniformName = "hasBaseColorTexture";
            break;
        case TextureType::Normal:
            uniformName = "hasNormalTexture";
            break;
        case TextureType::MetallicRoughness:
            uniformName = "hasMetallicRoughnessTexture";
            break;
        case TextureType::Occlusion:
            uniformName = "hasOcclusionTexture";
            break;
        case TextureType::Emissive:
            uniformName = "hasEmissiveTexture";
            break;
        case TextureType::Height:
            uniformName = "hasHeightTexture";
            break;
        case TextureType::Opacity:
            uniformName = "hasOpacityTexture";
            break;
    }

    if (!uniformName.empty()) {
        _stateSet->addUniform(new osg::Uniform(uniformName.c_str(), true));
    }

    return true;
}

void OsgVerseMaterial::removeTexture(TextureType type)
{
    int unit = getTextureUnit(type);
    _stateSet->removeTextureAttribute(unit, osg::StateAttribute::TEXTURE);
    _textures.erase(type);
}

bool OsgVerseMaterial::hasTexture(TextureType type) const
{
    return _textures.find(type) != _textures.end();
}

osg::Texture2D* OsgVerseMaterial::getTexture(TextureType type) const
{
    auto it = _textures.find(type);
    if (it != _textures.end()) {
        return it->second.get();
    }
    return nullptr;
}

//-----------------------------------------------------------------------
// Coin3D 兼容性 / Coin3D Compatibility
//-----------------------------------------------------------------------

void OsgVerseMaterial::setFromCoin3DMaterial(const Material& coinMaterial)
{
    // 转换环境光 / Convert ambient
    setAmbientColor(coinMaterial.ambient);

    // 转换漫反射（作为基础颜色）/ Convert diffuse (as base color)
    setDiffuseColor(coinMaterial.diffuse);
    setBaseColor(coinMaterial.diffuse);

    // 转换镜面反射 / Convert specular
    setSpecularColor(coinMaterial.specular);

    // 转换自发光 / Convert emissive
    setEmissive(coinMaterial.emissive);

    // 转换光泽度到粗糙度 / Convert shininess to roughness
    setShininess(coinMaterial.shininess);

    // 转换透明度 / Convert transparency
    setOpacity(1.0f - coinMaterial.transparency);

    // 根据镜面反射强度估算金属度 / Estimate metallic from specular intensity
    float specularIntensity = (coinMaterial.specular.r +
                               coinMaterial.specular.g +
                               coinMaterial.specular.b) / 3.0f;
    setMetallic(specularIntensity);
}

void OsgVerseMaterial::setAmbientColor(const Color& color)
{
    _ambientColor = color;

    // 在 PBR 中，环境光通过环境光遮蔽实现
    // In PBR, ambient is handled via ambient occlusion
    float aoValue = (color.r + color.g + color.b) / 3.0f;
    setAmbientOcclusion(aoValue);
}

Color OsgVerseMaterial::getAmbientColor() const
{
    return _ambientColor;
}

void OsgVerseMaterial::setDiffuseColor(const Color& color)
{
    _diffuseColor = color;
    // 漫反射颜色映射到基础颜色 / Diffuse maps to base color
    setBaseColor(color);
}

Color OsgVerseMaterial::getDiffuseColor() const
{
    return _diffuseColor;
}

void OsgVerseMaterial::setSpecularColor(const Color& color)
{
    _specularColor = color;

    // 镜面反射颜色影响金属度 / Specular color affects metallic
    float intensity = (color.r + color.g + color.b) / 3.0f;
    setMetallic(intensity);
}

Color OsgVerseMaterial::getSpecularColor() const
{
    return _specularColor;
}

void OsgVerseMaterial::setShininess(float shininess)
{
    _shininess = std::clamp(shininess, 0.0f, 128.0f);

    // 转换光泽度到粗糙度 / Convert shininess to roughness
    float roughness = shininessToRoughness(_shininess);
    setRoughness(roughness);
}

float OsgVerseMaterial::getShininess() const
{
    return _shininess;
}

float OsgVerseMaterial::shininessToRoughness(float shininess) const
{
    // Shininess 范围 [0, 128]，Roughness 范围 [0, 1]
    // Shininess range [0, 128], Roughness range [0, 1]
    // 高光泽度 = 低粗糙度 / High shininess = Low roughness
    return 1.0f - (shininess / 128.0f);
}

float OsgVerseMaterial::roughnessToShininess(float roughness) const
{
    return (1.0f - roughness) * 128.0f;
}

//-----------------------------------------------------------------------
// 渲染状态 / Rendering State
//-----------------------------------------------------------------------

void OsgVerseMaterial::setTwoSided(bool twoSided)
{
    _twoSided = twoSided;

    if (twoSided) {
        // 禁用背面剔除 / Disable backface culling
        _stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    }
    else {
        // 启用背面剔除 / Enable backface culling
        auto cullFace = new osg::CullFace(osg::CullFace::BACK);
        _stateSet->setAttributeAndModes(cullFace, osg::StateAttribute::ON);
    }
}

bool OsgVerseMaterial::isTwoSided() const
{
    return _twoSided;
}

void OsgVerseMaterial::setWireframe(bool wireframe)
{
    _wireframe = wireframe;

    auto polyMode = new osg::PolygonMode();
    if (wireframe) {
        polyMode->setMode(osg::PolygonMode::FRONT_AND_BACK,
                         osg::PolygonMode::LINE);
    }
    else {
        polyMode->setMode(osg::PolygonMode::FRONT_AND_BACK,
                         osg::PolygonMode::FILL);
    }

    _stateSet->setAttributeAndModes(polyMode, osg::StateAttribute::ON);
}

bool OsgVerseMaterial::isWireframe() const
{
    return _wireframe;
}

void OsgVerseMaterial::setBlendMode(BlendMode mode)
{
    _blendMode = mode;

    switch (mode) {
        case BlendMode::Opaque:
            // 禁用混合 / Disable blending
            _stateSet->setMode(GL_BLEND, osg::StateAttribute::OFF);
            _stateSet->setRenderingHint(osg::StateSet::OPAQUE_BIN);
            break;

        case BlendMode::Transparent:
            // 标准 alpha 混合 / Standard alpha blending
            {
                auto blendFunc = new osg::BlendFunc(
                    osg::BlendFunc::SRC_ALPHA,
                    osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
                _stateSet->setAttributeAndModes(blendFunc, osg::StateAttribute::ON);
                _stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
            }
            break;

        case BlendMode::Additive:
            // 加法混合 / Additive blending
            {
                auto blendFunc = new osg::BlendFunc(
                    osg::BlendFunc::SRC_ALPHA,
                    osg::BlendFunc::ONE);
                _stateSet->setAttributeAndModes(blendFunc, osg::StateAttribute::ON);
                _stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
            }
            break;

        case BlendMode::Multiply:
            // 乘法混合 / Multiply blending
            {
                auto blendFunc = new osg::BlendFunc(
                    osg::BlendFunc::DST_COLOR,
                    osg::BlendFunc::ZERO);
                _stateSet->setAttributeAndModes(blendFunc, osg::StateAttribute::ON);
                _stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
            }
            break;
    }
}

OsgVerseMaterial::BlendMode OsgVerseMaterial::getBlendMode() const
{
    return _blendMode;
}

void OsgVerseMaterial::setDepthTest(bool enable)
{
    _depthTest = enable;

    if (enable) {
        _stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    }
    else {
        _stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    }
}

bool OsgVerseMaterial::isDepthTestEnabled() const
{
    return _depthTest;
}

void OsgVerseMaterial::setDepthWrite(bool enable)
{
    _depthWrite = enable;

    auto depth = new osg::Depth();
    depth->setWriteMask(enable);
    _stateSet->setAttributeAndModes(depth, osg::StateAttribute::ON);
}

bool OsgVerseMaterial::isDepthWriteEnabled() const
{
    return _depthWrite;
}

//-----------------------------------------------------------------------
// Uniform 管理 / Uniform Management
//-----------------------------------------------------------------------

void OsgVerseMaterial::setUniform(const std::string& name, float value)
{
    _stateSet->addUniform(new osg::Uniform(name.c_str(), value));
}

void OsgVerseMaterial::setUniform(const std::string& name, const osg::Vec2f& value)
{
    _stateSet->addUniform(new osg::Uniform(name.c_str(), value));
}

void OsgVerseMaterial::setUniform(const std::string& name, const osg::Vec3f& value)
{
    _stateSet->addUniform(new osg::Uniform(name.c_str(), value));
}

void OsgVerseMaterial::setUniform(const std::string& name, const osg::Vec4f& value)
{
    _stateSet->addUniform(new osg::Uniform(name.c_str(), value));
}

bool OsgVerseMaterial::getUniform(const std::string& name, float& value) const
{
    auto uniform = _stateSet->getUniform(name);
    if (uniform) {
        return uniform->get(value);
    }
    return false;
}

bool OsgVerseMaterial::getUniform(const std::string& name, osg::Vec2f& value) const
{
    auto uniform = _stateSet->getUniform(name);
    if (uniform) {
        return uniform->get(value);
    }
    return false;
}

bool OsgVerseMaterial::getUniform(const std::string& name, osg::Vec3f& value) const
{
    auto uniform = _stateSet->getUniform(name);
    if (uniform) {
        return uniform->get(value);
    }
    return false;
}

bool OsgVerseMaterial::getUniform(const std::string& name, osg::Vec4f& value) const
{
    auto uniform = _stateSet->getUniform(name);
    if (uniform) {
        return uniform->get(value);
    }
    return false;
}

void OsgVerseMaterial::removeUniform(const std::string& name)
{
    _stateSet->removeUniform(name);
}

//-----------------------------------------------------------------------
// 工具方法 / Utility Methods
//-----------------------------------------------------------------------

void OsgVerseMaterial::applyToNode(osg::Node* node)
{
    if (node) {
        node->setStateSet(_stateSet.get());
    }
}

std::unique_ptr<OsgVerseMaterial> OsgVerseMaterial::clone() const
{
    auto cloned = std::make_unique<OsgVerseMaterial>();

    // 复制 PBR 参数 / Copy PBR parameters
    cloned->setBaseColor(_baseColor);
    cloned->setMetallic(_metallic);
    cloned->setRoughness(_roughness);
    cloned->setEmissive(_emissive);
    cloned->setOpacity(_opacity);
    cloned->setAmbientOcclusion(_ambientOcclusion);

    // 复制 Coin3D 参数 / Copy Coin3D parameters
    cloned->setAmbientColor(_ambientColor);
    cloned->setDiffuseColor(_diffuseColor);
    cloned->setSpecularColor(_specularColor);
    cloned->setShininess(_shininess);

    // 复制渲染状态 / Copy rendering state
    cloned->setTwoSided(_twoSided);
    cloned->setWireframe(_wireframe);
    cloned->setBlendMode(_blendMode);
    cloned->setDepthTest(_depthTest);
    cloned->setDepthWrite(_depthWrite);

    // 复制纹理 / Copy textures
    for (const auto& [type, texture] : _textures) {
        if (texture && texture->getImage()) {
            cloned->setTexture(type, texture->getImage());
        }
    }

    return cloned;
}

void OsgVerseMaterial::reset()
{
    // 重置 PBR 参数 / Reset PBR parameters
    _baseColor = Color{0.8f, 0.8f, 0.8f, 1.0f};
    _metallic = 0.0f;
    _roughness = 0.5f;
    _emissive = Color{0.0f, 0.0f, 0.0f, 0.0f};
    _opacity = 1.0f;
    _ambientOcclusion = 1.0f;

    // 重置 Coin3D 参数 / Reset Coin3D parameters
    _ambientColor = Color{0.2f, 0.2f, 0.2f, 1.0f};
    _diffuseColor = Color{0.8f, 0.8f, 0.8f, 1.0f};
    _specularColor = Color{0.0f, 0.0f, 0.0f, 1.0f};
    _shininess = 20.0f;

    // 清除纹理 / Clear textures
    _textures.clear();

    // 重置渲染状态 / Reset rendering state
    _twoSided = false;
    _wireframe = false;
    _blendMode = BlendMode::Opaque;
    _depthTest = true;
    _depthWrite = true;

    // 重新创建 StateSet / Recreate StateSet
    _stateSet = new osg::StateSet();
    updateStateSet();
}

void OsgVerseMaterial::updateStateSet()
{
    // 应用所有当前参数到 StateSet
    // Apply all current parameters to StateSet
    setBaseColor(_baseColor);
    setMetallic(_metallic);
    setRoughness(_roughness);
    setEmissive(_emissive);
    setAmbientOcclusion(_ambientOcclusion);

    setTwoSided(_twoSided);
    setWireframe(_wireframe);
    setBlendMode(_blendMode);
    setDepthTest(_depthTest);
    setDepthWrite(_depthWrite);
}

//===========================================================================
// OsgVerseMaterialManager Implementation
//===========================================================================

OsgVerseMaterialManager& OsgVerseMaterialManager::instance()
{
    static OsgVerseMaterialManager instance;
    return instance;
}

std::shared_ptr<OsgVerseMaterial> OsgVerseMaterialManager::createMaterial(const std::string& name)
{
    auto material = std::make_shared<OsgVerseMaterial>();

    if (!name.empty()) {
        registerMaterial(name, material);
    }
    else {
        // 生成唯一名称 / Generate unique name
        std::string autoName = "Material_" + std::to_string(_nextId++);
        registerMaterial(autoName, material);
    }

    return material;
}

std::shared_ptr<OsgVerseMaterial> OsgVerseMaterialManager::getMaterial(const std::string& name) const
{
    auto it = _materials.find(name);
    if (it != _materials.end()) {
        return it->second;
    }
    return nullptr;
}

void OsgVerseMaterialManager::registerMaterial(const std::string& name,
                                               std::shared_ptr<OsgVerseMaterial> material)
{
    _materials[name] = material;
}

void OsgVerseMaterialManager::removeMaterial(const std::string& name)
{
    _materials.erase(name);
}

void OsgVerseMaterialManager::clear()
{
    _materials.clear();
    _nextId = 0;
}

size_t OsgVerseMaterialManager::getMaterialCount() const
{
    return _materials.size();
}
