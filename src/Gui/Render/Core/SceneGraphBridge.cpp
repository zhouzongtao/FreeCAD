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
#include "SceneGraphBridge.h"

#include <Gui/ViewProvider.h>
#include <Gui/View3DInventorViewer.h>

#include <Base/Console.h>

FC_LOG_LEVEL_INIT("SceneGraphBridge", true, true)

namespace Gui {
namespace Render {

// Static member initialization
BackendType SceneGraphBridge::_defaultBackend = BackendType::Coin3D;

SceneGraphBridge& SceneGraphBridge::instance()
{
    static SceneGraphBridge instance;
    return instance;
}

SceneGraphBridge::SceneGraphBridge() = default;

SceneGraphBridge::~SceneGraphBridge()
{
    // Clear all mappings
    std::lock_guard<std::mutex> lock(_mutex);
    _viewers.clear();
    _viewProviders.clear();
    _nodeToVP.clear();
    _callbacks.clear();
}

//=============================================================================
// Viewer Management
//=============================================================================

void SceneGraphBridge::attachViewer(View3DInventorViewer* viewer, BackendType backend)
{
    if (!viewer) {
        return;
    }

    std::lock_guard<std::mutex> lock(_mutex);

    if (_viewers.find(viewer) != _viewers.end()) {
        FC_WARN("SceneGraphBridge: Viewer already attached");
        return;
    }

    ViewerInfo info;
    info.viewer = viewer;
    info.backend = backend;
    // Scene root and object group will be initialized lazily

    _viewers[viewer] = std::move(info);

    if (_debugEnabled) {
        FC_MSG("SceneGraphBridge: Attached viewer with backend "
               << static_cast<int>(backend));
    }
}

void SceneGraphBridge::detachViewer(View3DInventorViewer* viewer)
{
    if (!viewer) {
        return;
    }

    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _viewers.find(viewer);
    if (it == _viewers.end()) {
        return;
    }

    // Remove viewer from all ViewProvider info
    for (auto& vpPair : _viewProviders) {
        vpPair.second.viewers.erase(viewer);
    }

    _viewers.erase(it);

    if (_debugEnabled) {
        FC_MSG("SceneGraphBridge: Detached viewer");
    }
}

bool SceneGraphBridge::hasViewer(View3DInventorViewer* viewer) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _viewers.find(viewer) != _viewers.end();
}

BackendType SceneGraphBridge::getViewerBackend(View3DInventorViewer* viewer) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _viewers.find(viewer);
    if (it != _viewers.end()) {
        return it->second.backend;
    }
    return BackendType::None;
}

//=============================================================================
// ViewProvider Management
//=============================================================================

void SceneGraphBridge::addViewProvider(ViewProvider* vp)
{
    if (!vp) {
        return;
    }

    std::lock_guard<std::mutex> lock(_mutex);

    // Add to all attached viewers
    for (auto& viewerPair : _viewers) {
        auto* viewer = viewerPair.first;
        addViewProvider(viewer, vp);
    }
}

void SceneGraphBridge::addViewProvider(View3DInventorViewer* viewer, ViewProvider* vp)
{
    if (!viewer || !vp) {
        return;
    }

    // Note: Caller should hold the lock if needed

    auto viewerIt = _viewers.find(viewer);
    if (viewerIt == _viewers.end()) {
        FC_WARN("SceneGraphBridge: Viewer not attached");
        return;
    }

    ViewerInfo& viewerInfo = viewerIt->second;

    // Check if already added
    if (viewerInfo.viewProviders.find(vp) != viewerInfo.viewProviders.end()) {
        return;
    }

    // Get or create ViewProvider info
    auto& vpInfo = _viewProviders[vp];
    vpInfo.vp = vp;
    vpInfo.renderRoot = vp->getRenderRootPtr();
    vpInfo.viewers.insert(viewer);

    // Track node-to-VP mapping
    if (vpInfo.renderRoot) {
        _nodeToVP[vpInfo.renderRoot.get()] = vp;
    }

    // Add to viewer's ViewProvider set
    viewerInfo.viewProviders.insert(vp);

    // Sync with backend scene graph
    switch (viewerInfo.backend) {
        case BackendType::Coin3D:
            syncCoin3DNode(viewer, vp, true);
            break;
        case BackendType::OsgVerse:
            syncOsgVerseNode(viewer, vp, true);
            break;
        default:
            break;
    }

    notifySceneChanged(viewer, vp);

    if (_debugEnabled) {
        FC_MSG("SceneGraphBridge: Added ViewProvider to viewer");
    }
}

void SceneGraphBridge::removeViewProvider(ViewProvider* vp)
{
    if (!vp) {
        return;
    }

    std::lock_guard<std::mutex> lock(_mutex);

    auto vpIt = _viewProviders.find(vp);
    if (vpIt == _viewProviders.end()) {
        return;
    }

    // Remove from all viewers
    for (auto* viewer : vpIt->second.viewers) {
        auto viewerIt = _viewers.find(viewer);
        if (viewerIt != _viewers.end()) {
            viewerIt->second.viewProviders.erase(vp);

            // Sync removal with backend
            switch (viewerIt->second.backend) {
                case BackendType::Coin3D:
                    syncCoin3DNode(viewer, vp, false);
                    break;
                case BackendType::OsgVerse:
                    syncOsgVerseNode(viewer, vp, false);
                    break;
                default:
                    break;
            }

            notifySceneChanged(viewer, vp);
        }
    }

    // Remove node-to-VP mapping
    if (vpIt->second.renderRoot) {
        _nodeToVP.erase(vpIt->second.renderRoot.get());
    }

    // Remove ViewProvider info
    _viewProviders.erase(vpIt);

    if (_debugEnabled) {
        FC_MSG("SceneGraphBridge: Removed ViewProvider");
    }
}

void SceneGraphBridge::removeViewProvider(View3DInventorViewer* viewer, ViewProvider* vp)
{
    if (!viewer || !vp) {
        return;
    }

    std::lock_guard<std::mutex> lock(_mutex);

    auto viewerIt = _viewers.find(viewer);
    if (viewerIt == _viewers.end()) {
        return;
    }

    ViewerInfo& viewerInfo = viewerIt->second;
    viewerInfo.viewProviders.erase(vp);

    auto vpIt = _viewProviders.find(vp);
    if (vpIt != _viewProviders.end()) {
        vpIt->second.viewers.erase(viewer);

        // If no viewers left, remove the VP info
        if (vpIt->second.viewers.empty()) {
            if (vpIt->second.renderRoot) {
                _nodeToVP.erase(vpIt->second.renderRoot.get());
            }
            _viewProviders.erase(vpIt);
        }
    }

    // Sync removal with backend
    switch (viewerInfo.backend) {
        case BackendType::Coin3D:
            syncCoin3DNode(viewer, vp, false);
            break;
        case BackendType::OsgVerse:
            syncOsgVerseNode(viewer, vp, false);
            break;
        default:
            break;
    }

    notifySceneChanged(viewer, vp);
}

bool SceneGraphBridge::hasViewProvider(ViewProvider* vp) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _viewProviders.find(vp) != _viewProviders.end();
}

RenderNode* SceneGraphBridge::getRenderNode(ViewProvider* vp) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _viewProviders.find(vp);
    if (it != _viewProviders.end() && it->second.renderRoot) {
        return it->second.renderRoot.get();
    }
    return nullptr;
}

ViewProvider* SceneGraphBridge::getViewProvider(RenderNode* node) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _nodeToVP.find(node);
    if (it != _nodeToVP.end()) {
        return it->second;
    }
    return nullptr;
}

//=============================================================================
// Scene Graph Operations
//=============================================================================

RenderNode::Ptr SceneGraphBridge::getSceneRoot(View3DInventorViewer* viewer) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _viewers.find(viewer);
    if (it != _viewers.end()) {
        return it->second.sceneRoot;
    }
    return nullptr;
}

RenderNode::Ptr SceneGraphBridge::getObjectGroup(View3DInventorViewer* viewer) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _viewers.find(viewer);
    if (it != _viewers.end()) {
        return it->second.objectGroup;
    }
    return nullptr;
}

void SceneGraphBridge::synchronize(View3DInventorViewer* viewer)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (viewer) {
        auto it = _viewers.find(viewer);
        if (it != _viewers.end()) {
            // Synchronize specific viewer
            for (auto* vp : it->second.viewProviders) {
                switch (it->second.backend) {
                    case BackendType::Coin3D:
                        syncCoin3DNode(viewer, vp, true);
                        break;
                    case BackendType::OsgVerse:
                        syncOsgVerseNode(viewer, vp, true);
                        break;
                    default:
                        break;
                }
            }
        }
    } else {
        // Synchronize all viewers
        for (auto& viewerPair : _viewers) {
            for (auto* vp : viewerPair.second.viewProviders) {
                switch (viewerPair.second.backend) {
                    case BackendType::Coin3D:
                        syncCoin3DNode(viewerPair.first, vp, true);
                        break;
                    case BackendType::OsgVerse:
                        syncOsgVerseNode(viewerPair.first, vp, true);
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

void SceneGraphBridge::rebuild(View3DInventorViewer* viewer)
{
    // For now, just call synchronize
    // In the future, this could do a full scene graph rebuild
    synchronize(viewer);
}

//=============================================================================
// Backend Switching
//=============================================================================

bool SceneGraphBridge::switchBackend(View3DInventorViewer* viewer, BackendType newBackend)
{
    if (!viewer) {
        return false;
    }

    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _viewers.find(viewer);
    if (it == _viewers.end()) {
        FC_WARN("SceneGraphBridge: Cannot switch backend - viewer not attached");
        return false;
    }

    if (it->second.backend == newBackend) {
        return true; // Already using this backend
    }

    if (!isBackendAvailable(newBackend)) {
        FC_WARN("SceneGraphBridge: Backend not available: " << static_cast<int>(newBackend));
        return false;
    }

    // Store old backend for cleanup
    BackendType oldBackend = it->second.backend;

    // Update backend type
    it->second.backend = newBackend;

    // Rebuild scene graph with new backend
    // This is a simplified implementation - a full implementation would:
    // 1. Create new scene root for new backend
    // 2. Migrate all ViewProvider render nodes
    // 3. Clean up old backend resources

    FC_MSG("SceneGraphBridge: Switched viewer backend from "
           << static_cast<int>(oldBackend) << " to " << static_cast<int>(newBackend));

    return true;
}

bool SceneGraphBridge::isBackendAvailable(BackendType type)
{
    switch (type) {
        case BackendType::Coin3D:
            return true; // Always available
        case BackendType::OsgVerse:
#ifdef FREECAD_USE_OSGVERSE
            return true;
#else
            return false;
#endif
        default:
            return false;
    }
}

BackendType SceneGraphBridge::getDefaultBackend()
{
    return _defaultBackend;
}

void SceneGraphBridge::setDefaultBackend(BackendType type)
{
    _defaultBackend = type;
}

//=============================================================================
// Callbacks
//=============================================================================

int SceneGraphBridge::registerSceneChangedCallback(SceneChangedCallback callback)
{
    std::lock_guard<std::mutex> lock(_mutex);
    int handle = _nextCallbackHandle++;
    _callbacks[handle] = std::move(callback);
    return handle;
}

void SceneGraphBridge::unregisterSceneChangedCallback(int handle)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _callbacks.erase(handle);
}

void SceneGraphBridge::notifySceneChanged(View3DInventorViewer* viewer, ViewProvider* vp)
{
    // Note: _mutex should already be held by caller

    for (const auto& callbackPair : _callbacks) {
        try {
            callbackPair.second(viewer, vp);
        } catch (const std::exception& e) {
            FC_ERR("SceneGraphBridge: Callback exception: " << e.what());
        }
    }
}

//=============================================================================
// Internal Sync Methods
//=============================================================================

void SceneGraphBridge::syncCoin3DNode(View3DInventorViewer* viewer, ViewProvider* vp, bool add)
{
    // For Coin3D backend, the viewer already handles this through its
    // addViewProvider/removeViewProvider methods which work with SoSeparator.
    // The render abstraction layer nodes (m_renderRoot) are kept in sync
    // with the Coin3D nodes by the ViewProvider.

    // This method is a hook for future enhancements where we might want to
    // do additional synchronization or optimization.

    if (_debugEnabled) {
        FC_MSG("SceneGraphBridge: Sync Coin3D node - add=" << add);
    }
}

void SceneGraphBridge::syncOsgVerseNode(View3DInventorViewer* viewer, ViewProvider* vp, bool add)
{
    // For OsgVerse backend, we need to add the ViewProvider's render nodes
    // to the OsgVerse scene graph.

    // This is a placeholder for the OsgVerse integration.
    // The actual implementation would:
    // 1. Get the ViewProvider's render root
    // 2. Convert or use the OsgVerse-specific render nodes
    // 3. Add/remove from the OsgVerse scene graph

    if (_debugEnabled) {
        FC_MSG("SceneGraphBridge: Sync OsgVerse node - add=" << add);
    }
}

//=============================================================================
// Statistics
//=============================================================================

size_t SceneGraphBridge::getViewerCount() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _viewers.size();
}

size_t SceneGraphBridge::getViewProviderCount() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _viewProviders.size();
}

SceneGraphBridge::BridgeStats SceneGraphBridge::getStats() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    BridgeStats stats;
    stats.viewerCount = _viewers.size();
    stats.viewProviderCount = _viewProviders.size();
    stats.renderNodeCount = _nodeToVP.size();
    stats.defaultBackend = _defaultBackend;

    return stats;
}

} // namespace Render
} // namespace Gui
