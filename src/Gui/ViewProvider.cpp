/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <boost_graph_adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>

#include <QApplication>
#include <QKeyEvent>
#include <QTimer>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/details/SoDetail.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTransform.h>


#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Matrix.h>
#include <Base/Tools.h>

#include "Inventor/SoMouseWheelEvent.h"
#include "Inventor/SoFCTransform.h"
#include "ViewProvider.h"
#include "Render/Core/RenderNode.h"
#include "Render/Core/RenderNodeFactory.h"
#include "Render/Backends/Coin3D/Coin3DNode.h"
#include "Render/Backends/Coin3D/Coin3DNodeFactory.h"
#ifdef RENDER_HAS_OSGVERSE_BACKEND
#include "Render/Backends/OsgVerse/OsgVerseNode.h"
#endif
#include "ActionFunction.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Document.h"
#include "DockWindowManager.h"
#include "SoFCDB.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "ViewParams.h"
#include "ViewProviderExtension.h"
#include "ViewProviderLink.h"
#include "ViewProviderPy.h"

#include <Utilities.h>


FC_LOG_LEVEL_INIT("ViewProvider", true, true)

using namespace std;
using namespace Gui;


namespace Gui
{

void coinRemoveAllChildren(SoGroup* group)
{
    if (!group) {
        return;
    }
    int count = group->getNumChildren();
    if (!count) {
        return;
    }
    FC_TRACE("coin remove all children " << count);
    SbBool autonotify = group->enableNotify(FALSE);
    for (; count > 0; --count) {
        group->removeChild(count - 1);
    }
    group->enableNotify(autonotify);
    group->touch();
}

}  // namespace Gui

//**************************************************************************
//**************************************************************************
// ViewProvider
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

PROPERTY_SOURCE_ABSTRACT(Gui::ViewProvider, App::TransactionalObject)

ViewProvider::ViewProvider()
    : overrideMode("As Is")
    , toggleVisibilityMode(ToggleVisibilityMode::CanToggleVisibility)
{
    setStatus(UpdateData, true);

    //=========================================================================
    // 初始化 Coin3D 节点（向后兼容）
    // Initialize Coin3D nodes (backward compatibility)
    //=========================================================================

    // SoFCSeparator and SoFCSelectionRoot can both track render cache setting.
    // We change to SoFCSelectionRoot so that we can dynamically change full
    // selection mode (full highlight vs. boundbox). Note that comparing to
    // SoFCSeparator, there are some small overhead with SoFCSelectionRoot for
    // selection context tracking.
    //
    // pcRoot = new SoFCSeparator(true);
    pcRoot = new SoFCSelectionRoot(true);
    pcRoot->ref();
    pcModeSwitch = new SoSwitch();
    pcModeSwitch->ref();
    pcModeSwitch->setName("ModeSwitch");
    pcTransform = new SoFCTransform();
    pcTransform->ref();
    pcRoot->addChild(pcTransform);
    pcRoot->addChild(pcModeSwitch);
    sPixmap = "px";
    pcModeSwitch->whichChild = _iActualMode;

    setRenderCacheMode(ViewParams::instance()->getRenderCache());

    //=========================================================================
    // 初始化抽象层节点（方案B：后端完全平等）
    // Initialize abstraction layer nodes (Plan B: backends are fully equal)
    //=========================================================================

    initRenderNodes();
}

ViewProvider::~ViewProvider()
{
    if (pyViewObject) {
        Base::PyGILStateLocker lock;
        pyViewObject->setInvalid();
        pyViewObject->DecRef();
    }

    pcRoot->unref();
    pcTransform->unref();
    pcModeSwitch->unref();
    if (pcAnnotation) {
        pcAnnotation->unref();
    }
}

ViewProvider* ViewProvider::startEditing(int ModNum)
{
    try {
        if (setEdit(ModNum)) {
            _iEditMode = ModNum;
            return this;
        }
    }
    catch (const Base::Exception& e) {
        e.reportException();
    }
    return nullptr;
}

int ViewProvider::getEditingMode() const
{
    return _iEditMode;
}

bool ViewProvider::isEditing() const
{
    return getEditingMode() > -1;
}

void ViewProvider::finishEditing()
{
    unsetEdit(_iEditMode);
    _iEditMode = -1;
}

bool ViewProvider::setEdit(int ModNum)
{
    Q_UNUSED(ModNum);
    return true;
}

void ViewProvider::unsetEdit(int ModNum)
{
    Q_UNUSED(ModNum);
}

void ViewProvider::setEditViewer(View3DInventorViewer*, int ModNum)
{
    Q_UNUSED(ModNum);
}

void ViewProvider::unsetEditViewer(View3DInventorViewer*)
{}

bool ViewProvider::isUpdatesEnabled() const
{
    return testStatus(UpdateData);
}

void ViewProvider::setUpdatesEnabled(bool enable)
{
    setStatus(UpdateData, enable);
}

void highlight(const HighlightMode& high)
{
    Q_UNUSED(high);
}

void ViewProvider::eventCallback(void* ud, SoEventCallback* node)
{
    const SoEvent* ev = node->getEvent();
    auto viewer = static_cast<Gui::View3DInventorViewer*>(node->getUserData());
    auto self = static_cast<ViewProvider*>(ud);
    assert(self);

    try {
        // Keyboard events
        if (ev->getTypeId().isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
            auto ke = static_cast<const SoKeyboardEvent*>(ev);
            const SbBool press = ke->getState() == SoButtonEvent::DOWN ? true : false;
            switch (ke->getKey()) {
                case SoKeyboardEvent::ESCAPE:
                    if (self->keyPressed(press, ke->getKey())) {
                        node->setHandled();
                    }
                    else if (QApplication::mouseButtons() == Qt::NoButton) {
                        // Because of a Coin bug
                        // (https://bitbucket.org/Coin3D/coin/pull-requests/119), FC may crash if
                        // user hits ESC to cancel while still holding the mouse button while using
                        // some SoDragger. Therefore, we shall ignore ESC while any mouse button is
                        // pressed, until this Coin bug is fixed.
                        if (!press) {
                            // react only on key release
                            // Let first selection mode terminate
                            Gui::Document* doc = Gui::Application::Instance->activeDocument();
                            const auto view = qobject_cast<Gui::View3DInventor*>(doc->getActiveView());
                            if (view) {
                                Gui::View3DInventorViewer* viewer = view->getViewer();
                                if (viewer->isSelecting()) {
                                    return;
                                }
                            }

                            DockWindowManager* pDockMgr = DockWindowManager::instance();
                            if (QWidget* widget = pDockMgr->getDockWindow("Tasks")) {
                                auto* ev
                                    = new QKeyEvent(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
                                qApp->postEvent(widget, ev);
                            }

                            auto func = new Gui::TimerFunction();
                            func->setAutoDelete(true);
                            func->setFunction([doc]() { doc->resetEdit(); });
                            func->singleShot(0);
                        }
                    }
                    else if (press) {
                        FC_WARN("Release all mouse buttons before exiting editing");
                    }
                    break;
                default:
                    // call the virtual method
                    if (self->keyPressed(press, ke->getKey())) {
                        node->setHandled();
                    }
                    break;
            }
        }
        // switching the mouse buttons
        else if (ev->getTypeId().isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {

            const auto event = (const SoMouseButtonEvent*)ev;
            const int button = event->getButton();
            const SbBool press = event->getState() == SoButtonEvent::DOWN ? true : false;

            // call the virtual method
            if (self->mouseButtonPressed(button, press, ev->getPosition(), viewer)) {
                node->setHandled();
            }
        }
        else if (ev->getTypeId().isDerivedFrom(SoMouseWheelEvent::getClassTypeId())) {
            const auto event = (const SoMouseWheelEvent*)ev;

            if (self->mouseWheelEvent(event->getDelta(), event->getPosition(), viewer)) {
                node->setHandled();
            }
        }
        // Mouse Movement handling
        else if (ev->getTypeId().isDerivedFrom(SoLocation2Event::getClassTypeId())) {
            if (self->mouseMove(ev->getPosition(), viewer)) {
                node->setHandled();
            }
        }
    }
    catch (const Base::Exception& e) {
        Base::Console().error(
            "Unhandled exception in ViewProvider::eventCallback: %s\n"
            "(Event type: %s, object type: %s)\n",
            e.what(),
            ev->getTypeId().getName().getString(),
            self->getTypeId().getName()
        );
    }
    catch (const std::exception& e) {
        Base::Console().error(
            "Unhandled std exception in ViewProvider::eventCallback: %s\n"
            "(Event type: %s, object type: %s)\n",
            e.what(),
            ev->getTypeId().getName().getString(),
            self->getTypeId().getName()
        );
    }
    catch (...) {
        Base::Console().error(
            "Unhandled unknown C++ exception in ViewProvider::eventCallback"
            " (Event type: %s, object type: %s)\n",
            ev->getTypeId().getName().getString(),
            self->getTypeId().getName()
        );
    }
}

SoSeparator* ViewProvider::getAnnotation()
{
    if (!pcAnnotation) {
        pcAnnotation = new SoSeparator();
        pcAnnotation->ref();
        pcRoot->addChild(pcAnnotation);
    }
    return pcAnnotation;
}

void ViewProvider::update(const App::Property* prop)
{
    // Hide the object temporarily to speed up the update
    if (!isUpdatesEnabled()) {
        return;
    }
    bool vis = ViewProvider::isShow();
    if (vis) {
        ViewProvider::hide();
    }
    updateData(prop);
    if (vis) {
        ViewProvider::show();
    }
}

QIcon ViewProvider::getIcon() const
{
    return mergeGreyableOverlayIcons(Gui::BitmapFactory().pixmap(sPixmap));
}

QIcon ViewProvider::mergeGreyableOverlayIcons(const QIcon& orig) const
{
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();

    QIcon overlayedIcon = orig;

    for (Gui::ViewProviderExtension* ext : vector) {
        if (!ext->ignoreOverlayIcon()) {
            overlayedIcon = ext->extensionMergeGreyableOverlayIcons(overlayedIcon);
        }
    }

    return overlayedIcon;
}

QIcon ViewProvider::mergeColorfulOverlayIcons(const QIcon& orig) const
{
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();

    QIcon overlayedIcon = orig;

    for (Gui::ViewProviderExtension* ext : vector) {
        if (!ext->ignoreOverlayIcon()) {
            overlayedIcon = ext->extensionMergeColorfullOverlayIcons(overlayedIcon);
        }
    }

    return overlayedIcon;
}

void ViewProvider::setTransformation(const Base::Matrix4D& rcMatrix)
{
    pcTransform->setMatrix(convert(rcMatrix));
}

void ViewProvider::setTransformation(const SbMatrix& rcMatrix)
{
    pcTransform->setMatrix(rcMatrix);
}

SbMatrix ViewProvider::convert(const Base::Matrix4D& rcMatrix)
{
    return Base::convertTo<SbMatrix>(rcMatrix);
}

Base::Matrix4D ViewProvider::convert(const SbMatrix& smat)
{
    return Base::convertTo<Base::Matrix4D>(smat);
}

void ViewProvider::addDisplayMaskMode(SoNode* node, const char* type)
{
    _sDisplayMaskModes[type] = pcModeSwitch->getNumChildren();
    pcModeSwitch->addChild(node);
}

void ViewProvider::addDisplayMaskMode(std::shared_ptr<Render::RenderNode> node, const char* type)
{
    if (!node) {
        FC_WARN("ViewProvider::addDisplayMaskMode: null node for mode '" << type << "'");
        return;
    }

    // 存储抽象层节点 / Store abstraction layer node
    m_renderDisplayMaskModes[type] = node;

    // 同时添加到抽象层模式切换节点 / Also add to abstraction layer mode switch
    if (m_renderModeSwitch) {
        // 获取 Switch 节点并添加子节点
        // 由于 m_renderModeSwitch 可能是 Coin3DSwitch 或 OsgVerseSwitch，
        // 我们需要通过 RenderGroup 接口添加
        auto* group = dynamic_cast<Render::RenderGroup*>(m_renderModeSwitch.get());
        if (group) {
            group->addChild(node);
        }
    }

    // 同时添加到 Coin3D 模式切换节点（向后兼容）
    // Also add to Coin3D mode switch (backward compatibility)
    if (pcModeSwitch && node->getBackendType() == Render::BackendType::Coin3D) {
        auto* coinNode = node->getBackendNode<SoNode>();
        if (coinNode) {
            _sDisplayMaskModes[type] = pcModeSwitch->getNumChildren();
            pcModeSwitch->addChild(coinNode);
        }
    }
}

void ViewProvider::setDisplayMaskMode(const char* type)
{
    std::map<std::string, int>::const_iterator it = _sDisplayMaskModes.find(type);
    if (it != _sDisplayMaskModes.end()) {
        _iActualMode = it->second;
    }
    else {
        _iActualMode = -1;
    }
    setModeSwitch();
}

SoNode* ViewProvider::getDisplayMaskMode(const char* type) const
{
    std::map<std::string, int>::const_iterator it = _sDisplayMaskModes.find(type);
    if (it != _sDisplayMaskModes.end()) {
        return pcModeSwitch->getChild(it->second);
    }

    return nullptr;
}

std::shared_ptr<Render::RenderNode> ViewProvider::getRenderDisplayMaskMode(const char* type) const
{
    auto it = m_renderDisplayMaskModes.find(type);
    if (it != m_renderDisplayMaskModes.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> ViewProvider::getDisplayMaskModes() const
{
    std::vector<std::string> types;
    for (const auto& it : _sDisplayMaskModes) {
        types.push_back(it.first);
    }
    return types;
}

/**
 * If you add new viewing modes in @ref getDisplayModes() then you need to reimplement
 * also seDisplaytMode() to handle these new modes by setting the appropriate display
 * mode.
 */
void ViewProvider::setDisplayMode(const char* ModeName)
{
    _sCurrentMode = ModeName;

    // infom the exteensions
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        ext->extensionSetDisplayMode(ModeName);
    }
}

const char* ViewProvider::getDefaultDisplayMode() const
{

    return nullptr;
}

vector<std::string> ViewProvider::getDisplayModes() const
{

    std::vector<std::string> modes;
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        auto extModes = ext->extensionGetDisplayModes();
        modes.insert(modes.end(), extModes.begin(), extModes.end());
    }
    return modes;
}

std::string ViewProvider::getActiveDisplayMode() const
{
    return _sCurrentMode;
}

void ViewProvider::hide()
{
    auto exts = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();

    if (pcModeSwitch->whichChild.getValue() >= 0) {
        pcModeSwitch->whichChild = -1;
        for (auto ext : exts) {
            ext->extensionModeSwitchChange();
        }
    }

    // 同步到抽象层模式切换节点 / Sync to abstraction layer mode switch
    syncModeSwitchToRenderNode(-1);

    // tell extensions that we hide
    for (Gui::ViewProviderExtension* ext : exts) {
        ext->extensionHide();
    }
}

void ViewProvider::show()
{
    setModeSwitch();

    // tell extensions that we show
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        ext->extensionShow();
    }
}

bool ViewProvider::isShow() const
{
    return pcModeSwitch->whichChild.getValue() != -1;
}

void ViewProvider::setVisible(bool s)
{
    s ? show() : hide();
}

bool ViewProvider::isVisible() const
{
    return isShow();
}

void ViewProvider::setOverrideMode(const std::string& mode)
{
    if (mode == "As Is") {
        viewOverrideMode = -1;
        overrideMode = mode;
    }
    else {
        std::map<std::string, int>::const_iterator it = _sDisplayMaskModes.find(mode);
        if (it == _sDisplayMaskModes.end()) {
            return;  // view style not supported
        }
        viewOverrideMode = (*it).second;
        overrideMode = mode;
    }
    if (pcModeSwitch->whichChild.getValue() != -1) {
        setModeSwitch();
    }
    else {
        for (auto ext : getExtensionsDerivedFromType<Gui::ViewProviderExtension>()) {
            ext->extensionModeSwitchChange();
        }
    }
}

const string ViewProvider::getOverrideMode()
{
    return overrideMode;
}


void ViewProvider::setModeSwitch()
{
    int modeIndex = -1;

    if (viewOverrideMode == -1) {
        pcModeSwitch->whichChild = _iActualMode;
        modeIndex = _iActualMode;
    }
    else if (viewOverrideMode < pcModeSwitch->getNumChildren()) {
        pcModeSwitch->whichChild = viewOverrideMode;
        modeIndex = viewOverrideMode;
    }
    else {
        return;
    }

    // 同步到抽象层模式切换节点 / Sync to abstraction layer mode switch
    syncModeSwitchToRenderNode(modeIndex);

    for (auto ext : getExtensionsDerivedFromType<Gui::ViewProviderExtension>()) {
        ext->extensionModeSwitchChange();
    }
}

void ViewProvider::setDefaultMode(int val)
{
    _iActualMode = val;
    for (auto ext : getExtensionsDerivedFromType<Gui::ViewProviderExtension>()) {
        ext->extensionModeSwitchChange();
    }
}

int ViewProvider::getDefaultMode() const
{
    return viewOverrideMode >= 0 ? viewOverrideMode : _iActualMode;
}

void ViewProvider::onBeforeChange(const App::Property* prop)
{
    Application::Instance->signalBeforeChangeObject(*this, *prop);

    App::TransactionalObject::onBeforeChange(prop);
}

void ViewProvider::onChanged(const App::Property* prop)
{
    Application::Instance->signalChangedObject(*this, *prop);
    Application::Instance->updateActions();

    App::TransactionalObject::onChanged(prop);
}

std::string ViewProvider::toString() const
{
    return SoFCDB::writeNodesToString(pcRoot);
}

PyObject* ViewProvider::getPyObject()
{
    if (!pyViewObject) {
        pyViewObject = new ViewProviderPy(this);
    }
    pyViewObject->IncRef();
    return pyViewObject;
}


namespace Gui
{
using Graph = boost::adjacency_list<
    boost::vecS,         // class OutEdgeListS  : a Sequence or an AssociativeContainer
    boost::vecS,         // class VertexListS   : a Sequence or a RandomAccessContainer
    boost::directedS,    // class DirectedS     : This is a directed graph
    boost::no_property,  // class VertexProperty:
    boost::no_property,  // class EdgeProperty:
    boost::no_property,  // class GraphProperty:
    boost::listS         // class EdgeListS:
    >;
using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
using Edge = boost::graph_traits<Graph>::edge_descriptor;

void addNodes(Graph& graph, std::map<SoNode*, Vertex>& vertexNodeMap, SoNode* node)
{
    if (node->getTypeId().isDerivedFrom(SoGroup::getClassTypeId())) {
        auto group = static_cast<SoGroup*>(node);
        Vertex groupV = vertexNodeMap[group];

        for (int i = 0; i < group->getNumChildren(); i++) {
            SoNode* child = group->getChild(i);
            auto it = vertexNodeMap.find(child);

            // the child node is not yet added to the map
            if (it == vertexNodeMap.end()) {
                Vertex childV = add_vertex(graph);
                vertexNodeMap[child] = childV;
                add_edge(groupV, childV, graph);
                addNodes(graph, vertexNodeMap, child);
            }
            // the child is already there, only add the edge then
            else {
                add_edge(groupV, it->second, graph);
            }
        }
    }
}
}  // namespace Gui

bool ViewProvider::checkRecursion(SoNode* node)
{
    if (node->getTypeId().isDerivedFrom(SoGroup::getClassTypeId())) {
        std::list<Vertex> make_order;
        Graph graph;
        std::map<SoNode*, Vertex> vertexNodeMap;
        Vertex groupV = add_vertex(graph);
        vertexNodeMap[node] = groupV;
        addNodes(graph, vertexNodeMap, node);

        try {
            boost::topological_sort(graph, std::front_inserter(make_order));
        }
        catch (const std::exception&) {
            return false;
        }
    }

    return true;
}

SoPickedPoint* ViewProvider::getPointOnRay(const SbVec2s& pos, const View3DInventorViewer* viewer) const
{
    return viewer->getPointOnRay(pos, this);
}

SoPickedPoint* ViewProvider::getPointOnRay(
    const SbVec3f& pos,
    const SbVec3f& dir,
    const View3DInventorViewer* viewer
) const
{
    return viewer->getPointOnRay(pos, dir, this);
}

std::vector<Base::Vector3d> ViewProvider::getModelPoints(const SoPickedPoint* pp) const
{
    // the default implementation just returns the picked point from the visual representation
    std::vector<Base::Vector3d> pts;
    const SbVec3f& vec = pp->getPoint();
    pts.emplace_back(vec[0], vec[1], vec[2]);
    return pts;
}

bool ViewProvider::keyPressed(bool pressed, int key)
{
    (void)pressed;
    (void)key;
    return false;
}

bool ViewProvider::mouseMove(const SbVec2s& cursorPos, View3DInventorViewer* viewer)
{
    (void)cursorPos;
    (void)viewer;
    return false;
}

bool ViewProvider::mouseButtonPressed(
    int button,
    bool pressed,
    const SbVec2s& cursorPos,
    const View3DInventorViewer* viewer
)
{
    (void)button;
    (void)pressed;
    (void)cursorPos;
    (void)viewer;
    return false;
}

bool ViewProvider::mouseWheelEvent(int delta, const SbVec2s& cursorPos, const View3DInventorViewer* viewer)
{
    (void)delta;
    (void)cursorPos;
    (void)viewer;
    return false;
}

void ViewProvider::setupContextMenu(QMenu* menu, QObject* receiver, const char* method)
{
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        ext->extensionSetupContextMenu(menu, receiver, method);
    }
}

bool ViewProvider::onDelete(const vector<string>& subNames)
{
    bool del = true;
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        del &= ext->extensionOnDelete(subNames);
    }

    return del;
}

bool ViewProvider::canDelete(App::DocumentObject*) const
{
    return false;
}

bool ViewProvider::canDragObject(App::DocumentObject* obj) const
{
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        if (ext->extensionCanDragObject(obj)) {
            return true;
        }
    }

    return false;
}

bool ViewProvider::canDragObjectToTarget(
    App::DocumentObject* obj,
    [[maybe_unused]] App::DocumentObject* target
) const
{
    return canDragObject(obj);
}

bool ViewProvider::canDragObjects() const
{
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        if (ext->extensionCanDragObjects()) {
            return true;
        }
    }

    return false;
}

void ViewProvider::dragObject(App::DocumentObject* obj)
{
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        if (ext->extensionCanDragObject(obj)) {
            ext->extensionDragObject(obj);
            return;
        }
    }

    throw Base::RuntimeError(
        "ViewProvider::dragObject: no extension for dragging given object available."
    );
}

bool ViewProvider::canDropObject(App::DocumentObject* obj) const
{
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
#if FC_DEBUG
    Base::Console().log("Check extensions for drop\n");
#endif
    for (Gui::ViewProviderExtension* ext : vector) {
#if FC_DEBUG
        Base::Console().log("Check extensions %s\n", ext->name().c_str());
#endif
        if (ext->extensionCanDropObject(obj)) {
            return true;
        }
    }

    return false;
}

bool ViewProvider::canDropObjects() const
{

    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        if (ext->extensionCanDropObjects()) {
            return true;
        }
    }

    return false;
}

bool ViewProvider::canDragAndDropObject(App::DocumentObject* obj) const
{

    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        if (ext->extensionCanDragAndDropObject(obj)) {
            return true;
        }
    }

    return false;
}

void ViewProvider::dropObject(App::DocumentObject* obj)
{
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        if (ext->extensionCanDropObject(obj)) {
            ext->extensionDropObject(obj);
            break;
        }
    }
}

bool ViewProvider::canDropObjectEx(
    App::DocumentObject* obj,
    App::DocumentObject* owner,
    const char* subname,
    const std::vector<std::string>& elements
) const
{
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        if (ext->extensionCanDropObjectEx(obj, owner, subname, elements)) {
            return true;
        }
    }
    return canDropObject(obj);
}

std::string ViewProvider::dropObjectEx(
    App::DocumentObject* obj,
    App::DocumentObject* owner,
    const char* subname,
    const std::vector<std::string>& elements
)
{
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        if (ext->extensionCanDropObjectEx(obj, owner, subname, elements)) {
            return ext->extensionDropObjectEx(obj, owner, subname, elements);
        }
    }
    dropObject(obj);
    return {};
}

int ViewProvider::replaceObject(App::DocumentObject* oldValue, App::DocumentObject* newValue)
{
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        if (ext->extensionCanDropObject(newValue)) {
            int ret = ext->extensionReplaceObject(oldValue, newValue);
            if (ret >= 0) {
                return !!ret;
            }
        }
    }
    return -1;
}

void ViewProvider::Restore(Base::XMLReader& reader)
{
    // Because some PropertyLists type properties are stored in a separate file,
    // and is thus restored outside this function. So we rely on Gui::Document
    // to set the isRestoring flags for us.
    //
    // setStatus(Gui::isRestoring, true);

    TransactionalObject::Restore(reader);

    // setStatus(Gui::isRestoring, false);
}

void ViewProvider::updateData(const App::Property* prop)
{
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        ext->extensionUpdateData(prop);
    }
}

SoSeparator* ViewProvider::getBackRoot() const
{
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        auto* node = ext->extensionGetBackRoot();
        if (node) {
            return node;
        }
    }
    return nullptr;
}

SoGroup* ViewProvider::getChildRoot() const
{
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        auto* node = ext->extensionGetChildRoot();
        if (node) {
            return node;
        }
    }
    return nullptr;
}

//---------------------------------------------------------------------------
// 渲染抽象层集成 / Rendering Abstraction Layer Integration
//---------------------------------------------------------------------------

void ViewProvider::initRenderNodes()
{
    // TEMPORARILY DISABLED: Render abstraction layer initialization
    // This was causing crashes when loading documents
    // TODO: Re-enable once render abstraction is fully stable
#if 0
    // 检查是否已初始化（幂等性）/ Check if already initialized (idempotent)
    if (m_renderRoot) {
        return;
    }

    // 获取节点工厂 / Get node factory
    auto* factory = getNodeFactory();
    if (!factory) {
        // 如果没有工厂可用，使用 Coin3D 包装器作为后备
        // If no factory available, use Coin3D wrapper as fallback
        FC_WARN("ViewProvider::initRenderNodes: No factory available, using Coin3D wrapper");
        return;
    }

    // 使用工厂创建抽象层节点 / Create abstraction layer nodes using factory
    m_renderRoot = factory->createSeparator();
    m_renderTransform = factory->createTransform();
    m_renderModeSwitch = factory->createSwitch();

    if (m_renderRoot && m_renderTransform && m_renderModeSwitch) {
        // 构建节点层次 / Build node hierarchy
        // 需要转换为 RenderGroup 才能使用 addChild / Need to cast to RenderGroup for addChild
        auto* rootGroup = dynamic_cast<Render::RenderGroup*>(m_renderRoot.get());
        if (rootGroup) {
            rootGroup->addChild(m_renderTransform);
            rootGroup->addChild(m_renderModeSwitch);
        }

        m_renderRoot->setName("ViewProviderRoot");
        m_renderTransform->setName("Transform");
        m_renderModeSwitch->setName("ModeSwitch");

        FC_LOG("ViewProvider::initRenderNodes: Abstraction layer nodes initialized");
    }
    else {
        FC_WARN("ViewProvider::initRenderNodes: Failed to create some nodes");
    }
#endif
}

Render::RenderNode* ViewProvider::getRenderRoot() const
{
    // TEMPORARILY DISABLED: Render abstraction layer
    // This was causing crashes when loading documents
    // TODO: Re-enable once render abstraction is fully stable
#if 0
    // 优先返回抽象层节点 / Prefer returning abstraction layer node
    if (m_renderRoot) {
        return m_renderRoot.get();
    }

    // 后备：包装 Coin3D 节点 / Fallback: wrap Coin3D node
    if (!pcRoot) {
        return nullptr;
    }

    // Use static cache to avoid creating wrapper each time
    static std::unordered_map<SoSeparator*, std::shared_ptr<Gui::Render::Coin3DSeparator>> wrapperCache;

    auto it = wrapperCache.find(pcRoot);
    if (it != wrapperCache.end()) {
        return it->second.get();
    }

    // Create new wrapper (don't take ownership, pcRoot is managed by ViewProvider)
    auto wrapper = std::make_shared<Gui::Render::Coin3DSeparator>(pcRoot, false);
    wrapperCache[pcRoot] = wrapper;

    return wrapper.get();
#else
    // Always return nullptr - use Coin3D path only
    return nullptr;
#endif
}

std::shared_ptr<Render::RenderNode> ViewProvider::getRenderRootPtr() const
{
    return m_renderRoot;
}

std::shared_ptr<Render::RenderNode> ViewProvider::getRenderModeSwitch() const
{
    return m_renderModeSwitch;
}

std::shared_ptr<Render::RenderNode> ViewProvider::getRenderTransform() const
{
    return m_renderTransform;
}

Render::BackendType ViewProvider::getBackendType() const
{
    // 默认使用 Coin3D / Default to Coin3D
    return Render::BackendType::Coin3D;
}

Render::RenderNodeFactory* ViewProvider::getNodeFactory() const
{
    // 获取当前后端的工厂 / Get factory for current backend
    auto factory = Render::RenderNodeFactoryRegistry::instance().getFactory(getBackendType());
    return factory.get();
}

void ViewProvider::syncModeSwitchToRenderNode(int modeIndex)
{
    if (!m_renderModeSwitch) {
        return;
    }

    // 尝试作为 Coin3DSwitch / Try as Coin3DSwitch
    if (auto* coin3dSwitch = dynamic_cast<Render::Coin3DSwitch*>(m_renderModeSwitch.get())) {
        coin3dSwitch->setWhichChild(modeIndex);
        return;
    }

    // 尝试作为 OsgVerseSwitch / Try as OsgVerseSwitch
    // 注意：需要包含 OsgVerseNode.h，但为避免编译依赖，使用条件编译
#ifdef RENDER_HAS_OSGVERSE_BACKEND
    if (auto* osgSwitch = dynamic_cast<Render::OsgVerseSwitch*>(m_renderModeSwitch.get())) {
        osgSwitch->setWhichChild(modeIndex);
        return;
    }
#endif

    // 通用回退：设置可见性 / Generic fallback: set visibility
    m_renderModeSwitch->setVisible(modeIndex >= 0);
}

SoSeparator* ViewProvider::getFrontRoot() const
{
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        auto* node = ext->extensionGetFrontRoot();
        if (node) {
            return node;
        }
    }
    return nullptr;
}

std::shared_ptr<Render::RenderNode> ViewProvider::getRenderFrontRoot() const
{
    // 默认实现：尝试从扩展获取，或包装 Coin3D 节点
    // Default implementation: try to get from extensions, or wrap Coin3D node
    // TODO: 当扩展支持抽象层后，应优先调用扩展的抽象层方法
    // TODO: When extensions support abstraction layer, prefer calling extension's abstraction method

    SoSeparator* frontRoot = getFrontRoot();
    if (!frontRoot) {
        return nullptr;
    }

    // 创建 Coin3D 包装器 / Create Coin3D wrapper
    // 注意：这是临时方案，完整实现应在扩展中直接返回 RenderSeparator
    // Note: This is a temporary solution, full implementation should return RenderSeparator from extension
    return std::make_shared<Render::Coin3DSeparator>(frontRoot, false);
}

std::shared_ptr<Render::RenderNode> ViewProvider::getRenderChildRoot() const
{
    // 默认实现：尝试从扩展获取，或包装 Coin3D 节点
    // Default implementation: try to get from extensions, or wrap Coin3D node

    SoGroup* childRoot = getChildRoot();
    if (!childRoot) {
        return nullptr;
    }

    // 创建 Coin3D 包装器 / Create Coin3D wrapper
    return std::make_shared<Render::Coin3DGroup>(childRoot, false);
}

std::shared_ptr<Render::RenderNode> ViewProvider::getRenderBackRoot() const
{
    // 默认实现：尝试从扩展获取，或包装 Coin3D 节点
    // Default implementation: try to get from extensions, or wrap Coin3D node

    SoSeparator* backRoot = getBackRoot();
    if (!backRoot) {
        return nullptr;
    }

    // 创建 Coin3D 包装器 / Create Coin3D wrapper
    return std::make_shared<Render::Coin3DSeparator>(backRoot, false);
}

std::vector<App::DocumentObject*> ViewProvider::claimChildren() const
{
    std::vector<App::DocumentObject*> vec;
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        std::vector<App::DocumentObject*> nvec = ext->extensionClaimChildren();
        if (!nvec.empty()) {
            vec.insert(std::end(vec), std::begin(nvec), std::end(nvec));
        }
    }
    return vec;
}

std::vector<App::DocumentObject*> ViewProvider::claimChildrenRecursive() const
{
    std::vector<App::DocumentObject*> children = claimChildren();
    for (auto* child : claimChildren()) {
        auto* vp = Application::Instance->getViewProvider(child);
        if (!vp) {
            continue;
        }

        std::vector<App::DocumentObject*> nvec = vp->claimChildrenRecursive();
        if (!nvec.empty()) {
            children.insert(std::end(children), std::begin(nvec), std::end(nvec));
        }
    }
    return children;
}

std::vector<App::DocumentObject*> ViewProvider::claimChildren3D() const
{
    std::vector<App::DocumentObject*> vec;
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        std::vector<App::DocumentObject*> nvec = ext->extensionClaimChildren3D();
        if (!nvec.empty()) {
            vec.insert(std::end(vec), std::begin(nvec), std::end(nvec));
        }
    }
    return vec;
}

bool ViewProvider::getElementPicked(const SoPickedPoint* pp, std::string& subname) const
{
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        if (ext->extensionGetElementPicked(pp, subname)) {
            return true;
        }
    }
    subname = getElement(pp ? pp->getDetail() : nullptr);
    return true;
}

bool ViewProvider::getDetailPath(const char* subname, SoFullPath* pPath, bool append, SoDetail*& det) const
{
    if (pcRoot->findChild(pcModeSwitch) < 0) {
        // this is possible in case of editing, where the switch node
        // of the linked view object is temporarily removed from its root
        // if(append)
        //     pPath->append(pcRoot);
        return false;
    }
    if (append) {
        pPath->append(pcRoot);
        pPath->append(pcModeSwitch);
    }
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        if (ext->extensionGetDetailPath(subname, pPath, det)) {
            return true;
        }
    }
    det = getDetail(subname);
    return true;
}

const std::string& ViewProvider::hiddenMarker()
{
    return App::DocumentObject::hiddenMarker();
}

const char* ViewProvider::hasHiddenMarker(const char* subname)
{
    return App::DocumentObject::hasHiddenMarker(subname);
}

int ViewProvider::partialRender(const std::vector<std::string>& elements, bool clear)
{
    if (elements.empty()) {
        auto node = pcModeSwitch->getChild(_iActualMode);
        if (node) {
            FC_LOG("partial render clear");
            SoSelectionElementAction action(SoSelectionElementAction::None, true);
            action.apply(node);
        }
    }
    int count = 0;
    auto path = static_cast<SoFullPath*>(new SoPath);
    path->ref();
    SoSelectionElementAction action;
    action.setSecondary(true);
    for (auto element : elements) {
        bool hidden = hasHiddenMarker(element.c_str());
        if (hidden) {
            element.resize(element.size() - hiddenMarker().size());
        }
        path->truncate(0);
        SoDetail* det = nullptr;
        if (getDetailPath(element.c_str(), path, false, det)) {
            if (!hidden && !det) {
                FC_LOG("partial render element not found: " << element);
                continue;
            }
            FC_LOG("partial render (" << path->getLength() << "): " << element);
            if (!hidden) {
                action.setType(
                    clear ? SoSelectionElementAction::Remove : SoSelectionElementAction::Append
                );
            }
            else {
                action.setType(clear ? SoSelectionElementAction::Show : SoSelectionElementAction::Hide);
            }
            action.setElement(det);
            action.apply(path);
            ++count;
        }
        delete det;
    }
    path->unref();
    return count;
}

bool ViewProvider::useNewSelectionModel() const
{
    return ViewParams::instance()->getUseNewSelection();
}

void ViewProvider::beforeDelete()
{
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector) {
        ext->extensionBeforeDelete();
    }
}

void ViewProvider::setRenderCacheMode(int mode)
{
    pcRoot->renderCaching = mode == 0 ? SoSeparator::AUTO
                                      : (mode == 1 ? SoSeparator::ON : SoSeparator::OFF);
}

Base::BoundBox3d ViewProvider::getBoundingBox(const char* subname, bool transform, MDIView* view) const
{
    if (!pcRoot || !pcModeSwitch || pcRoot->findChild(pcModeSwitch) < 0) {
        return Base::BoundBox3d();
    }

    if (!view) {
        view = Application::Instance->activeView();
    }
    auto iview = qobject_cast<View3DInventor*>(view);
    if (!iview) {
        auto doc = Application::Instance->activeDocument();
        if (doc) {
            auto views = doc->getMDIViewsOfType(View3DInventor::getClassTypeId());
            if (!views.empty()) {
                iview = qobject_cast<View3DInventor*>(views.front());
            }
        }
        if (!iview) {
            FC_ERR("no view");
            return Base::BoundBox3d();
        }
    }

    View3DInventorViewer* viewer = iview->getViewer();
    SoGetBoundingBoxAction bboxAction(viewer->getSoRenderManager()->getViewportRegion());

    auto mode = pcModeSwitch->whichChild.getValue();
    if (mode < 0) {
        pcModeSwitch->whichChild = getDefaultMode();
    }

    SoTempPath path(20);
    path.ref();
    if (!Base::Tools::isNullOrEmpty(subname)) {
        SoDetail* det = nullptr;
        if (!getDetailPath(subname, &path, true, det)) {
            if (mode < 0) {
                pcModeSwitch->whichChild = mode;
            }
            path.unrefNoDelete();
            return Base::BoundBox3d();
        }
        delete det;
    }
    SoTempPath resetPath(3);
    resetPath.ref();
    if (!transform) {
        resetPath.append(pcRoot);
        resetPath.append(pcModeSwitch);
        bboxAction.setResetPath(&resetPath, true, SoGetBoundingBoxAction::TRANSFORM);
    }
    if (path.getLength()) {
        bboxAction.apply(&path);
    }
    else {
        bboxAction.apply(pcRoot);
    }
    if (mode < 0) {
        pcModeSwitch->whichChild = mode;
    }
    resetPath.unrefNoDelete();
    path.unrefNoDelete();
    auto bbox = bboxAction.getBoundingBox();
    float minX, minY, minZ, maxX, maxY, maxZ;
    bbox.getMax().getValue(maxX, maxY, maxZ);
    bbox.getMin().getValue(minX, minY, minZ);
    return Base::BoundBox3d(minX, minY, minZ, maxX, maxY, maxZ);
}

bool ViewProvider::isLinkVisible() const
{
    auto ext = getExtensionByType<ViewProviderLinkObserver>(true);
    if (!ext) {
        return true;
    }
    return ext->isLinkVisible();
}

void ViewProvider::setLinkVisible(bool visible)
{
    auto ext = getExtensionByType<ViewProviderLinkObserver>(true);
    if (ext) {
        ext->setLinkVisible(visible);
    }
}
