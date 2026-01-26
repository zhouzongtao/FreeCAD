/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Development Team                           *
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

#ifndef GUI_RENDER_OSGVERSEPICKINGSERVICE_H
#define GUI_RENDER_OSGVERSEPICKINGSERVICE_H

#include <Gui/Render/Core/PickingService.h>

#include <osg/ref_ptr>

// Forward declarations
namespace osg {
    class Camera;
    class Node;
}

namespace osgUtil {
    class LineSegmentIntersector;
}

namespace Gui {
namespace Render {

// Forward declaration
class OsgVerseViewer;

/**
 * @brief OsgVerse implementation of PickingService
 *
 * Uses osgUtil::LineSegmentIntersector for picking operations.
 * OsgVerse 拾取服务实现，使用 OSG 的线段相交检测器。
 */
class GuiExport OsgVersePickingService : public PickingService {
public:
    OsgVersePickingService();
    explicit OsgVersePickingService(View3DInventorViewer* viewer);
    ~OsgVersePickingService() override;

    //=========================================================================
    // PickingService interface
    //=========================================================================

    PickResults pick(int screenX, int screenY,
                    const PickOptions& options = PickOptions()) override;

    PickResults pickRay(const PickRay& ray,
                        const PickOptions& options = PickOptions()) override;

    PickResults pickRegion(int x1, int y1, int x2, int y2,
                           const PickOptions& options = PickOptions()) override;

    PickRay generateRay(int screenX, int screenY) override;

    bool projectToScreen(const Vector3& worldPoint,
                        int& screenX, int& screenY) override;

    bool unprojectToPlane(int screenX, int screenY,
                          const Vector3& planePoint,
                          const Vector3& planeNormal,
                          Vector3& worldPoint) override;

    void setViewer(View3DInventorViewer* viewer) override;

    BackendType getBackendType() const override { return BackendType::OsgVerse; }

    //=========================================================================
    // OsgVerse-specific methods
    //=========================================================================

    /**
     * @brief Set the OsgVerse viewer directly
     */
    void setOsgVerseViewer(OsgVerseViewer* viewer);

    /**
     * @brief Get the current OsgVerse viewer
     */
    OsgVerseViewer* getOsgVerseViewer() const { return _osgViewer; }

    //=========================================================================
    // Factory registration
    //=========================================================================

    /**
     * @brief Register this service with the PickingServiceRegistry
     */
    static void registerFactory();

private:
    /**
     * @brief Get the OSG camera for picking
     */
    osg::Camera* getCamera() const;

    /**
     * @brief Get the scene root for picking
     */
    osg::Node* getSceneRoot() const;

    /**
     * @brief Convert screen coordinates to normalized device coordinates
     */
    void screenToNDC(int screenX, int screenY, double& ndcX, double& ndcY) const;

    /**
     * @brief Get viewport dimensions
     */
    void getViewportSize(int& width, int& height) const;

    View3DInventorViewer* _viewer{nullptr};
    OsgVerseViewer* _osgViewer{nullptr};
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_OSGVERSEPICKINGSERVICE_H
