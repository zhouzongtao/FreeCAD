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

#ifdef RENDER_HAS_OSGVERSE_BACKEND

#include "PreCompiled.h"

#ifndef _PreComp_
# include <QOpenGLWidget>
#endif

#include "OsgVerseViewerAdapter.h"
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/Application.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Gui/Render/Backends/OsgVerse/OsgVersePickingService.h>
#include <Gui/Render/Backends/OsgVerse/OsgVerseSelection.h>

#include <QMouseEvent>
#include <QKeyEvent>

using namespace Gui::View3D::OsgVerse;
using namespace Gui::View3D;

OsgVerseViewerAdapter::OsgVerseViewerAdapter(QWidget* parent, const QOpenGLWidget* shareWidget)
    : SelectionObserver(true, Gui::ResolveMode::OldStyleElement)
    , _viewer(std::make_unique<Render::OsgVerseViewer>())
{
    Base::Console().log("OsgVerseViewerAdapter: Constructor called\n");
}

OsgVerseViewerAdapter::~OsgVerseViewerAdapter()
{
    Base::Console().log("OsgVerseViewerAdapter: Destructor called\n");
}

//-----------------------------------------------------------------------
// Basic rendering
//-----------------------------------------------------------------------

void OsgVerseViewerAdapter::render()
{
    if (_viewer) {
        _viewer->render();
    }
}

void OsgVerseViewerAdapter::resize(int width, int height)
{
    if (_viewer) {
        _viewer->resize(width, height);
    }
}

QWidget* OsgVerseViewerAdapter::getWidget()
{
    if (!_viewer) return nullptr;
    QWidget* w = _viewer->getWidget();
    if (w && !_callbacksSetup) {
        setupEventCallbacks();
        _callbacksSetup = true;
    }
    return w;
}

QOpenGLWidget* OsgVerseViewerAdapter::getGLWidget()
{
    if (_viewer) {
        return qobject_cast<QOpenGLWidget*>(_viewer->getWidget());
    }
    return nullptr;
}

//-----------------------------------------------------------------------
// Scene management
//-----------------------------------------------------------------------

void OsgVerseViewerAdapter::setSceneGraph(void* root)
{
    // TODO: Convert void* to RenderNode::Ptr and call _viewer->setSceneRoot()
    Base::Console().warning("OsgVerseViewerAdapter::setSceneGraph not yet implemented\n");
}

void* OsgVerseViewerAdapter::getSceneGraph()
{
    // TODO: Get scene root and convert to void*
    Base::Console().warning("OsgVerseViewerAdapter::getSceneGraph not yet implemented\n");
    return nullptr;
}

void OsgVerseViewerAdapter::updateScene()
{
    if (_viewer) {
        _viewer->updateScene();
    }
}

//-----------------------------------------------------------------------
// Camera control
//-----------------------------------------------------------------------

void OsgVerseViewerAdapter::setCamera(const CameraParams& params)
{
    if (!_viewer) return;

    Gui::Render::CameraParams rp;
    rp.position = Gui::Render::Vec3f(
        static_cast<float>(params.position.x),
        static_cast<float>(params.position.y),
        static_cast<float>(params.position.z));
    rp.target = Gui::Render::Vec3f(
        static_cast<float>(params.target.x),
        static_cast<float>(params.target.y),
        static_cast<float>(params.target.z));
    rp.upVector = Gui::Render::Vec3f(
        static_cast<float>(params.upVector.x),
        static_cast<float>(params.upVector.y),
        static_cast<float>(params.upVector.z));
    rp.fieldOfView = static_cast<float>(params.fieldOfView);
    rp.aspectRatio = static_cast<float>(params.aspectRatio);
    rp.nearPlane = static_cast<float>(params.nearPlane);
    rp.farPlane = static_cast<float>(params.farPlane);
    rp.orthographic = params.orthographic;
    rp.height = static_cast<float>(params.height);

    _viewer->setCamera(rp);
}

CameraParams OsgVerseViewerAdapter::getCamera() const
{
    if (!_viewer) return CameraParams();

    Gui::Render::CameraParams rp = _viewer->getCamera();
    CameraParams params;
    params.position = Base::Vector3d(rp.position.x, rp.position.y, rp.position.z);
    params.target = Base::Vector3d(rp.target.x, rp.target.y, rp.target.z);
    params.upVector = Base::Vector3d(rp.upVector.x, rp.upVector.y, rp.upVector.z);
    params.fieldOfView = rp.fieldOfView;
    params.aspectRatio = rp.aspectRatio;
    params.nearPlane = rp.nearPlane;
    params.farPlane = rp.farPlane;
    params.orthographic = rp.orthographic;
    params.height = rp.height;
    return params;
}

void OsgVerseViewerAdapter::viewAll()
{
    if (_viewer) {
        _viewer->fitAll();
    }
}

void OsgVerseViewerAdapter::fitSelection()
{
    if (_viewer) {
        _viewer->fitSelection();
    }
}

void OsgVerseViewerAdapter::resetCamera()
{
    if (_viewer) {
        _viewer->resetCamera();
    }
}

void OsgVerseViewerAdapter::setCameraType(bool orthographic)
{
    // TODO: Implement camera type switching
    Base::Console().warning("OsgVerseViewerAdapter::setCameraType not yet implemented\n");
}

bool OsgVerseViewerAdapter::isCameraOrthographic() const
{
    // TODO: Implement
    return false;
}

//-----------------------------------------------------------------------
// Event handling - Stub implementations
//-----------------------------------------------------------------------

bool OsgVerseViewerAdapter::handleMouseEvent(QMouseEvent* event)
{
    if (!_viewer || !event) return false;

    static int eventCount = 0;
    eventCount++;

    switch (event->type()) {
    case QEvent::MouseButtonPress:
        if (event->button() == Qt::LeftButton) {
            Base::Console().log("OsgVerseViewerAdapter: MousePress at (%d, %d)\n",
                               event->pos().x(), event->pos().y());
            _mousePressPos = event->pos();
            _mousePressed = true;
        }
        return false;  // Don't consume - let OSG handle camera

    case QEvent::MouseButtonRelease:
        if (event->button() == Qt::LeftButton && _mousePressed) {
            _mousePressed = false;

            // Check if this was a click (not a drag)
            QPoint delta = event->pos() - _mousePressPos;
            if (delta.manhattanLength() > 6) {
                return false;  // Was a drag, ignore
            }

            Base::Console().log("OsgVerseViewerAdapter: MouseRelease CLICK at (%d, %d)\n",
                               event->pos().x(), event->pos().y());

            // Perform pick for selection
            auto pickResult = pick(event->pos());

            Base::Console().log("OsgVerseViewerAdapter: Pick result: valid=%d, subElement=%s\n",
                               pickResult.valid,
                               pickResult.subElementName.c_str());

            bool ctrlPressed = event->modifiers() & Qt::ControlModifier;

            if (pickResult.valid && pickResult.viewProvider) {
                auto* vpDoc = dynamic_cast<Gui::ViewProviderDocumentObject*>(pickResult.viewProvider);
                if (vpDoc) {
                    auto* obj = vpDoc->getObject();
                    if (obj && obj->getDocument()) {
                        const char* docName = obj->getDocument()->getName();
                        const char* objName = obj->getNameInDocument();
                        const char* subName = pickResult.subElementName.c_str();

                        if (ctrlPressed) {
                            // Toggle selection
                            if (Gui::Selection().isSelected(obj, subName)) {
                                Gui::Selection().rmvSelection(docName, objName, subName);
                            } else {
                                Gui::Selection().addSelection(docName, objName, subName,
                                    pickResult.point.x, pickResult.point.y, pickResult.point.z);
                            }
                        } else {
                            // Replace selection
                            Gui::Selection().clearSelection();
                            Gui::Selection().addSelection(docName, objName, subName,
                                pickResult.point.x, pickResult.point.y, pickResult.point.z);
                        }
                    }
                }
            } else if (!ctrlPressed) {
                // Clicked empty space without Ctrl - clear selection
                Gui::Selection().clearSelection();
            }
        }
        return false;

    case QEvent::MouseMove:
    {
        // Only do preselection when no buttons are pressed (hover)
        if (event->buttons() != Qt::NoButton) {
            return false;
        }

        // Limit log spam
        static int moveCount = 0;
        if (++moveCount % 30 == 1) {
            Base::Console().log("OsgVerseViewerAdapter: MouseMove at (%d, %d)\n",
                               event->pos().x(), event->pos().y());
        }

        auto pickResult = pick(event->pos());

        if (pickResult.valid && pickResult.viewProvider) {
            auto* vpDoc = dynamic_cast<Gui::ViewProviderDocumentObject*>(pickResult.viewProvider);
            if (vpDoc) {
                auto* obj = vpDoc->getObject();
                if (obj && obj->getDocument()) {
                    Gui::Selection().setPreselect(
                        obj->getDocument()->getName(),
                        obj->getNameInDocument(),
                        pickResult.subElementName.c_str(),
                        static_cast<float>(pickResult.point.x),
                        static_cast<float>(pickResult.point.y),
                        static_cast<float>(pickResult.point.z));
                }
            }
        } else {
            Gui::Selection().rmvPreselect();
        }
        return false;  // Don't consume - let OSG handle camera
    }

    default:
        return false;
    }
}

bool OsgVerseViewerAdapter::handleKeyEvent(QKeyEvent* event)
{
    if (!event) return false;

    if (event->type() == QEvent::KeyPress && event->key() == Qt::Key_Escape) {
        if (_editingVP) {
            resetEditingViewProvider();
            return true;  // Consumed
        }
    }
    return false;
}

bool OsgVerseViewerAdapter::handleWheelEvent(QWheelEvent* event)
{
    // TODO: Implement wheel event handling
    return false;
}

//-----------------------------------------------------------------------
// Picking and selection - Stub implementations
//-----------------------------------------------------------------------

PickResult OsgVerseViewerAdapter::pick(const QPoint& pos)
{
    PickResult result;

    if (!_viewer) {
        Base::Console().warning("OsgVerseViewerAdapter::pick: No viewer\n");
        return result;
    }

    auto* pickingService = _viewer->getPickingService();
    if (!pickingService) {
        Base::Console().warning("OsgVerseViewerAdapter::pick: No picking service\n");
        return result;
    }

    Render::PickResults pickResults = pickingService->pick(pos.x(), pos.y());

    Base::Console().log("OsgVerseViewerAdapter::pick: pos=(%d,%d), hits=%d\n",
                       pos.x(), pos.y(), static_cast<int>(pickResults.hits.size()));

    if (!pickResults.hasHit()) return result;

    const Render::PickResult& hit = pickResults.closest();

    result.valid = hit.hit;
    result.point = Base::Vector3d(hit.point.x, hit.point.y, hit.point.z);
    result.normal = Base::Vector3d(hit.normal.x, hit.normal.y, hit.normal.z);
    result.distance = static_cast<double>(hit.distance);
    result.viewProvider = hit.viewProvider;
    result.subElementName = hit.elementName;
    result.primitiveIndex = hit.faceIndex >= 0 ? hit.faceIndex : (hit.edgeIndex >= 0 ? hit.edgeIndex : hit.vertexIndex);
    result.faceIndex = hit.faceIndex;
    result.edgeIndex = hit.edgeIndex;
    result.vertexIndex = hit.vertexIndex;

    switch (hit.type) {
    case Render::PickType::Face:   result.pickType = PickType::Face; break;
    case Render::PickType::Edge:   result.pickType = PickType::Edge; break;
    case Render::PickType::Vertex: result.pickType = PickType::Vertex; break;
    default:                       result.pickType = PickType::None; break;
    }

    return result;
}

void OsgVerseViewerAdapter::setSelectionMode(SelectionMode mode)
{
    // TODO: Implement
}

SelectionMode OsgVerseViewerAdapter::getSelectionMode() const
{
    // TODO: Implement
    return SelectionMode::None;
}

void OsgVerseViewerAdapter::startSelection(SelectionMode mode)
{
    // TODO: Implement
}

void OsgVerseViewerAdapter::stopSelection()
{
    // TODO: Implement
}

void OsgVerseViewerAdapter::abortSelection()
{
    // TODO: Implement
}

bool OsgVerseViewerAdapter::isSelecting() const
{
    // TODO: Implement
    return false;
}

//-----------------------------------------------------------------------
// ViewProvider management
//-----------------------------------------------------------------------

void OsgVerseViewerAdapter::addViewProvider(ViewProvider* vp)
{
    if (_viewer) {
        _viewer->addViewProvider(vp);
    }
}

void OsgVerseViewerAdapter::removeViewProvider(ViewProvider* vp)
{
    if (_viewer) {
        _viewer->removeViewProvider(vp);
    }
}

void OsgVerseViewerAdapter::updateViewProvider(ViewProvider* vp)
{
    if (_viewer) {
        _viewer->updateViewProvider(vp);
    }
}

bool OsgVerseViewerAdapter::hasViewProvider(ViewProvider* vp) const
{
    if (_viewer) {
        return _viewer->hasViewProvider(vp);
    }
    return false;
}

std::vector<Gui::ViewProvider*> OsgVerseViewerAdapter::getViewProviders() const
{
    if (_viewer) {
        return _viewer->getViewProviders();
    }
    return {};
}

void OsgVerseViewerAdapter::setEditingViewProvider(ViewProvider* vp, int mode)
{
    if (_editingVP) {
        resetEditingViewProvider();
    }
    _editingVP = vp;
    _editMode = mode;
    // Note: ViewProvider::setEditViewer() requires View3DInventorViewer*
    // For now we just store the state. TODO: Update VP API to support IViewer3D*
}

Gui::ViewProvider* OsgVerseViewerAdapter::getEditingViewProvider() const
{
    return _editingVP;
}

bool OsgVerseViewerAdapter::isEditingViewProvider() const
{
    return _editingVP != nullptr;
}

void OsgVerseViewerAdapter::resetEditingViewProvider()
{
    _editingVP = nullptr;
    _editMode = 0;
}

//-----------------------------------------------------------------------
// Rendering settings
//-----------------------------------------------------------------------

void OsgVerseViewerAdapter::setRenderMode(RenderMode mode)
{
    // TODO: Convert Gui::View3D::RenderMode to Gui::Render::RenderMode and call _viewer->setRenderMode()
    Base::Console().warning("OsgVerseViewerAdapter::setRenderMode not yet implemented\n");
}

RenderMode OsgVerseViewerAdapter::getRenderMode() const
{
    // TODO: Implement proper conversion from Gui::Render::RenderMode
    return RenderMode::AsIs;
}

void OsgVerseViewerAdapter::setBackgroundColor(const Base::Color& color)
{
    if (_viewer) {
        _viewer->setBackgroundColor(color);
    }
}

Base::Color OsgVerseViewerAdapter::getBackgroundColor() const
{
    if (_viewer) {
        return _viewer->getBackgroundColor();
    }
    return Base::Color();
}

void OsgVerseViewerAdapter::setBackgroundGradient(const BackgroundGradient& gradient)
{
    // TODO: Implement background gradient
    Base::Console().warning("OsgVerseViewerAdapter::setBackgroundGradient not yet implemented\n");
}

BackgroundGradient OsgVerseViewerAdapter::getBackgroundGradient() const
{
    // TODO: Implement
    return BackgroundGradient();
}

void OsgVerseViewerAdapter::setBacklightEnabled(bool enabled)
{
    if (_viewer) {
        _viewer->setBacklightEnabled(enabled);
    }
}

bool OsgVerseViewerAdapter::isBacklightEnabled() const
{
    if (_viewer) {
        return _viewer->isBacklightEnabled();
    }
    return false;
}

void OsgVerseViewerAdapter::setAmbientIntensity(float intensity)
{
    if (_viewer) {
        _viewer->setAmbientIntensity(intensity);
    }
}

float OsgVerseViewerAdapter::getAmbientIntensity() const
{
    if (_viewer) {
        return _viewer->getAmbientIntensity();
    }
    return 0.3f;
}

//-----------------------------------------------------------------------
// Navigation and interaction - Stub implementations
//-----------------------------------------------------------------------

void OsgVerseViewerAdapter::setNavigationStyle(const std::string& style)
{
    // TODO: Implement
    Base::Console().log("OsgVerseViewerAdapter::setNavigationStyle: %s\n", style.c_str());
}

std::string OsgVerseViewerAdapter::getNavigationStyle() const
{
    // TODO: Implement
    return "Trackball";
}

void OsgVerseViewerAdapter::setViewing(bool enable)
{
    // TODO: Implement
}

bool OsgVerseViewerAdapter::isViewing() const
{
    // TODO: Implement
    return true;
}

//-----------------------------------------------------------------------
// Backend info
//-----------------------------------------------------------------------

std::string OsgVerseViewerAdapter::getBackendVersion() const
{
    // OsgVerse backend version
    return "1.0";
}

//-----------------------------------------------------------------------
// Statistics and debugging
//-----------------------------------------------------------------------

Gui::Render::RenderStats OsgVerseViewerAdapter::getStats() const
{
    if (_viewer) {
        return _viewer->getStats();
    }
    return Gui::Render::RenderStats();
}

void OsgVerseViewerAdapter::resetStats()
{
    if (_viewer) {
        _viewer->resetStats();
    }
}

void OsgVerseViewerAdapter::setFPSEnabled(bool enabled)
{
    // TODO: Implement
    Base::Console().log("OsgVerseViewerAdapter::setFPSEnabled: %d\n", enabled);
}

bool OsgVerseViewerAdapter::isFPSEnabled() const
{
    // TODO: Implement
    return false;
}

//-----------------------------------------------------------------------
// Screenshots
//-----------------------------------------------------------------------

QImage OsgVerseViewerAdapter::grabImage(int width, int height)
{
    if (_viewer) {
        return _viewer->grabImage(width, height);
    }
    return QImage();
}

bool OsgVerseViewerAdapter::saveScreenshot(const QString& filename, int width, int height)
{
    if (_viewer) {
        return _viewer->saveScreenshot(filename, width, height);
    }
    return false;
}

//-----------------------------------------------------------------------
// SelectionObserver
//-----------------------------------------------------------------------

void OsgVerseViewerAdapter::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (!_viewer) return;

    static int callCount = 0;
    callCount++;
    Base::Console().log("OsgVerseViewerAdapter::onSelectionChanged: CALLED count=%d, Type=%d\n",
                       callCount, static_cast<int>(msg.Type));

    switch (msg.Type) {
    case Gui::SelectionChanges::SetPreselect:
    {
        Base::Console().log("OsgVerseViewerAdapter::onSelectionChanged: SetPreselect %s.%s\n",
                           msg.pDocName ? msg.pDocName : "?",
                           msg.pObjectName ? msg.pObjectName : "?");
        // Clear old preselection highlight
        if (_preselectedVP) {
            auto* selRoot = _viewer->getSelectionRootForVP(_preselectedVP);
            if (selRoot && selRoot->getSelectionState() == Render::OsgVerseSelectionRoot::SelectionState::Preselected) {
                selRoot->setSelectionState(Render::OsgVerseSelectionRoot::SelectionState::None);
            }
        }

        // Set new preselection
        auto* vp = resolveViewProvider(msg.pDocName, msg.pObjectName);
        if (vp) {
            auto* selRoot = _viewer->getSelectionRootForVP(vp);
            if (selRoot) {
                Base::Console().log("OsgVerseViewerAdapter: Setting preselection on VP\n");
                if (!selRoot->isSelected()) {
                    selRoot->setSelectionState(Render::OsgVerseSelectionRoot::SelectionState::Preselected);
                }
            } else {
                Base::Console().warning("OsgVerseViewerAdapter: selRoot is NULL for VP!\n");
            }
            _preselectedVP = vp;
        } else {
            Base::Console().warning("OsgVerseViewerAdapter: resolveViewProvider returned NULL\n");
        }
        break;
    }

    case Gui::SelectionChanges::RmvPreselect:
    {
        if (_preselectedVP) {
            auto* selRoot = _viewer->getSelectionRootForVP(_preselectedVP);
            if (selRoot && selRoot->isPreselected()) {
                selRoot->setSelectionState(Render::OsgVerseSelectionRoot::SelectionState::None);
            }
            _preselectedVP = nullptr;
        }
        break;
    }

    case Gui::SelectionChanges::AddSelection:
    {
        Base::Console().log("OsgVerseViewerAdapter::onSelectionChanged: AddSelection %s.%s\n",
                           msg.pDocName ? msg.pDocName : "?",
                           msg.pObjectName ? msg.pObjectName : "?");
        auto* vp = resolveViewProvider(msg.pDocName, msg.pObjectName);
        if (vp) {
            auto* selRoot = _viewer->getSelectionRootForVP(vp);
            if (selRoot) {
                Base::Console().log("OsgVerseViewerAdapter: Setting Selected state on selRoot\n");
                selRoot->setSelectionState(Render::OsgVerseSelectionRoot::SelectionState::Selected);
            } else {
                Base::Console().warning("OsgVerseViewerAdapter: selRoot is NULL for VP in AddSelection!\n");
            }
        }
        break;
    }

    case Gui::SelectionChanges::RmvSelection:
    {
        auto* vp = resolveViewProvider(msg.pDocName, msg.pObjectName);
        if (vp) {
            auto* selRoot = _viewer->getSelectionRootForVP(vp);
            if (selRoot) {
                // Check if VP is still in selection set
                auto* vpDoc = dynamic_cast<Gui::ViewProviderDocumentObject*>(vp);
                if (vpDoc && vpDoc->getObject()) {
                    if (!Gui::Selection().isSelected(vpDoc->getObject())) {
                        selRoot->setSelectionState(Render::OsgVerseSelectionRoot::SelectionState::None);
                    }
                } else {
                    selRoot->setSelectionState(Render::OsgVerseSelectionRoot::SelectionState::None);
                }
            }
        }
        break;
    }

    case Gui::SelectionChanges::ClrSelection:
    {
        // Clear all VP highlights
        auto viewProviders = _viewer->getViewProviders();
        for (auto* vp : viewProviders) {
            auto* selRoot = _viewer->getSelectionRootForVP(vp);
            if (selRoot && selRoot->isSelected()) {
                selRoot->setSelectionState(Render::OsgVerseSelectionRoot::SelectionState::None);
            }
        }
        break;
    }

    default:
        break;
    }
}

void OsgVerseViewerAdapter::setupEventCallbacks()
{
    if (!_viewer) return;

    Base::Console().log("OsgVerseViewerAdapter::setupEventCallbacks: Setting up callbacks\n");

    _viewer->setMouseEventCallback([this](QMouseEvent* e) {
        return this->handleMouseEvent(e);
    });
    _viewer->setKeyEventCallback([this](QKeyEvent* e) {
        return this->handleKeyEvent(e);
    });

    Base::Console().log("OsgVerseViewerAdapter::setupEventCallbacks: Callbacks set up\n");
}

Gui::ViewProvider* OsgVerseViewerAdapter::resolveViewProvider(const char* docName, const char* objName) const
{
    if (!docName || !objName) return nullptr;

    auto* doc = App::GetApplication().getDocument(docName);
    if (!doc) return nullptr;

    auto* obj = doc->getObject(objName);
    if (!obj) return nullptr;

    return Gui::Application::Instance->getViewProvider(obj);
}

#endif // RENDER_HAS_OSGVERSE_BACKEND
