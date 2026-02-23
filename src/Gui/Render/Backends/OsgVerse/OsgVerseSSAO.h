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

#ifndef GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSESSAO_H
#define GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSESSAO_H

#include <memory>
#include <vector>

#include <osg/Texture2D>
#include <osg/Vec3>

#include <FCGlobal.h>

namespace Gui {
namespace Render {

class PostProcessPass;

/**
 * @brief Screen-Space Ambient Occlusion (SSAO) effect manager
 *
 * Implements Crytek-style SSAO with a bilateral blur pass and
 * a compositing pass. All shaders are GLSL 1.20 compatible for
 * macOS GL 2.1.
 *
 * The effect produces three PostProcessPass objects:
 * 1. SSAO pass: generates grayscale AO from depth buffer
 * 2. Blur pass: bilateral blur to reduce noise
 * 3. Composite pass: multiplies scene color by AO factor
 */
class GuiExport OsgVerseSSAO {
public:
    OsgVerseSSAO();
    ~OsgVerseSSAO() = default;

    bool initialize(int width, int height);
    void shutdown();

    void setEnabled(bool enabled);
    bool isEnabled() const { return _enabled; }

    void setRadius(float radius);
    void setIntensity(float intensity);
    void setBias(float bias);
    void setSampleCount(int count);

    float getRadius() const { return _radius; }
    float getIntensity() const { return _intensity; }
    float getBias() const { return _bias; }
    int getSampleCount() const { return _sampleCount; }

    /// Get the three passes to add to PostProcessChain
    std::shared_ptr<PostProcessPass> getSSAOPass() const { return _ssaoPass; }
    std::shared_ptr<PostProcessPass> getBlurPass() const { return _blurPass; }
    std::shared_ptr<PostProcessPass> getCompositePass() const { return _compositePass; }

    /// Set the depth texture from the scene render
    void setDepthTexture(osg::Texture2D* depthTexture);

    /// Set the scene color texture (for composite pass)
    void setSceneColorTexture(osg::Texture2D* colorTexture);

    /// Generate sample kernel and noise texture
    void generateKernel();
    void generateNoiseTexture();

    /// Resize all passes
    void resize(int width, int height);

private:
    bool _enabled{true};
    float _radius{0.5f};
    float _intensity{1.0f};
    float _bias{0.025f};
    int _sampleCount{32};
    int _width{0};
    int _height{0};

    std::shared_ptr<PostProcessPass> _ssaoPass;
    std::shared_ptr<PostProcessPass> _blurPass;
    std::shared_ptr<PostProcessPass> _compositePass;

    osg::ref_ptr<osg::Texture2D> _noiseTexture;
    osg::ref_ptr<osg::Texture2D> _depthTexture;
    osg::ref_ptr<osg::Texture2D> _sceneColorTexture;
    std::vector<osg::Vec3> _kernel;
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSESSAO_H
