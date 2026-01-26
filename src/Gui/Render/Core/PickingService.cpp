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

#include "PreCompiled.h"
#include "PickingService.h"

#include <map>
#include <mutex>

#include <Base/Console.h>

FC_LOG_LEVEL_INIT("PickingService", true, true)

namespace Gui {
namespace Render {

//=============================================================================
// PickingServiceRegistry Implementation
//=============================================================================

struct PickingServiceRegistry::Impl {
    std::mutex mutex;
    std::map<View3DInventorViewer*, PickingService::Ptr> services;
    std::map<BackendType, FactoryFunc> factories;
};

PickingServiceRegistry& PickingServiceRegistry::instance()
{
    static PickingServiceRegistry instance;
    return instance;
}

PickingServiceRegistry::PickingServiceRegistry()
    : pImpl(std::make_unique<Impl>())
{
}

PickingServiceRegistry::~PickingServiceRegistry() = default;

PickingService::Ptr PickingServiceRegistry::getService(View3DInventorViewer* viewer,
                                                        BackendType backend)
{
    if (!viewer) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(pImpl->mutex);

    // Check if we already have a service for this viewer
    auto it = pImpl->services.find(viewer);
    if (it != pImpl->services.end()) {
        return it->second;
    }

    // Create new service using registered factory
    auto factoryIt = pImpl->factories.find(backend);
    if (factoryIt != pImpl->factories.end() && factoryIt->second) {
        auto service = factoryIt->second(viewer);
        if (service) {
            pImpl->services[viewer] = service;
            FC_MSG("PickingServiceRegistry: Created service for viewer, backend="
                   << static_cast<int>(backend));
            return service;
        }
    }

    FC_WARN("PickingServiceRegistry: No factory registered for backend "
            << static_cast<int>(backend));
    return nullptr;
}

void PickingServiceRegistry::removeService(View3DInventorViewer* viewer)
{
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    pImpl->services.erase(viewer);
}

void PickingServiceRegistry::clear()
{
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    pImpl->services.clear();
}

void PickingServiceRegistry::registerFactory(BackendType type, FactoryFunc factory)
{
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    pImpl->factories[type] = std::move(factory);
    FC_MSG("PickingServiceRegistry: Registered factory for backend "
           << static_cast<int>(type));
}

} // namespace Render
} // namespace Gui
