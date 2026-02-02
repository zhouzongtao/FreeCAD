// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef OSGVERSEGUI_BACKGROUND_H
#define OSGVERSEGUI_BACKGROUND_H

#include "PreCompiled.h"
#include <Gui/View3D/IViewer3D.h>
#include <osg/ref_ptr>
#include <osg/Camera>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Program>
#include <osg/Uniform>

namespace OsgVerseGui {

/**
 * @brief Background gradient renderer for OsgVerse
 *
 * This class creates a full-screen quad with a shader-based gradient effect.
 * It supports linear, radial, and corner gradients.
 */
class OsgVerseGuiExport OsgVerseBackground {
public:
    OsgVerseBackground();
    ~OsgVerseBackground();

    /**
     * @brief Get the background camera node
     *
     * This camera renders the background quad before the main scene.
     * It should be added to the viewer's scene graph.
     */
    osg::Camera* getCamera() const { return _camera.get(); }

    /**
     * @brief Set the background gradient
     */
    void setGradient(const Gui::View3D::BackgroundGradient& gradient);

    /**
     * @brief Get the current gradient settings
     */
    Gui::View3D::BackgroundGradient getGradient() const { return _gradient; }

    /**
     * @brief Set solid background color (for None gradient type)
     */
    void setSolidColor(const Base::Color& color);

    /**
     * @brief Update the background for the current viewport size
     */
    void resize(int width, int height);

    /**
     * @brief Check if the background is enabled
     */
    bool isEnabled() const { return _enabled; }

    /**
     * @brief Enable or disable the background
     */
    void setEnabled(bool enabled);

private:
    /**
     * @brief Create the background camera and geometry
     */
    void createBackground();

    /**
     * @brief Create the shader program for gradient rendering
     */
    void createShaders();

    /**
     * @brief Update shader uniforms based on current gradient settings
     */
    void updateUniforms();

    osg::ref_ptr<osg::Camera> _camera;          ///< Background camera
    osg::ref_ptr<osg::Geode> _geode;            ///< Background geometry container
    osg::ref_ptr<osg::Geometry> _quad;          ///< Full-screen quad
    osg::ref_ptr<osg::Program> _program;        ///< Shader program

    // Uniforms
    osg::ref_ptr<osg::Uniform> _gradientType;   ///< Gradient type (0=None, 1=Linear, 2=Radial, 3=Corner)
    osg::ref_ptr<osg::Uniform> _topColor;       ///< Top/center color
    osg::ref_ptr<osg::Uniform> _bottomColor;    ///< Bottom/edge color
    osg::ref_ptr<osg::Uniform> _topLeftColor;   ///< Top-left corner color
    osg::ref_ptr<osg::Uniform> _topRightColor;  ///< Top-right corner color
    osg::ref_ptr<osg::Uniform> _bottomLeftColor;  ///< Bottom-left corner color
    osg::ref_ptr<osg::Uniform> _bottomRightColor; ///< Bottom-right corner color
    osg::ref_ptr<osg::Uniform> _midPoint;       ///< Gradient midpoint
    osg::ref_ptr<osg::Uniform> _solidColor;     ///< Solid background color

    Gui::View3D::BackgroundGradient _gradient;  ///< Current gradient settings
    Base::Color _bgColor;                       ///< Solid background color
    bool _enabled{true};                        ///< Whether background is enabled
    int _width{800};                            ///< Viewport width
    int _height{600};                           ///< Viewport height
};

} // namespace OsgVerseGui

#endif // OSGVERSEGUI_BACKGROUND_H
