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

#include "SelectionBridge.h"
#include "RenderNode.h"

#include <App/Document.h>
#include <App/DocumentObject.h>
#include "../../ViewProvider.h"
#include "../../ViewProviderDocumentObject.h"
#include "../../Document.h"
#include "../../Application.h"
#include <Base/Console.h>

FC_LOG_LEVEL_INIT("Render")

namespace Gui {
namespace Render {

//===========================================================================
// SelectionBridge
//===========================================================================

SelectionBridge& SelectionBridge::instance()
{
    static SelectionBridge singleton;
    return singleton;
}

SelectionBridge::SelectionBridge()
    : SelectionObserver(true, ResolveMode::NoResolve)  // Attached automatically, no resolve
{
    FC_LOG("SelectionBridge: Constructor");
}

SelectionBridge::~SelectionBridge()
{
    FC_LOG("SelectionBridge: Destructor");
    shutdown();
}

void SelectionBridge::initialize()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_initialized) {
        return;
    }

    FC_LOG("SelectionBridge: Initializing");

    // Attach to selection singleton if not already attached
    // (Constructor passes true for 'attach', so we should already be attached)
    if (!isSelectionAttached()) {
        attachSelection();
    }

    _initialized = true;
    FC_LOG("SelectionBridge: Initialized successfully");
}

void SelectionBridge::shutdown()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_initialized) {
        return;
    }

    FC_LOG("SelectionBridge: Shutting down");

    // Detach from selection singleton
    if (isSelectionAttached()) {
        detachSelection();
    }

    // Clear mappings
    _nodeToVP.clear();
    _vpToNode.clear();

    _initialized = false;
    FC_LOG("SelectionBridge: Shutdown complete");
}

//===========================================================================
// Node <-> ViewProvider Mapping
//===========================================================================

void SelectionBridge::registerNode(RenderNode* node, ViewProvider* vp)
{
    if (!node || !vp) {
        return;
    }

    std::lock_guard<std::mutex> lock(_mutex);

    _nodeToVP[node] = vp;
    _vpToNode[vp] = node;

    // Set the ViewProvider reference on the node
    node->setViewProvider(vp);

    FC_LOG("SelectionBridge: Registered node '" << node->getName() << "' with ViewProvider");
}

void SelectionBridge::unregisterNode(RenderNode* node)
{
    if (!node) {
        return;
    }

    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _nodeToVP.find(node);
    if (it != _nodeToVP.end()) {
        ViewProvider* vp = it->second;
        _vpToNode.erase(vp);
        _nodeToVP.erase(it);

        // Clear the ViewProvider reference on the node
        node->setViewProvider(nullptr);

        FC_LOG("SelectionBridge: Unregistered node '" << node->getName() << "'");
    }
}

ViewProvider* SelectionBridge::getViewProvider(RenderNode* node) const
{
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _nodeToVP.find(node);
    return (it != _nodeToVP.end()) ? it->second : nullptr;
}

RenderNode* SelectionBridge::getRenderNode(ViewProvider* vp) const
{
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _vpToNode.find(vp);
    return (it != _vpToNode.end()) ? it->second : nullptr;
}

//===========================================================================
// Selection Operations
//===========================================================================

bool SelectionBridge::selectNode(RenderNode* node, const std::string& element, bool add)
{
    if (!node) {
        return false;
    }

    ViewProvider* vp = getViewProvider(node);
    if (!vp) {
        FC_WARN("SelectionBridge::selectNode: Node has no associated ViewProvider");
        return false;
    }

    // Get the document object from ViewProvider
    auto* vpDoc = dynamic_cast<ViewProviderDocumentObject*>(vp);
    if (!vpDoc || !vpDoc->getObject()) {
        FC_WARN("SelectionBridge::selectNode: ViewProvider is not a document object");
        return false;
    }

    App::DocumentObject* obj = vpDoc->getObject();
    App::Document* doc = obj->getDocument();
    if (!doc) {
        return false;
    }

    // Use FreeCAD's selection system
    if (!add) {
        Selection().clearSelection(doc->getName());
    }

    return Selection().addSelection(doc->getName(), obj->getNameInDocument(), element.c_str());
}

void SelectionBridge::deselectNode(RenderNode* node, const std::string& element)
{
    if (!node) {
        return;
    }

    ViewProvider* vp = getViewProvider(node);
    if (!vp) {
        return;
    }

    auto* vpDoc = dynamic_cast<ViewProviderDocumentObject*>(vp);
    if (!vpDoc || !vpDoc->getObject()) {
        return;
    }

    App::DocumentObject* obj = vpDoc->getObject();
    App::Document* doc = obj->getDocument();
    if (!doc) {
        return;
    }

    Selection().rmvSelection(doc->getName(), obj->getNameInDocument(), element.c_str());
}

void SelectionBridge::setPreselection(RenderNode* node, const std::string& element,
                                      float x, float y, float z)
{
    if (!node) {
        return;
    }

    ViewProvider* vp = getViewProvider(node);
    if (!vp) {
        return;
    }

    auto* vpDoc = dynamic_cast<ViewProviderDocumentObject*>(vp);
    if (!vpDoc || !vpDoc->getObject()) {
        return;
    }

    App::DocumentObject* obj = vpDoc->getObject();
    App::Document* doc = obj->getDocument();
    if (!doc) {
        return;
    }

    Selection().setPreselect(doc->getName(), obj->getNameInDocument(), element.c_str(), x, y, z);
}

void SelectionBridge::clearPreselection()
{
    Selection().rmvPreselect();
}

void SelectionBridge::clearSelection()
{
    Selection().clearCompleteSelection();
}

//===========================================================================
// Selection Query
//===========================================================================

bool SelectionBridge::isSelected(RenderNode* node, const std::string& element) const
{
    if (!node) {
        return false;
    }

    ViewProvider* vp = getViewProvider(node);
    if (!vp) {
        return node->isSelected();  // Fall back to node's internal state
    }

    auto* vpDoc = dynamic_cast<ViewProviderDocumentObject*>(vp);
    if (!vpDoc || !vpDoc->getObject()) {
        return node->isSelected();
    }

    App::DocumentObject* obj = vpDoc->getObject();
    App::Document* doc = obj->getDocument();
    if (!doc) {
        return false;
    }

    return Selection().isSelected(doc->getName(), obj->getNameInDocument(), element.c_str());
}

bool SelectionBridge::isPreselected(RenderNode* node, const std::string& element) const
{
    if (!node) {
        return false;
    }

    // Check node's internal state
    if (node->isHighlighted()) {
        if (element.empty() || node->getHighlightedElement() == element) {
            return true;
        }
    }

    // Also check FreeCAD's preselection
    ViewProvider* vp = getViewProvider(node);
    if (!vp) {
        return false;
    }

    auto* vpDoc = dynamic_cast<ViewProviderDocumentObject*>(vp);
    if (!vpDoc || !vpDoc->getObject()) {
        return false;
    }

    App::DocumentObject* obj = vpDoc->getObject();
    App::Document* doc = obj->getDocument();
    if (!doc) {
        return false;
    }

    const auto& preselect = Selection().getPreselection();
    return (preselect.pDocName && strcmp(preselect.pDocName, doc->getName()) == 0 &&
            preselect.pObjectName && strcmp(preselect.pObjectName, obj->getNameInDocument()) == 0);
}

std::vector<RenderNode*> SelectionBridge::getSelectedNodes() const
{
    std::vector<RenderNode*> result;
    std::lock_guard<std::mutex> lock(_mutex);

    for (const auto& pair : _nodeToVP) {
        RenderNode* node = pair.first;
        if (node->isSelected()) {
            result.push_back(node);
        }
    }

    return result;
}

//===========================================================================
// Color Configuration
//===========================================================================

void SelectionBridge::setHighlightColor(float r, float g, float b, float a)
{
    _highlightColor[0] = r;
    _highlightColor[1] = g;
    _highlightColor[2] = b;
    _highlightColor[3] = a;

    // Update all registered nodes
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& pair : _nodeToVP) {
        pair.first->setHighlightColor(r, g, b, a);
    }
}

void SelectionBridge::setSelectionColor(float r, float g, float b, float a)
{
    _selectionColor[0] = r;
    _selectionColor[1] = g;
    _selectionColor[2] = b;
    _selectionColor[3] = a;

    // Update all registered nodes
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& pair : _nodeToVP) {
        pair.first->setSelectionColor(r, g, b, a);
    }
}

void SelectionBridge::getHighlightColor(float& r, float& g, float& b, float& a) const
{
    r = _highlightColor[0];
    g = _highlightColor[1];
    b = _highlightColor[2];
    a = _highlightColor[3];
}

void SelectionBridge::getSelectionColor(float& r, float& g, float& b, float& a) const
{
    r = _selectionColor[0];
    g = _selectionColor[1];
    b = _selectionColor[2];
    a = _selectionColor[3];
}

//===========================================================================
// SelectionObserver Interface
//===========================================================================

void SelectionBridge::onSelectionChanged(const SelectionChanges& msg)
{
    if (!_initialized) {
        return;
    }

    switch (msg.Type) {
        case SelectionChanges::AddSelection:
            handleAddSelection(msg);
            break;
        case SelectionChanges::RmvSelection:
            handleRemoveSelection(msg);
            break;
        case SelectionChanges::SetPreselect:
            handleSetPreselect(msg);
            break;
        case SelectionChanges::RmvPreselect:
            handleRemovePreselect(msg);
            break;
        case SelectionChanges::ClrSelection:
            handleClearSelection(msg);
            break;
        default:
            // Other message types not handled
            break;
    }
}

void SelectionBridge::handleAddSelection(const SelectionChanges& msg)
{
    RenderNode* node = findNodeByName(msg.pDocName ? msg.pDocName : "",
                                      msg.pObjectName ? msg.pObjectName : "");
    if (node) {
        std::string element = msg.pSubName ? msg.pSubName : "";
        updateNodeSelectionState(node, true, element);
        FC_LOG("SelectionBridge: Added selection for '" << node->getName()
               << "' element='" << element << "'");
    }
}

void SelectionBridge::handleRemoveSelection(const SelectionChanges& msg)
{
    RenderNode* node = findNodeByName(msg.pDocName ? msg.pDocName : "",
                                      msg.pObjectName ? msg.pObjectName : "");
    if (node) {
        std::string element = msg.pSubName ? msg.pSubName : "";
        updateNodeSelectionState(node, false, element);
        FC_LOG("SelectionBridge: Removed selection for '" << node->getName()
               << "' element='" << element << "'");
    }
}

void SelectionBridge::handleSetPreselect(const SelectionChanges& msg)
{
    RenderNode* node = findNodeByName(msg.pDocName ? msg.pDocName : "",
                                      msg.pObjectName ? msg.pObjectName : "");
    if (node) {
        std::string element = msg.pSubName ? msg.pSubName : "";
        updateNodeHighlightState(node, true, element);
        FC_LOG("SelectionBridge: Set preselect for '" << node->getName()
               << "' element='" << element << "'");
    }
}

void SelectionBridge::handleRemovePreselect(const SelectionChanges& msg)
{
    // Clear highlight from all nodes (preselection is global)
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& pair : _nodeToVP) {
        if (pair.first->isHighlighted()) {
            pair.first->setHighlighted(false, "");
        }
    }

    (void)msg;  // msg may not contain specific object info for RmvPreselect
    FC_LOG("SelectionBridge: Removed preselect");
}

void SelectionBridge::handleClearSelection(const SelectionChanges& msg)
{
    std::lock_guard<std::mutex> lock(_mutex);

    std::string docName = msg.pDocName ? msg.pDocName : "";

    for (auto& pair : _nodeToVP) {
        RenderNode* node = pair.first;
        ViewProvider* vp = pair.second;

        // Check if this node belongs to the cleared document
        auto* vpDoc = dynamic_cast<ViewProviderDocumentObject*>(vp);
        if (vpDoc && vpDoc->getObject()) {
            App::Document* doc = vpDoc->getObject()->getDocument();
            if (doc && (docName.empty() || docName == doc->getName())) {
                if (node->isSelected()) {
                    node->setSelected(false, "");
                }
            }
        }
    }

    FC_LOG("SelectionBridge: Cleared selection for document '" << docName << "'");
}

//===========================================================================
// Helper Methods
//===========================================================================

void SelectionBridge::updateNodeSelectionState(RenderNode* node, bool selected,
                                               const std::string& element)
{
    if (!node) {
        return;
    }

    node->setSelected(selected, element);

    // Apply selection color if selected
    if (selected) {
        node->setSelectionColor(_selectionColor[0], _selectionColor[1],
                               _selectionColor[2], _selectionColor[3]);
    }
}

void SelectionBridge::updateNodeHighlightState(RenderNode* node, bool highlighted,
                                               const std::string& element)
{
    if (!node) {
        return;
    }

    node->setHighlighted(highlighted, element);

    // Apply highlight color if highlighted
    if (highlighted) {
        node->setHighlightColor(_highlightColor[0], _highlightColor[1],
                               _highlightColor[2], _highlightColor[3]);
    }
}

RenderNode* SelectionBridge::findNodeByName(const std::string& docName,
                                            const std::string& objName) const
{
    if (docName.empty() || objName.empty()) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(_mutex);

    for (const auto& pair : _nodeToVP) {
        ViewProvider* vp = pair.second;
        auto* vpDoc = dynamic_cast<ViewProviderDocumentObject*>(vp);
        if (vpDoc && vpDoc->getObject()) {
            App::DocumentObject* obj = vpDoc->getObject();
            App::Document* doc = obj->getDocument();
            if (doc && doc->getName() == docName &&
                obj->getNameInDocument() == objName) {
                return pair.first;
            }
        }
    }

    return nullptr;
}

} // namespace Render
} // namespace Gui
