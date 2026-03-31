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

#ifdef RENDER_HAS_OSGVERSE_BACKEND

#include <array>
#include <QCursor>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>
#include <QWheelEvent>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GeoFeature.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/GeometryPyCXX.h>
#include <Base/Interpreter.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Base/RotationPy.h>
#include <Base/VectorPy.h>

#include "View3DOsgVersePy.h"
#include "View3DOsgVerse.h"
#include "Document.h"
#include "ViewProviderDocumentObject.h"
#include "Navigation/NavigationStyle.h"

using namespace Gui;

void View3DOsgVersePy::init_type()
{
    behaviors().name("View3DOsgVersePy");
    behaviors().doc("Python binding class for the OsgVerse viewer class");
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();

    add_varargs_method("fitAll", &View3DOsgVersePy::fitAll, "fitAll()");

    add_noargs_method("viewBottom", &View3DOsgVersePy::viewBottom, "viewBottom()");
    add_noargs_method("viewFront", &View3DOsgVersePy::viewFront, "viewFront()");
    add_noargs_method("viewLeft", &View3DOsgVersePy::viewLeft, "viewLeft()");
    add_noargs_method("viewRear", &View3DOsgVersePy::viewRear, "viewRear()");
    add_noargs_method("viewRight", &View3DOsgVersePy::viewRight, "viewRight()");
    add_noargs_method("viewTop", &View3DOsgVersePy::viewTop, "viewTop()");
    add_noargs_method("viewAxometric", &View3DOsgVersePy::viewIsometric, "viewAxonometric()");
    add_noargs_method("viewAxonometric", &View3DOsgVersePy::viewIsometric, "viewAxonometric()");
    add_noargs_method("viewIsometric", &View3DOsgVersePy::viewIsometric, "viewIsometric()");
    add_noargs_method("viewDimetric", &View3DOsgVersePy::viewDimetric, "viewDimetric()");
    add_noargs_method("viewTrimetric", &View3DOsgVersePy::viewTrimetric, "viewTrimetric()");
    add_varargs_method("viewDefaultOrientation", &View3DOsgVersePy::viewDefaultOrientation,
        "viewDefaultOrientation(ori_str='', scale=-1)\n"
        "Set the camera back to the default orientation\n");
    add_varargs_method("viewPosition", &View3DOsgVersePy::viewPosition,
        "viewPosition(pla, steps, ms)\n"
        "Move the camera to a Placement orientation\n");
    add_noargs_method("viewRotateLeft", &View3DOsgVersePy::viewRotateLeft, "viewRotateLeft()");
    add_noargs_method("viewRotateRight", &View3DOsgVersePy::viewRotateRight, "viewRotateRight()");
    add_noargs_method("zoomIn", &View3DOsgVersePy::zoomIn, "zoomIn()");
    add_noargs_method("zoomOut", &View3DOsgVersePy::zoomOut, "zoomOut()");

    add_noargs_method("getCamera", &View3DOsgVersePy::getCamera, "getCamera()");
    add_varargs_method("setCamera", &View3DOsgVersePy::setCamera, "setCamera()");
    add_noargs_method("getViewDirection", &View3DOsgVersePy::getViewDirection,
        "getViewDirection() --> tuple of floats\n"
        "returns the direction vector the view is currently pointing at as tuple with xyz values\n");
    add_noargs_method("getUpDirection", &View3DOsgVersePy::getUpDirection,
        "getUpDirection() --> tuple of floats\n"
        "Returns the up direction vector\n");
    add_varargs_method("setViewDirection", &View3DOsgVersePy::setViewDirection,
        "setViewDirection(tuple) --> None\n"
        "Sets the direction the view is pointing at. The direction must be given as tuple with\n"
        "three coordinates xyz");
    add_varargs_method("setCameraOrientation", &View3DOsgVersePy::setCameraOrientation,
        "setCameraOrientation()");
    add_noargs_method("getCameraOrientation", &View3DOsgVersePy::getCameraOrientation,
        "getCameraOrientation()");
    add_noargs_method("getCameraType", &View3DOsgVersePy::getCameraType, "getCameraType()");
    add_varargs_method("setCameraType", &View3DOsgVersePy::setCameraType, "setCameraType()");
    add_noargs_method("listCameraTypes", &View3DOsgVersePy::listCameraTypes, "listCameraTypes()");

    add_noargs_method("getCursorPos", &View3DOsgVersePy::getCursorPos,
        "getCursorPos() -> tuple of integers\n\n"
        "Return the current cursor position relative to the coordinate system of the\n"
        "viewport region.\n");
    add_varargs_method("getObjectInfo", &View3DOsgVersePy::getObjectInfo,
        "getObjectInfo(tuple(int,int), [pick_radius]) -> dictionary or None\n\n"
        "Return a dictionary with the name of document, object and component. The\n"
        "dictionary also contains the coordinates of the appropriate 3d point of\n"
        "the underlying geometry in the scenegraph.\n"
        "If no geometry was found 'None' is returned, instead.\n");
    add_varargs_method("getObjectsInfo", &View3DOsgVersePy::getObjectsInfo,
        "getObjectsInfo(tuple(int,int), [pick_radius]) -> list of dictionaries or None\n\n"
        "Return a list of dictionaries with the name of document, object and component\n"
        "for all objects at the given screen position.\n"
        "If no geometry was found 'None' is returned, instead.\n");
    add_noargs_method("getSize", &View3DOsgVersePy::getSize, "getSize()");
    add_varargs_method("getObjectInfoRay", &View3DOsgVersePy::getObjectInfoRay,
        "getObjectInfoRay(tuple(3D vector,3D vector) or tuple of 6 floats) -> dictionary or None\n\n"
        "Vectors represent start point and direction of intersection ray\n");
    add_varargs_method("getPoint", &View3DOsgVersePy::getPointOnFocalPlane,
        "Same as getPointOnFocalPlane");
    add_varargs_method("getPointOnFocalPlane", &View3DOsgVersePy::getPointOnFocalPlane,
        "getPointOnFocalPlane(pixel coords (as integer)) -> 3D vector\n\n"
        "Return the according 3D point on the focal plane to the given 2D point (in\n"
        "pixel coordinates).\n");
    add_varargs_method("getPointOnScreen", &View3DOsgVersePy::getPointOnViewport,
        "Same as getPointOnViewport");
    add_varargs_method("getPointOnViewport", &View3DOsgVersePy::getPointOnViewport,
        "getPointOnViewport(3D vector) -> pixel coords (as integer)\n\n"
        "Return the projected 3D point (in pixel coordinates).\n");
    add_varargs_method("projectPointToLine", &View3DOsgVersePy::projectPointToLine,
        "projectPointToLine(pixel coords (as integer)) -> line defined by two points\n\n"
        "Return the projecting 3D line to the given 2D point");

    add_noargs_method("listNavigationTypes", &View3DOsgVersePy::listNavigationTypes,
        "listNavigationTypes()");
    add_noargs_method("getNavigationType", &View3DOsgVersePy::getNavigationType,
        "getNavigationType()");
    add_varargs_method("setNavigationType", &View3DOsgVersePy::setNavigationType,
        "setNavigationType()");

    add_varargs_method("setAxisCross", &View3DOsgVersePy::setAxisCross,
        "switch the big axis-cross on and off");
    add_noargs_method("hasAxisCross", &View3DOsgVersePy::hasAxisCross,
        "check if the big axis-cross is on or off()");

    add_varargs_method("startAnimating", &View3DOsgVersePy::startAnimating,
        "startAnimating(axis, velocity)");
    add_noargs_method("stopAnimating", &View3DOsgVersePy::stopAnimating,
        "stopAnimating()");
    add_varargs_method("setAnimationEnabled", &View3DOsgVersePy::setAnimationEnabled,
        "setAnimationEnabled(bool)");
    add_noargs_method("isAnimationEnabled", &View3DOsgVersePy::isAnimationEnabled,
        "isAnimationEnabled()");

    add_varargs_method("setPopupMenuEnabled", &View3DOsgVersePy::setPopupMenuEnabled,
        "setPopupMenuEnabled(bool)");
    add_noargs_method("isPopupMenuEnabled", &View3DOsgVersePy::isPopupMenuEnabled,
        "isPopupMenuEnabled()");

    add_varargs_method("setAnnotation", &View3DOsgVersePy::setAnnotation,
        "setAnnotation(name, buffer)");
    add_varargs_method("removeAnnotation", &View3DOsgVersePy::removeAnnotation,
        "removeAnnotation(name)");

    add_varargs_method("setStereoType", &View3DOsgVersePy::setStereoType,
        "setStereoType(type)");
    add_noargs_method("getStereoType", &View3DOsgVersePy::getStereoType,
        "getStereoType()");
    add_noargs_method("listStereoTypes", &View3DOsgVersePy::listStereoTypes,
        "listStereoTypes()");

    add_varargs_method("saveVectorGraphic", &View3DOsgVersePy::saveVectorGraphic,
        "saveVectorGraphic(filename, type)");

    add_noargs_method("graphicsView", &View3DOsgVersePy::graphicsView,
        "graphicsView(): Access this view as QGraphicsView");

    add_varargs_method("setCornerCrossVisible", &View3DOsgVersePy::setCornerCrossVisible,
        "setCornerCrossVisible(bool): Defines corner axis cross visibility");
    add_noargs_method("isCornerCrossVisible", &View3DOsgVersePy::isCornerCrossVisible,
        "isCornerCrossVisible(): Returns current corner axis cross visibility");
    add_varargs_method("setCornerCrossSize", &View3DOsgVersePy::setCornerCrossSize,
        "setCornerCrossSize(int): Defines corner axis cross size");
    add_noargs_method("getCornerCrossSize", &View3DOsgVersePy::getCornerCrossSize,
        "getCornerCrossSize(): Returns current corner axis cross size");

    add_varargs_method("getViewProvidersOfType", &View3DOsgVersePy::getViewProvidersOfType,
        "getViewProvidersOfType(name)\nreturns a list of view providers for the given type");

    add_varargs_method("addEventCallback", &View3DOsgVersePy::addEventCallback,
        "addEventCallback(eventtype, callable) -> callable");
    add_varargs_method("removeEventCallback", &View3DOsgVersePy::removeEventCallback,
        "removeEventCallback(eventtype, callable)");
    add_varargs_method("addEventCallbackPivy", &View3DOsgVersePy::addEventCallbackPivy,
        "addEventCallbackPivy(eventtype, callable) -> callable");
    add_varargs_method("removeEventCallbackPivy", &View3DOsgVersePy::removeEventCallbackPivy,
        "removeEventCallbackPivy(eventtype, callable)");
    add_varargs_method("addEventCallbackSWIG", &View3DOsgVersePy::addEventCallbackPivy,
        "Deprecated -- use addEventCallbackPivy()");
    add_varargs_method("removeEventCallbackSWIG", &View3DOsgVersePy::removeEventCallbackPivy,
        "Deprecated -- use removeEventCallbackPivy()");

    add_keyword_method("boxZoom", &View3DOsgVersePy::boxZoom,
        "boxZoom(XMin, YMin, XMax, YMax)");

    add_varargs_method("saveImage", &View3DOsgVersePy::saveImage, "saveImage()");
    add_noargs_method("redraw", &View3DOsgVersePy::redraw,
        "redraw(): renders the scene on screen (useful for animations)");

    add_keyword_method("toggleClippingPlane", &View3DOsgVersePy::toggleClippingPlane,
        "toggleClippingPlane(toggle=-1)\n"
        "Toggle a global clipping plane\n\n"
        "toggle: -1 toggle, 1 show, 0 hide\n");
    add_noargs_method("hasClippingPlane", &View3DOsgVersePy::hasClippingPlane,
        "hasClippingPlane(): check whether this clipping plane is active");

    add_varargs_method("setName", &View3DOsgVersePy::setName,
        "setName(str): sets the name of the view");

    add_noargs_method("cast_to_base", &View3DOsgVersePy::cast_to_base,
        "cast_to_base() cast to MDIView class");
}

View3DOsgVersePy::View3DOsgVersePy(View3DOsgVerse* vi)
    : base(vi)
{}

View3DOsgVersePy::~View3DOsgVersePy()
{
    for (auto* cb : _callbacks) {
        Py_DECREF(cb);
    }
    _callbacks.clear();
    _callbackWrappers.clear();
}

View3DOsgVerse* View3DOsgVersePy::getView3DOsgVersePtr()
{
    return qobject_cast<View3DOsgVerse*>(base.getMDIViewPtr());
}

Py::Object View3DOsgVersePy::repr()
{
    if (!getView3DOsgVersePtr()) {
        throw Py::RuntimeError("Cannot print representation of deleted object");
    }
    return Py::String("View3DOsgVerse");
}

View3DOsgVersePy::method_varargs_handler View3DOsgVersePy::pycxx_handler = nullptr;

PyObject* View3DOsgVersePy::method_varargs_ext_handler(PyObject* _self_and_name_tuple, PyObject* _args)
{
    try {
        return pycxx_handler(_self_and_name_tuple, _args);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const Py::Exception&) {
        throw;
    }
    catch (...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }
}

Py::Object View3DOsgVersePy::getattribute(const char* attr)
{
    if (!getView3DOsgVersePtr()) {
        throw Py::RuntimeError("Cannot print representation of deleted object");
    }
    std::string name(attr);
    if (name == "__dict__" || name == "__class__") {
        Py::Dict dict_self(BaseType::getattr("__dict__"));
        Py::Dict dict_base(base.getattr("__dict__"));
        for (const auto& it : dict_base) {
            dict_self.setItem(it.first, it.second);
        }
        return dict_self;
    }

    try {
        return BaseType::getattr(attr);
    }
    catch (Py::AttributeError& e) {
        e.clear();
        return base.getattr(attr);
    }
}

Py::Object View3DOsgVersePy::getattr(const char* attr)
{
    if (!getView3DOsgVersePtr()) {
        std::ostringstream s_out;
        s_out << "Cannot access attribute '" << attr << "' of deleted object";
        throw Py::RuntimeError(s_out.str());
    }
    else {
        App::DocumentObject* docObj = getView3DOsgVersePtr()->getActiveObject<App::DocumentObject*>(attr);
        if (docObj) {
            return Py::Object(docObj->getPyObject(), true);
        }
        else {
            Py::Object obj = getattribute(attr);
            if (PyCFunction_Check(obj.ptr())) {
                auto op = reinterpret_cast<PyCFunctionObject*>(obj.ptr());
                if (op->m_ml->ml_flags == METH_VARARGS) {
                    if (!pycxx_handler) {
                        pycxx_handler = op->m_ml->ml_meth;
                    }
                    op->m_ml->ml_meth = method_varargs_ext_handler;
                }
            }
            return obj;
        }
    }
}

int View3DOsgVersePy::setattr(const char* attr, const Py::Object& value)
{
    if (!getView3DOsgVersePtr()) {
        std::ostringstream s_out;
        s_out << "Cannot access attribute '" << attr << "' of deleted object";
        throw Py::RuntimeError(s_out.str());
    }
    else {
        return BaseType::setattr(attr, value);
    }
}

Py::Object View3DOsgVersePy::cast_to_base()
{
    return Gui::MDIViewPy::create(getView3DOsgVersePtr());
}

// Camera type enum strings
static const char* OsgVerseCameraTypeEnums[] = {"Orthographic", "Perspective", nullptr};

Py::Object View3DOsgVersePy::getCamera()
{
    try {
        std::string camStr = getView3DOsgVersePtr()->getCameraString();
        return Py::String(camStr);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

Py::Object View3DOsgVersePy::setCamera(const Py::Tuple& args)
{
    char* buffer;
    if (!PyArg_ParseTuple(args.ptr(), "s", &buffer)) {
        throw Py::Exception();
    }
    try {
        // Build "SetCamera <string>" message
        std::string msg = std::string("SetCamera ") + buffer;
        getView3DOsgVersePtr()->onMsg(msg.c_str());
        return Py::None();
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

Py::Object View3DOsgVersePy::getViewDirection()
{
    try {
        auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
        if (!viewer) {
            throw Py::RuntimeError("No viewer available");
        }
        Base::Vector3d dir = viewer->getViewDirection();
        return Py::Vector(Base::Vector3f(dir.x, dir.y, dir.z));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

Py::Object View3DOsgVersePy::getUpDirection()
{
    try {
        auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
        if (!viewer) {
            throw Py::RuntimeError("No viewer available");
        }
        Base::Vector3d dir = viewer->getUpDirection();
        return Py::Vector(Base::Vector3f(dir.x, dir.y, dir.z));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

Py::Object View3DOsgVersePy::setViewDirection(const Py::Tuple& args)
{
    PyObject* object;
    if (!PyArg_ParseTuple(args.ptr(), "O", &object)) {
        throw Py::Exception();
    }
    try {
        if (PyTuple_Check(object)) {
            Py::Tuple tuple(object);
            float x = (float)Py::Float(tuple.getItem(0));
            float y = (float)Py::Float(tuple.getItem(1));
            float z = (float)Py::Float(tuple.getItem(2));
            Base::Vector3d dir(x, y, z);
            if (dir.Length() < 0.001) {
                throw Py::ValueError("Null vector cannot be used to set direction");
            }
            auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
            if (!viewer) {
                throw Py::RuntimeError("No viewer available");
            }
            // Set camera looking in the given direction
            auto cam = viewer->getCamera();
            dir.Normalize();
            double dist = (cam.target - cam.position).Length();
            if (dist < 0.1) dist = 10.0;
            cam.target = cam.position + dir * dist;
            // Compute a reasonable up vector perpendicular to direction
            Base::Vector3d up(0, 0, 1);
            if (std::abs(dir.z) > 0.999) {
                up = Base::Vector3d(0, 1, 0);
            }
            Base::Vector3d right = dir % up;
            right.Normalize();
            up = right % dir;
            up.Normalize();
            cam.upVector = up;
            viewer->setCamera(cam);
        }
    }
    catch (const Py::Exception&) {
        throw;
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    return Py::None();
}

Py::Object View3DOsgVersePy::setCameraOrientation(const Py::Tuple& args)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args.ptr(), "O", &o)) {
        throw Py::Exception();
    }
    try {
        double q0, q1, q2, q3;
        if (PyTuple_Check(o)) {
            Py::Tuple tuple(o);
            q0 = (double)Py::Float(tuple[0]);
            q1 = (double)Py::Float(tuple[1]);
            q2 = (double)Py::Float(tuple[2]);
            q3 = (double)Py::Float(tuple[3]);
        }
        else if (PyObject_TypeCheck(o, &Base::RotationPy::Type)) {
            Base::Rotation r = static_cast<Base::Rotation>(Py::Rotation(o, false));
            r.getValue(q0, q1, q2, q3);
        }
        else {
            throw Py::ValueError("Neither tuple nor rotation object");
        }
        // Convert quaternion to camera params
        // The quaternion represents camera orientation: forward = q * (0,0,-1), up = q * (0,1,0)
        Base::Rotation rot(q0, q1, q2, q3);
        Base::Vector3d forward(0, 0, -1);
        Base::Vector3d up(0, 1, 0);
        rot.multVec(forward, forward);
        rot.multVec(up, up);

        auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
        if (!viewer) {
            throw Py::RuntimeError("No viewer available");
        }
        auto cam = viewer->getCamera();
        double dist = (cam.target - cam.position).Length();
        if (dist < 0.1) dist = 10.0;
        cam.target = cam.position + forward * dist;
        cam.upVector = up;
        viewer->setCamera(cam);
    }
    catch (const Py::Exception&) {
        throw;
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    return Py::None();
}

Py::Object View3DOsgVersePy::getCameraOrientation()
{
    auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
    if (!viewer) {
        throw Py::RuntimeError("No viewer available");
    }
    auto cam = viewer->getCamera();
    // Compute rotation from default camera orientation to current
    Base::Vector3d forward = cam.target - cam.position;
    forward.Normalize();
    Base::Vector3d up = cam.upVector;
    up.Normalize();
    Base::Vector3d right = forward % up;
    right.Normalize();
    up = right % forward;
    up.Normalize();
    // Build rotation matrix (maps default axes to camera axes)
    // Default: forward=(0,0,-1), up=(0,1,0), right=(1,0,0)
    // Camera: forward, up, right
    // The rotation R satisfies: R*(0,0,-1)=forward, R*(0,1,0)=up, R*(1,0,0)=right
    // So the rotation matrix columns are: right, up, -forward
    Base::Matrix4D mat;
    mat[0][0] = right.x;  mat[0][1] = up.x;  mat[0][2] = -forward.x;
    mat[1][0] = right.y;  mat[1][1] = up.y;  mat[1][2] = -forward.y;
    mat[2][0] = right.z;  mat[2][1] = up.z;  mat[2][2] = -forward.z;
    Base::Rotation rot(mat);
    double rq0, rq1, rq2, rq3;
    rot.getValue(rq0, rq1, rq2, rq3);
    return Py::Rotation(Base::Rotation(rq0, rq1, rq2, rq3));
}

Py::Object View3DOsgVersePy::getCameraType()
{
    auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
    if (!viewer) {
        throw Py::RuntimeError("No viewer available");
    }
    if (viewer->isCameraOrthographic()) {
        return Py::String(OsgVerseCameraTypeEnums[0]);
    }
    return Py::String(OsgVerseCameraTypeEnums[1]);
}

Py::Object View3DOsgVersePy::setCameraType(const Py::Tuple& args)
{
    int cameratype = -1;
    if (!PyArg_ParseTuple(args.ptr(), "i", &cameratype)) {
        char* modename;
        PyErr_Clear();
        if (!PyArg_ParseTuple(args.ptr(), "s", &modename)) {
            throw Py::Exception();
        }
        for (int i = 0; i < 2; i++) {
            if (strncmp(OsgVerseCameraTypeEnums[i], modename, 20) == 0) {
                cameratype = i;
                break;
            }
        }
        if (cameratype < 0) {
            std::ostringstream s_out;
            s_out << "Unknown camera type '" << modename << "'";
            throw Py::NameError(s_out.str());
        }
    }
    if (cameratype < 0 || cameratype > 1) {
        throw Py::IndexError("Out of range");
    }
    auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
    if (!viewer) {
        throw Py::RuntimeError("No viewer available");
    }
    viewer->setCameraType(cameratype == 0);  // 0 = Orthographic
    return Py::None();
}

Py::Object View3DOsgVersePy::listCameraTypes()
{
    Py::List list(2);
    for (int i = 0; i < 2; i++) {
        list[i] = Py::String(OsgVerseCameraTypeEnums[i]);
    }
    return list;
}

Py::Object View3DOsgVersePy::fitAll(const Py::Tuple& args)
{
    // Accept optional factor argument for compatibility, but ignore it
    double factor = 1.0;
    if (!PyArg_ParseTuple(args.ptr(), "|d", &factor)) {
        throw Py::Exception();
    }
    try {
        getView3DOsgVersePtr()->onMsg("ViewFit");
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    return Py::None();
}

Py::Object View3DOsgVersePy::viewBottom()
{
    try { getView3DOsgVersePtr()->onMsg("ViewBottom"); }
    catch (const Base::Exception& e) { throw Py::RuntimeError(e.what()); }
    return Py::None();
}

Py::Object View3DOsgVersePy::viewFront()
{
    try { getView3DOsgVersePtr()->onMsg("ViewFront"); }
    catch (const Base::Exception& e) { throw Py::RuntimeError(e.what()); }
    return Py::None();
}

Py::Object View3DOsgVersePy::viewLeft()
{
    try { getView3DOsgVersePtr()->onMsg("ViewLeft"); }
    catch (const Base::Exception& e) { throw Py::RuntimeError(e.what()); }
    return Py::None();
}

Py::Object View3DOsgVersePy::viewRear()
{
    try { getView3DOsgVersePtr()->onMsg("ViewRear"); }
    catch (const Base::Exception& e) { throw Py::RuntimeError(e.what()); }
    return Py::None();
}

Py::Object View3DOsgVersePy::viewRight()
{
    try { getView3DOsgVersePtr()->onMsg("ViewRight"); }
    catch (const Base::Exception& e) { throw Py::RuntimeError(e.what()); }
    return Py::None();
}

Py::Object View3DOsgVersePy::viewTop()
{
    try { getView3DOsgVersePtr()->onMsg("ViewTop"); }
    catch (const Base::Exception& e) { throw Py::RuntimeError(e.what()); }
    return Py::None();
}

Py::Object View3DOsgVersePy::viewIsometric()
{
    try { getView3DOsgVersePtr()->onMsg("ViewAxo"); }
    catch (const Base::Exception& e) { throw Py::RuntimeError(e.what()); }
    return Py::None();
}

Py::Object View3DOsgVersePy::zoomIn()
{
    try { getView3DOsgVersePtr()->onMsg("ZoomIn"); }
    catch (const Base::Exception& e) { throw Py::RuntimeError(e.what()); }
    return Py::None();
}

Py::Object View3DOsgVersePy::zoomOut()
{
    try { getView3DOsgVersePtr()->onMsg("ZoomOut"); }
    catch (const Base::Exception& e) { throw Py::RuntimeError(e.what()); }
    return Py::None();
}

Py::Object View3DOsgVersePy::getPointOnFocalPlane(const Py::Tuple& args)
{
    short x, y;
    if (!PyArg_ParseTuple(args.ptr(), "hh", &x, &y)) {
        PyErr_Clear();
        Py::Tuple t(args[0]);
        x = (int)Py::Long(t[0]);
        y = (int)Py::Long(t[1]);
    }
    try {
        auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
        if (!viewer) {
            throw Py::RuntimeError("No viewer available");
        }
        Base::Vector3d pt = viewer->getPointOnFocalPlane(x, y);
        return Py::Vector(Base::Vector3f(pt.x, pt.y, pt.z));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DOsgVersePy::projectPointToLine(const Py::Tuple& args)
{
    short x, y;
    if (!PyArg_ParseTuple(args.ptr(), "hh", &x, &y)) {
        PyErr_Clear();
        Py::Tuple t(args[0]);
        x = (int)Py::Long(t[0]);
        y = (int)Py::Long(t[1]);
    }
    try {
        auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
        if (!viewer) {
            throw Py::RuntimeError("No viewer available");
        }
        Base::Vector3d pt1, pt2;
        viewer->projectPointToLine(QPoint(x, y), pt1, pt2);
        Py::Tuple tuple(2);
        tuple.setItem(0, Py::Vector(Base::Vector3f(pt1.x, pt1.y, pt1.z)));
        tuple.setItem(1, Py::Vector(Base::Vector3f(pt2.x, pt2.y, pt2.z)));
        return tuple;
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DOsgVersePy::getPointOnViewport(const Py::Tuple& args)
{
    PyObject* v;
    double vx, vy, vz;
    if (PyArg_ParseTuple(args.ptr(), "O!", &Base::VectorPy::Type, &v)) {
        Base::Vector3d* vec = static_cast<Base::VectorPy*>(v)->getVectorPtr();
        vx = vec->x;
        vy = vec->y;
        vz = vec->z;
    }
    else {
        PyErr_Clear();
        if (!PyArg_ParseTuple(args.ptr(), "ddd", &vx, &vy, &vz)) {
            throw Py::TypeError("Wrong argument, Vector or three floats expected");
        }
    }
    try {
        auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
        if (!viewer) {
            throw Py::RuntimeError("No viewer available");
        }
        QPoint pt = viewer->getPointOnViewport(Base::Vector3d(vx, vy, vz));
        Py::Tuple tuple(2);
        tuple.setItem(0, Py::Long(pt.x()));
        tuple.setItem(1, Py::Long(pt.y()));
        return tuple;
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DOsgVersePy::getObjectInfo(const Py::Tuple& args)
{
    PyObject* object;
    float r = 5.0f;
    if (!PyArg_ParseTuple(args.ptr(), "O|f", &object, &r)) {
        throw Py::Exception();
    }
    try {
        const Py::Tuple tuple(object);
        Py::Long x(tuple[0]);
        Py::Long y(tuple[1]);

        auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
        if (!viewer) {
            return Py::None();
        }

        float oldRadius = viewer->getPickRadius();
        viewer->setPickRadius(r);
        View3D::PickResult result = viewer->pick(QPoint((long)x, (long)y));
        viewer->setPickRadius(oldRadius);

        if (!result.valid) {
            return Py::None();
        }

        Py::Dict dict;
        dict.setItem("x", Py::Float(result.point.x));
        dict.setItem("y", Py::Float(result.point.y));
        dict.setItem("z", Py::Float(result.point.z));

        if (result.viewProvider && result.viewProvider->isDerivedFrom<ViewProviderDocumentObject>()) {
            if (!result.viewProvider->isSelectable()) {
                return Py::None();
            }
            auto vpd = static_cast<ViewProviderDocumentObject*>(result.viewProvider);
            auto obj = vpd->getObject();
            if (obj) {
                dict.setItem("Document", Py::String(obj->getDocument()->getName()));
                dict.setItem("Object", Py::String(obj->getNameInDocument()));
                dict.setItem("Component", Py::String(result.subElementName));
            }
        }
        return dict;
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DOsgVersePy::getSize()
{
    try {
        auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
        if (!viewer) {
            throw Py::RuntimeError("No viewer available");
        }
        QWidget* w = viewer->getWidget();
        Py::Tuple tuple(2);
        if (w) {
            tuple.setItem(0, Py::Long(w->width()));
            tuple.setItem(1, Py::Long(w->height()));
        }
        else {
            tuple.setItem(0, Py::Long(0));
            tuple.setItem(1, Py::Long(0));
        }
        return tuple;
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DOsgVersePy::getCursorPos()
{
    try {
        QPoint pos = getView3DOsgVersePtr()->mapFromGlobal(QCursor::pos());
        Py::Tuple tuple(2);
        tuple.setItem(0, Py::Long(pos.x()));
        tuple.setItem(1, Py::Long(pos.y()));
        return tuple;
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DOsgVersePy::listNavigationTypes()
{
    std::vector<Base::Type> types;
    Py::List styles;
    Base::Type::getAllDerivedFrom(UserNavigationStyle::getClassTypeId(), types);
    for (auto it = types.begin() + 1; it != types.end(); ++it) {
        styles.append(Py::String(it->getName()));
    }
    return styles;
}

Py::Object View3DOsgVersePy::getNavigationType()
{
    auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
    if (!viewer) {
        return Py::String("Gui::CADNavigationStyle");
    }
    return Py::String(viewer->getNavigationStyle());
}

Py::Object View3DOsgVersePy::setNavigationType(const Py::Tuple& args)
{
    char* style;
    if (!PyArg_ParseTuple(args.ptr(), "s", &style)) {
        throw Py::Exception();
    }
    auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
    if (viewer) {
        viewer->setNavigationStyle(style);
    }
    return Py::None();
}

Py::Object View3DOsgVersePy::saveImage(const Py::Tuple& args)
{
    char* cFileName = nullptr;
    int w = -1, h = -1;
    if (!PyArg_ParseTuple(args.ptr(), "et|ii", "utf-8", &cFileName, &w, &h)) {
        throw Py::Exception();
    }
    std::string encodedName = std::string(cFileName);
    PyMem_Free(cFileName);
    QFileInfo fi(QString::fromUtf8(encodedName.c_str()));
    if (!fi.absoluteDir().exists()) {
        throw Py::RuntimeError("Directory where to save image doesn't exist");
    }
    try {
        auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
        if (!viewer) {
            throw Py::RuntimeError("No viewer available");
        }
        viewer->saveScreenshot(QString::fromStdString(encodedName), w, h);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    return Py::None();
}

Py::Object View3DOsgVersePy::redraw()
{
    auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
    if (viewer) {
        viewer->updateScene();
    }
    return Py::None();
}

Py::Object View3DOsgVersePy::setAxisCross(const Py::Tuple& args)
{
    int ok;
    if (!PyArg_ParseTuple(args.ptr(), "i", &ok)) {
        throw Py::Exception();
    }
    // OsgVerse doesn't have a separate axis cross toggle yet, but accept the call
    // to maintain API compatibility
    return Py::None();
}

Py::Object View3DOsgVersePy::hasAxisCross()
{
    // OsgVerse doesn't have a separate axis cross toggle yet
    return Py::Boolean(false);
}

Py::Object View3DOsgVersePy::toggleClippingPlane(const Py::Tuple& args, const Py::Dict& kwds)
{
    static const std::array<const char*, 2> keywords {"toggle", nullptr};
    int toggle = -1;
    if (!Base::Wrapped_ParseTupleAndKeywords(
            args.ptr(), kwds.ptr(), "|i", keywords, &toggle)) {
        throw Py::Exception();
    }
    auto* view = getView3DOsgVersePtr();
    if (toggle == -1) {
        view->toggleClippingPlane();
    }
    else if (toggle == 1 && !view->hasClippingPlane()) {
        view->toggleClippingPlane();
    }
    else if (toggle == 0 && view->hasClippingPlane()) {
        view->toggleClippingPlane();
    }
    return Py::None();
}

Py::Object View3DOsgVersePy::hasClippingPlane()
{
    return Py::Boolean(getView3DOsgVersePtr()->hasClippingPlane());
}

Py::Object View3DOsgVersePy::viewDimetric()
{
    try {
        // Dimetric view: standard CAD dimetric orientation
        // Uses the same quaternion convention as the Coin3D backend
        auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
        if (!viewer) {
            throw Py::RuntimeError("No viewer available");
        }
        auto cam = viewer->getCamera();
        // Dimetric rotation quaternion (matching Camera::Dimetric from Coin3D)
        // Elevation ~20°, azimuth ~20°
        Base::Rotation rot(0.567952, 0.103751, -0.146128, 0.803205);
        Base::Vector3d forward(0, 0, -1);
        Base::Vector3d up(0, 1, 0);
        rot.multVec(forward, forward);
        rot.multVec(up, up);
        double dist = (cam.target - cam.position).Length();
        if (dist < 0.1) dist = 10.0;
        cam.target = cam.position + forward * dist;
        cam.upVector = up;
        viewer->setCamera(cam);
    }
    catch (const Base::Exception& e) { throw Py::RuntimeError(e.what()); }
    return Py::None();
}

Py::Object View3DOsgVersePy::viewTrimetric()
{
    try {
        auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
        if (!viewer) {
            throw Py::RuntimeError("No viewer available");
        }
        auto cam = viewer->getCamera();
        // Trimetric rotation quaternion (matching Camera::Trimetric from Coin3D)
        // Elevation ~15°, azimuth ~30°
        Base::Rotation rot(0.446015, 0.119509, -0.229575, 0.856706);
        Base::Vector3d forward(0, 0, -1);
        Base::Vector3d up(0, 1, 0);
        rot.multVec(forward, forward);
        rot.multVec(up, up);
        double dist = (cam.target - cam.position).Length();
        if (dist < 0.1) dist = 10.0;
        cam.target = cam.position + forward * dist;
        cam.upVector = up;
        viewer->setCamera(cam);
    }
    catch (const Base::Exception& e) { throw Py::RuntimeError(e.what()); }
    return Py::None();
}

Py::Object View3DOsgVersePy::getObjectsInfo(const Py::Tuple& args)
{
    PyObject* object;
    float r = 5.0f;
    if (!PyArg_ParseTuple(args.ptr(), "O|f", &object, &r)) {
        throw Py::Exception();
    }
    try {
        const Py::Tuple tuple(object);
        Py::Long x(tuple[0]);
        Py::Long y(tuple[1]);

        auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
        if (!viewer) {
            return Py::None();
        }

        float oldRadius = viewer->getPickRadius();
        viewer->setPickRadius(r);
        View3D::PickResult result = viewer->pick(QPoint((long)x, (long)y));
        viewer->setPickRadius(oldRadius);

        if (!result.valid) {
            return Py::None();
        }

        Py::List list;
        Py::Dict dict;
        dict.setItem("x", Py::Float(result.point.x));
        dict.setItem("y", Py::Float(result.point.y));
        dict.setItem("z", Py::Float(result.point.z));

        if (result.viewProvider && result.viewProvider->isDerivedFrom<ViewProviderDocumentObject>()) {
            if (!result.viewProvider->isSelectable()) {
                return Py::None();
            }
            auto vpd = static_cast<ViewProviderDocumentObject*>(result.viewProvider);
            auto obj = vpd->getObject();
            if (obj) {
                dict.setItem("Document", Py::String(obj->getDocument()->getName()));
                dict.setItem("Object", Py::String(obj->getNameInDocument()));
                dict.setItem("Component", Py::String(result.subElementName));
            }
        }
        list.append(dict);
        return list;
    }
    catch (const Py::Exception&) {
        throw;
    }
}

Py::Object View3DOsgVersePy::startAnimating(const Py::Tuple& args)
{
    float x, y, z;
    float velocity;
    if (!PyArg_ParseTuple(args.ptr(), "ffff", &x, &y, &z, &velocity)) {
        throw Py::Exception();
    }
    // Accept the call for API compatibility; OsgVerse handles spin internally
    return Py::None();
}

Py::Object View3DOsgVersePy::stopAnimating()
{
    // Accept the call for API compatibility
    return Py::None();
}

Py::Object View3DOsgVersePy::setAnnotation(const Py::Tuple& args)
{
    char* psAnnoName;
    char* psBuffer;
    if (!PyArg_ParseTuple(args.ptr(), "ss", &psAnnoName, &psBuffer)) {
        throw Py::Exception();
    }
    Base::Console().developerWarning("View3DOsgVersePy",
        "setAnnotation() is not supported in OsgVerse backend\n");
    return Py::None();
}

Py::Object View3DOsgVersePy::removeAnnotation(const Py::Tuple& args)
{
    char* psAnnoName;
    if (!PyArg_ParseTuple(args.ptr(), "s", &psAnnoName)) {
        throw Py::Exception();
    }
    Base::Console().developerWarning("View3DOsgVersePy",
        "removeAnnotation() is not supported in OsgVerse backend\n");
    return Py::None();
}

Py::Object View3DOsgVersePy::setStereoType(const Py::Tuple& args)
{
    char* type;
    if (!PyArg_ParseTuple(args.ptr(), "s", &type)) {
        throw Py::Exception();
    }
    Base::Console().developerWarning("View3DOsgVersePy",
        "setStereoType() is not supported in OsgVerse backend\n");
    return Py::None();
}

Py::Object View3DOsgVersePy::getStereoType()
{
    return Py::String("None");
}

Py::Object View3DOsgVersePy::saveVectorGraphic(const Py::Tuple& args)
{
    char* filename;
    int type = 0;
    if (!PyArg_ParseTuple(args.ptr(), "s|i", &filename, &type)) {
        throw Py::Exception();
    }
    Base::Console().developerWarning("View3DOsgVersePy",
        "saveVectorGraphic() is not supported in OsgVerse backend\n");
    return Py::Boolean(false);
}

Py::Object View3DOsgVersePy::graphicsView()
{
    return Py::None();
}

Py::Object View3DOsgVersePy::setCornerCrossVisible(const Py::Tuple& args)
{
    int visible;
    if (!PyArg_ParseTuple(args.ptr(), "i", &visible)) {
        throw Py::Exception();
    }
    // Accept the call for API compatibility
    return Py::None();
}

Py::Object View3DOsgVersePy::setCornerCrossSize(const Py::Tuple& args)
{
    int size;
    if (!PyArg_ParseTuple(args.ptr(), "i", &size)) {
        throw Py::Exception();
    }
    // OsgVerse axis cross doesn't have a configurable size yet, accept silently
    return Py::None();
}

Py::Object View3DOsgVersePy::getViewProvidersOfType(const Py::Tuple& args)
{
    char* name;
    if (!PyArg_ParseTuple(args.ptr(), "s", &name)) {
        throw Py::Exception();
    }
    auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
    if (!viewer) {
        return Py::List();
    }
    Base::Type type = Base::Type::fromName(name);
    std::vector<Gui::ViewProvider*> allVPs = viewer->getViewProviders();
    Py::List list;
    for (auto* vp : allVPs) {
        if (vp && vp->isDerivedFrom(type)) {
            list.append(Py::asObject(vp->getPyObject()));
        }
    }
    return list;
}

Py::Object View3DOsgVersePy::boxZoom(const Py::Tuple& args, const Py::Dict& kwds)
{
    static const std::array<const char*, 5> kwds_box {"XMin", "YMin", "XMax", "YMax", nullptr};
    short xmin, ymin, xmax, ymax;
    if (!Base::Wrapped_ParseTupleAndKeywords(
            args.ptr(), kwds.ptr(), "hhhh", kwds_box,
            &xmin, &ymin, &xmax, &ymax)) {
        throw Py::Exception();
    }
    auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
    if (viewer) {
        viewer->boxZoom(xmin, ymin, xmax, ymax);
    }
    return Py::None();
}

// =========================================================================
// Event Callback System
// =========================================================================

using EventType = View3D::IViewer3D::EventType;

View3D::IViewer3D::EventType View3DOsgVersePy::mapEventType(const char* eventTypeStr)
{
    if (strcmp(eventTypeStr, "SoMouseButtonEvent") == 0) {
        return EventType::MouseButtonPress;
    }
    else if (strcmp(eventTypeStr, "SoLocation2Event") == 0) {
        return EventType::MouseMove;
    }
    else if (strcmp(eventTypeStr, "SoKeyboardEvent") == 0) {
        return EventType::KeyPress;
    }
    else if (strcmp(eventTypeStr, "SoEvent") == 0) {
        return EventType::Any;
    }
    else if (strcmp(eventTypeStr, "SoMouseWheelEvent") == 0) {
        return EventType::Wheel;
    }
    return EventType::Any;
}

static const char* eventTypeToString(EventType type)
{
    switch (type) {
        case EventType::MouseButtonPress:
        case EventType::MouseButtonRelease:
            return "SoMouseButtonEvent";
        case EventType::MouseMove:
            return "SoLocation2Event";
        case EventType::KeyPress:
        case EventType::KeyRelease:
            return "SoKeyboardEvent";
        case EventType::Wheel:
            return "SoMouseWheelEvent";
        case EventType::Any:
        default:
            return "SoEvent";
    }
}

void View3DOsgVersePy::eventCallback(EventType type, void* event, void* userData)
{
    Base::PyGILStateLocker lock;
    auto* pyCallable = static_cast<PyObject*>(userData);
    if (!pyCallable) {
        return;
    }

    try {
        Py::Dict dict;
        dict.setItem("Type", Py::String(eventTypeToString(type)));

        // Extract event info from QEvent
        auto* qevent = static_cast<QEvent*>(event);
        if (qevent) {
            bool shiftDown = false, ctrlDown = false, altDown = false;

            if (auto* me = dynamic_cast<QMouseEvent*>(qevent)) {
                Py::Tuple pos(2);
                pos.setItem(0, Py::Long(static_cast<long>(me->pos().x())));
                pos.setItem(1, Py::Long(static_cast<long>(me->pos().y())));
                dict.setItem("Position", pos);

                // Button
                if (me->button() == Qt::LeftButton) {
                    dict.setItem("Button", Py::String("BUTTON1"));
                }
                else if (me->button() == Qt::RightButton) {
                    dict.setItem("Button", Py::String("BUTTON2"));
                }
                else if (me->button() == Qt::MiddleButton) {
                    dict.setItem("Button", Py::String("BUTTON3"));
                }

                // State (press/release)
                if (type == EventType::MouseButtonPress) {
                    dict.setItem("State", Py::String("DOWN"));
                }
                else if (type == EventType::MouseButtonRelease) {
                    dict.setItem("State", Py::String("UP"));
                }

                shiftDown = me->modifiers() & Qt::ShiftModifier;
                ctrlDown = me->modifiers() & Qt::ControlModifier;
                altDown = me->modifiers() & Qt::AltModifier;
            }
            else if (auto* ke = dynamic_cast<QKeyEvent*>(qevent)) {
                // Key name
                QString keyText = ke->text();
                if (!keyText.isEmpty()) {
                    dict.setItem("Key", Py::String(keyText.toStdString()));
                }
                else {
                    dict.setItem("Key", Py::String(
                        QKeySequence(ke->key()).toString().toStdString()));
                }

                if (type == EventType::KeyPress) {
                    dict.setItem("State", Py::String("DOWN"));
                }
                else if (type == EventType::KeyRelease) {
                    dict.setItem("State", Py::String("UP"));
                }

                shiftDown = ke->modifiers() & Qt::ShiftModifier;
                ctrlDown = ke->modifiers() & Qt::ControlModifier;
                altDown = ke->modifiers() & Qt::AltModifier;
            }
            else if (auto* we = dynamic_cast<QWheelEvent*>(qevent)) {
                Py::Tuple pos(2);
                pos.setItem(0, Py::Long(static_cast<long>(we->position().x())));
                pos.setItem(1, Py::Long(static_cast<long>(we->position().y())));
                dict.setItem("Position", pos);
                dict.setItem("Delta", Py::Long(static_cast<long>(we->angleDelta().y())));

                shiftDown = we->modifiers() & Qt::ShiftModifier;
                ctrlDown = we->modifiers() & Qt::ControlModifier;
                altDown = we->modifiers() & Qt::AltModifier;
            }

            dict.setItem("ShiftDown", Py::Boolean(shiftDown));
            dict.setItem("CtrlDown", Py::Boolean(ctrlDown));
            dict.setItem("AltDown", Py::Boolean(altDown));
        }

        Py::Callable method(pyCallable);
        Py::Tuple args(1);
        args.setItem(0, dict);
        method.apply(args);
    }
    catch (const Py::Exception& e) {
        Py::Object o = Py::type(e);
        if (o.isString()) {
            Py::String s(o);
            Base::Console().warning("%s\n", s.as_std_string("utf-8").c_str());
        }
        else {
            Py::String s(o.repr());
            Base::Console().warning("%s\n", s.as_std_string("utf-8").c_str());
        }
        PyErr_Print();
    }
}

Py::Object View3DOsgVersePy::addEventCallback(const Py::Tuple& args)
{
    char* eventtype;
    PyObject* method;
    if (!PyArg_ParseTuple(args.ptr(), "sO", &eventtype, &method)) {
        throw Py::Exception();
    }
    if (PyCallable_Check(method) == 0) {
        throw Py::TypeError("object is not callable");
    }

    EventType type = mapEventType(eventtype);

    auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
    if (!viewer) {
        throw Py::RuntimeError("No viewer available");
    }

    // Create wrapper that calls the static eventCallback with the PyObject as userData
    View3D::IViewer3D::EventCallbackFunc wrapper =
        [](EventType et, void* ev, void* ud) -> bool {
            eventCallback(et, ev, ud);
            return false;
        };

    viewer->addEventCallback(type, wrapper, static_cast<void*>(method));
    _callbacks.push_back(method);
    _callbackWrappers[method] = wrapper;
    Py_INCREF(method);
    return Py::Callable(method, false);
}

Py::Object View3DOsgVersePy::removeEventCallback(const Py::Tuple& args)
{
    char* eventtype;
    PyObject* method;
    if (!PyArg_ParseTuple(args.ptr(), "sO", &eventtype, &method)) {
        throw Py::Exception();
    }
    if (PyCallable_Check(method) == 0) {
        throw Py::RuntimeError("object is not callable");
    }

    EventType type = mapEventType(eventtype);

    auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
    if (!viewer) {
        throw Py::RuntimeError("No viewer available");
    }

    auto it = _callbackWrappers.find(method);
    if (it != _callbackWrappers.end()) {
        viewer->removeEventCallback(type, it->second, static_cast<void*>(method));
        _callbackWrappers.erase(it);
        _callbacks.remove(method);
        Py_DECREF(method);
    }
    return Py::None();
}

Py::Object View3DOsgVersePy::addEventCallbackPivy(const Py::Tuple& args)
{
    PyObject* proxy;
    PyObject* method;
    if (!PyArg_ParseTuple(args.ptr(), "OO", &proxy, &method)) {
        throw Py::Exception();
    }
    if (PyCallable_Check(method) == 0) {
        throw Py::TypeError("object is not callable");
    }

    // Try to extract event type name from pivy SoType object
    std::string eventTypeName = "SoEvent";
    try {
        Py::Object proxyObj(proxy);
        if (proxyObj.hasAttr("getName")) {
            Py::Callable getName(proxyObj.getAttr("getName"));
            Py::Object nameObj = getName.apply(Py::Tuple());
            Py::String nameStr(nameObj);
            eventTypeName = nameStr.as_std_string("utf-8");
        }
    }
    catch (...) {
        if (PyUnicode_Check(proxy)) {
            const char* str = PyUnicode_AsUTF8(proxy);
            if (str) {
                eventTypeName = str;
            }
        }
        else {
            Base::Console().warning(
                "OsgVerse: Could not extract event type from pivy object, using SoEvent\n");
        }
    }

    EventType type = mapEventType(eventTypeName.c_str());

    auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
    if (!viewer) {
        throw Py::RuntimeError("No viewer available");
    }

    View3D::IViewer3D::EventCallbackFunc wrapper =
        [](EventType et, void* ev, void* ud) -> bool {
            eventCallback(et, ev, ud);
            return false;
        };

    viewer->addEventCallback(type, wrapper, static_cast<void*>(method));
    _callbacks.push_back(method);
    _callbackWrappers[method] = wrapper;
    Py_INCREF(method);
    return Py::Callable(method, false);
}

Py::Object View3DOsgVersePy::removeEventCallbackPivy(const Py::Tuple& args)
{
    PyObject* proxy;
    PyObject* method;
    if (!PyArg_ParseTuple(args.ptr(), "OO", &proxy, &method)) {
        throw Py::Exception();
    }
    if (PyCallable_Check(method) == 0) {
        throw Py::RuntimeError("object is not callable");
    }

    // Extract event type (same logic as addEventCallbackPivy)
    std::string eventTypeName = "SoEvent";
    try {
        Py::Object proxyObj(proxy);
        if (proxyObj.hasAttr("getName")) {
            Py::Callable getName(proxyObj.getAttr("getName"));
            Py::Object nameObj = getName.apply(Py::Tuple());
            Py::String nameStr(nameObj);
            eventTypeName = nameStr.as_std_string("utf-8");
        }
    }
    catch (...) {
        if (PyUnicode_Check(proxy)) {
            const char* str = PyUnicode_AsUTF8(proxy);
            if (str) {
                eventTypeName = str;
            }
        }
    }

    EventType type = mapEventType(eventTypeName.c_str());

    auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
    if (!viewer) {
        throw Py::RuntimeError("No viewer available");
    }

    auto it = _callbackWrappers.find(method);
    if (it != _callbackWrappers.end()) {
        viewer->removeEventCallback(type, it->second, static_cast<void*>(method));
        _callbackWrappers.erase(it);
        _callbacks.remove(method);
        Py_DECREF(method);
    }
    return Py::None();
}

// =========================================================================
// New methods (gap closure)
// =========================================================================

Py::Object View3DOsgVersePy::viewDefaultOrientation(const Py::Tuple& args)
{
    char* ori_str = nullptr;
    double scale = -1.0;
    if (!PyArg_ParseTuple(args.ptr(), "|sd", &ori_str, &scale)) {
        throw Py::Exception();
    }
    // Reset to home position via the viewer interface
    auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
    if (viewer) {
        viewer->resetToHomePosition();
    }
    return Py::None();
}

Py::Object View3DOsgVersePy::viewPosition(const Py::Tuple& args)
{
    // Accept placement, steps, ms for API compatibility
    // OsgVerse doesn't support animated transitions yet, just set the orientation
    PyObject* pla = nullptr;
    int steps = 0;
    int ms = 0;
    if (!PyArg_ParseTuple(args.ptr(), "|Oii", &pla, &steps, &ms)) {
        throw Py::Exception();
    }
    if (pla) {
        try {
            Py::Object plaObj(pla);
            if (plaObj.hasAttr("Rotation")) {
                Py::Object rotObj = plaObj.getAttr("Rotation");
                if (PyObject_TypeCheck(rotObj.ptr(), &Base::RotationPy::Type)) {
                    Base::Rotation rot = static_cast<Base::Rotation>(Py::Rotation(rotObj.ptr(), false));
                    Base::Vector3d forward(0, 0, -1);
                    Base::Vector3d up(0, 1, 0);
                    rot.multVec(forward, forward);
                    rot.multVec(up, up);
                    auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
                    if (viewer) {
                        auto cam = viewer->getCamera();
                        double dist = (cam.target - cam.position).Length();
                        if (dist < 0.1) dist = 10.0;
                        cam.target = cam.position + forward * dist;
                        cam.upVector = up;
                        viewer->setCamera(cam);
                    }
                }
            }
        }
        catch (const Py::Exception&) { throw; }
        catch (...) {}
    }
    return Py::None();
}

Py::Object View3DOsgVersePy::viewRotateLeft()
{
    try {
        auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
        if (!viewer) throw Py::RuntimeError("No viewer available");
        auto cam = viewer->getCamera();
        Base::Vector3d forward = cam.target - cam.position;
        forward.Normalize();
        // Rotate up vector 90 degrees around forward axis
        Base::Rotation rot(forward, M_PI / 2.0);
        Base::Vector3d newUp;
        rot.multVec(cam.upVector, newUp);
        cam.upVector = newUp;
        viewer->setCamera(cam);
    }
    catch (const Base::Exception& e) { throw Py::RuntimeError(e.what()); }
    return Py::None();
}

Py::Object View3DOsgVersePy::viewRotateRight()
{
    try {
        auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
        if (!viewer) throw Py::RuntimeError("No viewer available");
        auto cam = viewer->getCamera();
        Base::Vector3d forward = cam.target - cam.position;
        forward.Normalize();
        // Rotate up vector -90 degrees around forward axis
        Base::Rotation rot(forward, -M_PI / 2.0);
        Base::Vector3d newUp;
        rot.multVec(cam.upVector, newUp);
        cam.upVector = newUp;
        viewer->setCamera(cam);
    }
    catch (const Base::Exception& e) { throw Py::RuntimeError(e.what()); }
    return Py::None();
}

Py::Object View3DOsgVersePy::getObjectInfoRay(const Py::Tuple& args)
{
    // Ray-based picking is not yet implemented in OsgVerse backend
    // Accept the call for API compatibility
    return Py::None();
}

Py::Object View3DOsgVersePy::setAnimationEnabled(const Py::Tuple& args)
{
    int ok;
    if (!PyArg_ParseTuple(args.ptr(), "i", &ok)) {
        throw Py::Exception();
    }
    auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
    if (viewer) {
        viewer->setAnimationEnabled(ok != 0);
    }
    return Py::None();
}

Py::Object View3DOsgVersePy::isAnimationEnabled()
{
    auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
    if (viewer) {
        return Py::Boolean(viewer->isAnimationEnabled());
    }
    return Py::Boolean(false);
}

Py::Object View3DOsgVersePy::setPopupMenuEnabled(const Py::Tuple& args)
{
    int ok;
    if (!PyArg_ParseTuple(args.ptr(), "i", &ok)) {
        throw Py::Exception();
    }
    auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
    if (viewer) {
        viewer->setPopupMenuEnabled(ok != 0);
    }
    return Py::None();
}

Py::Object View3DOsgVersePy::isPopupMenuEnabled()
{
    auto* viewer = getView3DOsgVersePtr()->getViewerInterface();
    if (viewer) {
        return Py::Boolean(viewer->isPopupMenuEnabled());
    }
    return Py::Boolean(true);
}

Py::Object View3DOsgVersePy::listStereoTypes()
{
    Py::List list;
    list.append(Py::String("None"));
    return list;
}

Py::Object View3DOsgVersePy::isCornerCrossVisible()
{
    return Py::Boolean(false);
}

Py::Object View3DOsgVersePy::getCornerCrossSize()
{
    return Py::Long(0);
}

Py::Object View3DOsgVersePy::setName(const Py::Tuple& args)
{
    char* buffer;
    if (!PyArg_ParseTuple(args.ptr(), "s", &buffer)) {
        throw Py::Exception();
    }
    getView3DOsgVersePtr()->setWindowTitle(QString::fromUtf8(buffer));
    return Py::None();
}

#endif // RENDER_HAS_OSGVERSE_BACKEND
