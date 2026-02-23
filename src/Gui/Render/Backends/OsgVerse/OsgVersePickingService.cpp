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
#include "OsgVersePickingService.h"
#include "OsgVerseViewer.h"

#include <algorithm>
#include <cmath>

#include <osg/Camera>
#include <osg/Node>
#include <osg/Viewport>
#include <osgViewer/Viewer>
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/PolytopeIntersector>
#include <osgUtil/IntersectionVisitor>

#include <Base/Console.h>

FC_LOG_LEVEL_INIT("OsgVersePickingService", true, true)

namespace Gui {
namespace Render {

//=============================================================================
// Factory Registration
//=============================================================================

// Note: OsgVerse picking service is registered on-demand when OsgVerseGui
// module is loaded, not via static initialization. This avoids DLL load
// order issues on Windows.

//=============================================================================
// OsgVersePickingService Implementation
//=============================================================================

OsgVersePickingService::OsgVersePickingService() = default;

OsgVersePickingService::OsgVersePickingService(View3DInventorViewer* viewer)
    : _viewer(viewer)
{
}

OsgVersePickingService::~OsgVersePickingService() = default;

void OsgVersePickingService::setViewer(View3DInventorViewer* viewer)
{
    _viewer = viewer;
}

void OsgVersePickingService::setOsgVerseViewer(OsgVerseViewer* viewer)
{
    _osgViewer = viewer;
}

PickResults OsgVersePickingService::pick(int screenX, int screenY,
                                          const PickOptions& options)
{
    PickResults results;
    results.screenX = screenX;
    results.screenY = screenY;

    osg::Camera* camera = getCamera();
    osg::Node* sceneRoot = getSceneRoot();

    if (!camera || !sceneRoot) {
        FC_WARN("OsgVersePickingService::pick: No camera or scene root");
        return results;
    }

    // Generate ray for results
    PickRay ray = generateRay(screenX, screenY);
    results.rayOrigin = ray.origin;
    results.rayDirection = ray.direction;

    // Convert screen coordinates to NDC
    double ndcX, ndcY;
    screenToNDC(screenX, screenY, ndcX, ndcY);

    // Create line segment intersector
    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector =
        new osgUtil::LineSegmentIntersector(
            osgUtil::Intersector::WINDOW,
            static_cast<double>(screenX),
            static_cast<double>(screenY)
        );

    // Configure intersector
    osgUtil::IntersectionVisitor iv(intersector.get());

    // Apply to scene
    camera->accept(iv);

    if (!intersector->containsIntersections()) {
        return results;
    }

    // Process intersections
    const osgUtil::LineSegmentIntersector::Intersections& intersections =
        intersector->getIntersections();

    int hitCount = 0;
    for (const auto& intersection : intersections) {
        if (hitCount >= options.maxHits) {
            break;
        }

        PickResult result;
        result.hit = true;

        // Get intersection point
        const osg::Vec3d& point = intersection.getWorldIntersectPoint();
        result.point = Vector3(
            static_cast<float>(point.x()),
            static_cast<float>(point.y()),
            static_cast<float>(point.z())
        );

        // Get normal
        const osg::Vec3& normal = intersection.getWorldIntersectNormal();
        result.normal = Vector3(normal.x(), normal.y(), normal.z());

        // Calculate distance from ray origin
        osg::Vec3d diff = point - osg::Vec3d(ray.origin.x, ray.origin.y, ray.origin.z);
        result.distance = static_cast<float>(diff.length());

        // Determine pick type from primitive index
        if (intersection.indexList.size() >= 3) {
            result.type = PickType::Face;
            result.faceIndex = static_cast<int>(intersection.primitiveIndex);
        } else if (intersection.indexList.size() == 2) {
            result.type = PickType::Edge;
            result.edgeIndex = static_cast<int>(intersection.primitiveIndex);
        } else if (intersection.indexList.size() == 1) {
            result.type = PickType::Vertex;
            result.vertexIndex = static_cast<int>(intersection.indexList[0]);
        }

        // Check pick type filters
        if (result.type == PickType::Face && !options.pickFaces) continue;
        if (result.type == PickType::Edge && !options.pickEdges) continue;
        if (result.type == PickType::Vertex && !options.pickVertices) continue;

        // Resolve ViewProvider from the node path
        if (_osgViewer && !intersection.nodePath.empty()) {
            for (auto it = intersection.nodePath.rbegin(); it != intersection.nodePath.rend(); ++it) {
                Gui::ViewProvider* vp = _osgViewer->findViewProviderForNode(*it);
                if (vp) {
                    result.viewProvider = vp;
                    // Build sub-element name from node name if available
                    osg::Node* hitNode = intersection.nodePath.back();
                    if (hitNode && !hitNode->getName().empty()) {
                        std::string nodeName = hitNode->getName();
                        if (nodeName.find("Face") == 0 || nodeName.find("Edge") == 0 ||
                            nodeName.find("Vertex") == 0) {
                            result.elementName = nodeName;
                        }
                    }
                    // Generate element name from pick type if not set
                    if (result.elementName.empty()) {
                        if (result.type == PickType::Face && result.faceIndex >= 0) {
                            result.elementName = "Face" + std::to_string(result.faceIndex + 1);
                        } else if (result.type == PickType::Edge && result.edgeIndex >= 0) {
                            result.elementName = "Edge" + std::to_string(result.edgeIndex + 1);
                        } else if (result.type == PickType::Vertex && result.vertexIndex >= 0) {
                            result.elementName = "Vertex" + std::to_string(result.vertexIndex + 1);
                        }
                    }
                    break;
                }
            }
        }

        results.hits.push_back(result);
        ++hitCount;

        if (options.pickClosestOnly) {
            break;
        }
    }

    // Sort by distance and mark closest
    std::sort(results.hits.begin(), results.hits.end());
    if (!results.hits.empty()) {
        results.hits[0].isClosest = true;
    }

    return results;
}

PickResults OsgVersePickingService::pickRay(const PickRay& ray,
                                             const PickOptions& options)
{
    PickResults results;
    results.rayOrigin = ray.origin;
    results.rayDirection = ray.direction;

    osg::Camera* camera = getCamera();
    osg::Node* sceneRoot = getSceneRoot();

    if (!camera || !sceneRoot) {
        return results;
    }

    // Create line segment from ray
    osg::Vec3d start(ray.origin.x, ray.origin.y, ray.origin.z);
    osg::Vec3d dir(ray.direction.x, ray.direction.y, ray.direction.z);
    dir.normalize();
    osg::Vec3d end = start + dir * static_cast<double>(ray.farDistance);

    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector =
        new osgUtil::LineSegmentIntersector(start, end);

    osgUtil::IntersectionVisitor iv(intersector.get());
    sceneRoot->accept(iv);

    if (!intersector->containsIntersections()) {
        return results;
    }

    // Process intersections
    const osgUtil::LineSegmentIntersector::Intersections& intersections =
        intersector->getIntersections();

    int hitCount = 0;
    for (const auto& intersection : intersections) {
        if (hitCount >= options.maxHits) {
            break;
        }

        PickResult result;
        result.hit = true;

        const osg::Vec3d& point = intersection.getWorldIntersectPoint();
        result.point = Vector3(
            static_cast<float>(point.x()),
            static_cast<float>(point.y()),
            static_cast<float>(point.z())
        );

        const osg::Vec3& normal = intersection.getWorldIntersectNormal();
        result.normal = Vector3(normal.x(), normal.y(), normal.z());

        osg::Vec3d diff = point - start;
        result.distance = static_cast<float>(diff.length());

        results.hits.push_back(result);
        ++hitCount;

        if (options.pickClosestOnly) {
            break;
        }
    }

    std::sort(results.hits.begin(), results.hits.end());
    if (!results.hits.empty()) {
        results.hits[0].isClosest = true;
    }

    return results;
}

PickResults OsgVersePickingService::pickRegion(int x1, int y1, int x2, int y2,
                                                const PickOptions& /*options*/)
{
    PickResults results;

    osg::Camera* camera = getCamera();
    osg::Node* sceneRoot = getSceneRoot();

    if (!camera || !sceneRoot) {
        return results;
    }

    // Use polytope intersector for region picking
    // Normalize coordinates
    double minX = std::min(x1, x2);
    double maxX = std::max(x1, x2);
    double minY = std::min(y1, y2);
    double maxY = std::max(y1, y2);

    osg::ref_ptr<osgUtil::PolytopeIntersector> intersector =
        new osgUtil::PolytopeIntersector(
            osgUtil::Intersector::WINDOW,
            minX, minY, maxX, maxY
        );

    osgUtil::IntersectionVisitor iv(intersector.get());
    camera->accept(iv);

    if (!intersector->containsIntersections()) {
        return results;
    }

    // Process intersections
    const osgUtil::PolytopeIntersector::Intersections& intersections =
        intersector->getIntersections();

    for (const auto& intersection : intersections) {
        PickResult result;
        result.hit = true;

        // Get center point of intersection - use first point from intersection
        // Note: PolytopeIntersector may have multiple intersection points
        osg::Vec3d point(0, 0, 0);
        if (intersection.numIntersectionPoints > 0) {
            point = intersection.intersectionPoints[0];
        }

        result.point = Vector3(
            static_cast<float>(point.x()),
            static_cast<float>(point.y()),
            static_cast<float>(point.z())
        );

        result.distance = static_cast<float>(intersection.distance);

        results.hits.push_back(result);
    }

    return results;
}

PickRay OsgVersePickingService::generateRay(int screenX, int screenY)
{
    PickRay ray;

    osg::Camera* camera = getCamera();
    if (!camera) {
        return ray;
    }

    // Get viewport dimensions
    int width, height;
    getViewportSize(width, height);

    if (width <= 0 || height <= 0) {
        return ray;
    }

    // Convert screen to NDC (OpenGL convention: origin at bottom-left)
    double ndcX = (2.0 * screenX / width) - 1.0;
    double ndcY = 1.0 - (2.0 * screenY / height);  // Flip Y

    // Get camera matrices
    osg::Matrixd viewMatrix = camera->getViewMatrix();
    osg::Matrixd projMatrix = camera->getProjectionMatrix();
    osg::Matrixd invVP = osg::Matrixd::inverse(viewMatrix * projMatrix);

    // Compute near and far points
    osg::Vec3d nearPoint = osg::Vec3d(ndcX, ndcY, -1.0) * invVP;
    osg::Vec3d farPoint = osg::Vec3d(ndcX, ndcY, 1.0) * invVP;

    osg::Vec3d direction = farPoint - nearPoint;
    direction.normalize();

    ray.origin = Vector3(
        static_cast<float>(nearPoint.x()),
        static_cast<float>(nearPoint.y()),
        static_cast<float>(nearPoint.z())
    );

    ray.direction = Vector3(
        static_cast<float>(direction.x()),
        static_cast<float>(direction.y()),
        static_cast<float>(direction.z())
    );

    // Get near/far from projection matrix
    double fovy, aspectRatio, zNear, zFar;
    if (projMatrix.getPerspective(fovy, aspectRatio, zNear, zFar)) {
        ray.nearDistance = static_cast<float>(zNear);
        ray.farDistance = static_cast<float>(zFar);
    } else {
        ray.nearDistance = 0.1f;
        ray.farDistance = 10000.0f;
    }

    return ray;
}

bool OsgVersePickingService::projectToScreen(const Vector3& worldPoint,
                                              int& screenX, int& screenY)
{
    osg::Camera* camera = getCamera();
    if (!camera) {
        return false;
    }

    int width, height;
    getViewportSize(width, height);

    if (width <= 0 || height <= 0) {
        return false;
    }

    // Get camera matrices
    osg::Matrixd viewMatrix = camera->getViewMatrix();
    osg::Matrixd projMatrix = camera->getProjectionMatrix();
    osg::Matrixd vpMatrix = viewMatrix * projMatrix;

    // Transform world point to clip coordinates
    osg::Vec4d worldVec(worldPoint.x, worldPoint.y, worldPoint.z, 1.0);
    osg::Vec4d clipVec = worldVec * vpMatrix;

    // Perspective divide
    if (std::abs(clipVec.w()) < 1e-6) {
        return false;
    }

    double ndcX = clipVec.x() / clipVec.w();
    double ndcY = clipVec.y() / clipVec.w();
    double ndcZ = clipVec.z() / clipVec.w();

    // Check if point is behind camera or outside view frustum
    if (ndcZ < -1.0 || ndcZ > 1.0) {
        return false;
    }

    // Convert NDC to screen coordinates
    screenX = static_cast<int>((ndcX + 1.0) * 0.5 * width);
    screenY = static_cast<int>((1.0 - ndcY) * 0.5 * height);  // Flip Y

    return true;
}

bool OsgVersePickingService::unprojectToPlane(int screenX, int screenY,
                                               const Vector3& planePoint,
                                               const Vector3& planeNormal,
                                               Vector3& worldPoint)
{
    PickRay ray = generateRay(screenX, screenY);

    // Ray-plane intersection
    // t = (planePoint - rayOrigin) . planeNormal / (rayDirection . planeNormal)
    float denom = ray.direction.x * planeNormal.x +
                  ray.direction.y * planeNormal.y +
                  ray.direction.z * planeNormal.z;

    if (std::abs(denom) < 1e-6f) {
        // Ray parallel to plane
        return false;
    }

    float t = ((planePoint.x - ray.origin.x) * planeNormal.x +
               (planePoint.y - ray.origin.y) * planeNormal.y +
               (planePoint.z - ray.origin.z) * planeNormal.z) / denom;

    if (t < 0) {
        // Intersection behind camera
        return false;
    }

    worldPoint = ray.getPoint(t);
    return true;
}

osg::Camera* OsgVersePickingService::getCamera() const
{
    if (_osgViewer) {
        osgViewer::Viewer* viewer = _osgViewer->getOsgViewer();
        if (viewer) {
            return viewer->getCamera();
        }
    }
    return nullptr;
}

osg::Node* OsgVersePickingService::getSceneRoot() const
{
    if (_osgViewer) {
        osgViewer::Viewer* viewer = _osgViewer->getOsgViewer();
        if (viewer) {
            return viewer->getSceneData();
        }
    }
    return nullptr;
}

void OsgVersePickingService::screenToNDC(int screenX, int screenY,
                                          double& ndcX, double& ndcY) const
{
    int width, height;
    getViewportSize(width, height);

    if (width > 0 && height > 0) {
        ndcX = (2.0 * screenX / width) - 1.0;
        ndcY = 1.0 - (2.0 * screenY / height);
    } else {
        ndcX = 0.0;
        ndcY = 0.0;
    }
}

void OsgVersePickingService::getViewportSize(int& width, int& height) const
{
    width = 0;
    height = 0;

    osg::Camera* camera = getCamera();
    if (camera) {
        const osg::Viewport* viewport = camera->getViewport();
        if (viewport) {
            width = static_cast<int>(viewport->width());
            height = static_cast<int>(viewport->height());
        }
    }
}

void OsgVersePickingService::registerFactory()
{
    PickingServiceRegistry::instance().registerFactory(
        BackendType::OsgVerse,
        [](View3DInventorViewer* viewer) -> PickingService::Ptr {
            return std::make_shared<OsgVersePickingService>(viewer);
        }
    );
}

} // namespace Render
} // namespace Gui
