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
#include "Coin3DPickingService.h"

#include <algorithm>

#include <Inventor/SbViewVolume.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SoPath.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/nodes/SoCamera.h>

#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/Selection/SoFCSelection.h>

#include <Base/Console.h>

FC_LOG_LEVEL_INIT("Coin3DPickingService", true, true)

namespace Gui {
namespace Render {

//=============================================================================
// Static Factory Registration
//=============================================================================

namespace {
// Auto-register Coin3D picking service factory
struct Coin3DPickingServiceRegistrar {
    Coin3DPickingServiceRegistrar() {
        Coin3DPickingService::registerFactory();
    }
};
static Coin3DPickingServiceRegistrar coin3dPickingRegistrar;
}

//=============================================================================
// Coin3DPickingService Implementation
//=============================================================================

Coin3DPickingService::Coin3DPickingService() = default;

Coin3DPickingService::Coin3DPickingService(View3DInventorViewer* viewer)
    : _viewer(viewer)
{
}

Coin3DPickingService::~Coin3DPickingService() = default;

void Coin3DPickingService::setViewer(View3DInventorViewer* viewer)
{
    _viewer = viewer;
}

PickResults Coin3DPickingService::pick(int screenX, int screenY,
                                        const PickOptions& options)
{
    PickResults results;
    results.screenX = screenX;
    results.screenY = screenY;

    if (!_viewer) {
        return results;
    }

    // Get the scene graph root
    SoNode* sceneRoot = _viewer->getSceneGraph();
    if (!sceneRoot) {
        return results;
    }

    // Get viewport info
    const SbViewportRegion& vp = _viewer->getSoRenderManager()->getViewportRegion();

    // Create ray pick action
    SoRayPickAction pickAction(vp);

    // Set up pick parameters
    if (options.pickClosestOnly) {
        pickAction.setPickAll(false);
    } else {
        pickAction.setPickAll(true);
    }

    pickAction.setRadius(options.pickRadius);

    // Set the pick point (convert from screen to normalized coordinates)
    // Note: Coin3D uses bottom-left as origin for setPoint
    SbVec2s viewportSize = vp.getViewportSizePixels();
    int adjustedY = viewportSize[1] - screenY - 1;
    pickAction.setPoint(SbVec2s(static_cast<short>(screenX),
                                 static_cast<short>(adjustedY)));

    // Generate the ray for results
    PickRay ray = generateRay(screenX, screenY);
    results.rayOrigin = ray.origin;
    results.rayDirection = ray.direction;

    // Apply the action
    pickAction.apply(sceneRoot);

    // Process results
    const SoPickedPointList& pickedPoints = pickAction.getPickedPointList();
    int numHits = std::min(static_cast<int>(pickedPoints.getLength()), options.maxHits);

    for (int i = 0; i < numHits; ++i) {
        SoPickedPoint* pp = pickedPoints[i];
        if (!pp) continue;

        PickResult result = processPickedPoint(pp);

        // Apply filters
        if (options.pickVisibleOnly && result.viewProvider) {
            if (!result.viewProvider->isVisible()) {
                continue;
            }
        }

        if (options.pickSelectableOnly && result.viewProvider) {
            if (!result.viewProvider->isSelectable()) {
                continue;
            }
        }

        if (!options.filterViewProviders.empty() && result.viewProvider) {
            if (std::find(options.filterViewProviders.begin(),
                          options.filterViewProviders.end(),
                          result.viewProvider) == options.filterViewProviders.end()) {
                continue;
            }
        }

        // Check pick type filters
        if (result.type == PickType::Face && !options.pickFaces) continue;
        if (result.type == PickType::Edge && !options.pickEdges) continue;
        if (result.type == PickType::Vertex && !options.pickVertices) continue;

        results.hits.push_back(result);
    }

    // Sort by distance and mark closest
    std::sort(results.hits.begin(), results.hits.end());
    if (!results.hits.empty()) {
        results.hits[0].isClosest = true;
    }

    return results;
}

PickResults Coin3DPickingService::pickRay(const PickRay& ray,
                                           const PickOptions& options)
{
    PickResults results;
    results.rayOrigin = ray.origin;
    results.rayDirection = ray.direction;

    if (!_viewer) {
        return results;
    }

    SoNode* sceneRoot = _viewer->getSceneGraph();
    if (!sceneRoot) {
        return results;
    }

    const SbViewportRegion& vp = _viewer->getSoRenderManager()->getViewportRegion();

    SoRayPickAction pickAction(vp);
    pickAction.setPickAll(!options.pickClosestOnly);
    pickAction.setRadius(options.pickRadius);

    // Set ray directly
    SbVec3f origin(ray.origin.x, ray.origin.y, ray.origin.z);
    SbVec3f direction(ray.direction.x, ray.direction.y, ray.direction.z);
    pickAction.setRay(origin, direction, ray.nearDistance, ray.farDistance);

    pickAction.apply(sceneRoot);

    // Process results (similar to pick())
    const SoPickedPointList& pickedPoints = pickAction.getPickedPointList();
    int numHits = std::min(static_cast<int>(pickedPoints.getLength()), options.maxHits);

    for (int i = 0; i < numHits; ++i) {
        SoPickedPoint* pp = pickedPoints[i];
        if (!pp) continue;

        PickResult result = processPickedPoint(pp);
        results.hits.push_back(result);
    }

    std::sort(results.hits.begin(), results.hits.end());
    if (!results.hits.empty()) {
        results.hits[0].isClosest = true;
    }

    return results;
}

PickResults Coin3DPickingService::pickRegion(int x1, int y1, int x2, int y2,
                                              const PickOptions& /*options*/)
{
    PickResults results;

    if (!_viewer) {
        return results;
    }

    // TODO: Implement region picking using SoBoxHighlightRenderAction
    // or multiple ray picks

    FC_WARN("Coin3DPickingService::pickRegion: Not fully implemented yet");

    // For now, return empty results
    // A proper implementation would use selection lasso or box selection

    return results;
}

PickRay Coin3DPickingService::generateRay(int screenX, int screenY)
{
    PickRay ray;

    if (!_viewer) {
        return ray;
    }

    SoCamera* camera = _viewer->getSoRenderManager()->getCamera();
    if (!camera) {
        return ray;
    }

    const SbViewportRegion& vp = _viewer->getSoRenderManager()->getViewportRegion();
    SbVec2s viewportSize = vp.getViewportSizePixels();

    // Convert screen to normalized device coordinates
    float ndcX = static_cast<float>(screenX) / viewportSize[0];
    float ndcY = 1.0f - static_cast<float>(screenY) / viewportSize[1];

    // Get view volume
    SbViewVolume viewVolume = camera->getViewVolume(vp.getViewportAspectRatio());

    // Get ray from view volume
    SbVec3f start, dir;
    viewVolume.projectPointToLine(SbVec2f(ndcX, ndcY), start, dir);

    ray.origin = Vector3{start[0], start[1], start[2]};
    ray.direction = Vector3{dir[0], dir[1], dir[2]};
    ray.nearDistance = viewVolume.getNearDist();
    ray.farDistance = viewVolume.getNearDist() + viewVolume.getDepth();

    return ray;
}

bool Coin3DPickingService::projectToScreen(const Vector3& worldPoint,
                                            int& screenX, int& screenY)
{
    if (!_viewer) {
        return false;
    }

    SoCamera* camera = _viewer->getSoRenderManager()->getCamera();
    if (!camera) {
        return false;
    }

    const SbViewportRegion& vp = _viewer->getSoRenderManager()->getViewportRegion();
    SbVec2s viewportSize = vp.getViewportSizePixels();

    SbViewVolume viewVolume = camera->getViewVolume(vp.getViewportAspectRatio());

    SbVec3f point(worldPoint.x, worldPoint.y, worldPoint.z);
    SbVec3f screenPoint;

    viewVolume.projectToScreen(point, screenPoint);

    // Check if point is in front of camera
    if (screenPoint[2] < 0 || screenPoint[2] > 1) {
        return false;
    }

    screenX = static_cast<int>(screenPoint[0] * viewportSize[0]);
    screenY = static_cast<int>((1.0f - screenPoint[1]) * viewportSize[1]);

    return true;
}

bool Coin3DPickingService::unprojectToPlane(int screenX, int screenY,
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

PickResult Coin3DPickingService::processPickedPoint(SoPickedPoint* pickedPoint)
{
    PickResult result;
    result.hit = true;

    // Get intersection point
    SbVec3f point = pickedPoint->getPoint();
    result.point = Vector3{point[0], point[1], point[2]};

    // Get normal
    SbVec3f normal = pickedPoint->getNormal();
    result.normal = Vector3{normal[0], normal[1], normal[2]};

    // Get path and find ViewProvider
    SoPath* path = pickedPoint->getPath();
    result.viewProvider = findViewProvider(path);

    // Extract element name
    result.elementName = extractElementName(pickedPoint);

    // Determine pick type from detail
    const SoDetail* detail = pickedPoint->getDetail();
    if (detail) {
        if (detail->isOfType(SoFaceDetail::getClassTypeId())) {
            result.type = PickType::Face;
            auto* faceDetail = static_cast<const SoFaceDetail*>(detail);
            result.faceIndex = faceDetail->getFaceIndex();
        }
        else if (detail->isOfType(SoLineDetail::getClassTypeId())) {
            result.type = PickType::Edge;
            auto* lineDetail = static_cast<const SoLineDetail*>(detail);
            result.edgeIndex = lineDetail->getLineIndex();
        }
        else if (detail->isOfType(SoPointDetail::getClassTypeId())) {
            result.type = PickType::Vertex;
            auto* pointDetail = static_cast<const SoPointDetail*>(detail);
            result.vertexIndex = pointDetail->getCoordinateIndex();
        }
    }

    // Calculate distance (approximate from camera)
    if (_viewer) {
        SoCamera* camera = _viewer->getSoRenderManager()->getCamera();
        if (camera) {
            SbVec3f camPos;
            camera->position.getValue().getValue(camPos[0], camPos[1], camPos[2]);
            SbVec3f diff = point - camPos;
            result.distance = diff.length();
        }
    }

    return result;
}

ViewProvider* Coin3DPickingService::findViewProvider(SoPath* path)
{
    if (!path || !_viewer) {
        return nullptr;
    }

    // Walk up the path looking for SoFCSelection node
    for (int i = path->getLength() - 1; i >= 0; --i) {
        SoNode* node = path->getNode(i);
        if (node && node->isOfType(SoFCSelection::getClassTypeId())) {
            auto* fcsel = static_cast<SoFCSelection*>(node);
            // Get ViewProvider from SoFCSelection's user data or document
            // This is a simplified lookup - actual implementation may need
            // to query the Document
            const char* docName = fcsel->documentName.getValue().getString();
            const char* objName = fcsel->objectName.getValue().getString();

            if (docName && objName) {
                // Find ViewProvider through FreeCAD's Application
                // For now, iterate through viewer's ViewProviders
                for (auto* vp : _viewer->getViewProvidersOfType(ViewProvider::getClassTypeId())) {
                    // Match by object name - need to cast to ViewProviderDocumentObject
                    auto* vpDoc = dynamic_cast<ViewProviderDocumentObject*>(vp);
                    if (vpDoc) {
                        auto* obj = vpDoc->getObject();
                        if (obj && strcmp(obj->getNameInDocument(), objName) == 0) {
                            return vp;
                        }
                    }
                }
            }
        }
    }

    return nullptr;
}

std::string Coin3DPickingService::extractElementName(SoPickedPoint* pickedPoint)
{
    std::string elementName;

    // Try to get element name from path
    SoPath* path = pickedPoint->getPath();
    if (path) {
        for (int i = path->getLength() - 1; i >= 0; --i) {
            SoNode* node = path->getNode(i);
            if (node && node->isOfType(SoFCSelection::getClassTypeId())) {
                auto* fcsel = static_cast<SoFCSelection*>(node);
                const char* subName = fcsel->subElementName.getValue().getString();
                if (subName && strlen(subName) > 0) {
                    elementName = subName;
                    break;
                }
            }
        }
    }

    // If no element name from path, generate from detail
    if (elementName.empty()) {
        const SoDetail* detail = pickedPoint->getDetail();
        if (detail) {
            if (detail->isOfType(SoFaceDetail::getClassTypeId())) {
                auto* faceDetail = static_cast<const SoFaceDetail*>(detail);
                elementName = "Face" + std::to_string(faceDetail->getFaceIndex() + 1);
            }
            else if (detail->isOfType(SoLineDetail::getClassTypeId())) {
                auto* lineDetail = static_cast<const SoLineDetail*>(detail);
                elementName = "Edge" + std::to_string(lineDetail->getLineIndex() + 1);
            }
            else if (detail->isOfType(SoPointDetail::getClassTypeId())) {
                auto* pointDetail = static_cast<const SoPointDetail*>(detail);
                elementName = "Vertex" + std::to_string(pointDetail->getCoordinateIndex() + 1);
            }
        }
    }

    return elementName;
}

void Coin3DPickingService::registerFactory()
{
    PickingServiceRegistry::instance().registerFactory(
        BackendType::Coin3D,
        [](View3DInventorViewer* viewer) -> PickingService::Ptr {
            return std::make_shared<Coin3DPickingService>(viewer);
        }
    );
}

} // namespace Render
} // namespace Gui
