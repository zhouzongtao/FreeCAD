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

#include "OsgVerseShadowMap.h"
#include <Base/Console.h>

#include <osg/FrameBufferObject>
#include <osg/StateSet>

using namespace Gui::Render;

// Bias matrix: maps NDC [-1,1] to texture coords [0,1]
const osg::Matrix OsgVerseShadowMap::BIAS_MATRIX(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.5, 0.5, 0.5, 1.0
);

OsgVerseShadowMap::OsgVerseShadowMap()
{
}

int OsgVerseShadowMap::getResolution() const
{
    switch (_quality) {
        case 0: return 0;
        case 1: return 1024;
        case 2: return 2048;
        case 3: return 4096;
        default: return 2048;
    }
}

bool OsgVerseShadowMap::initialize(osg::Group* sceneRoot)
{
    if (_initialized) {
        return true;
    }

    if (!sceneRoot) {
        Base::Console().error("OsgVerseShadowMap: Cannot initialize without scene root\n");
        return false;
    }

    int resolution = getResolution();
    if (resolution == 0) {
        Base::Console().log("OsgVerseShadowMap: Quality 0, shadows disabled\n");
        _initialized = true;
        return true;
    }

    Base::Console().log("OsgVerseShadowMap: Initializing with %dx%d resolution\n",
                       resolution, resolution);

    // Create depth texture
    _depthTexture = new osg::Texture2D();
    _depthTexture->setTextureSize(resolution, resolution);
    _depthTexture->setInternalFormat(GL_DEPTH_COMPONENT);
    _depthTexture->setSourceFormat(GL_DEPTH_COMPONENT);
    _depthTexture->setSourceType(GL_FLOAT);
    _depthTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
    _depthTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
    _depthTexture->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_BORDER);
    _depthTexture->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_BORDER);
    _depthTexture->setBorderColor(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // Create shadow camera (RTT pre-render pass)
    _shadowCamera = new osg::Camera();
    _shadowCamera->setName("ShadowCamera");
    _shadowCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    _shadowCamera->setRenderOrder(osg::Camera::PRE_RENDER);
    _shadowCamera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    _shadowCamera->setViewport(0, 0, resolution, resolution);
    _shadowCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    _shadowCamera->setClearDepth(1.0);
    _shadowCamera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

    // Attach depth texture to FBO
    _shadowCamera->attach(osg::Camera::DEPTH_BUFFER, _depthTexture.get());

    // Disable color buffer writing
    _shadowCamera->setColorMask(false, false, false, false);

    // Create an intermediate group for shadow scene content.
    // IMPORTANT: Do NOT add sceneRoot directly as child of _shadowCamera,
    // because _shadowCamera itself will be added as child of sceneRoot,
    // which would create a parent-child cycle causing infinite recursion.
    _shadowedScene = new osg::Group();
    _shadowedScene->setName("ShadowedScene");
    _shadowCamera->addChild(_shadowedScene.get());

    _initialized = true;
    Base::Console().log("OsgVerseShadowMap: Initialization complete\n");
    return true;
}

void OsgVerseShadowMap::shutdown()
{
    if (_shadowCamera) {
        _shadowCamera->removeChildren(0, _shadowCamera->getNumChildren());
        _shadowCamera = nullptr;
    }
    _shadowedScene = nullptr;
    _depthTexture = nullptr;
    _initialized = false;
    Base::Console().log("OsgVerseShadowMap: Shutdown complete\n");
}

void OsgVerseShadowMap::setEnabled(bool enabled)
{
    _enabled = enabled;
}

void OsgVerseShadowMap::setQuality(int quality)
{
    if (quality < 0) quality = 0;
    if (quality > 3) quality = 3;

    if (quality == _quality) {
        return;
    }

    _quality = quality;

    // If already initialized, recreate texture at new resolution
    if (_initialized && _depthTexture) {
        int resolution = getResolution();
        if (resolution > 0) {
            _depthTexture->setTextureSize(resolution, resolution);
            _depthTexture->dirtyTextureObject();
            if (_shadowCamera) {
                _shadowCamera->setViewport(0, 0, resolution, resolution);
            }
            Base::Console().log("OsgVerseShadowMap: Quality changed to %d (%dx%d)\n",
                               quality, resolution, resolution);
        }
    }
}

void OsgVerseShadowMap::update(const osg::Vec3& lightDirection,
                                const osg::BoundingSphere& sceneBounds)
{
    if (!_enabled || !_shadowCamera || getResolution() == 0) {
        return;
    }

    // Compute light view matrix looking at scene center from light direction
    osg::Vec3 center = sceneBounds.center();
    float radius = sceneBounds.radius();
    if (radius < 0.001f) {
        radius = 100.0f;
    }

    osg::Vec3 lightDir = lightDirection;
    lightDir.normalize();

    // Position the light camera far enough to encompass the scene
    osg::Vec3 lightPos = center - lightDir * radius * 2.0f;

    // Choose an up vector that isn't parallel to lightDir
    osg::Vec3 up(0.0f, 0.0f, 1.0f);
    if (std::abs(lightDir * up) > 0.99f) {
        up = osg::Vec3(0.0f, 1.0f, 0.0f);
    }

    osg::Matrix lightView = osg::Matrix::lookAt(lightPos, center, up);

    // Orthographic projection fitted to scene bounding sphere
    float extent = radius * 1.5f;
    osg::Matrix lightProj = osg::Matrix::ortho(
        -extent, extent, -extent, extent,
        0.1f, radius * 4.0f
    );

    _shadowCamera->setViewMatrix(lightView);
    _shadowCamera->setProjectionMatrix(lightProj);

    // Compute shadow matrix: bias * proj * view
    _shadowMatrix = lightView * lightProj * BIAS_MATRIX;
}

void OsgVerseShadowMap::applyUniforms(osg::StateSet* stateSet) const
{
    if (!stateSet) {
        return;
    }

    stateSet->addUniform(new osg::Uniform("u_shadowEnabled",
                         _enabled && _initialized && getResolution() > 0));

    if (_depthTexture) {
        // Bind shadow map to texture unit 7 (high unit to avoid conflicts)
        stateSet->setTextureAttributeAndModes(7, _depthTexture.get(),
                                               osg::StateAttribute::ON);
        stateSet->addUniform(new osg::Uniform("u_shadowMap", 7));
    }

    stateSet->addUniform(new osg::Uniform("u_shadowMatrix",
                         _shadowMatrix));

    // Bias depends on quality
    float bias = 0.005f;
    if (_quality == 1) bias = 0.008f;
    if (_quality == 3) bias = 0.003f;
    stateSet->addUniform(new osg::Uniform("u_shadowBias", bias));
}
