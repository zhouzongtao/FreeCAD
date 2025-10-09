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

#ifndef DRAWING_FEATURE_H
#define DRAWING_FEATURE_H

#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>
#include <App/PropertyLinks.h>
#include <App/PropertyUnits.h>
#include <Mod/Part/App/PropertyTopoShape.h>
#include <Mod/Drawing/DrawingGlobal.h>

namespace Drawing
{

/** Base class for all drawing objects
 */
class DrawingExport Feature : public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Drawing::Feature);

public:
    Feature();
    ~Feature() override;

    // Common properties for all drawing objects
    App::PropertyVector StartPoint;
    App::PropertyVector EndPoint;
    App::PropertyFloat LineWidth;
    App::PropertyColor LineColor;
    App::PropertyEnumeration LineStyle;
    App::PropertyBool Construction;

    /** @name methods override Feature */
    //@{
    short mustExecute() const override;
    App::DocumentObjectExecReturn* execute() override;
    //@}

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override;

    /// Get Python object
    PyObject* getPyObject() override;

protected:
    /// recalculate the feature
    virtual App::DocumentObjectExecReturn* recompute();
    void onChanged(const App::Property* prop) override;

private:
    static const char* LineStyleEnums[];
};

/** Line feature class
 */
class DrawingExport Line : public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Drawing::Line);

public:
    Line();

    // Line specific properties
    App::PropertyDistance Length;
    App::PropertyAngle Angle;

    /** @name methods override Feature */
    //@{
    App::DocumentObjectExecReturn* execute() override;
    //@}

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override;

    /// Calculate line geometry
    void calculateGeometry();

    /// Get line direction vector
    Base::Vector3d getDirection() const;
};

/** Circle feature class
 */
class DrawingExport Circle : public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Drawing::Circle);

public:
    Circle();

    // Circle specific properties
    App::PropertyVector Center;
    App::PropertyDistance Radius;
    App::PropertyAngle FirstAngle;
    App::PropertyAngle LastAngle;

    /** @name methods override Feature */
    //@{
    App::DocumentObjectExecReturn* execute() override;
    //@}

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override;

    /// Calculate circle geometry
    void calculateGeometry();

    /// Check if this is a full circle
    bool isFullCircle() const;
};

/** Rectangle feature class
 */
class DrawingExport Rectangle : public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Drawing::Rectangle);

public:
    Rectangle();

    // Rectangle specific properties
    App::PropertyLength Width;
    App::PropertyLength Height;
    App::PropertyBool Rounded;
    App::PropertyLength CornerRadius;

    /** @name methods override Feature */
    //@{
    App::DocumentObjectExecReturn* execute() override;
    //@}

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override;

    /// Calculate rectangle geometry
    void calculateGeometry();
};

/** Polygon feature class
 */
class DrawingExport Polygon : public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Drawing::Polygon);

public:
    Polygon();

    // Polygon specific properties
    App::PropertyVectorList Points;
    App::PropertyInteger Sides;
    App::PropertyLength Radius;
    App::PropertyBool Closed;

    /** @name methods override Feature */
    //@{
    App::DocumentObjectExecReturn* execute() override;
    //@}

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override;

    /// Calculate polygon geometry
    void calculateGeometry();

    /// Add point to polygon
    void addPoint(const Base::Vector3d& point);

    /// Remove point from polygon
    void removePoint(int index);
};

/** Text feature class
 */
class DrawingExport Text : public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Drawing::Text);

public:
    Text();

    // Text specific properties
    App::PropertyString TextString;
    App::PropertyFont FontName;
    App::PropertyFloat FontSize;
    App::PropertyVector Position;
    App::PropertyAngle Rotation;
    App::PropertyEnumeration Justification;

    /** @name methods override Feature */
    //@{
    App::DocumentObjectExecReturn* execute() override;
    //@}

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override;

    /// Calculate text geometry
    void calculateGeometry();

private:
    static const char* JustificationEnums[];
};

/** Dimension feature class
 */
class DrawingExport Dimension : public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Drawing::Dimension);

public:
    Dimension();

    // Dimension specific properties
    App::PropertyLinkSub First;
    App::PropertyLinkSub Second;
    App::PropertyVector DimLinePosition;
    App::PropertyString FormatSpec;
    App::PropertyFloat TextSize;
    App::PropertyBool ShowUnits;

    /** @name methods override Feature */
    //@{
    App::DocumentObjectExecReturn* execute() override;
    //@}

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override;

    /// Calculate dimension value
    double calculateDimension();

    /// Update dimension text
    void updateDimensionText();
};

} //namespace Drawing

#endif // DRAWING_FEATURE_H
