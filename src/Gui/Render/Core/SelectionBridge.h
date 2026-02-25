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

#ifndef GUI_RENDER_SELECTIONBRIDGE_H
#define GUI_RENDER_SELECTIONBRIDGE_H

#include <map>
#include <mutex>
#include <string>

#include <FCGlobal.h>
#include "../../Selection/Selection.h"

namespace Gui {

class ViewProvider;
class View3DInventorViewer;

namespace Render {

class RenderNode;

/**
 * @brief Selection bridge connecting render abstraction layer with FreeCAD's selection system
 *
 * This class bridges the gap between FreeCAD's existing selection system (based on Coin3D)
 * and the new render abstraction layer. It:
 * - Observes selection changes from SelectionSingleton
 * - Updates RenderNode selection states
 * - Maps between RenderNodes and ViewProviders for selection
 *
 * 选择桥接类，连接渲染抽象层与 FreeCAD 选择系统
 */
class GuiExport SelectionBridge : public SelectionObserver {
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the SelectionBridge instance
     */
    static SelectionBridge& instance();

    /**
     * @brief Initialize the selection bridge
     *
     * Call this during FreeCAD GUI initialization to start observing selection changes.
     */
    void initialize();

    /**
     * @brief Shutdown the selection bridge
     *
     * Call this during FreeCAD GUI shutdown.
     */
    void shutdown();

    /**
     * @brief Check if the bridge is initialized
     */
    bool isInitialized() const { return _initialized; }

    //-----------------------------------------------------------------------
    // RenderNode <-> ViewProvider Mapping
    //-----------------------------------------------------------------------

    /**
     * @brief Register a RenderNode with its ViewProvider
     * @param node The RenderNode
     * @param vp The associated ViewProvider
     */
    void registerNode(RenderNode* node, ViewProvider* vp);

    /**
     * @brief Unregister a RenderNode
     * @param node The RenderNode to unregister
     */
    void unregisterNode(RenderNode* node);

    /**
     * @brief Get ViewProvider for a RenderNode
     * @param node The RenderNode
     * @return Associated ViewProvider or nullptr
     */
    ViewProvider* getViewProvider(RenderNode* node) const;

    /**
     * @brief Get RenderNode for a ViewProvider
     * @param vp The ViewProvider
     * @return Associated RenderNode or nullptr
     */
    RenderNode* getRenderNode(ViewProvider* vp) const;

    //-----------------------------------------------------------------------
    // Selection Operations
    //-----------------------------------------------------------------------

    /**
     * @brief Select a RenderNode
     * @param node The RenderNode to select
     * @param element Sub-element name (e.g., "Face1", "Edge2")
     * @param add Add to current selection (true) or replace (false)
     * @return true if selection was successful
     */
    bool selectNode(RenderNode* node, const std::string& element = "", bool add = true);

    /**
     * @brief Deselect a RenderNode
     * @param node The RenderNode to deselect
     * @param element Sub-element name (empty for whole object)
     */
    void deselectNode(RenderNode* node, const std::string& element = "");

    /**
     * @brief Set preselection (highlight) on a RenderNode
     * @param node The RenderNode to highlight
     * @param element Sub-element name
     * @param x 3D x coordinate of pick point
     * @param y 3D y coordinate of pick point
     * @param z 3D z coordinate of pick point
     */
    void setPreselection(RenderNode* node, const std::string& element = "",
                        float x = 0, float y = 0, float z = 0);

    /**
     * @brief Clear preselection
     */
    void clearPreselection();

    /**
     * @brief Clear all selections
     */
    void clearSelection();

    //-----------------------------------------------------------------------
    // Selection Query
    //-----------------------------------------------------------------------

    /**
     * @brief Check if a RenderNode is selected
     * @param node The RenderNode to check
     * @param element Optional sub-element name
     * @return true if selected
     */
    bool isSelected(RenderNode* node, const std::string& element = "") const;

    /**
     * @brief Check if a RenderNode is preselected (highlighted)
     * @param node The RenderNode to check
     * @param element Optional sub-element name
     * @return true if preselected
     */
    bool isPreselected(RenderNode* node, const std::string& element = "") const;

    /**
     * @brief Get all selected RenderNodes
     * @return Vector of selected RenderNode pointers
     */
    std::vector<RenderNode*> getSelectedNodes() const;

    //-----------------------------------------------------------------------
    // Color Configuration
    //-----------------------------------------------------------------------

    /**
     * @brief Set the highlight (preselection) color
     */
    void setHighlightColor(float r, float g, float b, float a = 1.0f);

    /**
     * @brief Set the selection color
     */
    void setSelectionColor(float r, float g, float b, float a = 1.0f);

    /**
     * @brief Get the highlight color
     */
    void getHighlightColor(float& r, float& g, float& b, float& a) const;

    /**
     * @brief Get the selection color
     */
    void getSelectionColor(float& r, float& g, float& b, float& a) const;

protected:
    /**
     * @brief Called when selection changes (SelectionObserver interface)
     * @param msg The selection change message
     */
    void onSelectionChanged(const SelectionChanges& msg) override;

private:
    // Private constructor for singleton
    SelectionBridge();
    ~SelectionBridge() override;

    // Disable copy
    SelectionBridge(const SelectionBridge&) = delete;
    SelectionBridge& operator=(const SelectionBridge&) = delete;

    // Handle specific selection message types
    void handleAddSelection(const SelectionChanges& msg);
    void handleRemoveSelection(const SelectionChanges& msg);
    void handleSetPreselect(const SelectionChanges& msg);
    void handleRemovePreselect(const SelectionChanges& msg);
    void handleClearSelection(const SelectionChanges& msg);

    // Update RenderNode visual state based on selection
    void updateNodeSelectionState(RenderNode* node, bool selected, const std::string& element);
    void updateNodeHighlightState(RenderNode* node, bool highlighted, const std::string& element);

    // Find RenderNode by document/object name
    RenderNode* findNodeByName(const std::string& docName, const std::string& objName) const;

private:
    bool _initialized{false};

    // Node <-> ViewProvider mappings
    std::map<RenderNode*, ViewProvider*> _nodeToVP;
    std::map<ViewProvider*, RenderNode*> _vpToNode;

    // Selection colors
    float _highlightColor[4]{0.9f, 0.9f, 0.0f, 1.0f};  // Yellow
    float _selectionColor[4]{0.0f, 0.8f, 0.0f, 1.0f};  // Green

    // Thread safety
    mutable std::mutex _mutex;
};

} // namespace Render
} // namespace Gui

#endif // GUI_RENDER_SELECTIONBRIDGE_H
