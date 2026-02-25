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

#ifndef GUI_RENDER_SCENEGRAPHBRIDGE_H
#define GUI_RENDER_SCENEGRAPHBRIDGE_H

#include <memory>
#include <map>
#include <set>
#include <mutex>
#include <functional>

#include <FCGlobal.h>
#include "RenderTypes.h"
#include "RenderNode.h"

// Forward declarations for Coin3D
class SoSeparator;
class SoGroup;
class SoNode;

namespace Gui {

// Forward declarations
class ViewProvider;
class View3DInventorViewer;
class Document;

namespace Render {

/**
 * @brief Scene Graph Bridge for Document/View Integration
 *
 * This class manages the integration between the render abstraction layer
 * and the View3DInventorViewer. It provides:
 *
 * 1. Mapping between ViewProviders and their RenderNodes
 * 2. Scene graph synchronization between backends
 * 3. Backend-agnostic ViewProvider management
 *
 * Design Goals:
 * - Maintain backward compatibility with Coin3D-based code
 * - Enable seamless backend switching at runtime
 * - Provide a unified interface for scene graph operations
 *
 * Usage:
 * @code
 * SceneGraphBridge& bridge = SceneGraphBridge::instance();
 * bridge.attachViewer(viewer);
 * bridge.addViewProvider(vp);
 * @endcode
 */
class GuiExport SceneGraphBridge {
public:
    /**
     * @brief Get singleton instance
     */
    static SceneGraphBridge& instance();

    //=========================================================================
    // Viewer Management
    //=========================================================================

    /**
     * @brief Attach a viewer to the bridge
     *
     * Registers the viewer for scene graph synchronization.
     * Each viewer can have its own backend type.
     *
     * @param viewer The viewer to attach
     * @param backend The backend type to use for this viewer
     */
    void attachViewer(View3DInventorViewer* viewer, BackendType backend = BackendType::Coin3D);

    /**
     * @brief Detach a viewer from the bridge
     *
     * @param viewer The viewer to detach
     */
    void detachViewer(View3DInventorViewer* viewer);

    /**
     * @brief Check if a viewer is attached
     */
    bool hasViewer(View3DInventorViewer* viewer) const;

    /**
     * @brief Get the backend type for a viewer
     */
    BackendType getViewerBackend(View3DInventorViewer* viewer) const;

    //=========================================================================
    // ViewProvider Management
    //=========================================================================

    /**
     * @brief Add a ViewProvider to all attached viewers
     *
     * This method adds the ViewProvider's render nodes to all attached
     * viewers using the appropriate backend.
     *
     * @param vp The ViewProvider to add
     */
    void addViewProvider(ViewProvider* vp);

    /**
     * @brief Add a ViewProvider to a specific viewer
     *
     * @param viewer The target viewer
     * @param vp The ViewProvider to add
     */
    void addViewProvider(View3DInventorViewer* viewer, ViewProvider* vp);

    /**
     * @brief Remove a ViewProvider from all attached viewers
     *
     * @param vp The ViewProvider to remove
     */
    void removeViewProvider(ViewProvider* vp);

    /**
     * @brief Remove a ViewProvider from a specific viewer
     *
     * @param viewer The target viewer
     * @param vp The ViewProvider to remove
     */
    void removeViewProvider(View3DInventorViewer* viewer, ViewProvider* vp);

    /**
     * @brief Check if a ViewProvider is managed by the bridge
     */
    bool hasViewProvider(ViewProvider* vp) const;

    /**
     * @brief Get the RenderNode for a ViewProvider
     *
     * @param vp The ViewProvider
     * @return The associated RenderNode, or nullptr if not found
     */
    RenderNode* getRenderNode(ViewProvider* vp) const;

    /**
     * @brief Get the ViewProvider for a RenderNode
     *
     * @param node The RenderNode
     * @return The associated ViewProvider, or nullptr if not found
     */
    ViewProvider* getViewProvider(RenderNode* node) const;

    //=========================================================================
    // Scene Graph Operations
    //=========================================================================

    /**
     * @brief Get the scene root node for a viewer
     *
     * Returns the backend-specific scene root node.
     * For Coin3D: SoSeparator*
     * For OsgVerse: osg::Group*
     *
     * @param viewer The viewer
     * @return The scene root node (cast to appropriate type)
     */
    RenderNode::Ptr getSceneRoot(View3DInventorViewer* viewer) const;

    /**
     * @brief Get the object group node for a viewer
     *
     * This is the group that contains all physical objects.
     *
     * @param viewer The viewer
     * @return The object group node
     */
    RenderNode::Ptr getObjectGroup(View3DInventorViewer* viewer) const;

    /**
     * @brief Synchronize scene graph after changes
     *
     * Call this after making changes to ViewProvider render nodes
     * to ensure the backend scene graph is updated.
     *
     * @param viewer Optional specific viewer, or nullptr for all viewers
     */
    void synchronize(View3DInventorViewer* viewer = nullptr);

    /**
     * @brief Force a full scene graph rebuild
     *
     * Use with caution - this is expensive.
     *
     * @param viewer Optional specific viewer, or nullptr for all viewers
     */
    void rebuild(View3DInventorViewer* viewer = nullptr);

    //=========================================================================
    // Backend Switching
    //=========================================================================

    /**
     * @brief Switch the backend for a viewer
     *
     * This will rebuild the scene graph with the new backend.
     *
     * @param viewer The viewer to switch
     * @param newBackend The new backend type
     * @return true if successful
     */
    bool switchBackend(View3DInventorViewer* viewer, BackendType newBackend);

    /**
     * @brief Check if a backend is available
     */
    static bool isBackendAvailable(BackendType type);

    /**
     * @brief Get the default backend type
     */
    static BackendType getDefaultBackend();

    /**
     * @brief Set the default backend type
     */
    static void setDefaultBackend(BackendType type);

    //=========================================================================
    // Callbacks
    //=========================================================================

    /**
     * @brief Callback type for scene changes
     */
    using SceneChangedCallback = std::function<void(View3DInventorViewer*, ViewProvider*)>;

    /**
     * @brief Register a callback for scene changes
     *
     * @param callback The callback function
     * @return A handle to unregister the callback
     */
    int registerSceneChangedCallback(SceneChangedCallback callback);

    /**
     * @brief Unregister a scene changed callback
     *
     * @param handle The handle returned by registerSceneChangedCallback
     */
    void unregisterSceneChangedCallback(int handle);

    //=========================================================================
    // Statistics and Debugging
    //=========================================================================

    /**
     * @brief Get the number of attached viewers
     */
    size_t getViewerCount() const;

    /**
     * @brief Get the number of managed ViewProviders
     */
    size_t getViewProviderCount() const;

    /**
     * @brief Get statistics about the bridge
     */
    struct BridgeStats {
        size_t viewerCount{0};
        size_t viewProviderCount{0};
        size_t renderNodeCount{0};
        BackendType defaultBackend{BackendType::Coin3D};
    };
    BridgeStats getStats() const;

    /**
     * @brief Enable/disable debug logging
     */
    void setDebugEnabled(bool enabled) { _debugEnabled = enabled; }
    bool isDebugEnabled() const { return _debugEnabled; }

private:
    SceneGraphBridge();
    ~SceneGraphBridge();

    // Disable copy and move
    SceneGraphBridge(const SceneGraphBridge&) = delete;
    SceneGraphBridge& operator=(const SceneGraphBridge&) = delete;

    //-------------------------------------------------------------------------
    // Internal structures
    //-------------------------------------------------------------------------

    struct ViewerInfo {
        View3DInventorViewer* viewer{nullptr};
        BackendType backend{BackendType::Coin3D};
        RenderNode::Ptr sceneRoot;
        RenderNode::Ptr objectGroup;
        std::set<ViewProvider*> viewProviders;
    };

    struct ViewProviderInfo {
        ViewProvider* vp{nullptr};
        RenderNode::Ptr renderRoot;
        std::set<View3DInventorViewer*> viewers;
    };

    //-------------------------------------------------------------------------
    // Internal methods
    //-------------------------------------------------------------------------

    void notifySceneChanged(View3DInventorViewer* viewer, ViewProvider* vp);
    void syncCoin3DNode(View3DInventorViewer* viewer, ViewProvider* vp, bool add);
    void syncOsgVerseNode(View3DInventorViewer* viewer, ViewProvider* vp, bool add);

    //-------------------------------------------------------------------------
    // Data members
    //-------------------------------------------------------------------------

    mutable std::mutex _mutex;
    std::map<View3DInventorViewer*, ViewerInfo> _viewers;
    std::map<ViewProvider*, ViewProviderInfo> _viewProviders;
    std::map<RenderNode*, ViewProvider*> _nodeToVP;

    std::map<int, SceneChangedCallback> _callbacks;
    int _nextCallbackHandle{0};

    bool _debugEnabled{false};
    static BackendType _defaultBackend;
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_SCENEGRAPHBRIDGE_H
