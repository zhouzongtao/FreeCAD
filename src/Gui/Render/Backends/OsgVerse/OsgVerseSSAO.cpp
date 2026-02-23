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
# include <cstdlib>
# include <cmath>
# include <osg/StateSet>
# include <osg/Uniform>
# include <osg/Image>
#endif

#include "OsgVerseSSAO.h"
#include "OsgVersePostProcessing.h"
#include "OsgVerseShaderManager.h"
#include <Base/Console.h>

using namespace Gui::Render;

OsgVerseSSAO::OsgVerseSSAO() = default;

bool OsgVerseSSAO::initialize(int width, int height)
{
    _width = width;
    _height = height;

    generateKernel();
    generateNoiseTexture();

    // Create SSAO pass (outputs grayscale AO)
    _ssaoPass = std::make_shared<PostProcessPass>("SSAO");
    auto* ssaoProgram = OsgVerseShaderManager::instance().getProgram(ShaderType::SSAO);
    if (ssaoProgram) {
        _ssaoPass->setProgram(ssaoProgram);
    }

    // Create blur pass (bilateral blur on AO)
    _blurPass = std::make_shared<PostProcessPass>("SSAOBlur");
    auto* blurProgram = OsgVerseShaderManager::instance().getProgram(ShaderType::SSAOBlur);
    if (blurProgram) {
        _blurPass->setProgram(blurProgram);
    }

    // Create composite pass (scene_color * AO)
    _compositePass = std::make_shared<PostProcessPass>("SSAOComposite");
    auto* compositeProgram = OsgVerseShaderManager::instance().getProgram(ShaderType::SSAOComposite);
    if (compositeProgram) {
        _compositePass->setProgram(compositeProgram);
    }

    // Set SSAO uniforms
    if (_ssaoPass) {
        osg::StateSet* ss = _ssaoPass->getCamera()->getOrCreateStateSet();

        // Kernel samples
        for (int i = 0; i < 32; ++i) {
            std::string name = "u_ssaoKernel[" + std::to_string(i) + "]";
            if (i < static_cast<int>(_kernel.size())) {
                ss->addUniform(new osg::Uniform(name.c_str(), _kernel[i]));
            } else {
                ss->addUniform(new osg::Uniform(name.c_str(), osg::Vec3(0, 0, 0)));
            }
        }

        ss->addUniform(new osg::Uniform("u_kernelSize", _sampleCount));
        ss->addUniform(new osg::Uniform("u_ssaoRadius", _radius));
        ss->addUniform(new osg::Uniform("u_ssaoBias", _bias));
        ss->addUniform(new osg::Uniform("u_ssaoIntensity", _intensity));
        ss->addUniform(new osg::Uniform("u_screenSize",
            osg::Vec2(static_cast<float>(width), static_cast<float>(height))));

        // Bind noise texture to unit 2
        if (_noiseTexture) {
            ss->setTextureAttributeAndModes(2, _noiseTexture.get(), osg::StateAttribute::ON);
            ss->addUniform(new osg::Uniform("u_noiseTexture", 2));
        }

        // Depth texture will be bound to unit 1 when setDepthTexture is called
        ss->addUniform(new osg::Uniform("u_depthTexture", 1));

        // Projection matrices will be set per-frame by the viewer
        ss->addUniform(new osg::Uniform("u_projMatrix", osg::Matrixf()));
        ss->addUniform(new osg::Uniform("u_invProjMatrix", osg::Matrixf()));
    }

    // Set blur uniforms
    if (_blurPass) {
        osg::StateSet* ss = _blurPass->getCamera()->getOrCreateStateSet();
        ss->addUniform(new osg::Uniform("u_screenSize",
            osg::Vec2(static_cast<float>(width), static_cast<float>(height))));
        // Depth texture on unit 1 for bilateral weighting
        ss->addUniform(new osg::Uniform("u_depthTexture", 1));
    }

    // Composite pass: scene color on unit 1, AO on unit 0 (default input)
    if (_compositePass) {
        osg::StateSet* ss = _compositePass->getCamera()->getOrCreateStateSet();
        ss->addUniform(new osg::Uniform("u_sceneTexture", 1));
    }

    Base::Console().log("OsgVerseSSAO: Initialized (%dx%d, %d samples)\n",
                       width, height, _sampleCount);
    return true;
}

void OsgVerseSSAO::shutdown()
{
    _ssaoPass.reset();
    _blurPass.reset();
    _compositePass.reset();
    _noiseTexture = nullptr;
    _depthTexture = nullptr;
    _sceneColorTexture = nullptr;
    _kernel.clear();
}

void OsgVerseSSAO::setEnabled(bool enabled)
{
    _enabled = enabled;
}

void OsgVerseSSAO::setRadius(float radius)
{
    _radius = radius;
    if (_ssaoPass && _ssaoPass->getCamera()) {
        _ssaoPass->getCamera()->getOrCreateStateSet()->addUniform(
            new osg::Uniform("u_ssaoRadius", radius));
    }
}

void OsgVerseSSAO::setIntensity(float intensity)
{
    _intensity = intensity;
    if (_ssaoPass && _ssaoPass->getCamera()) {
        _ssaoPass->getCamera()->getOrCreateStateSet()->addUniform(
            new osg::Uniform("u_ssaoIntensity", intensity));
    }
}

void OsgVerseSSAO::setBias(float bias)
{
    _bias = bias;
    if (_ssaoPass && _ssaoPass->getCamera()) {
        _ssaoPass->getCamera()->getOrCreateStateSet()->addUniform(
            new osg::Uniform("u_ssaoBias", bias));
    }
}

void OsgVerseSSAO::setSampleCount(int count)
{
    _sampleCount = (count > 32) ? 32 : count;
    if (_ssaoPass && _ssaoPass->getCamera()) {
        _ssaoPass->getCamera()->getOrCreateStateSet()->addUniform(
            new osg::Uniform("u_kernelSize", _sampleCount));
    }
}

void OsgVerseSSAO::setDepthTexture(osg::Texture2D* depthTexture)
{
    _depthTexture = depthTexture;
    if (!depthTexture) return;

    // Bind depth to unit 1 on SSAO pass
    if (_ssaoPass && _ssaoPass->getCamera()) {
        osg::StateSet* ss = _ssaoPass->getCamera()->getOrCreateStateSet();
        ss->setTextureAttributeAndModes(1, depthTexture, osg::StateAttribute::ON);
    }
    // Bind depth to unit 1 on blur pass (for bilateral weighting)
    if (_blurPass && _blurPass->getCamera()) {
        osg::StateSet* ss = _blurPass->getCamera()->getOrCreateStateSet();
        ss->setTextureAttributeAndModes(1, depthTexture, osg::StateAttribute::ON);
    }
}

void OsgVerseSSAO::setSceneColorTexture(osg::Texture2D* colorTexture)
{
    _sceneColorTexture = colorTexture;
    if (!colorTexture) return;

    // Bind scene color to unit 1 on composite pass
    if (_compositePass && _compositePass->getCamera()) {
        osg::StateSet* ss = _compositePass->getCamera()->getOrCreateStateSet();
        ss->setTextureAttributeAndModes(1, colorTexture, osg::StateAttribute::ON);
    }
}

void OsgVerseSSAO::generateKernel()
{
    _kernel.clear();
    _kernel.reserve(32);

    for (int i = 0; i < 32; ++i) {
        // Random point in hemisphere oriented along +Z
        float x = static_cast<float>(std::rand()) / RAND_MAX * 2.0f - 1.0f;
        float y = static_cast<float>(std::rand()) / RAND_MAX * 2.0f - 1.0f;
        float z = static_cast<float>(std::rand()) / RAND_MAX; // [0, 1] hemisphere

        osg::Vec3 sample(x, y, z);
        sample.normalize();

        // Random length with cosine-weighted distribution
        float r = static_cast<float>(std::rand()) / RAND_MAX;
        sample *= r;

        // Accelerating interpolation: more samples closer to origin
        float scale = static_cast<float>(i) / 32.0f;
        scale = 0.1f + scale * scale * 0.9f; // lerp(0.1, 1.0, scale^2)
        sample *= scale;

        _kernel.push_back(sample);
    }

    Base::Console().log("OsgVerseSSAO: Generated %d kernel samples\n",
                       static_cast<int>(_kernel.size()));
}

void OsgVerseSSAO::generateNoiseTexture()
{
    // 4x4 noise texture with random rotation vectors in XY plane
    const int noiseSize = 4;
    osg::Image* image = new osg::Image();
    image->allocateImage(noiseSize, noiseSize, 1, GL_RGB, GL_FLOAT);

    float* data = reinterpret_cast<float*>(image->data());
    for (int i = 0; i < noiseSize * noiseSize; ++i) {
        float x = static_cast<float>(std::rand()) / RAND_MAX * 2.0f - 1.0f;
        float y = static_cast<float>(std::rand()) / RAND_MAX * 2.0f - 1.0f;
        float len = std::sqrt(x * x + y * y);
        if (len > 0.0001f) {
            x /= len;
            y /= len;
        }
        data[i * 3 + 0] = x;
        data[i * 3 + 1] = y;
        data[i * 3 + 2] = 0.0f;
    }

    _noiseTexture = new osg::Texture2D();
    _noiseTexture->setImage(image);
    _noiseTexture->setTextureSize(noiseSize, noiseSize);
    _noiseTexture->setInternalFormat(GL_RGB);
    _noiseTexture->setSourceFormat(GL_RGB);
    _noiseTexture->setSourceType(GL_FLOAT);
    _noiseTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    _noiseTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    _noiseTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    _noiseTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);

    Base::Console().log("OsgVerseSSAO: Generated %dx%d noise texture\n", noiseSize, noiseSize);
}

void OsgVerseSSAO::resize(int width, int height)
{
    _width = width;
    _height = height;

    if (_ssaoPass) {
        _ssaoPass->resize(width, height);
        if (_ssaoPass->getCamera()) {
            _ssaoPass->getCamera()->getOrCreateStateSet()->addUniform(
                new osg::Uniform("u_screenSize",
                    osg::Vec2(static_cast<float>(width), static_cast<float>(height))));
        }
    }
    if (_blurPass) {
        _blurPass->resize(width, height);
        if (_blurPass->getCamera()) {
            _blurPass->getCamera()->getOrCreateStateSet()->addUniform(
                new osg::Uniform("u_screenSize",
                    osg::Vec2(static_cast<float>(width), static_cast<float>(height))));
        }
    }
    if (_compositePass) {
        _compositePass->resize(width, height);
    }
}
