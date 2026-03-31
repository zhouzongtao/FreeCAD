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

// Only compile this file when OsgVerse backend is available
#ifdef RENDER_HAS_OSGVERSE_BACKEND

#include "PreCompiled.h"

#ifndef _PreComp_
# include <QVBoxLayout>
# include <QOpenGLWidget>
# include <QPrinter>
# include <QPrintDialog>
# include <QPrintPreviewDialog>
# include <QPainter>
# include <QMessageBox>
# include <QStackedWidget>
# include <QApplication>
# include <QContextMenuEvent>
# include <QMimeData>
# include <QMdiSubWindow>
# include <QWindow>
#endif

#include "View3DOsgVerse.h"
#include "View3DOsgVersePy.h"
#include "View3D/ViewerFactory.h"
#include "Core/RenderManager.h"
#include "Document.h"
#include "Application.h"
#include "MainWindow.h"
#include "BitmapFactory.h"
#include "FileDialog.h"
#include "WaitCursor.h"
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <App/Document.h>
#include <App/Application.h>
#include <App/GeoFeature.h>
#include <osg/Quat>
#include <osg/Matrix>
#include "Render/Backends/OsgVerse/OsgVerseViewer.h"
#include "Render/Core/RenderViewer.h"
#include "Selection/Selection.h"
#include "Navigation/NavigationStyle.h"
#include <cmath>
#include <sstream>

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

        // ViewerFactory creates OsgVerseViewer (a RenderViewer subclass) stored as IViewer3D.
        // Keep a typed pointer for backend-specific operations not in IViewer3D.
        _osgViewer = static_cast<Gui::Render::OsgVerseViewer*>(
            static_cast<void*>(_viewer.get()));
        
        // Get the widget from viewer
        QWidget* viewerWidget = _viewer->getWidget();
        if (!viewerWidget) {
            throw std::runtime_error("Viewer did not provide a widget");
        }
        
        // Wrap viewer widget in a stacked widget for overlay support
        _stack = new QStackedWidget(this);
        _stack->addWidget(viewerWidget);
        setCentralWidget(_stack);
        
        // Set window properties
        setWindowIcon(Gui::BitmapFactory().pixmap("Document"));
        setFocusPolicy(Qt::StrongFocus);
        setMouseTracking(true);
        setAcceptDrops(true);

        // Apply user preferences
        applySettings();

        // Attach to parameter groups for dynamic settings changes
        _hViewGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View");
        _hViewGrp->Attach(this);

        _hNaviCubeGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/NaviCube");
        _hNaviCubeGrp->Attach(this);
    }
    catch (const std::exception& e) {
        Base::Console().error("View3DOsgVerse: Failed to create viewer: %s\n", e.what());
        throw;
    }
}

View3DOsgVerse::~View3DOsgVerse()
{
    if (_hViewGrp) {
        _hViewGrp->Detach(this);
    }
    if (_hNaviCubeGrp) {
        _hNaviCubeGrp->Detach(this);
    }
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

std::string View3DOsgVerse::getCameraString() const
{
    if (!_viewer) {
        return std::string();
    }

    auto cam = _viewer->getCamera();
    bool ortho = _viewer->isCameraOrthographic();
    std::ostringstream ss;

    auto buildOrientation = [&](const Gui::View3D::CameraParams& c) {
        osg::Vec3d eye(c.position.x, c.position.y, c.position.z);
        osg::Vec3d center(c.target.x, c.target.y, c.target.z);
        osg::Vec3d up(c.upVector.x, c.upVector.y, c.upVector.z);
        osg::Vec3d forward = center - eye;
        forward.normalize();
        osg::Vec3d right = forward ^ up;
        right.normalize();
        osg::Vec3d realUp = right ^ forward;
        realUp.normalize();
        osg::Matrixd rotMat;
        rotMat.set(right.x(), right.y(), right.z(), 0,
                   realUp.x(), realUp.y(), realUp.z(), 0,
                   -forward.x(), -forward.y(), -forward.z(), 0,
                   0, 0, 0, 1);
        osg::Quat q;
        q.set(rotMat);
        double angle;
        osg::Vec3d axis;
        q.getRotate(angle, axis);
        ss << "  orientation " << axis.x() << " " << axis.y() << " " << axis.z() << " " << angle << "\n";
        double distance = (eye - center).length();
        ss << "  nearDistance " << c.nearPlane << "\n"
           << "  farDistance " << c.farPlane << "\n"
           << "  aspectRatio " << c.aspectRatio << "\n"
           << "  focalDistance " << distance << "\n";
    };

    if (ortho) {
        ss << "OrthographicCamera {\n"
           << "  viewportMapping ADJUST_CAMERA\n"
           << "  position " << cam.position.x << " " << cam.position.y << " " << cam.position.z << "\n";
        buildOrientation(cam);
        ss << "  height " << cam.height << "\n"
           << "}\n";
    } else {
        ss << "PerspectiveCamera {\n"
           << "  viewportMapping ADJUST_CAMERA\n"
           << "  position " << cam.position.x << " " << cam.position.y << " " << cam.position.z << "\n";
        buildOrientation(cam);
        ss << "  heightAngle " << (cam.fieldOfView * M_PI / 180.0) << "\n"
           << "}\n";
    }

    return ss.str();
}

bool View3DOsgVerse::onMsg(const char* pMsg)
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
        _viewer->fitSelection();
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
        // Isometric view: standard CAD isometric orientation
        Gui::View3D::CameraParams params = _viewer->getCamera();
        // Standard isometric: rotate 45° around Z, then ~35.264° around X
        // Quaternion for isometric view (world-to-camera)
        double s2 = std::sqrt(2.0);
        double s3 = std::sqrt(3.0);
        double s6 = std::sqrt(6.0);
        // This quaternion gives the standard isometric view matching Coin3D
        osg::Quat rot(
            (s3 + 1.0) / (2.0 * s6),   // x
            (s3 - 1.0) / (2.0 * s6),   // y
            (1.0 - s3) / (2.0 * s6),   // z
            (1.0 + s3) / (2.0 * s6)    // w
        );
        setCameraParamsFromQuat(params, rot);
        _viewer->setCamera(params);
        _viewer->viewAll();
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
    else if (strcmp(pMsg, "GetCamera") == 0) {
        getCameraString();
        return true;
    }
    else if (strncmp(pMsg, "SetCamera", 9) == 0) {
        // Parse Inventor ASCII camera string and apply
        const char* camStr = pMsg + 10;
        if (!camStr || !*camStr) return false;

        Gui::View3D::CameraParams params = _viewer->getCamera();
        std::string str(camStr);

        bool ortho = (str.find("OrthographicCamera") != std::string::npos);

        // Parse position
        auto parseVec3 = [&str](const std::string& key, double& x, double& y, double& z) {
            size_t pos = str.find(key);
            if (pos != std::string::npos) {
                pos += key.length();
                std::istringstream iss(str.substr(pos));
                iss >> x >> y >> z;
            }
        };

        double px = 0, py = 0, pz = 0;
        parseVec3("position ", px, py, pz);
        params.position = Base::Vector3d(px, py, pz);

        // Parse orientation (axis-angle)
        size_t oriPos = str.find("orientation ");
        if (oriPos != std::string::npos) {
            oriPos += 12;
            double ax, ay, az, angle;
            std::istringstream iss(str.substr(oriPos));
            iss >> ax >> ay >> az >> angle;

            osg::Quat q(angle, osg::Vec3d(ax, ay, az));
            // q is camera rotation: forward = q * (0,0,-1), up = q * (0,1,0)
            osg::Vec3d forward = q * osg::Vec3d(0, 0, -1);
            osg::Vec3d up = q * osg::Vec3d(0, 1, 0);

            // Parse focal distance for target computation
            double focalDist = 5.0;
            size_t fdPos = str.find("focalDistance ");
            if (fdPos != std::string::npos) {
                std::istringstream fiss(str.substr(fdPos + 14));
                fiss >> focalDist;
            }

            osg::Vec3d eye(px, py, pz);
            osg::Vec3d center = eye + forward * focalDist;
            params.target = Base::Vector3d(center.x(), center.y(), center.z());
            params.upVector = Base::Vector3d(up.x(), up.y(), up.z());
        }

        params.orthographic = ortho;

        // Parse height (ortho) or heightAngle (perspective)
        if (ortho) {
            size_t hPos = str.find("height ");
            if (hPos != std::string::npos) {
                // Make sure we don't match "heightAngle"
                if (hPos == 0 || str[hPos - 1] == '\n' || str[hPos - 1] == ' ') {
                    std::istringstream iss(str.substr(hPos + 7));
                    iss >> params.height;
                }
            }
        } else {
            size_t haPos = str.find("heightAngle ");
            if (haPos != std::string::npos) {
                double heightAngle;
                std::istringstream iss(str.substr(haPos + 12));
                iss >> heightAngle;
                params.fieldOfView = heightAngle * 180.0 / M_PI;
            }
        }

        _viewer->setCamera(params);
        _viewer->setCameraType(ortho);
        return true;
    }
    else if (strcmp(pMsg, "ZoomIn") == 0) {
        auto cam = _viewer->getCamera();
        Base::Vector3d dir = cam.target - cam.position;
        double dist = dir.Length();
        dir.Normalize();
        // Move 20% closer
        double newDist = dist * 0.8;
        cam.position = cam.target - dir * newDist;
        if (cam.orthographic) {
            cam.height *= 0.8;
        }
        _viewer->setCamera(cam);
        return true;
    }
    else if (strcmp(pMsg, "ZoomOut") == 0) {
        auto cam = _viewer->getCamera();
        Base::Vector3d dir = cam.target - cam.position;
        double dist = dir.Length();
        dir.Normalize();
        // Move 25% farther
        double newDist = dist * 1.25;
        cam.position = cam.target - dir * newDist;
        if (cam.orthographic) {
            cam.height *= 1.25;
        }
        _viewer->setCamera(cam);
        return true;
    }
    else if (strcmp(pMsg, "Undo") == 0) {
        getGuiDocument()->undo(1);
        return true;
    }
    else if (strcmp(pMsg, "Redo") == 0) {
        getGuiDocument()->redo(1);
        return true;
    }
    else if (strcmp(pMsg, "Save") == 0) {
        getGuiDocument()->save();
        return true;
    }
    else if (strcmp(pMsg, "SaveAs") == 0) {
        getGuiDocument()->saveAs();
        return true;
    }
    else if (strcmp(pMsg, "SaveCopy") == 0) {
        getGuiDocument()->saveCopy();
        return true;
    }
    else if (strcmp(pMsg, "Print") == 0) {
        print();
        return true;
    }
    else if (strcmp(pMsg, "PrintPreview") == 0) {
        printPreview();
        return true;
    }
    else if (strcmp(pMsg, "PrintPdf") == 0) {
        printPdf();
        return true;
    }
    else if (strcmp(pMsg, "AlignToSelection") == 0) {
        // Align camera to look at the selected object's face normal
        // Try to get selected object's placement and compute a view direction
        if (auto* guiDoc = getGuiDocument()) {
            auto sel = Gui::Selection().getSelection(guiDoc->getDocument()->getName());
            if (!sel.empty()) {
                auto* obj = sel.front().pObject;
                if (obj) {
                    auto* geoFeature = dynamic_cast<App::GeoFeature*>(obj);
                    if (geoFeature) {
                        Base::Placement plc = geoFeature->Placement.getValue();
                        Base::Rotation rot = plc.getRotation();
                        // Get the Z axis of the placement (face normal approximation)
                        Base::Vector3d zAxis(0, 0, 1);
                        rot.multVec(zAxis, zAxis);

                        // Set camera to look along the negative Z axis of the placement
                        auto cam = _viewer->getCamera();
                        Base::Vector3d center(plc.getPosition().x,
                                              plc.getPosition().y,
                                              plc.getPosition().z);
                        double dist = (Base::Vector3d(cam.position.x, cam.position.y, cam.position.z) -
                                       Base::Vector3d(cam.target.x, cam.target.y, cam.target.z)).Length();
                        if (dist < 0.1) dist = 10.0;

                        Base::Vector3d eye = center + zAxis * dist;
                        // Compute a reasonable up vector perpendicular to zAxis
                        Base::Vector3d up(0, 0, 1);
                        if (std::abs(zAxis.Dot(up)) > 0.9) {
                            up = Base::Vector3d(0, 1, 0);
                        }
                        Base::Vector3d right = zAxis.Cross(up);
                        right.Normalize();
                        up = right.Cross(zAxis);
                        up.Normalize();

                        cam.position = eye;
                        cam.target = center;
                        cam.upVector = up;
                        _viewer->setCamera(cam);
                        _viewer->fitSelection();
                        return true;
                    }
                }
            }
        }
        // Fallback: just fit selection
        _viewer->fitSelection();
        return true;
    }
    else if (strcmp(pMsg, "SetStereoRedGreen") == 0 ||
             strcmp(pMsg, "SetStereoQuadBuff") == 0 ||
             strcmp(pMsg, "SetStereoInterleavedRows") == 0 ||
             strcmp(pMsg, "SetStereoInterleavedColumns") == 0 ||
             strcmp(pMsg, "SetStereoOff") == 0) {
        Base::Console().log("View3DOsgVerse: Stereo mode '%s' not yet implemented\n", pMsg);
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
        strcmp(pMsg, "DumpInfo") == 0 ||
        strcmp(pMsg, "GetCamera") == 0 ||
        strcmp(pMsg, "ZoomIn") == 0 ||
        strcmp(pMsg, "ZoomOut") == 0) {
        return true;
    }
    if (strncmp(pMsg, "SetCamera", 9) == 0) {
        return true;
    }
    if (strcmp(pMsg, "Save") == 0 ||
        strcmp(pMsg, "SaveAs") == 0 ||
        strcmp(pMsg, "SaveCopy") == 0) {
        return true;
    }
    if (strcmp(pMsg, "Undo") == 0) {
        App::Document* doc = getAppDocument();
        return doc && doc->getAvailableUndos() > 0;
    }
    if (strcmp(pMsg, "Redo") == 0) {
        App::Document* doc = getAppDocument();
        return doc && doc->getAvailableRedos() > 0;
    }
    if (strcmp(pMsg, "Print") == 0 ||
        strcmp(pMsg, "PrintPreview") == 0 ||
        strcmp(pMsg, "PrintPdf") == 0) {
        return true;
    }
    if (strcmp(pMsg, "CanPan") == 0) {
        return true;
    }
    if (strcmp(pMsg, "AllowsOverlayOnHover") == 0) {
        return true;
    }
    if (strcmp(pMsg, "AlignToSelection") == 0) {
        return true;
    }
    if (strcmp(pMsg, "SetStereoRedGreen") == 0 ||
        strcmp(pMsg, "SetStereoQuadBuff") == 0 ||
        strcmp(pMsg, "SetStereoInterleavedRows") == 0 ||
        strcmp(pMsg, "SetStereoInterleavedColumns") == 0 ||
        strcmp(pMsg, "SetStereoOff") == 0) {
        return true;
    }

    return false;
}

void View3DOsgVerse::print()
{
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setFullPage(true);
    restorePrinterSettings(&printer);

    QPrintDialog dlg(&printer, this);
    if (dlg.exec() == QDialog::Accepted) {
        Gui::WaitCursor wc;
        print(&printer);
        savePrinterSettings(&printer);
    }
}

void View3DOsgVerse::printPdf()
{
    QString filename = FileDialog::getSaveFileName(
        this,
        tr("Export PDF"),
        QString(),
        QStringLiteral("%1 (*.pdf)").arg(tr("PDF file"))
    );
    if (!filename.isEmpty()) {
        Gui::WaitCursor wc;
        QPrinter printer(QPrinter::ScreenResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setPageOrientation(QPageLayout::Landscape);
        printer.setOutputFileName(filename);
        printer.setCreator(QString::fromStdString(App::Application::getNameWithVersion()));
        print(&printer);
    }
}

void View3DOsgVerse::printPreview()
{
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setFullPage(true);
    restorePrinterSettings(&printer);

    QPrintPreviewDialog dlg(&printer, this);
    connect(&dlg, &QPrintPreviewDialog::paintRequested,
            this, qOverload<QPrinter*>(&View3DOsgVerse::print));
    dlg.exec();
    savePrinterSettings(&printer);
}

void View3DOsgVerse::print(QPrinter* printer)
{
    QPainter p(printer);
    p.setRenderHints(QPainter::Antialiasing);
    if (!p.isActive() && !printer->outputFileName().isEmpty()) {
        qApp->setOverrideCursor(Qt::ArrowCursor);
        QMessageBox::critical(this, tr("Opening file failed"),
            tr("Can't open file '%1' for writing.").arg(printer->outputFileName()));
        qApp->restoreOverrideCursor();
        return;
    }

    QRect rect = printer->pageLayout().paintRectPixels(printer->resolution());
    QImage img = _viewer->grabImage(rect.width(), rect.height());
    p.drawImage(0, 0, img);
    p.end();
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

bool View3DOsgVerse::containsViewProvider(const ViewProvider* vp) const
{
    if (!_viewer) return false;
    auto* osgViewer = _osgViewer;
    if (!osgViewer) return false;
    return osgViewer->hasViewProvider(const_cast<ViewProvider*>(vp));
}

void View3DOsgVerse::onRename(Gui::Document* pDoc)
{
    if (pDoc && pDoc->getDocument()) {
        setWindowTitle(QString::fromUtf8(pDoc->getDocument()->Label.getValue()));
    }
}

void View3DOsgVerse::stopAnimating()
{
    if (!_viewer) return;
    auto* osgViewer = _osgViewer;
    if (osgViewer) {
        osgViewer->stopAnimation();
    }
}

void View3DOsgVerse::contextMenuEvent(QContextMenuEvent* e)
{
    MDIView::contextMenuEvent(e);
}

void View3DOsgVerse::keyPressEvent(QKeyEvent* e)
{
    QMainWindow::keyPressEvent(e);
}

void View3DOsgVerse::keyReleaseEvent(QKeyEvent* e)
{
    QMainWindow::keyReleaseEvent(e);
}

void View3DOsgVerse::focusInEvent(QFocusEvent*)
{
    if (_viewer && _viewer->getWidget()) {
        _viewer->getWidget()->setFocus();
    }
}

void View3DOsgVerse::dropEvent(QDropEvent* e)
{
    const QMimeData* data = e->mimeData();
    if (data->hasUrls()) {
        getMainWindow()->loadUrls(getAppDocument(), data->urls());
    }
    else {
        MDIView::dropEvent(e);
    }
}

void View3DOsgVerse::dragEnterEvent(QDragEnterEvent* e)
{
    const QMimeData* data = e->mimeData();
    if (data->hasUrls()) {
        e->accept();
    }
    else {
        e->ignore();
    }
}


//===========================================================================
// Python interface
//===========================================================================

PyObject* View3DOsgVerse::getPyObject()
{
    if (!_viewerPy) {
        _viewerPy = new View3DOsgVersePy(this);
    }

    Py_INCREF(_viewerPy);
    return _viewerPy;
}

void View3DOsgVerse::toggleClippingPlane()
{
    if (!_viewer) return;

    auto* osgViewer = _osgViewer;
    if (!osgViewer) return;

    if (_clippingPlaneActive) {
        osgViewer->removeClipPlane(0);
        _clippingPlaneActive = false;
    } else {
        auto cam = _viewer->getCamera();
        Base::Vector3d dir = cam.target - cam.position;
        dir.Normalize();
        double d = -(dir.x * cam.target.x + dir.y * cam.target.y + dir.z * cam.target.z);
        osgViewer->setClipPlane(0, dir.x, dir.y, dir.z, d);
        _clippingPlaneActive = true;
    }
}

bool View3DOsgVerse::hasClippingPlane() const
{
    return _clippingPlaneActive;
}

void View3DOsgVerse::applySettings()
{
    if (!_viewer) return;

    // Don't trigger OsgVerse initialization during construction.
    // Settings will be applied on first paint/resize when the viewer is ready.
    if (_osgViewer && !_osgViewer->isInitialized()) return;

    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View");

    auto* osgViewer = _osgViewer;

    // Background color
    bool useGradient = hGrp->GetBool("Gradient", true);
    if (useGradient) {
        unsigned long col1 = hGrp->GetUnsigned("BackgroundColor", 0x1a1a2cff);
        unsigned long col2 = hGrp->GetUnsigned("BackgroundColor2", 0x46465aff);

        float r1 = ((col1 >> 24) & 0xff) / 255.0f;
        float g1 = ((col1 >> 16) & 0xff) / 255.0f;
        float b1 = ((col1 >> 8) & 0xff) / 255.0f;
        float r2 = ((col2 >> 24) & 0xff) / 255.0f;
        float g2 = ((col2 >> 16) & 0xff) / 255.0f;
        float b2 = ((col2 >> 8) & 0xff) / 255.0f;

        if (osgViewer) {
            osgViewer->setGradientBackground(r2, g2, b2, r1, g1, b1);
        }
    } else {
        unsigned long col = hGrp->GetUnsigned("BackgroundColor", 0x1a1a2cff);
        float r = ((col >> 24) & 0xff) / 255.0f;
        float g = ((col >> 16) & 0xff) / 255.0f;
        float b = ((col >> 8) & 0xff) / 255.0f;
        _viewer->setBackgroundColor(Base::Color(r, g, b));
    }

    // Camera type (orthographic/perspective)
    bool ortho = hGrp->GetBool("Orthographic", false);
    _viewer->setCameraType(ortho);

    // Navigation style
    std::string navStyle = hGrp->GetASCII("NavigationStyle", "Gui::CADNavigationStyle");

    if (osgViewer) {
        if (navStyle.find("CAD") != std::string::npos) {
            osgViewer->setNavigationStyle("CAD");
        } else if (navStyle.find("Blender") != std::string::npos) {
            osgViewer->setNavigationStyle("Blender");
        } else if (navStyle.find("Inventor") != std::string::npos) {
            osgViewer->setNavigationStyle("Inventor");
        } else {
            osgViewer->setNavigationStyle("CAD");
        }

        // Axis cross
        bool showAxisCross = hGrp->GetBool("ShowAxisCross", true);
        osgViewer->setAxisCrossEnabled(showAxisCross);

        // Animation settings
        bool useNavAnim = hGrp->GetBool("UseNavigationAnimations", true);
        osgViewer->setAnimationEnabled(useNavAnim);

        bool useSpinAnim = hGrp->GetBool("UseSpinningAnimations", true);
        osgViewer->setSpinAnimationEnabled(useSpinAnim);

        // Headlight settings
        // (OsgVerse uses its own lighting; store values for future use)
        // hGrp->GetUnsigned("HeadlightColor", 0xFFFFFFFF);
        // hGrp->GetASCII("HeadlightDirection", "(0.6841049,-0.12062616,-0.7193398)");
        // hGrp->GetInt("HeadlightIntensity", 90);

        // Selection/preselection
        osgViewer->setSelectionEnabled(hGrp->GetBool("EnableSelection", true));

        // Zoom settings (stored for navigation style use)
        bool invertZoom = hGrp->GetBool("InvertZoom", false);
        (void)invertZoom; // TODO: pass to navigation style when available

        // Show FPS
        osgViewer->setStatsEnabled(hGrp->GetBool("ShowFPS", false));

        // Pick radius
        osgViewer->setPickRadius(static_cast<float>(hGrp->GetInt("PickRadius", 5)));
    }

    // NaviCube settings
    auto hNaviCube = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/NaviCube");
    // CornerNaviCube, CubeSize, FontString — stored for NaviCube use
    // These are read by OsgVerseNaviCube directly when needed
}

void View3DOsgVerse::OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason)
{
    if (!_viewer) return;

    auto* osgViewer = _osgViewer;

    if (strcmp(Reason, "Gradient") == 0 ||
        strcmp(Reason, "BackgroundColor") == 0 ||
        strcmp(Reason, "BackgroundColor2") == 0) {
        // Re-apply background settings
        auto hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View");
        bool useGradient = hGrp->GetBool("Gradient", true);
        if (useGradient) {
            unsigned long col1 = hGrp->GetUnsigned("BackgroundColor", 0x1a1a2cff);
            unsigned long col2 = hGrp->GetUnsigned("BackgroundColor2", 0x46465aff);
            float r1 = ((col1 >> 24) & 0xff) / 255.0f;
            float g1 = ((col1 >> 16) & 0xff) / 255.0f;
            float b1 = ((col1 >> 8) & 0xff) / 255.0f;
            float r2 = ((col2 >> 24) & 0xff) / 255.0f;
            float g2 = ((col2 >> 16) & 0xff) / 255.0f;
            float b2 = ((col2 >> 8) & 0xff) / 255.0f;
            if (osgViewer) {
                osgViewer->setGradientBackground(r2, g2, b2, r1, g1, b1);
            }
        } else {
            unsigned long col = static_cast<ParameterGrp&>(rCaller).GetUnsigned("BackgroundColor", 0x1a1a2cff);
            float r = ((col >> 24) & 0xff) / 255.0f;
            float g = ((col >> 16) & 0xff) / 255.0f;
            float b = ((col >> 8) & 0xff) / 255.0f;
            _viewer->setBackgroundColor(Base::Color(r, g, b));
        }
    }
    else if (strcmp(Reason, "Orthographic") == 0) {
        bool ortho = static_cast<ParameterGrp&>(rCaller).GetBool("Orthographic", false);
        _viewer->setCameraType(ortho);
    }
    else if (strcmp(Reason, "NavigationStyle") == 0) {
        std::string navStyle = static_cast<ParameterGrp&>(rCaller).GetASCII("NavigationStyle", "Gui::CADNavigationStyle");
        if (osgViewer) {
            if (navStyle.find("CAD") != std::string::npos) {
                osgViewer->setNavigationStyle("CAD");
            } else if (navStyle.find("Blender") != std::string::npos) {
                osgViewer->setNavigationStyle("Blender");
            } else if (navStyle.find("Inventor") != std::string::npos) {
                osgViewer->setNavigationStyle("Inventor");
            } else {
                osgViewer->setNavigationStyle("CAD");
            }
        }
    }
    else if (strcmp(Reason, "ShowAxisCross") == 0) {
        if (osgViewer) {
            osgViewer->setAxisCrossEnabled(static_cast<ParameterGrp&>(rCaller).GetBool("ShowAxisCross", true));
        }
    }
    else if (strcmp(Reason, "UseNavigationAnimations") == 0) {
        if (osgViewer) {
            osgViewer->setAnimationEnabled(static_cast<ParameterGrp&>(rCaller).GetBool("UseNavigationAnimations", true));
        }
    }
    else if (strcmp(Reason, "UseSpinningAnimations") == 0) {
        if (osgViewer) {
            osgViewer->setSpinAnimationEnabled(static_cast<ParameterGrp&>(rCaller).GetBool("UseSpinningAnimations", true));
        }
    }
    else if (strcmp(Reason, "ShowFPS") == 0) {
        if (osgViewer) {
            osgViewer->setStatsEnabled(static_cast<ParameterGrp&>(rCaller).GetBool("ShowFPS", false));
        }
    }
    else if (strcmp(Reason, "EnableSelection") == 0) {
        if (osgViewer) {
            osgViewer->setSelectionEnabled(static_cast<ParameterGrp&>(rCaller).GetBool("EnableSelection", true));
        }
    }
    else if (strcmp(Reason, "PickRadius") == 0) {
        if (osgViewer) {
            osgViewer->setPickRadius(static_cast<float>(static_cast<ParameterGrp&>(rCaller).GetInt("PickRadius", 5)));
        }
    }
}

void View3DOsgVerse::setCurrentViewMode(ViewMode mode)
{
    ViewMode oldmode = currentViewMode();
    if (mode == oldmode) {
        return;
    }

    if (mode == Child) {
        // When resetting to child widget, destroy the QWindow to avoid layout issues
        QWindow* winHandle = this->windowHandle();
        if (winHandle) {
            winHandle->destroy();
        }
    }

    MDIView::setCurrentViewMode(mode);

    // Set focus proxy when leaving/entering Child mode
    if (_viewer && _viewer->getWidget()) {
        if (oldmode == Child) {
            _viewer->getWidget()->setFocusProxy(this);
        }
        else if (mode == Child) {
            _viewer->getWidget()->setFocusProxy(nullptr);

            auto mdi = qobject_cast<QMdiSubWindow*>(parentWidget());
            if (mdi && mdi->layout()) {
                mdi->layout()->invalidate();
            }
        }
    }
}

void View3DOsgVerse::customEvent(QEvent* e)
{
    if (e->type() == QEvent::User) {
        auto se = static_cast<NavigationStyleEvent*>(e);
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View"
        );
        if (hGrp->GetBool("SameStyleForAllViews", true)) {
            hGrp->SetASCII("NavigationStyle", se->style().getName());
        }
        else {
            // Apply navigation style change to this view only
            auto* osgViewer = _osgViewer;
            if (osgViewer) {
                std::string styleName = se->style().getName();
                if (styleName.find("CAD") != std::string::npos) {
                    osgViewer->setNavigationStyle("CAD");
                } else if (styleName.find("Blender") != std::string::npos) {
                    osgViewer->setNavigationStyle("Blender");
                } else if (styleName.find("Inventor") != std::string::npos) {
                    osgViewer->setNavigationStyle("Inventor");
                }
            }
        }
    }
}

//===========================================================================
// Overlay widget and cursor management
//===========================================================================

void View3DOsgVerse::setOverlayWidget(QWidget* widget)
{
    removeOverlayWidget();
    _stack->addWidget(widget);
    _stack->setCurrentIndex(1);
}

void View3DOsgVerse::removeOverlayWidget()
{
    _stack->setCurrentIndex(0);
    QWidget* overlay = _stack->widget(1);
    if (overlay) {
        _stack->removeWidget(overlay);
    }
}

void View3DOsgVerse::setOverrideCursor(const QCursor& aCursor)
{
    if (_viewer) {
        _viewer->getWidget()->setCursor(aCursor);
    }
}

void View3DOsgVerse::restoreOverrideCursor()
{
    if (_viewer) {
        _viewer->getWidget()->setCursor(QCursor(Qt::ArrowCursor));
    }
}

#endif // RENDER_HAS_OSGVERSE_BACKEND
