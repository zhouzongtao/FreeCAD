// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <juergen.riegel@web.de>             *
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

#ifndef PARTGUI_VIEWPROVIDERPARTEXT_H
#define PARTGUI_VIEWPROVIDERPARTEXT_H

#include "SoFCShapeObject.h"


#include <map>
#include <memory>

#include <App/PropertyUnits.h>
#include <Gui/ViewProviderGeometryObject.h>
#include <Gui/ViewProviderTextureExtension.h>
#include <Gui/Render/Core/RenderNode.h>
#include <Gui/Render/Core/GeometryData.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/PartGlobal.h>


class TopoDS_Shape;
class TopoDS_Edge;
class TopoDS_Wire;
class TopoDS_Face;
class SoSeparator;
class SoGroup;
class SoSwitch;
class SoVertexShape;
class SoPickedPoint;
class SoShapeHints;
class SoEventCallback;
class SbVec3f;
class SoSphere;
class SoScale;
class SoCoordinate3;
class SoIndexedFaceSet;
class SoNormal;
class SoNormalBinding;
class SoMaterialBinding;
class SoIndexedLineSet;

namespace PartGui
{

class SoBrepFaceSet;
class SoBrepEdgeSet;
class SoBrepPointSet;

class PartGuiExport ViewProviderPartExt: public Gui::ViewProviderGeometryObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderPartExt);

public:
    /// constructor
    ViewProviderPartExt();
    /// destructor
    ~ViewProviderPartExt() override;

    // Display properties
    App::PropertyFloatConstraint Deviation;
    App::PropertyBool ControlPoints;
    App::PropertyAngle AngularDeflection;
    App::PropertyEnumeration Lighting;
    App::PropertyEnumeration DrawStyle;
    /// Property controlling visibility of the placement indicator, useful for displaying origin
    /// position of attached Document Object.
    App::PropertyBool ShowPlacement;
    // Points
    App::PropertyFloatConstraint PointSize;
    App::PropertyColor PointColor;
    App::PropertyMaterial PointMaterial;
    App::PropertyColorList PointColorArray;
    // Lines
    App::PropertyFloatConstraint LineWidth;
    App::PropertyColor LineColor;
    App::PropertyMaterial LineMaterial;
    App::PropertyColorList LineColorArray;

    void attach(App::DocumentObject*) override;
    void setDisplayMode(const char* ModeName) override;
    /// returns a list of all possible modes
    std::vector<std::string> getDisplayModes() const override;
    /// Update the view representation
    void reload();
    /// If no other task is pending it opens a dialog to allow one to change face colors
    bool changeFaceAppearances();

    void updateData(const App::Property*) override;

    /** @name Restoring view provider from document load */
    //@{
    void startRestoring() override;
    void finishRestoring() override;
    //@}

    /** @name Selection handling
     * This group of methods do the selection handling.
     * Here you can define how the selection for your ViewProfider
     * works.
     */
    //@{
    /// indicates if the ViewProvider use the new Selection model
    bool useNewSelectionModel() const override
    {
        return true;
    }
    /// return a hit element to the selection path or 0
    std::string getElement(const SoDetail*) const override;
    SoDetail* getDetail(const char*) const override;
    std::vector<Base::Vector3d> getModelPoints(const SoPickedPoint*) const override;
    /// return the highlight lines for a given element or the whole shape
    std::vector<Base::Vector3d> getSelectionShape(const char* Element) const override;
    //@}

    virtual Part::TopoShape getRenderedShape() const
    {
        return Part::Feature::getTopoShape(
            getObject(),
            Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
        );
    }

    /** @name Highlight handling
     * This group of methods do the highlighting of elements.
     */
    //@{
    void setHighlightedFaces(const std::vector<App::Material>& materials);
    void setHighlightedFaces(const App::PropertyMaterialList& appearance);
    void unsetHighlightedFaces();
    void setHighlightedEdges(const std::vector<Base::Color>& colors);
    void unsetHighlightedEdges();
    void setHighlightedPoints(const std::vector<Base::Color>& colors);
    void unsetHighlightedPoints();
    //@}

    /** @name Color management methods
     */
    //@{
    std::map<std::string, Base::Color> getElementColors(const char* element = nullptr) const override;
    //@}

    bool isUpdateForced() const override
    {
        return forceUpdateCount > 0;
    }
    void forceUpdate(bool enable = true) override;

    bool allowOverride(const App::DocumentObject&) const override;

    void setFaceHighlightActive(bool active)
    {
        faceHighlightActive = active;
    }
    bool isFaceHighlightActive() const
    {
        return faceHighlightActive;
    }

    /** @name Edit methods */
    //@{
    void setupContextMenu(QMenu*, QObject*, const char*) override;

    /// Get the python wrapper for that ViewProvider
    PyObject* getPyObject() override;

    /// configures Coin nodes so they render given toposhape
    static void setupCoinGeometry(
        TopoDS_Shape shape,
        SoCoordinate3* coords,
        SoBrepFaceSet* faceset,
        SoNormal* norm,
        SoBrepEdgeSet* lineset,
        SoBrepPointSet* nodeset,
        double deviation,
        double angularDeflection,
        bool normalsFromUV = false
    );

    static void setupCoinGeometry(
        TopoDS_Shape shape,
        SoFCShape* node,
        double deviation,
        double angularDeflection,
        bool normalsFromUV = false
    );

protected:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
    //@}

    //=========================================================================
    // 渲染抽象层方法 / Render Abstraction Layer Methods
    //=========================================================================

    /**
     * @brief 初始化渲染节点 / Initialize render nodes
     *
     * 创建抽象层的几何和材质节点。
     * Creates geometry and material nodes for the abstraction layer.
     */
    void initRenderNodes() override;

    /**
     * @brief 同步几何数据到渲染节点 / Sync geometry data to render nodes
     *
     * 将当前形状的几何数据转换并同步到抽象层节点。
     * Converts current shape geometry data and syncs to abstraction layer nodes.
     */
    void syncGeometryToRenderNodes();

    /**
     * @brief 同步线材质到渲染节点 / Sync line material to render nodes
     */
    void syncLineMaterialToRenderNode();

    /**
     * @brief 同步点材质到渲染节点 / Sync point material to render nodes
     */
    void syncPointMaterialToRenderNode();

    /**
     * @brief 同步绘制样式到渲染节点 / Sync draw style to render nodes
     */
    void syncDrawStyleToRenderNode();

protected:
    /// get called by the container whenever a property has been changed
    void onChanged(const App::Property* prop) override;
    bool loadParameter();
    void updateVisual();
    void handleChangedPropertyName(
        Base::XMLReader& reader,
        const char* TypeName,
        const char* PropName
    ) override;

    // nodes for the data representation
    SoMaterialBinding* pcFaceBind;
    SoMaterialBinding* pcLineBind;
    SoMaterialBinding* pcPointBind;
    SoMaterial* pcLineMaterial;
    SoMaterial* pcPointMaterial;
    SoDrawStyle* pcLineStyle;
    SoDrawStyle* pcPointStyle;
    SoShapeHints* pShapeHints;

    SoCoordinate3* coords;
    SoBrepFaceSet* faceset;
    SoNormal* norm;
    SoNormalBinding* normb;
    SoBrepEdgeSet* lineset;
    SoBrepPointSet* nodeset;

    bool VisualTouched;
    bool NormalsFromUV;
    bool faceHighlightActive = false;

    //=========================================================================
    // 渲染抽象层成员 / Render Abstraction Layer Members
    //=========================================================================

    /// 几何数据缓存 / Geometry data cache
    std::shared_ptr<Gui::Render::GeometryData> m_renderGeometryData;

    /// 抽象层坐标节点 / Abstraction layer coordinate node
    std::shared_ptr<Gui::Render::RenderNode> m_renderCoords;

    /// 抽象层法线节点 / Abstraction layer normal node
    std::shared_ptr<Gui::Render::RenderNode> m_renderNormals;

    /// 抽象层面集节点 / Abstraction layer face set node
    std::shared_ptr<Gui::Render::RenderNode> m_renderFaceSet;

    /// 抽象层线集节点 / Abstraction layer line set node
    std::shared_ptr<Gui::Render::RenderNode> m_renderLineSet;

    /// 抽象层点集节点 / Abstraction layer point set node
    std::shared_ptr<Gui::Render::RenderNode> m_renderPointSet;

    /// 抽象层线材质 / Abstraction layer line material
    std::shared_ptr<Gui::Render::RenderNode> m_renderLineMaterial;

    /// 抽象层点材质 / Abstraction layer point material
    std::shared_ptr<Gui::Render::RenderNode> m_renderPointMaterial;

    /// 抽象层线样式 / Abstraction layer line style
    std::shared_ptr<Gui::Render::RenderNode> m_renderLineStyle;

    /// 抽象层点样式 / Abstraction layer point style
    std::shared_ptr<Gui::Render::RenderNode> m_renderPointStyle;

private:
    Gui::ViewProviderFaceTexture texture;
    // settings stuff
    int forceUpdateCount;
    static App::PropertyFloatConstraint::Constraints sizeRange;
    static App::PropertyFloatConstraint::Constraints tessRange;
    static App::PropertyQuantityConstraint::Constraints angDeflectionRange;
    static const char* LightingEnums[];
    static const char* DrawStyleEnums[];

    // This is needed to restore old DiffuseColor values since the restore
    // function is asynchronous
    App::PropertyColorList _diffuseColor;
};

}  // namespace PartGui

#endif  // PARTGUI_VIEWPROVIDERPARTEXT_H
