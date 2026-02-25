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

#ifndef GUI_RENDER_COIN3DPICKINGSERVICE_H
#define GUI_RENDER_COIN3DPICKINGSERVICE_H

#include <Gui/Render/Core/PickingService.h>

// Forward declarations
class SoRayPickAction;
class SoPickedPoint;
class SoPath;

namespace Gui {
namespace Render {

/**
 * @brief Coin3D implementation of PickingService
 *
 * Uses SoRayPickAction for picking operations.
 */
class GuiExport Coin3DPickingService : public PickingService {
public:
    Coin3DPickingService();
    explicit Coin3DPickingService(View3DInventorViewer* viewer);
    ~Coin3DPickingService() override;

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

    BackendType getBackendType() const override { return BackendType::Coin3D; }

    //=========================================================================
    // Factory registration
    //=========================================================================

    /**
     * @brief Register this service with the PickingServiceRegistry
     */
    static void registerFactory();

private:
    /**
     * @brief Process SoPickedPoint into PickResult
     */
    PickResult processPickedPoint(SoPickedPoint* pickedPoint);

    /**
     * @brief Find ViewProvider from Coin3D path
     */
    ViewProvider* findViewProvider(SoPath* path);

    /**
     * @brief Extract element name from detail
     */
    std::string extractElementName(SoPickedPoint* pickedPoint);

    View3DInventorViewer* _viewer{nullptr};
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_COIN3DPICKINGSERVICE_H
