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

#ifndef GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEPOSTPROCESSING_H
#define GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEPOSTPROCESSING_H

#include <vector>
#include <memory>
#include <string>

#include <osg/Camera>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osg/Program>
#include <osg/Group>

#include <FCGlobal.h>

namespace Gui {
namespace Render {

/**
 * @brief Tone mapping operator selection
 */
enum class ToneMapOperator {
    Reinhard,       ///< color / (color + 1)
    ACESFilmic,     ///< ACES filmic curve
    Exposure        ///< Exposure-based: 1 - exp(-color * exposure)
};

/**
 * @brief A single full-screen post-processing pass
 *
 * Reads from an input texture, applies a shader, writes to an output texture
 * (or the default framebuffer for the final pass).
 * All shaders are GLSL 1.20 compatible.
 */
class GuiExport PostProcessPass {
public:
    PostProcessPass(const std::string& name);
    ~PostProcessPass() = default;

    /// Initialize the pass with given dimensions
    bool initialize(int width, int height);

    /// Set the shader program for this pass
    void setProgram(osg::Program* program);

    /// Set input texture (from previous pass or scene render)
    void setInputTexture(osg::Texture2D* texture, int unit = 0);

    /// Get the output texture (nullptr for final pass = screen)
    osg::Texture2D* getOutputTexture() const { return _outputTexture.get(); }

    /// Set whether this is the final pass (renders to screen)
    void setFinalPass(bool final) { _isFinalPass = final; }
    bool isFinalPass() const { return _isFinalPass; }

    /// Get the RTT camera for this pass
    osg::Camera* getCamera() const { return _camera.get(); }

    /// Set a float uniform
    void setUniform(const std::string& name, float value);

    /// Set an int uniform
    void setUniform(const std::string& name, int value);

    /// Resize the pass
    void resize(int width, int height);

    std::string getName() const { return _name; }

private:
    osg::Geometry* createFullScreenQuad();

    std::string _name;
    bool _isFinalPass{false};
    int _width{0};
    int _height{0};

    osg::ref_ptr<osg::Camera> _camera;
    osg::ref_ptr<osg::Texture2D> _outputTexture;
    osg::ref_ptr<osg::Program> _program;
    osg::ref_ptr<osg::Geometry> _quad;
};

/**
 * @brief Post-processing chain manager
 *
 * Manages a sequence of post-processing passes with ping-pong textures.
 * The main scene renders to an FBO, then passes process the result
 * sequentially, with the final pass outputting to the screen.
 *
 * GL 2.1 compatible: uses GL_RGBA (8-bit) by default,
 * upgrades to GL_RGBA16F if GL_ARB_texture_float is available.
 */
class GuiExport PostProcessChain {
public:
    PostProcessChain();
    ~PostProcessChain() = default;

    /// Initialize the chain. Call after GL context is ready.
    bool initialize(int width, int height, osg::Group* sceneRoot);

    /// Shutdown and release resources
    void shutdown();

    /// Add a pass to the chain
    void addPass(std::shared_ptr<PostProcessPass> pass);

    /// Remove a pass by name
    void removePass(const std::string& name);

    /// Get the scene render camera (renders scene to FBO)
    osg::Camera* getSceneCamera() const { return _sceneCamera.get(); }

    /// Get the scene color texture (input to first pass)
    osg::Texture2D* getSceneColorTexture() const { return _sceneColorTexture.get(); }

    /// Get the scene depth texture
    osg::Texture2D* getSceneDepthTexture() const { return _sceneDepthTexture.get(); }

    /// Resize all passes
    void resize(int width, int height);

    /// Enable/disable the entire chain (bypass = direct render)
    void setEnabled(bool enabled);
    bool isEnabled() const { return _enabled; }

    /// Set tone mapping operator
    void setToneMapOperator(ToneMapOperator op);
    ToneMapOperator getToneMapOperator() const { return _toneMapOp; }

    /// Set exposure for exposure-based tone mapping
    void setExposure(float exposure);
    float getExposure() const { return _exposure; }

    /// Check if floating-point textures are available
    bool hasFloatTextures() const { return _hasFloatTextures; }

    /// Get the root group containing all pass cameras
    osg::Group* getPassRoot() const { return _passRoot.get(); }

    /// Rebuild the pass chain (call after adding/removing passes)
    void rebuildChain();

private:
    bool detectFloatTextureSupport();
    osg::Texture2D* createColorTexture(int width, int height);
    osg::Texture2D* createDepthTexture(int width, int height);
    void setupSceneCamera(int width, int height);

    bool _enabled{false};
    bool _initialized{false};
    bool _hasFloatTextures{false};
    int _width{0};
    int _height{0};

    ToneMapOperator _toneMapOp{ToneMapOperator::Reinhard};
    float _exposure{1.0f};

    osg::ref_ptr<osg::Camera> _sceneCamera;
    osg::ref_ptr<osg::Texture2D> _sceneColorTexture;
    osg::ref_ptr<osg::Texture2D> _sceneDepthTexture;
    osg::ref_ptr<osg::Group> _passRoot;

    // Ping-pong textures for intermediate passes
    osg::ref_ptr<osg::Texture2D> _pingTexture;
    osg::ref_ptr<osg::Texture2D> _pongTexture;

    std::vector<std::shared_ptr<PostProcessPass>> _passes;
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEPOSTPROCESSING_H
