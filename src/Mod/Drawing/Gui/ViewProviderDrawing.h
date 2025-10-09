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

#ifndef DRAWINGUI_VIEWPROVIDERDRAWING_H
#define DRAWINGUI_VIEWPROVIDERDRAWING_H

#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/ViewProviderBuilder.h>
#include <Mod/Drawing/DrawingGlobal.h>

class SoCoordinate3;
class SoDrawStyle;
class SoLineSet;
class SoMarkerSet;
class SoSeparator;
class SoMaterial;

namespace DrawingGui {

/** Base ViewProvider for Drawing objects
 */
class DrawingGuiExport ViewProviderDrawing : public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(DrawingGui::ViewProviderDrawing);

public:
    ViewProviderDrawing();
    ~ViewProviderDrawing() override;

    // Display properties
    App::PropertyColor LineColor;
    App::PropertyFloat LineWidth;
    App::PropertyEnumeration LineStyle;
    App::PropertyFloat PointSize;
    App::PropertyColor PointColor;
    App::PropertyBool ShowPoints;

    /** @name Display modes */
    //@{
    std::vector<std::string> getDisplayModes() const override;
    void setDisplayMode(const char* ModeName) override;
    //@}

    /** @name Selection handling */
    //@{
    bool allowOverride(const App::DocumentObject&) const override;
    void updateData(const App::Property*) override;
    void onChanged(const App::Property* prop) override;
    //@}

    /** @name Mouse interaction */
    //@{
    bool mouseMove(const SbVec2s &pos, Gui::View3DInventorViewer *viewer) override;
    bool mouseButtonPressed(int Button, bool pressed, const SbVec2s &pos, 
                           const Gui::View3DInventorViewer *viewer) override;
    //@}

    /** @name Context menu */
    //@{
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;
    //@}

protected:
    /// Create the visual representation
    void attach(App::DocumentObject* obj) override;

    /// Update visual representation
    virtual void updateVisual();

    /// Create line visual
    virtual void createLineVisual();

    /// Update line coordinates
    virtual void updateCoordinates();

    /// Update line style
    virtual void updateLineStyle();

    /// Update point markers
    virtual void updatePointMarkers();

protected:
    // Coin3D scene graph nodes
    SoSeparator* pcRoot;
    SoCoordinate3* pcCoords;
    SoDrawStyle* pcDrawStyle;
    SoLineSet* pcLineSet;
    SoMarkerSet* pcPointSet;
    SoMaterial* pcLineMaterial;
    SoMaterial* pcPointMaterial;

private:
    static const char* LineStyleEnums[];
};

/** ViewProvider for Line objects
 */
class DrawingGuiExport ViewProviderLine : public ViewProviderDrawing
{
    PROPERTY_HEADER_WITH_OVERRIDE(DrawingGui::ViewProviderLine);

public:
    ViewProviderLine();

protected:
    void createLineVisual() override;
    void updateCoordinates() override;
};

/** ViewProvider for Circle objects  
 */
class DrawingGuiExport ViewProviderCircle : public ViewProviderDrawing
{
    PROPERTY_HEADER_WITH_OVERRIDE(DrawingGui::ViewProviderCircle);

public:
    ViewProviderCircle();

    // Circle specific properties
    App::PropertyInteger Resolution;

protected:
    void createLineVisual() override;
    void updateCoordinates() override;

private:
    void generateCirclePoints(std::vector<SbVec3f>& points);
};

/** ViewProvider for Rectangle objects
 */
class DrawingGuiExport ViewProviderRectangle : public ViewProviderDrawing
{
    PROPERTY_HEADER_WITH_OVERRIDE(DrawingGui::ViewProviderRectangle);

public:
    ViewProviderRectangle();

protected:
    void createLineVisual() override;
    void updateCoordinates() override;

private:
    void generateRectanglePoints(std::vector<SbVec3f>& points);
};

/** ViewProvider for Polygon objects
 */
class DrawingGuiExport ViewProviderPolygon : public ViewProviderDrawing
{
    PROPERTY_HEADER_WITH_OVERRIDE(DrawingGui::ViewProviderPolygon);

public:
    ViewProviderPolygon();

protected:
    void createLineVisual() override;
    void updateCoordinates() override;
};

/** ViewProvider for Text objects
 */
class DrawingGuiExport ViewProviderText : public ViewProviderDrawing
{
    PROPERTY_HEADER_WITH_OVERRIDE(DrawingGui::ViewProviderText);

public:
    ViewProviderText();

    // Text specific properties
    App::PropertyFont FontName;
    App::PropertyFloat FontSize;
    App::PropertyEnumeration Justification;

protected:
    void attach(App::DocumentObject* obj) override;
    void updateVisual() override;

private:
    class SoText2;
    SoText2* pcTextNode;
    static const char* JustificationEnums[];
};

/** ViewProvider for Dimension objects
 */
class DrawingGuiExport ViewProviderDimension : public ViewProviderDrawing
{
    PROPERTY_HEADER_WITH_OVERRIDE(DrawingGui::ViewProviderDimension);

public:
    ViewProviderDimension();

    // Dimension specific properties
    App::PropertyFloat TextSize;
    App::PropertyColor TextColor;

protected:
    void attach(App::DocumentObject* obj) override;
    void updateVisual() override;
    void createDimensionVisual();

private:
    SoSeparator* pcDimRoot;
    class SoText2;
    SoText2* pcDimText;
    SoLineSet* pcDimLines;
    SoCoordinate3* pcDimCoords;
};

} // namespace DrawingGui

#endif // DRAWINGUI_VIEWPROVIDERDRAWING_H
