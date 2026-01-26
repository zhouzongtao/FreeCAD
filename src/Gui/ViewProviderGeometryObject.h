/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_VIEWPROVIDER_GEOMETRYOBJECT_H
#define GUI_VIEWPROVIDER_GEOMETRYOBJECT_H

#include "ViewProviderDragger.h"
#include <Inventor/lists/SoPickedPointList.h>
#include <memory>

class SoPickedPointList;
class SoPickStyle;
class SoSwitch;
class SoSensor;
class SbVec2s;
class SoBaseColor;

// 抽象层前向声明 / Abstraction layer forward declarations
namespace Gui {
namespace Render {
    class RenderNode;
    class RenderMaterial;
}
}

namespace Gui
{

class SoFCSelection;
class SoFCBoundingBox;
class View3DInventorViewer;

/**
 * The base class for all view providers that display geometric data, like mesh, point clouds and
 * shapes.
 * @author Werner Mayer
 */
class GuiExport ViewProviderGeometryObject: public ViewProviderDragger
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderGeometryObject);

public:
    /// constructor.
    ViewProviderGeometryObject();

    /// destructor.
    ~ViewProviderGeometryObject() override;

    // Display properties
    App::PropertyPercent Transparency;
    App::PropertyMaterialList ShapeAppearance;  // May be different from material
    App::PropertyBool BoundingBox;
    App::PropertyBool Selectable;

    /**
     * Attaches the document object to this view provider.
     */
    void attach(App::DocumentObject* pcObject) override;
    void updateData(const App::Property*) override;

    bool isSelectable() const override
    {
        return Selectable.getValue();
    }

    /**
     * Returns a list of picked points from the geometry under \a getRoot().
     * If \a pickAll is false (the default) only the intersection point closest to the camera will
     * be picked, otherwise all intersection points will be picked.
     */
    SoPickedPointList getPickedPoints(
        const SbVec2s& pos,
        const View3DInventorViewer& viewer,
        bool pickAll = false
    ) const;
    /**
     * This method is provided for convenience and does basically the same as getPickedPoints()
     * unless that only the closest point to the camera will be picked. \note It is in the response
     * of the client programmer to delete the returned SoPickedPoint object.
     */
    SoPickedPoint* getPickedPoint(const SbVec2s& pos, const View3DInventorViewer& viewer) const;

    /** @name Edit methods */
    //@{
    virtual void showBoundingBox(bool);
    //@}

    /// Get the python wrapper for that ViewProvider
    PyObject* getPyObject() override;

protected:
    /// get called by the container whenever a property has been changed
    void onChanged(const App::Property* prop) override;
    void setSelectable(bool Selectable = true);

    virtual unsigned long getBoundColor() const;

    void handleChangedPropertyName(
        Base::XMLReader& reader,
        const char* TypeName,
        const char* PropName
    ) override;
    void setCoinAppearance(const App::Material& source);

    /**
     * @brief 初始化抽象层几何渲染节点
     * Initialize abstraction layer geometry render nodes
     */
    void initRenderNodes() override;

    /**
     * @brief 同步材质到抽象层节点
     * Sync material to abstraction layer node
     */
    void syncMaterialToRenderNode(const App::Material& mat);

    /**
     * @brief 同步拾取样式到抽象层节点
     * Sync pick style to abstraction layer node
     *
     * @param selectable true=SHAPE, false=UNPICKABLE
     */
    void syncPickStyleToRenderNode(bool selectable);

private:
    bool isSelectionEnabled() const;

protected:
    //=========================================================================
    // Coin3D 节点（向后兼容）/ Coin3D nodes (backward compatibility)
    //=========================================================================
    SoMaterial* pcShapeMaterial {nullptr};
    SoFCBoundingBox* pcBoundingBox {nullptr};
    SoSwitch* pcBoundSwitch {nullptr};
    SoBaseColor* pcBoundColor {nullptr};
    SoPickStyle* pickStyle {nullptr};

    //=========================================================================
    // 抽象层节点 / Abstraction layer nodes
    //=========================================================================

    /// 抽象层材质节点 / Abstraction layer material node
    /// 可通过 dynamic_cast<Render::Coin3DMaterial*> 访问扩展功能
    /// Can access extended features via dynamic_cast<Render::Coin3DMaterial*>
    std::shared_ptr<Render::RenderNode> m_renderMaterial;
    /// 抽象层拾取样式节点 / Abstraction layer pick style node
    std::shared_ptr<Render::RenderNode> m_renderPickStyle;
    /// 抽象层边界框节点 / Abstraction layer bounding box node
    std::shared_ptr<Render::RenderNode> m_renderBoundingBox;

    App::Material materialAppearance;
};

}  // namespace Gui


#endif  // GUI_VIEWPROVIDER_GEOMETRYOBJECT_H
