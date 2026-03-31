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

#ifndef GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSESHADOWMAP_H
#define GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSESHADOWMAP_H

#include <osg/Camera>
#include <osg/Texture2D>
#include <osg/Group>
#include <osg/Matrix>
#include <osg/Uniform>
#include <FCGlobal.h>

namespace Gui {
namespace Render {

/**
 * Shadow map implementation using a single depth texture with PCF filtering.
 * Uses an osg::Camera as a pre-render RTT pass from the light's perspective.
 *
 * Quality levels:
 *   0 = disabled
 *   1 = 1024x1024, no PCF
 *   2 = 2048x2048, 3x3 PCF (default)
 *   3 = 4096x4096, 5x5 PCF
 */
class GuiExport OsgVerseShadowMap {
public:
    OsgVerseShadowMap();
    ~OsgVerseShadowMap() = default;

    /// Initialize shadow map resources. Call once after GL context is ready.
    bool initialize(osg::Group* sceneRoot);

    /// Shutdown and release resources
    void shutdown();

    /// Enable/disable shadow rendering
    void setEnabled(bool enabled);
    bool isEnabled() const { return _enabled; }

    /// Set shadow quality (0-3)
    void setQuality(int quality);
    int getQuality() const { return _quality; }

    /// Update shadow camera matrices from the main directional light.
    /// Call once per frame before rendering.
    /// @param lightDirection Direction of the main directional light (world space)
    /// @param sceneBounds Bounding sphere of the scene for tight frustum fitting
    void update(const osg::Vec3& lightDirection, const osg::BoundingSphere& sceneBounds);

    /// Apply shadow uniforms to a StateSet (u_shadowMatrix, u_shadowMap, etc.)
    void applyUniforms(osg::StateSet* stateSet) const;

    /// Get the shadow camera (for adding to scene graph)
    osg::Camera* getShadowCamera() const { return _shadowCamera.get(); }

    /// Get the shadowed scene group (add geometry children here, NOT the scene root)
    osg::Group* getShadowedScene() const { return _shadowedScene.get(); }

    /// Get the depth texture
    osg::Texture2D* getDepthTexture() const { return _depthTexture.get(); }

private:
    int getResolution() const;

    bool _enabled{true};
    int _quality{2};
    bool _initialized{false};

    osg::ref_ptr<osg::Camera> _shadowCamera;
    osg::ref_ptr<osg::Group> _shadowedScene;  ///< Intermediate group to avoid scene graph cycle
    osg::ref_ptr<osg::Texture2D> _depthTexture;
    osg::Matrix _shadowMatrix;  // bias * lightProj * lightView

    // Bias matrix: maps [-1,1] to [0,1] for texture lookup
    static const osg::Matrix BIAS_MATRIX;
};

} // namespace Render
} // namespace Gui

#endif
