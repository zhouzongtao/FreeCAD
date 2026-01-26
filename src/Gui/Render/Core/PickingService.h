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

#ifndef GUI_RENDER_PICKINGSERVICE_H
#define GUI_RENDER_PICKINGSERVICE_H

#include <memory>
#include <functional>

#include <FCGlobal.h>
#include "RenderTypes.h"
#include "InteractionTypes.h"

namespace Gui {

class View3DInventorViewer;

namespace Render {

/**
 * @brief Abstract interface for backend-agnostic picking
 *
 * This class provides a unified interface for ray-casting and picking
 * that works with both Coin3D and OsgVerse backends.
 *
 * Usage:
 * @code
 * PickingService* service = PickingServiceRegistry::instance().getService(viewer);
 * PickResults results = service->pick(screenX, screenY);
 * if (results.hasHit()) {
 *     const PickResult& hit = results.closest();
 *     // Process hit...
 * }
 * @endcode
 */
class GuiExport PickingService {
public:
    using Ptr = std::shared_ptr<PickingService>;

    virtual ~PickingService() = default;

    //=========================================================================
    // Picking Operations
    //=========================================================================

    /**
     * @brief Pick at screen position
     *
     * Performs a ray-cast from the camera through the given screen position
     * and returns all intersections with scene geometry.
     *
     * @param screenX Screen X coordinate (pixels, left = 0)
     * @param screenY Screen Y coordinate (pixels, top = 0)
     * @param options Picking options
     * @return Pick results
     */
    virtual PickResults pick(int screenX, int screenY,
                            const PickOptions& options = PickOptions()) = 0;

    /**
     * @brief Pick with a ray
     *
     * Performs a ray-cast with the given ray and returns all intersections.
     *
     * @param ray The pick ray in world coordinates
     * @param options Picking options
     * @return Pick results
     */
    virtual PickResults pickRay(const PickRay& ray,
                                const PickOptions& options = PickOptions()) = 0;

    /**
     * @brief Pick in a rectangular region
     *
     * Returns all objects within the given screen rectangle.
     *
     * @param x1, y1 First corner of rectangle (screen coordinates)
     * @param x2, y2 Second corner of rectangle (screen coordinates)
     * @param options Picking options
     * @return Pick results (all objects in region)
     */
    virtual PickResults pickRegion(int x1, int y1, int x2, int y2,
                                   const PickOptions& options = PickOptions()) = 0;

    //=========================================================================
    // Ray Generation
    //=========================================================================

    /**
     * @brief Generate a pick ray from screen position
     *
     * @param screenX Screen X coordinate
     * @param screenY Screen Y coordinate
     * @return Pick ray in world coordinates
     */
    virtual PickRay generateRay(int screenX, int screenY) = 0;

    /**
     * @brief Project a world point to screen coordinates
     *
     * @param worldPoint Point in world coordinates
     * @param screenX Output screen X
     * @param screenY Output screen Y
     * @return true if point is in front of camera
     */
    virtual bool projectToScreen(const Vector3& worldPoint,
                                 int& screenX, int& screenY) = 0;

    /**
     * @brief Unproject screen coordinates to a world point on a plane
     *
     * @param screenX Screen X coordinate
     * @param screenY Screen Y coordinate
     * @param plane Plane to intersect with (point and normal)
     * @param worldPoint Output world point
     * @return true if intersection exists
     */
    virtual bool unprojectToPlane(int screenX, int screenY,
                                  const Vector3& planePoint,
                                  const Vector3& planeNormal,
                                  Vector3& worldPoint) = 0;

    //=========================================================================
    // Configuration
    //=========================================================================

    /**
     * @brief Set the associated viewer
     */
    virtual void setViewer(View3DInventorViewer* viewer) = 0;

    /**
     * @brief Get the backend type
     */
    virtual BackendType getBackendType() const = 0;
};

/**
 * @brief Registry for PickingService instances
 *
 * Manages PickingService instances per viewer, creating the appropriate
 * backend-specific implementation as needed.
 */
class GuiExport PickingServiceRegistry {
public:
    /**
     * @brief Get the singleton instance
     */
    static PickingServiceRegistry& instance();

    /**
     * @brief Get or create a picking service for a viewer
     *
     * @param viewer The viewer
     * @param backend Backend type to use (or Auto to detect)
     * @return PickingService instance
     */
    PickingService::Ptr getService(View3DInventorViewer* viewer,
                                   BackendType backend = BackendType::Coin3D);

    /**
     * @brief Remove the picking service for a viewer
     */
    void removeService(View3DInventorViewer* viewer);

    /**
     * @brief Clear all services
     */
    void clear();

    /**
     * @brief Factory function type
     */
    using FactoryFunc = std::function<PickingService::Ptr(View3DInventorViewer*)>;

    /**
     * @brief Register a factory for a backend type
     */
    void registerFactory(BackendType type, FactoryFunc factory);

private:
    PickingServiceRegistry();
    ~PickingServiceRegistry();

    PickingServiceRegistry(const PickingServiceRegistry&) = delete;
    PickingServiceRegistry& operator=(const PickingServiceRegistry&) = delete;

    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_PICKINGSERVICE_H
