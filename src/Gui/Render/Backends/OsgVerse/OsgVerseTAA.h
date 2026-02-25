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

#ifndef GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSETAA_H
#define GUI_RENDER_BACKENDS_OSGVERSE_OSGVERSETAA_H

#include <memory>
#include <osg/Texture2D>
#include <osg/Matrix>
#include <osg/Camera>
#include <FCGlobal.h>
#include "OsgVersePostProcessing.h"

namespace Gui {
namespace Render {

/**
 * Temporal Anti-Aliasing using sub-pixel jitter and history blending.
 *
 * For CAD scenes (mostly static), TAA is very effective:
 * - Halton(2,3) sequence for sub-pixel jitter
 * - History buffer with exponential moving average
 * - Neighborhood clamping to prevent ghosting
 * - Camera motion vector from MVP delta (no per-object motion vectors needed)
 */
class GuiExport OsgVerseTAA {
public:
    OsgVerseTAA();
    ~OsgVerseTAA() = default;

    bool initialize(int width, int height);
    void shutdown();

    void setEnabled(bool enabled);
    bool isEnabled() const { return _enabled; }

    void setBlendFactor(float factor);
    float getBlendFactor() const { return _blendFactor; }

    /// Call once per frame BEFORE rendering to apply jitter to projection matrix
    /// Returns the jitter offset applied (for unjittering in resolve)
    osg::Vec2 applyJitter(osg::Camera* camera, int frameIndex);

    /// Call once per frame AFTER rendering to update history
    void update(const osg::Matrix& currentMVP, const osg::Matrix& prevMVP);

    /// Get the TAA resolve pass
    std::shared_ptr<PostProcessPass> getResolvePass() const;

    /// Set current frame texture and depth texture
    void setCurrentFrameTexture(osg::Texture2D* colorTexture);
    void setDepthTexture(osg::Texture2D* depthTexture);

    void resize(int width, int height);

    /// Reset history (call on camera teleport/cut)
    void resetHistory();

private:
    float halton(int index, int base) const;
    void swapHistoryBuffers();

    bool _enabled{true};
    float _blendFactor{0.1f};
    int _jitterIndex{0};
    int _width{0};
    int _height{0};
    bool _historyValid{false};

    std::shared_ptr<PostProcessPass> _resolvePass;

    // Double-buffered history
    osg::ref_ptr<osg::Texture2D> _historyTexture[2];
    int _currentHistory{0};

    // Jitter sequence (Halton 2,3)
    static constexpr int JITTER_SEQUENCE_LENGTH = 16;
    osg::Vec2 _jitterSequence[JITTER_SEQUENCE_LENGTH];
};

} // namespace Render
} // namespace Gui

#endif
