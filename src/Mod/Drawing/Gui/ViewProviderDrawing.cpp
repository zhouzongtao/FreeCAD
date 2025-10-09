/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project                                   *
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

#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMarkerSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoText2.h>

#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Document.h>

#include "ViewProviderDrawing.h"
#include "../App/DrawingFeature.h"

using namespace DrawingGui;

// =============================================================================
// ViewProviderDrawing base class
// =============================================================================

PROPERTY_SOURCE(DrawingGui::ViewProviderDrawing, Gui::ViewProviderDocumentObject)

const char* ViewProviderDrawing::LineStyleEnums[] = {"Solid", "Dashed", "Dotted", "DashDot", nullptr};

ViewProviderDrawing::ViewProviderDrawing()
    : pcRoot(nullptr)
    , pcCoords(nullptr)
    , pcDrawStyle(nullptr)
    , pcLineSet(nullptr)
    , pcPointSet(nullptr)
    , pcLineMaterial(nullptr)
    , pcPointMaterial(nullptr)
{
    ADD_PROPERTY_TYPE(LineColor, (0.0f, 0.0f, 0.0f, 0.0f), "Display", App::Prop_None, "Color of the line");
    ADD_PROPERTY_TYPE(LineWidth, (1.0f), "Display", App::Prop_None, "Width of the line");
    ADD_PROPERTY_TYPE(LineStyle, (0L), "Display", App::Prop_None, "Style of the line");
    ADD_PROPERTY_TYPE(PointSize, (3.0f), "Display", App::Prop_None, "Size of the points");
    ADD_PROPERTY_TYPE(PointColor, (1.0f, 0.0f, 0.0f, 0.0f), "Display", App::Prop_None, "Color of the points");
    ADD_PROPERTY_TYPE(ShowPoints, (true), "Display", App::Prop_None, "Show control points");

    LineStyle.setEnums(LineStyleEnums);
}

ViewProviderDrawing::~ViewProviderDrawing() = default;

void ViewProviderDrawing::attach(App::DocumentObject* obj)
{
    ViewProviderDocumentObject::attach(obj);
    
    // Create scene graph
    pcRoot = new SoSeparator();
    pcRoot->ref();
    
    // Line material
    pcLineMaterial = new SoMaterial();
    pcRoot->addChild(pcLineMaterial);
    
    // Draw style
    pcDrawStyle = new SoDrawStyle();
    pcRoot->addChild(pcDrawStyle);
    
    // Coordinates
    pcCoords = new SoCoordinate3();
    pcRoot->addChild(pcCoords);
    
    // Line set
    pcLineSet = new SoLineSet();
    pcRoot->addChild(pcLineSet);
    
    // Point material
    pcPointMaterial = new SoMaterial();
    pcRoot->addChild(pcPointMaterial);
    
    // Point markers
    pcPointSet = new SoMarkerSet();
    pcRoot->addChild(pcPointSet);
    
    addDisplayMaskMode(pcRoot, "Standard");
    
    updateVisual();
}

std::vector<std::string> ViewProviderDrawing::getDisplayModes() const
{
    std::vector<std::string> modes;
    modes.emplace_back("Standard");
    return modes;
}

void ViewProviderDrawing::setDisplayMode(const char* ModeName)
{
    setDisplayMaskMode(ModeName);
}

bool ViewProviderDrawing::allowOverride(const App::DocumentObject&) const
{
    return true;
}

void ViewProviderDrawing::updateData(const App::Property* prop)
{
    Drawing::Feature* feature = static_cast<Drawing::Feature*>(getObject());
    
    if (prop == &feature->StartPoint || prop == &feature->EndPoint ||
        prop == &feature->LineColor || prop == &feature->LineWidth || 
        prop == &feature->LineStyle) {
        updateVisual();
    }
    
    ViewProviderDocumentObject::updateData(prop);
}

void ViewProviderDrawing::onChanged(const App::Property* prop)
{
    if (prop == &LineColor || prop == &LineWidth || prop == &LineStyle ||
        prop == &PointSize || prop == &PointColor || prop == &ShowPoints) {
        updateVisual();
    }
    
    ViewProviderDocumentObject::onChanged(prop);
}

void ViewProviderDrawing::updateVisual()
{
    if (!pcRoot)
        return;
    
    updateCoordinates();
    updateLineStyle();
    updatePointMarkers();
}

void ViewProviderDrawing::createLineVisual()
{
    // Base implementation - to be overridden by derived classes
}

void ViewProviderDrawing::updateCoordinates()
{
    if (!pcCoords)
        return;
    
    Drawing::Feature* feature = static_cast<Drawing::Feature*>(getObject());
    
    Base::Vector3d start = feature->StartPoint.getValue();
    Base::Vector3d end = feature->EndPoint.getValue();
    
    pcCoords->point.setNum(2);
    SbVec3f* points = pcCoords->point.startEditing();
    points[0].setValue(start.x, start.y, start.z);
    points[1].setValue(end.x, end.y, end.z);
    pcCoords->point.finishEditing();
}

void ViewProviderDrawing::updateLineStyle()
{
    if (!pcDrawStyle || !pcLineMaterial)
        return;
    
    // Update line width
    float lineWidth = LineWidth.getValue();
    pcDrawStyle->lineWidth.setValue(lineWidth);
    
    // Update line style
    int style = LineStyle.getValue();
    switch (style) {
        case 0: // Solid
            pcDrawStyle->linePattern.setValue(0xFFFF);
            break;
        case 1: // Dashed
            pcDrawStyle->linePattern.setValue(0xFF00);
            break;
        case 2: // Dotted
            pcDrawStyle->linePattern.setValue(0xAAAA);
            break;
        case 3: // DashDot
            pcDrawStyle->linePattern.setValue(0xFF88);
            break;
    }
    
    // Update line color
        // App::Color color = LineColor.getValue();
        // pcLineMaterial->diffuseColor.setValue(color.r, color.g, color.b);
        pcLineMaterial->diffuseColor.setValue(0.0f, 0.0f, 0.0f);
}

void ViewProviderDrawing::updatePointMarkers()
{
    if (!pcPointSet || !pcPointMaterial)
        return;
    
    if (ShowPoints.getValue()) {
        pcPointSet->markerIndex.setValue(SoMarkerSet::CIRCLE_FILLED_5_5);
        
        // App::Color pointColor = PointColor.getValue();
        // pcPointMaterial->diffuseColor.setValue(pointColor.r, pointColor.g, pointColor.b);
        pcPointMaterial->diffuseColor.setValue(1.0f, 0.0f, 0.0f);
        
        pcPointSet->numPoints.setValue(pcCoords->point.getNum());
    } else {
        pcPointSet->numPoints.setValue(0);
    }
}

bool ViewProviderDrawing::mouseMove(const SbVec2s &pos, Gui::View3DInventorViewer *viewer)
{
    Q_UNUSED(pos);
    Q_UNUSED(viewer);
    return false;
}

bool ViewProviderDrawing::mouseButtonPressed(int Button, bool pressed, const SbVec2s &pos, 
                                            const Gui::View3DInventorViewer *viewer)
{
    Q_UNUSED(Button);
    Q_UNUSED(pressed);
    Q_UNUSED(pos);
    Q_UNUSED(viewer);
    return false;
}

void ViewProviderDrawing::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    Q_UNUSED(menu);
    Q_UNUSED(receiver);
    Q_UNUSED(member);
    // Add context menu items specific to drawing objects
}

// =============================================================================
// ViewProviderLine implementation
// =============================================================================

PROPERTY_SOURCE(DrawingGui::ViewProviderLine, DrawingGui::ViewProviderDrawing)

ViewProviderLine::ViewProviderLine() = default;

void ViewProviderLine::createLineVisual()
{
    // Line-specific visual creation
    if (pcLineSet) {
        pcLineSet->numVertices.setValue(2); // Simple line with 2 vertices
    }
}

void ViewProviderLine::updateCoordinates()
{
    ViewProviderDrawing::updateCoordinates();
    
    if (pcLineSet) {
        pcLineSet->numVertices.setValue(2);
    }
}

// =============================================================================
// ViewProviderCircle implementation  
// =============================================================================

PROPERTY_SOURCE(DrawingGui::ViewProviderCircle, DrawingGui::ViewProviderDrawing)

ViewProviderCircle::ViewProviderCircle()
{
    ADD_PROPERTY_TYPE(Resolution, (32), "Display", App::Prop_None, "Number of segments for circle display");
}

void ViewProviderCircle::createLineVisual()
{
    std::vector<SbVec3f> points;
    generateCirclePoints(points);
}

void ViewProviderCircle::updateCoordinates()
{
    if (!pcCoords)
        return;
    
    std::vector<SbVec3f> points;
    generateCirclePoints(points);
    
    pcCoords->point.setNum(points.size());
    SbVec3f* coordPoints = pcCoords->point.startEditing();
    for (size_t i = 0; i < points.size(); ++i) {
        coordPoints[i] = points[i];
    }
    pcCoords->point.finishEditing();
    
    if (pcLineSet) {
        pcLineSet->numVertices.setValue(points.size());
    }
}

void ViewProviderCircle::generateCirclePoints(std::vector<SbVec3f>& points)
{
    Drawing::Circle* circle = static_cast<Drawing::Circle*>(getObject());
    
    Base::Vector3d center = circle->Center.getValue();
    double radius = circle->Radius.getValue();
    double firstAngle = circle->FirstAngle.getValue() * M_PI / 180.0;
    double lastAngle = circle->LastAngle.getValue() * M_PI / 180.0;
    int resolution = Resolution.getValue();
    
    points.clear();
    
    if (circle->isFullCircle()) {
        // Full circle
        for (int i = 0; i <= resolution; ++i) {
            double angle = 2.0 * M_PI * i / resolution;
            double x = center.x + radius * cos(angle);
            double y = center.y + radius * sin(angle);
            points.emplace_back(x, y, center.z);
        }
    } else {
        // Arc
        double angleRange = lastAngle - firstAngle;
        for (int i = 0; i <= resolution; ++i) {
            double angle = firstAngle + angleRange * i / resolution;
            double x = center.x + radius * cos(angle);
            double y = center.y + radius * sin(angle);
            points.emplace_back(x, y, center.z);
        }
    }
}
