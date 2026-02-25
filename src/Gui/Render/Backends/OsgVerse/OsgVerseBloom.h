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

#ifndef GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEBLOOM_H
#define GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEBLOOM_H

#include <memory>
#include <vector>
#include <osg/Texture2D>
#include <FCGlobal.h>
#include "OsgVersePostProcessing.h"

namespace Gui {
namespace Render {

/**
 * Bloom effect: extracts bright areas, blurs them, and composites back.
 * Uses separable Gaussian blur with multi-level downsampling.
 */
class GuiExport OsgVerseBloom {
public:
    OsgVerseBloom();
    ~OsgVerseBloom() = default;

    bool initialize(int width, int height);
    void shutdown();

    void setEnabled(bool enabled);
    bool isEnabled() const { return _enabled; }

    void setThreshold(float threshold);
    float getThreshold() const { return _threshold; }

    void setIntensity(float intensity);
    float getIntensity() const { return _intensity; }

    void setBlurIterations(int iterations);

    /// Get passes to add to PostProcessChain
    /// Order: BrightExtract -> BlurH -> BlurV -> (repeat) -> Composite
    std::vector<std::shared_ptr<PostProcessPass>> getPasses() const;

    /// Set the scene color texture as input
    void setSceneTexture(osg::Texture2D* sceneTexture);

    void resize(int width, int height);

private:
    void createPasses();

    bool _enabled{true};
    float _threshold{0.8f};
    float _intensity{0.5f};
    int _blurIterations{2};
    int _width{0};
    int _height{0};

    std::shared_ptr<PostProcessPass> _brightExtractPass;
    std::vector<std::shared_ptr<PostProcessPass>> _blurPasses; // H,V pairs
    std::shared_ptr<PostProcessPass> _compositePass;

    // Downsampled textures for blur
    std::vector<osg::ref_ptr<osg::Texture2D>> _blurTextures;
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSEBLOOM_H
