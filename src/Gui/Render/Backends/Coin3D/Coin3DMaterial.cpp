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

#include "Coin3DMaterial.h"
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <App/Material.h>
#include <Base/Console.h>

namespace Gui {
namespace Render {

//===========================================================================
// Coin3DMaterial - SoMaterial 包装器 / SoMaterial Wrapper
//===========================================================================

Coin3DMaterial::Coin3DMaterial()
    : Coin3DNode(new SoMaterial(), true, NodeType::Material)
    , _materialBinding(new SoMaterialBinding())
{
    _coinNode->ref();
    _materialBinding->ref();
}

Coin3DMaterial::Coin3DMaterial(SoMaterial* material, bool ownsNode)
    : Coin3DNode(material, ownsNode, NodeType::Material)
    , _materialBinding(new SoMaterialBinding())
{
    _materialBinding->ref();
}

Coin3DMaterial::~Coin3DMaterial()
{
    if (_materialBinding) {
        _materialBinding->unref();
    }
}

SoMaterial* Coin3DMaterial::getSoMaterial() const
{
    return static_cast<SoMaterial*>(_coinNode);
}

//---------------------------------------------------------------------------
// RenderMaterial 接口实现 / RenderMaterial Interface Implementation
//---------------------------------------------------------------------------

void Coin3DMaterial::setAmbientColor(float r, float g, float b)
{
    if (auto* mat = getSoMaterial()) {
        mat->ambientColor.setValue(r, g, b);
        touch();
    }
}

void Coin3DMaterial::getAmbientColor(float& r, float& g, float& b) const
{
    if (auto* mat = getSoMaterial()) {
        const SbColor& c = mat->ambientColor[0];
        r = c[0];
        g = c[1];
        b = c[2];
    } else {
        r = g = b = 0.2f;
    }
}

void Coin3DMaterial::setDiffuseColor(float r, float g, float b)
{
    if (auto* mat = getSoMaterial()) {
        mat->diffuseColor.setValue(r, g, b);
        touch();
    }
}

void Coin3DMaterial::getDiffuseColor(float& r, float& g, float& b) const
{
    if (auto* mat = getSoMaterial()) {
        const SbColor& c = mat->diffuseColor[0];
        r = c[0];
        g = c[1];
        b = c[2];
    } else {
        r = g = b = 0.8f;
    }
}

void Coin3DMaterial::setSpecularColor(float r, float g, float b)
{
    if (auto* mat = getSoMaterial()) {
        mat->specularColor.setValue(r, g, b);
        touch();
    }
}

void Coin3DMaterial::getSpecularColor(float& r, float& g, float& b) const
{
    if (auto* mat = getSoMaterial()) {
        const SbColor& c = mat->specularColor[0];
        r = c[0];
        g = c[1];
        b = c[2];
    } else {
        r = g = b = 0.0f;
    }
}

void Coin3DMaterial::setEmissiveColor(float r, float g, float b)
{
    if (auto* mat = getSoMaterial()) {
        mat->emissiveColor.setValue(r, g, b);
        touch();
    }
}

void Coin3DMaterial::getEmissiveColor(float& r, float& g, float& b) const
{
    if (auto* mat = getSoMaterial()) {
        const SbColor& c = mat->emissiveColor[0];
        r = c[0];
        g = c[1];
        b = c[2];
    } else {
        r = g = b = 0.0f;
    }
}

//---------------------------------------------------------------------------
// 环境光属性 / Ambient Properties
//---------------------------------------------------------------------------

void Coin3DMaterial::setAmbientIntensity(float intensity)
{
    if (auto* mat = getSoMaterial()) {
        mat->ambientColor.setValue(intensity, intensity, intensity);
        touch();
    }
}

float Coin3DMaterial::getAmbientIntensity() const
{
    if (auto* mat = getSoMaterial()) {
        return mat->ambientColor[0][0];  // 假设 RGB 相等 / Assuming RGB equal
    }
    return 0.2f;
}

//---------------------------------------------------------------------------
// 漫反射颜色 / Diffuse Color
//---------------------------------------------------------------------------

void Coin3DMaterial::setDiffuseColor(const Color& color)
{
    if (auto* mat = getSoMaterial()) {
        mat->diffuseColor.setValue(color.r, color.g, color.b);
        touch();
    }
}

Color Coin3DMaterial::getDiffuseColor() const
{
    if (auto* mat = getSoMaterial()) {
        const SbColor& c = mat->diffuseColor[0];
        return Color(c[0], c[1], c[2], 1.0f - getTransparency());
    }
    return Color();
}

void Coin3DMaterial::setDiffuseColors(const std::vector<Color>& colors)
{
    if (auto* mat = getSoMaterial()) {
        mat->diffuseColor.setNum(static_cast<int>(colors.size()));
        for (size_t i = 0; i < colors.size(); ++i) {
            mat->diffuseColor.set1Value(static_cast<int>(i),
                                        colors[i].r,
                                        colors[i].g,
                                        colors[i].b);
        }
        touch();
    }
}

std::vector<Color> Coin3DMaterial::getDiffuseColors() const
{
    std::vector<Color> result;
    if (auto* mat = getSoMaterial()) {
        int num = mat->diffuseColor.getNum();
        result.reserve(num);
        for (int i = 0; i < num; ++i) {
            const SbColor& c = mat->diffuseColor[i];
            result.emplace_back(c[0], c[1], c[2], 1.0f - getTransparency());
        }
    }
    return result;
}

//---------------------------------------------------------------------------
// 镜面反射属性 / Specular Properties
//---------------------------------------------------------------------------

void Coin3DMaterial::setSpecularColor(const Color& color)
{
    if (auto* mat = getSoMaterial()) {
        mat->specularColor.setValue(color.r, color.g, color.b);
        touch();
    }
}

Color Coin3DMaterial::getSpecularColor() const
{
    if (auto* mat = getSoMaterial()) {
        const SbColor& c = mat->specularColor[0];
        return Color(c[0], c[1], c[2]);
    }
    return Color();
}

void Coin3DMaterial::setEmissiveColor(const Color& color)
{
    if (auto* mat = getSoMaterial()) {
        mat->emissiveColor.setValue(color.r, color.g, color.b);
        touch();
    }
}

Color Coin3DMaterial::getEmissiveColor() const
{
    if (auto* mat = getSoMaterial()) {
        const SbColor& c = mat->emissiveColor[0];
        return Color(c[0], c[1], c[2]);
    }
    return Color();
}

//---------------------------------------------------------------------------
// 光泽度 / Shininess
//---------------------------------------------------------------------------

void Coin3DMaterial::setShininess(float shininess)
{
    if (auto* mat = getSoMaterial()) {
        mat->shininess = shininess;
        touch();
    }
}

float Coin3DMaterial::getShininess() const
{
    if (auto* mat = getSoMaterial()) {
        return mat->shininess[0];
    }
    return 0.2f;
}

//---------------------------------------------------------------------------
// 透明度 / Transparency
//---------------------------------------------------------------------------

void Coin3DMaterial::setTransparency(float transparency)
{
    if (auto* mat = getSoMaterial()) {
        mat->transparency = transparency;
        touch();
    }
}

float Coin3DMaterial::getTransparency() const
{
    if (auto* mat = getSoMaterial()) {
        return mat->transparency[0];
    }
    return 0.0f;
}

//---------------------------------------------------------------------------
// 批量操作 / Batch Operations
//---------------------------------------------------------------------------

void Coin3DMaterial::setMaterial(const Material& material)
{
    if (auto* mat = getSoMaterial()) {
        mat->ambientColor.setValue(material.ambientIntensity,
                                    material.ambientIntensity,
                                    material.ambientIntensity);
        mat->diffuseColor.setValue(material.diffuseColor.r,
                                    material.diffuseColor.g,
                                    material.diffuseColor.b);
        mat->specularColor.setValue(material.specularColor.r,
                                     material.specularColor.g,
                                     material.specularColor.b);
        mat->emissiveColor.setValue(material.emissiveColor.r,
                                     material.emissiveColor.g,
                                     material.emissiveColor.b);
        mat->shininess = material.shininess;
        mat->transparency = material.transparency;
        touch();
    }
}

Material Coin3DMaterial::getMaterial() const
{
    Material mat;
    mat.ambientIntensity = getAmbientIntensity();
    mat.diffuseColor = getDiffuseColor();
    mat.specularColor = getSpecularColor();
    mat.emissiveColor = getEmissiveColor();
    mat.shininess = getShininess();
    mat.transparency = getTransparency();
    return mat;
}

void Coin3DMaterial::setMaterialBinding(Binding binding)
{
    if (_materialBinding) {
        int value = SoMaterialBinding::OVERALL;
        switch (binding) {
            case Binding::Overall:
                value = SoMaterialBinding::OVERALL;
                break;
            case Binding::PerPart:
                value = SoMaterialBinding::PER_PART;
                break;
            case Binding::PerPartIndexed:
                value = SoMaterialBinding::PER_PART_INDEXED;
                break;
            case Binding::PerFace:
                value = SoMaterialBinding::PER_FACE;
                break;
            case Binding::PerFaceIndexed:
                value = SoMaterialBinding::PER_FACE_INDEXED;
                break;
            case Binding::PerVertex:
                value = SoMaterialBinding::PER_VERTEX;
                break;
            case Binding::PerVertexIndexed:
                value = SoMaterialBinding::PER_VERTEX_INDEXED;
                break;
            case Binding::Default:
            default:
                value = SoMaterialBinding::DEFAULT;
                break;
        }
        _materialBinding->value = static_cast<SoMaterialBinding::Binding>(value);
        touch();
    }
}

Coin3DMaterial::Binding Coin3DMaterial::getMaterialBinding() const
{
    if (_materialBinding) {
        switch (_materialBinding->value.getValue()) {
            case SoMaterialBinding::OVERALL:
                return Binding::Overall;
            case SoMaterialBinding::PER_PART:
                return Binding::PerPart;
            case SoMaterialBinding::PER_PART_INDEXED:
                return Binding::PerPartIndexed;
            case SoMaterialBinding::PER_FACE:
                return Binding::PerFace;
            case SoMaterialBinding::PER_FACE_INDEXED:
                return Binding::PerFaceIndexed;
            case SoMaterialBinding::PER_VERTEX:
                return Binding::PerVertex;
            case SoMaterialBinding::PER_VERTEX_INDEXED:
                return Binding::PerVertexIndexed;
            default:
                return Binding::Default;
        }
    }
    return Binding::Default;
}

//===========================================================================
// 材质转换工具 / Material Conversion Utilities
//===========================================================================

namespace Coin3DMaterialUtils {

SbColor colorToSbColor(const Color& color)
{
    return SbColor(color.r, color.g, color.b);
}

Color sbColorToColor(const SbColor& sbColor)
{
    return Color(sbColor[0], sbColor[1], sbColor[2]);
}

void fromAppMaterial(const App::Material& appMaterial, Material& output)
{
    // App::Material doesn't have ambientIntensity, skip it
    output.ambientIntensity = 0.2f;  // Default value
    output.diffuseColor = Color(appMaterial.diffuseColor.r,
                                 appMaterial.diffuseColor.g,
                                 appMaterial.diffuseColor.b,
                                 1.0f);
    output.specularColor = Color(appMaterial.specularColor.r,
                                  appMaterial.specularColor.g,
                                  appMaterial.specularColor.b,
                                  1.0f);
    output.emissiveColor = Color(appMaterial.emissiveColor.r,
                                  appMaterial.emissiveColor.g,
                                  appMaterial.emissiveColor.b,
                                  1.0f);
    output.shininess = appMaterial.shininess;
    output.transparency = appMaterial.transparency;
}

void toAppMaterial(const Material& input, App::Material& appMaterial)
{
    // App::Material doesn't have ambientIntensity, skip it
    appMaterial.diffuseColor.set(input.diffuseColor.r,
                                  input.diffuseColor.g,
                                  input.diffuseColor.b);
    appMaterial.specularColor.set(input.specularColor.r,
                                   input.specularColor.g,
                                   input.specularColor.b);
    appMaterial.emissiveColor.set(input.emissiveColor.r,
                                   input.emissiveColor.g,
                                   input.emissiveColor.b);
    appMaterial.shininess = input.shininess;
    appMaterial.transparency = input.transparency;
}

Material getStandardMaterial(StandardMaterial type)
{
    Material mat;

    switch (type) {
        case StandardMaterial::Gold:
            mat.ambientIntensity = 0.4f;
            mat.diffuseColor = Color(0.92f, 0.78f, 0.41f, 1.0f);
            mat.specularColor = Color(1.0f, 0.96f, 0.78f, 1.0f);
            mat.shininess = 0.9f;
            mat.transparency = 0.0f;
            break;

        case StandardMaterial::Silver:
            mat.ambientIntensity = 0.4f;
            mat.diffuseColor = Color(0.85f, 0.85f, 0.85f, 1.0f);
            mat.specularColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
            mat.shininess = 0.9f;
            mat.transparency = 0.0f;
            break;

        case StandardMaterial::Bronze:
            mat.ambientIntensity = 0.3f;
            mat.diffuseColor = Color(0.72f, 0.45f, 0.20f, 1.0f);
            mat.specularColor = Color(0.88f, 0.66f, 0.38f, 1.0f);
            mat.shininess = 0.7f;
            mat.transparency = 0.0f;
            break;

        case StandardMaterial::Copper:
            mat.ambientIntensity = 0.3f;
            mat.diffuseColor = Color(0.72f, 0.45f, 0.20f, 1.0f);
            mat.specularColor = Color(0.88f, 0.49f, 0.31f, 1.0f);
            mat.shininess = 0.7f;
            mat.transparency = 0.0f;
            break;

        case StandardMaterial::Plastic:
            mat.ambientIntensity = 0.2f;
            mat.diffuseColor = Color(0.2f, 0.2f, 0.2f, 1.0f);
            mat.specularColor = Color(0.4f, 0.4f, 0.4f, 1.0f);
            mat.shininess = 0.6f;
            mat.transparency = 0.0f;
            break;

        case StandardMaterial::Rubber:
            mat.ambientIntensity = 0.2f;
            mat.diffuseColor = Color(0.1f, 0.1f, 0.1f, 1.0f);
            mat.specularColor = Color(0.05f, 0.05f, 0.05f, 1.0f);
            mat.shininess = 0.1f;
            mat.transparency = 0.0f;
            break;

        case StandardMaterial::Glass:
            mat.ambientIntensity = 0.3f;
            mat.diffuseColor = Color(0.9f, 0.9f, 0.9f, 1.0f);
            mat.specularColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
            mat.shininess = 0.9f;
            mat.transparency = 0.3f;
            break;

        case StandardMaterial::Metal:
            mat.ambientIntensity = 0.3f;
            mat.diffuseColor = Color(0.6f, 0.6f, 0.6f, 1.0f);
            mat.specularColor = Color(0.8f, 0.8f, 0.8f, 1.0f);
            mat.shininess = 0.8f;
            mat.transparency = 0.0f;
            break;

        case StandardMaterial::Wood:
            mat.ambientIntensity = 0.2f;
            mat.diffuseColor = Color(0.6f, 0.4f, 0.2f, 1.0f);
            mat.specularColor = Color(0.1f, 0.1f, 0.1f, 1.0f);
            mat.shininess = 0.1f;
            mat.transparency = 0.0f;
            break;

        case StandardMaterial::Stone:
            mat.ambientIntensity = 0.2f;
            mat.diffuseColor = Color(0.5f, 0.5f, 0.5f, 1.0f);
            mat.specularColor = Color(0.1f, 0.1f, 0.1f, 1.0f);
            mat.shininess = 0.05f;
            mat.transparency = 0.0f;
            break;

        case StandardMaterial::Default:
        default:
            mat.ambientIntensity = 0.2f;
            mat.diffuseColor = Color(0.8f, 0.8f, 0.8f, 1.0f);
            mat.specularColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
            mat.shininess = 0.2f;
            mat.transparency = 0.0f;
            break;
    }

    return mat;
}

} // namespace Coin3DMaterialUtils

} // namespace Render
} // namespace Gui
