/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                    *
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

#ifndef _PreComp_
# include <QVBoxLayout>
# include <QOpenGLWidget>
#endif

#include "View3DOsgVerse.h"
#include "View3D/ViewerFactory.h"
#include "Core/RenderManager.h"
#include "Document.h"
#include "Application.h"
#include "MainWindow.h"
#include "BitmapFactory.h"
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <osg/Quat>
#include <cmath>

using namespace Gui;

// Helper function to set camera from quaternion (matching NaviCube implementation)
static void setCameraParamsFromQuat(Gui::View3D::CameraParams& params, const osg::Quat& quat)
{
    osg::Vec3d eye(params.position.x, params.position.y, params.position.z);
    osg::Vec3d center(params.target.x, params.target.y, params.target.z);
    double distance = (eye - center).length();

    // Ensure minimum distance to avoid numerical issues
    if (distance < 0.1) {
        distance = 10.0;  // Default reasonable distance
    }

    // quat is world-to-camera, so we need inverse (camera-to-world) to transform camera vectors to world
    osg::Quat camToWorld = quat.inverse();

    // Default forward is -Z (0, 0, -1) in OpenGL convention
    // Transform from camera space to world space
    osg::Vec3d forward = camToWorld * osg::Vec3d(0, 0, -1);
    forward.normalize();

    osg::Vec3d newUp = camToWorld * osg::Vec3d(0, 1, 0);
    newUp.normalize();

    osg::Vec3d newEye = center - forward * distance;

    params.position.x = newEye.x();
    params.position.y = newEye.y();
    params.position.z = newEye.z();
    params.upVector.x = newUp.x();
    params.upVector.y = newUp.y();
    params.upVector.z = newUp.z();
}

TYPESYSTEM_SOURCE_ABSTRACT(Gui::View3DOsgVerse, Gui::View3DBase)

View3DOsgVerse::View3DOsgVerse(Gui::Document* pcDocument,
                               QWidget* parent,
                               const QOpenGLWidget* shareWidget,
                               Qt::WindowFlags wflags)
    : View3DBase(pcDocument, parent, wflags)
    , _viewer(nullptr)
{
    Base::Console().log("View3DOsgVerse: Constructor called\n");
    
    try {
        // Create OsgVerse viewer using ViewerFactory
        Base::Console().log("View3DOsgVerse: Creating OsgVerse viewer via ViewerFactory\n");
        
        _viewer = View3D::ViewerFactory::create(
            Render::BackendType::OsgVerse,
            this,
            shareWidget
        );
        
        if (!_viewer) {
            throw std::runtime_error("ViewerFactory returned null viewer");
        }
        
        Base::Console().log("View3DOsgVerse: Viewer created successfully\n");
        Base::Console().log("View3DOsgVerse: Backend: %s\n", 
                           _viewer->getBackendName().c_str());
        Base::Console().log("View3DOsgVerse: Version: %s\n",
                           _viewer->getBackendVersion().c_str());
        
        // Get the widget from viewer
        QWidget* viewerWidget = _viewer->getWidget();
        if (!viewerWidget) {
            throw std::runtime_error("Viewer did not provide a widget");
        }
        
        // Set as central widget (like View3DInventor does)
        setCentralWidget(viewerWidget);
        
        // Set window properties
        setWindowIcon(Gui::BitmapFactory().pixmap("Document"));
        setFocusPolicy(Qt::StrongFocus);
        setMouseTracking(true);
        setAcceptDrops(true);
        
        Base::Console().log("View3DOsgVerse: Constructor completed successfully\n");
    }
    catch (const std::exception& e) {
        Base::Console().error("View3DOsgVerse: Failed to create viewer: %s\n", e.what());
        throw;
    }
}

View3DOsgVerse::~View3DOsgVerse()
{
    Base::Console().log("View3DOsgVerse: Destructor called\n");
    _viewer.reset();
}

//===========================================================================
// View3DBase interface
//===========================================================================

View3D::IViewer3D* View3DOsgVerse::getViewerInterface()
{
    return _viewer.get();
}

View3DBase::BackendType View3DOsgVerse::getBackendType() const
{
    return BackendType::OsgVerse;
}

//===========================================================================
// MDIView interface
//===========================================================================

void View3DOsgVerse::onUpdate()
{
    if (_viewer) {
        _viewer->updateScene();
    }
}

const char* View3DOsgVerse::getName() const
{
    return "View3DOsgVerse";
}

bool View3DOsgVerse::onMsg(const char* pMsg, const char** ppReturn)
{
    if (!_viewer) {
        return false;
    }
    
    // Handle common view messages
    if (strcmp(pMsg, "ViewFit") == 0) {
        _viewer->viewAll();
        return true;
    }
    else if (strcmp(pMsg, "ViewSelection") == 0) {
        _viewer->viewAll();  // TODO: Implement view selection
        return true;
    }
    else if (strcmp(pMsg, "ViewHome") == 0) {
        _viewer->resetCamera();
        return true;
    }
    else if (strcmp(pMsg, "ViewBottom") == 0) {
        // Bottom view: looking up from below (rotate 180° around X)
        Gui::View3D::CameraParams params = _viewer->getCamera();
        osg::Quat rot(1, 0, 0, 0);  // {1, 0, 0, 0} = 180° around X
        setCameraParamsFromQuat(params, rot);
        _viewer->setCamera(params);
        _viewer->viewAll();
        return true;
    }
    else if (strcmp(pMsg, "ViewFront") == 0) {
        // Front view: looking from front (rotate 90° around X)
        Gui::View3D::CameraParams params = _viewer->getCamera();
        float root = std::sqrt(2.0f) / 2.0f;
        osg::Quat rot(root, 0, 0, root);  // {root, 0, 0, root} = 90° around X
        setCameraParamsFromQuat(params, rot);
        _viewer->setCamera(params);
        _viewer->viewAll();
        return true;
    }
    else if (strcmp(pMsg, "ViewLeft") == 0) {
        // Left view: looking from left
        Gui::View3D::CameraParams params = _viewer->getCamera();
        osg::Quat rot(-0.5, 0.5, 0.5, -0.5);
        setCameraParamsFromQuat(params, rot);
        _viewer->setCamera(params);
        _viewer->viewAll();
        return true;
    }
    else if (strcmp(pMsg, "ViewRear") == 0) {
        // Rear view: looking from behind
        Gui::View3D::CameraParams params = _viewer->getCamera();
        float root = std::sqrt(2.0f) / 2.0f;
        osg::Quat rot(0, root, root, 0);
        setCameraParamsFromQuat(params, rot);
        _viewer->setCamera(params);
        _viewer->viewAll();
        return true;
    }
    else if (strcmp(pMsg, "ViewRight") == 0) {
        // Right view: looking from right
        Gui::View3D::CameraParams params = _viewer->getCamera();
        osg::Quat rot(0.5, 0.5, 0.5, 0.5);
        setCameraParamsFromQuat(params, rot);
        _viewer->setCamera(params);
        _viewer->viewAll();
        return true;
    }
    else if (strcmp(pMsg, "ViewTop") == 0) {
        // Top view: looking down from above
        Gui::View3D::CameraParams params = _viewer->getCamera();
        osg::Quat rot(0, 0, 0, 1);  // Identity = no rotation
        setCameraParamsFromQuat(params, rot);
        _viewer->setCamera(params);
        _viewer->viewAll();
        return true;
    }
    else if (strcmp(pMsg, "ViewAxo") == 0) {
        // TODO: Implement axonometric view
        return true;
    }
    else if (strcmp(pMsg, "Orthographic") == 0) {
        _viewer->setCameraType(true);
        return true;
    }
    else if (strcmp(pMsg, "Perspective") == 0) {
        _viewer->setCameraType(false);
        return true;
    }
    else if (strcmp(pMsg, "DumpInfo") == 0) {
        dump();
        return true;
    }
    
    return false;
}

bool View3DOsgVerse::onHasMsg(const char* pMsg) const
{
    if (!_viewer) {
        return false;
    }
    
    // Check which messages are supported
    if (strcmp(pMsg, "ViewFit") == 0 ||
        strcmp(pMsg, "ViewSelection") == 0 ||
        strcmp(pMsg, "ViewHome") == 0 ||
        strcmp(pMsg, "ViewBottom") == 0 ||
        strcmp(pMsg, "ViewFront") == 0 ||
        strcmp(pMsg, "ViewLeft") == 0 ||
        strcmp(pMsg, "ViewRear") == 0 ||
        strcmp(pMsg, "ViewRight") == 0 ||
        strcmp(pMsg, "ViewTop") == 0 ||
        strcmp(pMsg, "ViewAxo") == 0 ||
        strcmp(pMsg, "Orthographic") == 0 ||
        strcmp(pMsg, "Perspective") == 0 ||
        strcmp(pMsg, "DumpInfo") == 0) {
        return true;
    }
    
    return false;
}

void View3DOsgVerse::print()
{
    // TODO: Implement printing
    Base::Console().warning("View3DOsgVerse::print() not yet implemented\n");
}

void View3DOsgVerse::printPdf()
{
    // TODO: Implement PDF printing
    Base::Console().warning("View3DOsgVerse::printPdf() not yet implemented\n");
}

void View3DOsgVerse::printPreview()
{
    // TODO: Implement print preview
    Base::Console().warning("View3DOsgVerse::printPreview() not yet implemented\n");
}

//===========================================================================
// View operations
//===========================================================================

void View3DOsgVerse::viewAll()
{
    if (_viewer) {
        _viewer->viewAll();
    }
}

void View3DOsgVerse::dump()
{
    if (!_viewer) {
        Base::Console().message("View3DOsgVerse: No viewer\n");
        return;
    }
    
    Base::Console().message("View3DOsgVerse Information:\n");
    Base::Console().message("  Backend: %s\n", _viewer->getBackendName().c_str());
    Base::Console().message("  Version: %s\n", _viewer->getBackendVersion().c_str());
    Base::Console().message("  Backend Type: %d\n", static_cast<int>(_viewer->getBackendType()));
    
    // Camera info
    auto camera = _viewer->getCamera();
    Base::Console().message("  Camera:\n");
    Base::Console().message("    Position: (%.2f, %.2f, %.2f)\n",
                           camera.position.x, camera.position.y, camera.position.z);
    Base::Console().message("    Target: (%.2f, %.2f, %.2f)\n",
                           camera.target.x, camera.target.y, camera.target.z);
    Base::Console().message("    Orthographic: %s\n",
                           camera.orthographic ? "Yes" : "No");
    
    // Render stats
    auto stats = _viewer->getStats();
    Base::Console().message("  Render Stats:\n");
    Base::Console().message("    FPS: %.1f\n", stats.fps);
    Base::Console().message("    Frame Time: %.2f ms\n", stats.frameTime);
    Base::Console().message("    Triangles: %ld\n", stats.triangleCount);
    Base::Console().message("    Vertices: %ld\n", stats.vertexCount);
    Base::Console().message("    Draw Calls: %ld\n", stats.drawCalls);
}

//===========================================================================
// Event handling
//===========================================================================

void View3DOsgVerse::resizeEvent(QResizeEvent* event)
{
    View3DBase::resizeEvent(event);
    
    if (_viewer) {
        _viewer->resize(event->size().width(), event->size().height());
    }
}


//===========================================================================
// Python interface
//===========================================================================

PyObject* View3DOsgVerse::getPyObject()
{
    // Use base class implementation which creates MDIViewPy
    // This provides basic view functionality through Python
    // TODO: Create View3DOsgVersePy for full API support
    return View3DBase::getPyObject();
}
