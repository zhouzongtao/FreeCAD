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
# include <osg/FrameBufferObject>
# include <osg/StateSet>
# include <osg/Uniform>
# include <osg/BlendFunc>
# include <osg/Depth>
# include <osg/Geode>
# include <osg/Geometry>
# include <osg/Camera>
# include <osg/Texture2D>
# include <osg/GL>
#endif

#include "OsgVersePostProcessing.h"
#include "OsgVerseShaderManager.h"
#include <Base/Console.h>

using namespace Gui::Render;

//===========================================================================
// PostProcessPass Implementation
//===========================================================================

PostProcessPass::PostProcessPass(const std::string& name)
    : _name(name)
{
}

bool PostProcessPass::initialize(int width, int height)
{
    _width = width;
    _height = height;

    // Create RTT camera
    _camera = new osg::Camera();
    _camera->setName("PostProcess_" + _name);
    _camera->setClearMask(GL_COLOR_BUFFER_BIT);
    _camera->setClearColor(osg::Vec4(0, 0, 0, 1));
    _camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    _camera->setProjectionMatrix(osg::Matrix::ortho2D(0, 1, 0, 1));
    _camera->setViewMatrix(osg::Matrix::identity());
    _camera->setViewport(0, 0, width, height);
    _camera->setRenderOrder(osg::Camera::POST_RENDER);

    if (!_isFinalPass) {
        // Render to texture
        _camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

        _outputTexture = new osg::Texture2D();
        _outputTexture->setTextureSize(width, height);
        _outputTexture->setInternalFormat(GL_RGBA);
        _outputTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        _outputTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        _outputTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        _outputTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

        _camera->attach(osg::Camera::COLOR_BUFFER, _outputTexture.get());
    }

    // Create full-screen quad
    _quad = createFullScreenQuad();
    osg::Geode* geode = new osg::Geode();
    geode->addDrawable(_quad.get());
    _camera->addChild(geode);

    // Disable depth test for post-processing
    osg::StateSet* ss = _camera->getOrCreateStateSet();
    ss->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    Base::Console().log("PostProcessPass '%s': Initialized (%dx%d)\n",
                       _name.c_str(), width, height);
    return true;
}

void PostProcessPass::setProgram(osg::Program* program)
{
    _program = program;
    if (_camera) {
        _camera->getOrCreateStateSet()->setAttributeAndModes(
            program, osg::StateAttribute::ON);
    }
}

void PostProcessPass::setInputTexture(osg::Texture2D* texture, int unit)
{
    if (_camera && texture) {
        osg::StateSet* ss = _camera->getOrCreateStateSet();
        ss->setTextureAttributeAndModes(unit, texture, osg::StateAttribute::ON);
        ss->addUniform(new osg::Uniform("u_inputTexture", unit));
    }
}

void PostProcessPass::setUniform(const std::string& name, float value)
{
    if (_camera) {
        _camera->getOrCreateStateSet()->addUniform(new osg::Uniform(name.c_str(), value));
    }
}

void PostProcessPass::setUniform(const std::string& name, int value)
{
    if (_camera) {
        _camera->getOrCreateStateSet()->addUniform(new osg::Uniform(name.c_str(), value));
    }
}

void PostProcessPass::setUniform(const std::string& name, const osg::Vec2f& value)
{
    if (_camera) {
        _camera->getOrCreateStateSet()->addUniform(new osg::Uniform(name.c_str(), value));
    }
}

void PostProcessPass::resize(int width, int height)
{
    _width = width;
    _height = height;
    if (_camera) {
        _camera->setViewport(0, 0, width, height);
    }
    if (_outputTexture) {
        _outputTexture->setTextureSize(width, height);
        _outputTexture->dirtyTextureObject();
    }
}

osg::Geometry* PostProcessPass::createFullScreenQuad()
{
    osg::Geometry* quad = new osg::Geometry();

    // Vertices: full-screen quad [0,1] x [0,1]
    osg::Vec3Array* vertices = new osg::Vec3Array(4);
    (*vertices)[0].set(0.0f, 0.0f, 0.0f);
    (*vertices)[1].set(1.0f, 0.0f, 0.0f);
    (*vertices)[2].set(1.0f, 1.0f, 0.0f);
    (*vertices)[3].set(0.0f, 1.0f, 0.0f);
    quad->setVertexArray(vertices);

    // Texture coordinates
    osg::Vec2Array* texCoords = new osg::Vec2Array(4);
    (*texCoords)[0].set(0.0f, 0.0f);
    (*texCoords)[1].set(1.0f, 0.0f);
    (*texCoords)[2].set(1.0f, 1.0f);
    (*texCoords)[3].set(0.0f, 1.0f);
    quad->setTexCoordArray(0, texCoords);

    // Draw as quad
    quad->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, 4));

    return quad;
}

//===========================================================================
// PostProcessChain Implementation
//===========================================================================

PostProcessChain::PostProcessChain() = default;

bool PostProcessChain::initialize(int width, int height, osg::Group* sceneRoot)
{
    if (_initialized) return true;
    if (!sceneRoot) return false;

    _width = width;
    _height = height;

    // Detect float texture support
    _hasFloatTextures = detectFloatTextureSupport();
    Base::Console().log("PostProcessChain: Float textures %s\n",
                       _hasFloatTextures ? "available" : "not available (using 8-bit)");

    // Create scene render textures
    _sceneColorTexture = createColorTexture(width, height);
    _sceneDepthTexture = createDepthTexture(width, height);

    // Create ping-pong textures for intermediate passes
    _pingTexture = createColorTexture(width, height);
    _pongTexture = createColorTexture(width, height);

    // Setup scene camera (renders scene to FBO)
    setupSceneCamera(width, height);

    // Create pass root group
    _passRoot = new osg::Group();
    _passRoot->setName("PostProcessPassRoot");

    _initialized = true;
    Base::Console().log("PostProcessChain: Initialized (%dx%d)\n", width, height);
    return true;
}

void PostProcessChain::shutdown()
{
    _passes.clear();
    _sceneCamera = nullptr;
    _sceneColorTexture = nullptr;
    _sceneDepthTexture = nullptr;
    _pingTexture = nullptr;
    _pongTexture = nullptr;
    _passRoot = nullptr;
    _initialized = false;
}

void PostProcessChain::addPass(std::shared_ptr<PostProcessPass> pass)
{
    _passes.push_back(pass);
}

void PostProcessChain::removePass(const std::string& name)
{
    _passes.erase(
        std::remove_if(_passes.begin(), _passes.end(),
            [&name](const std::shared_ptr<PostProcessPass>& p) {
                return p->getName() == name;
            }),
        _passes.end());
}

void PostProcessChain::resize(int width, int height)
{
    _width = width;
    _height = height;

    if (_sceneCamera) {
        _sceneCamera->setViewport(0, 0, width, height);
    }
    if (_sceneColorTexture) {
        _sceneColorTexture->setTextureSize(width, height);
        _sceneColorTexture->dirtyTextureObject();
    }
    if (_sceneDepthTexture) {
        _sceneDepthTexture->setTextureSize(width, height);
        _sceneDepthTexture->dirtyTextureObject();
    }
    if (_pingTexture) {
        _pingTexture->setTextureSize(width, height);
        _pingTexture->dirtyTextureObject();
    }
    if (_pongTexture) {
        _pongTexture->setTextureSize(width, height);
        _pongTexture->dirtyTextureObject();
    }

    for (auto& pass : _passes) {
        pass->resize(width, height);
    }
}

void PostProcessChain::setEnabled(bool enabled)
{
    _enabled = enabled;
}

void PostProcessChain::setToneMapOperator(ToneMapOperator op)
{
    _toneMapOp = op;
}

void PostProcessChain::setExposure(float exposure)
{
    _exposure = std::max(0.01f, exposure);
}

void PostProcessChain::rebuildChain()
{
    if (!_passRoot) return;

    _passRoot->removeChildren(0, _passRoot->getNumChildren());

    if (_passes.empty()) return;

    // Wire up ping-pong: scene → pass0 → pass1 → ... → screen
    osg::Texture2D* currentInput = _sceneColorTexture.get();
    bool usePing = true;

    for (size_t i = 0; i < _passes.size(); ++i) {
        auto& pass = _passes[i];
        bool isLast = (i == _passes.size() - 1);

        pass->setFinalPass(isLast);
        pass->initialize(_width, _height);
        pass->setInputTexture(currentInput);

        _passRoot->addChild(pass->getCamera());

        if (!isLast) {
            currentInput = usePing ? _pingTexture.get() : _pongTexture.get();
            usePing = !usePing;
        }
    }
}

bool PostProcessChain::detectFloatTextureSupport()
{
    // Query GL_ARB_texture_float at runtime.
    // This is called during deferred init when GL context is active.
    const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (extensions) {
        std::string extStr(extensions);
        if (extStr.find("GL_ARB_texture_float") != std::string::npos) {
            Base::Console().log("PostProcessChain: GL_ARB_texture_float detected\n");
            return true;
        }
    }
    Base::Console().log("PostProcessChain: GL_ARB_texture_float not available, using 8-bit textures\n");
    return false;
}

osg::Texture2D* PostProcessChain::createColorTexture(int width, int height)
{
    auto texture = new osg::Texture2D();
    texture->setTextureSize(width, height);
    texture->setInternalFormat(_hasFloatTextures ? GL_RGBA16F_ARB : GL_RGBA);
    texture->setSourceFormat(GL_RGBA);
    texture->setSourceType(_hasFloatTextures ? GL_FLOAT : GL_UNSIGNED_BYTE);
    texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    return texture;
}

osg::Texture2D* PostProcessChain::createDepthTexture(int width, int height)
{
    auto texture = new osg::Texture2D();
    texture->setTextureSize(width, height);
    texture->setInternalFormat(GL_DEPTH_COMPONENT);
    texture->setSourceFormat(GL_DEPTH_COMPONENT);
    texture->setSourceType(GL_FLOAT);
    texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    return texture;
}

void PostProcessChain::setupSceneCamera(int width, int height)
{
    _sceneCamera = new osg::Camera();
    _sceneCamera->setName("PostProcess_SceneCapture");
    _sceneCamera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    _sceneCamera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    _sceneCamera->setViewport(0, 0, width, height);
    _sceneCamera->setRenderOrder(osg::Camera::PRE_RENDER);

    // Attach color and depth textures
    _sceneCamera->attach(osg::Camera::COLOR_BUFFER, _sceneColorTexture.get());
    _sceneCamera->attach(osg::Camera::DEPTH_BUFFER, _sceneDepthTexture.get());
}
