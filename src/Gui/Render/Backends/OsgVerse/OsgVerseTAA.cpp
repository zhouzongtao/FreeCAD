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
# include <osg/Camera>
# include <osg/Vec2>
#endif

#include "OsgVerseTAA.h"
#include "OsgVerseShaderManager.h"
#include <Base/Console.h>

using namespace Gui::Render;

OsgVerseTAA::OsgVerseTAA()
{
    // Pre-compute Halton(2,3) jitter sequence with offsets in [-0.5, 0.5]
    for (int i = 0; i < JITTER_SEQUENCE_LENGTH; ++i) {
        _jitterSequence[i] = osg::Vec2(
            halton(i + 1, 2) - 0.5f,
            halton(i + 1, 3) - 0.5f
        );
    }
}

float OsgVerseTAA::halton(int index, int base) const
{
    float result = 0.0f;
    float f = 1.0f / static_cast<float>(base);
    int i = index;
    while (i > 0) {
        result += f * static_cast<float>(i % base);
        i /= base;
        f /= static_cast<float>(base);
    }
    return result;
}

bool OsgVerseTAA::initialize(int width, int height)
{
    _width = width;
    _height = height;

    // Create double-buffered history textures
    for (int i = 0; i < 2; ++i) {
        _historyTexture[i] = new osg::Texture2D();
        _historyTexture[i]->setTextureSize(width, height);
        _historyTexture[i]->setInternalFormat(GL_RGBA);
        _historyTexture[i]->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        _historyTexture[i]->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        _historyTexture[i]->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        _historyTexture[i]->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    }

    // Create resolve pass
    _resolvePass = std::make_shared<PostProcessPass>("TAAResolve");
    _resolvePass->initialize(width, height);

    auto* program = OsgVerseShaderManager::instance().getProgram(ShaderType::TAA);
    if (program) {
        _resolvePass->setProgram(program);
    }

    _resolvePass->setUniform("u_blendFactor", _blendFactor);
    _resolvePass->setUniform("u_screenSize", osg::Vec2f(
        static_cast<float>(width), static_cast<float>(height)));
    _resolvePass->setUniform("u_historyValid", 0);

    _historyValid = false;
    _currentHistory = 0;

    Base::Console().log("OsgVerseTAA: Initialized (%dx%d)\n", width, height);
    return true;
}

void OsgVerseTAA::shutdown()
{
    _resolvePass.reset();
    _historyTexture[0] = nullptr;
    _historyTexture[1] = nullptr;
    _historyValid = false;
}

void OsgVerseTAA::setEnabled(bool enabled)
{
    _enabled = enabled;
    if (!enabled) {
        _historyValid = false;
    }
}

void OsgVerseTAA::setBlendFactor(float factor)
{
    _blendFactor = std::max(0.01f, std::min(1.0f, factor));
    if (_resolvePass) {
        _resolvePass->setUniform("u_blendFactor", _blendFactor);
    }
}

osg::Vec2 OsgVerseTAA::applyJitter(osg::Camera* camera, int frameIndex)
{
    if (!camera || !_enabled || _width <= 0 || _height <= 0) {
        return osg::Vec2(0.0f, 0.0f);
    }

    osg::Vec2 jitter = _jitterSequence[frameIndex % JITTER_SEQUENCE_LENGTH];

    // Scale jitter from pixel space to NDC
    float jitterX = jitter.x() * 2.0f / static_cast<float>(_width);
    float jitterY = jitter.y() * 2.0f / static_cast<float>(_height);

    // Apply jitter to projection matrix
    osg::Matrix proj = camera->getProjectionMatrix();
    proj(2, 0) += jitterX;
    proj(2, 1) += jitterY;
    camera->setProjectionMatrix(proj);

    // Store jitter offset for unjittering in resolve shader
    if (_resolvePass && _resolvePass->getCamera()) {
        osg::StateSet* ss = _resolvePass->getCamera()->getOrCreateStateSet();
        ss->addUniform(new osg::Uniform("u_jitterOffset",
            osg::Vec2f(jitter.x(), jitter.y())));
    }

    return jitter;
}

void OsgVerseTAA::update(const osg::Matrix& currentMVP, const osg::Matrix& prevMVP)
{
    if (!_resolvePass || !_resolvePass->getCamera() || !_enabled) {
        return;
    }

    osg::StateSet* ss = _resolvePass->getCamera()->getOrCreateStateSet();

    // Set MVP uniforms for reprojection
    ss->addUniform(new osg::Uniform("u_currentMVP", osg::Matrixf(currentMVP)));
    ss->addUniform(new osg::Uniform("u_prevMVP", osg::Matrixf(prevMVP)));

    // Compute inverse of current MVP for world-space reconstruction
    osg::Matrix invCurrentMVP = osg::Matrix::inverse(currentMVP);
    ss->addUniform(new osg::Uniform("u_invCurrentMVP", osg::Matrixf(invCurrentMVP)));

    // Set history texture (previous frame's result)
    int prevHistory = 1 - _currentHistory;
    ss->setTextureAttributeAndModes(1, _historyTexture[prevHistory].get(),
                                     osg::StateAttribute::ON);
    ss->addUniform(new osg::Uniform("u_historyTexture", 1));

    // Set history valid flag
    ss->addUniform(new osg::Uniform("u_historyValid", _historyValid ? 1 : 0));

    // Swap history buffers for next frame
    swapHistoryBuffers();

    // After first frame, history becomes valid
    _historyValid = true;
}

void OsgVerseTAA::swapHistoryBuffers()
{
    _currentHistory = 1 - _currentHistory;
}

std::shared_ptr<PostProcessPass> OsgVerseTAA::getResolvePass() const
{
    return _resolvePass;
}

void OsgVerseTAA::setCurrentFrameTexture(osg::Texture2D* colorTexture)
{
    if (_resolvePass && colorTexture) {
        _resolvePass->setInputTexture(colorTexture, 0);
    }
}

void OsgVerseTAA::setDepthTexture(osg::Texture2D* depthTexture)
{
    if (_resolvePass && _resolvePass->getCamera() && depthTexture) {
        osg::StateSet* ss = _resolvePass->getCamera()->getOrCreateStateSet();
        ss->setTextureAttributeAndModes(2, depthTexture, osg::StateAttribute::ON);
        ss->addUniform(new osg::Uniform("u_depthTexture", 2));
    }
}

void OsgVerseTAA::resize(int width, int height)
{
    _width = width;
    _height = height;

    for (int i = 0; i < 2; ++i) {
        if (_historyTexture[i]) {
            _historyTexture[i]->setTextureSize(width, height);
            _historyTexture[i]->dirtyTextureObject();
        }
    }

    if (_resolvePass) {
        _resolvePass->resize(width, height);
        _resolvePass->setUniform("u_screenSize", osg::Vec2f(
            static_cast<float>(width), static_cast<float>(height)));
    }

    // Reset history on resize since dimensions changed
    _historyValid = false;
}

void OsgVerseTAA::resetHistory()
{
    _historyValid = false;
}
