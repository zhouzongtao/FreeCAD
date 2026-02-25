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
# include <osg/Uniform>
#endif

#include "OsgVerseBloom.h"
#include "OsgVerseShaderManager.h"
#include <Base/Console.h>

using namespace Gui::Render;

OsgVerseBloom::OsgVerseBloom() = default;

bool OsgVerseBloom::initialize(int width, int height)
{
    _width = width;
    _height = height;

    createPasses();

    Base::Console().log("OsgVerseBloom: Initialized (%dx%d)\n", width, height);
    return true;
}

void OsgVerseBloom::shutdown()
{
    _brightExtractPass.reset();
    _blurPasses.clear();
    _compositePass.reset();
    _blurTextures.clear();
}

void OsgVerseBloom::setEnabled(bool enabled)
{
    _enabled = enabled;
}

void OsgVerseBloom::setThreshold(float threshold)
{
    _threshold = threshold;
    if (_brightExtractPass) {
        _brightExtractPass->setUniform("u_bloomThreshold", _threshold);
    }
}

void OsgVerseBloom::setIntensity(float intensity)
{
    _intensity = intensity;
    if (_compositePass) {
        _compositePass->setUniform("u_bloomIntensity", _intensity);
    }
}

void OsgVerseBloom::setBlurIterations(int iterations)
{
    if (iterations != _blurIterations && iterations > 0 && iterations <= 4) {
        _blurIterations = iterations;
        createPasses();
    }
}

std::vector<std::shared_ptr<PostProcessPass>> OsgVerseBloom::getPasses() const
{
    std::vector<std::shared_ptr<PostProcessPass>> passes;
    if (!_enabled) return passes;

    if (_brightExtractPass) passes.push_back(_brightExtractPass);
    for (auto& p : _blurPasses) {
        passes.push_back(p);
    }
    if (_compositePass) passes.push_back(_compositePass);

    return passes;
}

void OsgVerseBloom::setSceneTexture(osg::Texture2D* sceneTexture)
{
    if (_compositePass && sceneTexture) {
        osg::StateSet* ss = _compositePass->getCamera()->getOrCreateStateSet();
        ss->setTextureAttributeAndModes(1, sceneTexture, osg::StateAttribute::ON);
        ss->addUniform(new osg::Uniform("u_sceneTexture", 1));
    }
}

void OsgVerseBloom::resize(int width, int height)
{
    _width = width;
    _height = height;

    if (_brightExtractPass) _brightExtractPass->resize(width / 2, height / 2);
    for (auto& p : _blurPasses) {
        p->resize(width / 2, height / 2);
    }
    if (_compositePass) _compositePass->resize(width, height);

    // Update texel size uniforms for blur passes
    float texelW = 1.0f / static_cast<float>(width / 2);
    float texelH = 1.0f / static_cast<float>(height / 2);
    for (auto& p : _blurPasses) {
        osg::StateSet* ss = p->getCamera()->getOrCreateStateSet();
        ss->addUniform(new osg::Uniform("u_texelSize", osg::Vec2f(texelW, texelH)));
    }
}

void OsgVerseBloom::createPasses()
{
    auto& shaderMgr = OsgVerseShaderManager::instance();

    int halfW = _width / 2;
    int halfH = _height / 2;
    float texelW = 1.0f / static_cast<float>(halfW > 0 ? halfW : 1);
    float texelH = 1.0f / static_cast<float>(halfH > 0 ? halfH : 1);

    // --- Bright extract pass (at half resolution) ---
    _brightExtractPass = std::make_shared<PostProcessPass>("BloomBrightExtract");
    _brightExtractPass->initialize(halfW, halfH);
    auto* brightProg = shaderMgr.getProgram(ShaderType::BloomBrightExtract);
    if (brightProg) {
        _brightExtractPass->setProgram(brightProg);
    }
    _brightExtractPass->setUniform("u_bloomThreshold", _threshold);

    // --- Blur passes (H,V pairs at half resolution) ---
    _blurPasses.clear();
    _blurTextures.clear();
    auto* blurProg = shaderMgr.getProgram(ShaderType::BloomBlur);

    for (int i = 0; i < _blurIterations; ++i) {
        // Horizontal blur
        auto blurH = std::make_shared<PostProcessPass>(
            "BloomBlurH_" + std::to_string(i));
        blurH->initialize(halfW, halfH);
        if (blurProg) blurH->setProgram(blurProg);
        blurH->setUniform("u_horizontal", 1);
        osg::StateSet* ssH = blurH->getCamera()->getOrCreateStateSet();
        ssH->addUniform(new osg::Uniform("u_texelSize", osg::Vec2f(texelW, texelH)));
        _blurPasses.push_back(blurH);

        // Vertical blur
        auto blurV = std::make_shared<PostProcessPass>(
            "BloomBlurV_" + std::to_string(i));
        blurV->initialize(halfW, halfH);
        if (blurProg) blurV->setProgram(blurProg);
        blurV->setUniform("u_horizontal", 0);
        osg::StateSet* ssV = blurV->getCamera()->getOrCreateStateSet();
        ssV->addUniform(new osg::Uniform("u_texelSize", osg::Vec2f(texelW, texelH)));
        _blurPasses.push_back(blurV);
    }

    // --- Composite pass (full resolution) ---
    _compositePass = std::make_shared<PostProcessPass>("BloomComposite");
    _compositePass->initialize(_width, _height);
    auto* compositeProg = shaderMgr.getProgram(ShaderType::BloomComposite);
    if (compositeProg) {
        _compositePass->setProgram(compositeProg);
    }
    _compositePass->setUniform("u_bloomIntensity", _intensity);

    Base::Console().log("OsgVerseBloom: Created %d blur passes (%d iterations)\n",
                       static_cast<int>(_blurPasses.size()), _blurIterations);
}
