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

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/GeometryPyCXX.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/PartFeature.h>

#include "DrawingFeature.h"
// #include "DrawingFeaturePy.h" // Temporarily disabled

using namespace Drawing;
using namespace App;

// =============================================================================
// Feature base class implementation
// =============================================================================

PROPERTY_SOURCE(Drawing::Feature, App::DocumentObject)

const char* Feature::LineStyleEnums[] = {"Solid", "Dashed", "Dotted", "DashDot", nullptr};

Feature::Feature()
{
    ADD_PROPERTY_TYPE(StartPoint, (Base::Vector3d(0,0,0)), "Geometry", Prop_None, "Start point of the drawing object");
    ADD_PROPERTY_TYPE(EndPoint, (Base::Vector3d(0,0,0)), "Geometry", Prop_None, "End point of the drawing object");
    ADD_PROPERTY_TYPE(LineWidth, (1.0f), "Appearance", Prop_None, "Width of the line");
    ADD_PROPERTY_TYPE(LineColor, (0.0f, 0.0f, 0.0f, 0.0f), "Appearance", Prop_None, "Color of the line");
    ADD_PROPERTY_TYPE(LineStyle, (0L), "Appearance", Prop_None, "Style of the line");
    ADD_PROPERTY_TYPE(Construction, (false), "Geometry", Prop_None, "Construction geometry flag");

    LineStyle.setEnums(LineStyleEnums);
}

Feature::~Feature() = default;

short Feature::mustExecute() const
{
    return DocumentObject::mustExecute();
}

App::DocumentObjectExecReturn* Feature::execute()
{
    return recompute();
}

App::DocumentObjectExecReturn* Feature::recompute()
{
    return DocumentObject::execute();
}

const char* Feature::getViewProviderName() const
{
    return "DrawingGui::ViewProviderDrawing";
}

PyObject* Feature::getPyObject()
{
    // Temporarily return DocumentObject's Python object until bindings are fixed
    return DocumentObject::getPyObject();
}

void Feature::onChanged(const App::Property* prop)
{
    if (prop == &StartPoint || prop == &EndPoint || 
        prop == &LineWidth || prop == &LineColor || prop == &LineStyle) {
        // Mark for recomputation when geometry or appearance changes
        touch();
    }
    DocumentObject::onChanged(prop);
}

// =============================================================================
// Line class implementation
// =============================================================================

PROPERTY_SOURCE(Drawing::Line, Drawing::Feature)

Line::Line()
{
    ADD_PROPERTY_TYPE(Length, (10.0), "Dimensions", Prop_None, "Length of the line");
    ADD_PROPERTY_TYPE(Angle, (0.0), "Dimensions", Prop_None, "Angle of the line in degrees");
}

App::DocumentObjectExecReturn* Line::execute()
{
    calculateGeometry();
    return Feature::execute();
}

const char* Line::getViewProviderName() const
{
    return "DrawingGui::ViewProviderLine";
}

void Line::calculateGeometry()
{
    Base::Vector3d start = StartPoint.getValue();
    double length = Length.getValue();
    double angle = Angle.getValue() * M_PI / 180.0; // Convert to radians
    
    Base::Vector3d direction(cos(angle), sin(angle), 0.0);
    Base::Vector3d end = start + direction * length;
    
    EndPoint.setValue(end);
}

Base::Vector3d Line::getDirection() const
{
    Base::Vector3d start = StartPoint.getValue();
    Base::Vector3d end = EndPoint.getValue();
    Base::Vector3d dir = end - start;
    return dir.Normalize();
}

// =============================================================================
// Circle class implementation
// =============================================================================

PROPERTY_SOURCE(Drawing::Circle, Drawing::Feature)

Circle::Circle()
{
    ADD_PROPERTY_TYPE(Center, (Base::Vector3d(0,0,0)), "Geometry", Prop_None, "Center point of the circle");
    ADD_PROPERTY_TYPE(Radius, (5.0), "Dimensions", Prop_None, "Radius of the circle");
    ADD_PROPERTY_TYPE(FirstAngle, (0.0), "Dimensions", Prop_None, "First angle of arc in degrees");
    ADD_PROPERTY_TYPE(LastAngle, (360.0), "Dimensions", Prop_None, "Last angle of arc in degrees");
}

App::DocumentObjectExecReturn* Circle::execute()
{
    calculateGeometry();
    return Feature::execute();
}

const char* Circle::getViewProviderName() const
{
    return "DrawingGui::ViewProviderCircle";
}

void Circle::calculateGeometry()
{
    Base::Vector3d center = Center.getValue();
    double radius = Radius.getValue();
    
    // Update start and end points for arc
    double firstAngle = FirstAngle.getValue() * M_PI / 180.0;
    double lastAngle = LastAngle.getValue() * M_PI / 180.0;
    
    Base::Vector3d start(center.x + radius * cos(firstAngle),
                        center.y + radius * sin(firstAngle),
                        center.z);
    Base::Vector3d end(center.x + radius * cos(lastAngle),
                      center.y + radius * sin(lastAngle),
                      center.z);
    
    StartPoint.setValue(start);
    EndPoint.setValue(end);
}

bool Circle::isFullCircle() const
{
    double firstAngle = FirstAngle.getValue();
    double lastAngle = LastAngle.getValue();
    return fabs(lastAngle - firstAngle - 360.0) < 1e-6;
}

// =============================================================================
// Rectangle class implementation
// =============================================================================

PROPERTY_SOURCE(Drawing::Rectangle, Drawing::Feature)

Rectangle::Rectangle()
{
    ADD_PROPERTY_TYPE(Width, (10.0), "Dimensions", Prop_None, "Width of the rectangle");
    ADD_PROPERTY_TYPE(Height, (5.0), "Dimensions", Prop_None, "Height of the rectangle");
    ADD_PROPERTY_TYPE(Rounded, (false), "Geometry", Prop_None, "Create rounded corners");
    ADD_PROPERTY_TYPE(CornerRadius, (1.0), "Dimensions", Prop_None, "Radius of rounded corners");
}

App::DocumentObjectExecReturn* Rectangle::execute()
{
    calculateGeometry();
    return Feature::execute();
}

const char* Rectangle::getViewProviderName() const
{
    return "DrawingGui::ViewProviderRectangle";
}

void Rectangle::calculateGeometry()
{
    Base::Vector3d start = StartPoint.getValue();
    double width = Width.getValue();
    double height = Height.getValue();
    
    Base::Vector3d end(start.x + width, start.y + height, start.z);
    EndPoint.setValue(end);
}

// =============================================================================
// Polygon class implementation
// =============================================================================

PROPERTY_SOURCE(Drawing::Polygon, Drawing::Feature)

Polygon::Polygon()
{
    ADD_PROPERTY_TYPE(Points, (), "Geometry", Prop_None, "Points defining the polygon");
    ADD_PROPERTY_TYPE(Sides, (6), "Dimensions", Prop_None, "Number of sides for regular polygon");
    ADD_PROPERTY_TYPE(Radius, (5.0), "Dimensions", Prop_None, "Radius for regular polygon");
    ADD_PROPERTY_TYPE(Closed, (true), "Geometry", Prop_None, "Whether polygon is closed");
}

App::DocumentObjectExecReturn* Polygon::execute()
{
    calculateGeometry();
    return Feature::execute();
}

const char* Polygon::getViewProviderName() const
{
    return "DrawingGui::ViewProviderPolygon";
}

void Polygon::calculateGeometry()
{
    const std::vector<Base::Vector3d>& points = Points.getValues();
    if (!points.empty()) {
        StartPoint.setValue(points.front());
        EndPoint.setValue(points.back());
    }
}

void Polygon::addPoint(const Base::Vector3d& point)
{
    std::vector<Base::Vector3d> points = Points.getValues();
    points.push_back(point);
    Points.setValues(points);
}

void Polygon::removePoint(int index)
{
    std::vector<Base::Vector3d> points = Points.getValues();
    if (index >= 0 && index < static_cast<int>(points.size())) {
        points.erase(points.begin() + index);
        Points.setValues(points);
    }
}

// =============================================================================
// Text class implementation
// =============================================================================

PROPERTY_SOURCE(Drawing::Text, Drawing::Feature)

const char* Text::JustificationEnums[] = {"Left", "Center", "Right", nullptr};

Text::Text()
{
    ADD_PROPERTY_TYPE(TextString, ("Text"), "Content", Prop_None, "Text content");
    ADD_PROPERTY_TYPE(FontName, ("Arial"), "Font", Prop_None, "Font name");
    ADD_PROPERTY_TYPE(FontSize, (12.0f), "Font", Prop_None, "Font size");
    ADD_PROPERTY_TYPE(Position, (Base::Vector3d(0,0,0)), "Placement", Prop_None, "Text position");
    ADD_PROPERTY_TYPE(Rotation, (0.0), "Placement", Prop_None, "Text rotation angle");
    ADD_PROPERTY_TYPE(Justification, (0L), "Font", Prop_None, "Text justification");

    Justification.setEnums(JustificationEnums);
}

App::DocumentObjectExecReturn* Text::execute()
{
    calculateGeometry();
    return Feature::execute();
}

const char* Text::getViewProviderName() const
{
    return "DrawingGui::ViewProviderText";
}

void Text::calculateGeometry()
{
    Base::Vector3d pos = Position.getValue();
    StartPoint.setValue(pos);
    EndPoint.setValue(pos); // Text has no end point
}

// =============================================================================
// Dimension class implementation
// =============================================================================

PROPERTY_SOURCE(Drawing::Dimension, Drawing::Feature)

Dimension::Dimension()
{
    // PropertyLinkSub initialization - leave empty for now
    // ADD_PROPERTY_TYPE(First, (), "References", Prop_None, "First measurement point");
    // ADD_PROPERTY_TYPE(Second, (), "References", Prop_None, "Second measurement point");
    ADD_PROPERTY_TYPE(DimLinePosition, (Base::Vector3d(0,0,0)), "Dimension", Prop_None, "Position of dimension line");
    ADD_PROPERTY_TYPE(FormatSpec, ("%.2f"), "Dimension", Prop_None, "Format specification for dimension text");
    ADD_PROPERTY_TYPE(TextSize, (3.0f), "Dimension", Prop_None, "Size of dimension text");
    ADD_PROPERTY_TYPE(ShowUnits, (true), "Dimension", Prop_None, "Show measurement units");
}

App::DocumentObjectExecReturn* Dimension::execute()
{
    updateDimensionText();
    return Feature::execute();
}

const char* Dimension::getViewProviderName() const
{
    return "DrawingGui::ViewProviderDimension";
}

double Dimension::calculateDimension()
{
    // Get referenced points and calculate distance
    App::DocumentObject* obj1 = First.getValue();
    App::DocumentObject* obj2 = Second.getValue();
    
    if (!obj1 || !obj2) {
        return 0.0;
    }
    
    // Extract points from referenced objects
    // This is a simplified implementation
    Base::Vector3d point1 = StartPoint.getValue();
    Base::Vector3d point2 = EndPoint.getValue();
    
    return (point2 - point1).Length();
}

void Dimension::updateDimensionText()
{
    double value = calculateDimension();
    std::string format = FormatSpec.getValue();
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer), format.c_str(), value);
    
    if (ShowUnits.getValue()) {
        strcat(buffer, " mm"); // Default unit
    }
    
    // Store formatted text (would be used by ViewProvider)
}
