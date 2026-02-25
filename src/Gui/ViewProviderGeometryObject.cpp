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


#include <Inventor/SoPickedPoint.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoResetTransform.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>


#include <App/GeoFeature.h>
#include <App/PropertyGeo.h>

#include "Application.h"
#include "Document.h"
#include "Inventor/SoFCBoundingBox.h"
#include "SoFCSelection.h"
#include "View3DInventorViewer.h"
#include "ViewProviderGeometryObject.h"
#include "ViewProviderGeometryObjectPy.h"
#include "Render/Core/RenderNode.h"
#include "Render/Core/RenderNodeFactory.h"
#include "Render/Backends/Coin3D/Coin3DMaterial.h"

#include <Base/Tools.h>

using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderGeometryObject, Gui::ViewProviderDragger)

const App::PropertyIntegerConstraint::Constraints intPercent = {0, 100, 5};

ViewProviderGeometryObject::ViewProviderGeometryObject()
{
    App::Material mat = App::Material::getDefaultAppearance();

    long initialTransparency = Base::toPercent(mat.transparency);

    static const char* dogroup = "Display Options";
    static const char* sgroup = "Selection";
    static const char* osgroup = "Object Style";

    ADD_PROPERTY_TYPE(
        Transparency,
        (initialTransparency),
        osgroup,
        App::Prop_None,
        "Set object transparency"
    );
    Transparency.setConstraints(&intPercent);

    ADD_PROPERTY_TYPE(ShapeAppearance, (mat), osgroup, App::Prop_None, "Shape appearance");
    ADD_PROPERTY_TYPE(BoundingBox, (false), dogroup, App::Prop_None, "Display object bounding box");
    ADD_PROPERTY_TYPE(
        Selectable,
        (true),
        sgroup,
        App::Prop_None,
        "Set if the object is selectable in the 3d view"
    );

    pickStyle = new SoPickStyle();
    pickStyle->ref();
    pickStyle->style.setValue(SoPickStyle::SHAPE);
    pcRoot->insertChild(pickStyle, 1);
    Selectable.setValue(isSelectionEnabled());

    pcShapeMaterial = new SoMaterial;
    setCoinAppearance(mat);
    pcShapeMaterial->ref();
    pcShapeMaterial->setName("ShapeMaterial");

    pcBoundingBox = new Gui::SoFCBoundingBox;
    pcBoundingBox->ref();
    pcBoundingBox->setName("BoundingBox");

    pcBoundColor = new SoBaseColor();
    pcBoundColor->ref();
    pcBoundColor->setName("BoundColor");

    sPixmap = "Feature";
}

ViewProviderGeometryObject::~ViewProviderGeometryObject()
{
    pcShapeMaterial->unref();
    pcBoundingBox->unref();
    pcBoundColor->unref();
    pickStyle->unref();
}

bool ViewProviderGeometryObject::isSelectionEnabled() const
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );
    return hGrp->GetBool("EnableSelection", true);
}

void ViewProviderGeometryObject::onChanged(const App::Property* prop)
{
    // Actually, the properties 'ShapeColor' and 'Transparency' are part of the property
    // 'ShapeMaterial'. Both redundant properties are kept due to more convenience for the user. But
    // we must keep the values consistent of all these properties.
    std::string propName = prop->getName();
    if (prop == &Selectable) {
        bool Sel = Selectable.getValue();
        setSelectable(Sel);
    }
    else if (prop == &Transparency) {
        long value = Base::toPercent(ShapeAppearance.getTransparency());
        float trans = Base::fromPercent(Transparency.getValue());
        if (value != Transparency.getValue()) {
            ShapeAppearance.setTransparency(trans);
        }

        pcShapeMaterial->transparency = trans;
    }
    else if (prop == &ShapeAppearance) {
        if (getObject() && getObject()->testStatus(App::ObjectStatus::TouchOnColorChange)) {
            getObject()->touch(true);
        }
        long value = Base::toPercent(ShapeAppearance.getTransparency());
        if (value != Transparency.getValue()) {
            Transparency.setValue(value);
        }
        if (ShapeAppearance.getSize() == 1) {
            const App::Material& Mat = ShapeAppearance[0];
            setCoinAppearance(Mat);
        }
    }
    else if (prop == &BoundingBox) {
        showBoundingBox(BoundingBox.getValue());
    }

    ViewProviderDragger::onChanged(prop);
}

void ViewProviderGeometryObject::attach(App::DocumentObject* pcObj)
{
    ViewProviderDragger::attach(pcObj);
}

void ViewProviderGeometryObject::updateData(const App::Property* prop)
{
    if (prop->isDerivedFrom<App::PropertyComplexGeoData>()) {
        Base::BoundBox3d box = static_cast<const App::PropertyComplexGeoData*>(prop)->getBoundingBox();
        pcBoundingBox->minBounds.setValue(box.MinX, box.MinY, box.MinZ);
        pcBoundingBox->maxBounds.setValue(box.MaxX, box.MaxY, box.MaxZ);
    }
    else if (prop->isDerivedFrom<App::PropertyPlacement>()) {
        auto geometry = getObject<App::GeoFeature>();
        if (geometry && prop == &geometry->Placement) {
            const App::PropertyComplexGeoData* data = geometry->getPropertyOfGeometry();
            if (data) {
                Base::BoundBox3d box = data->getBoundingBox();
                pcBoundingBox->minBounds.setValue(box.MinX, box.MinY, box.MinZ);
                pcBoundingBox->maxBounds.setValue(box.MaxX, box.MaxY, box.MaxZ);
            }
        }
    }
    else if (std::string(prop->getName()) == "ShapeMaterial") {
        // Set the appearance from the material
        if (auto geometry = getObject<App::GeoFeature>()) {
            /*
             * Change the appearance only if the appearance hasn't been set explicitly. A cached
             * material appearance is used to see if the current appearance matches the last
             * material. It is also compared against an empty material to see if the saved
             * material value has been initialized.
             */
            App::Material defaultMaterial;
            auto material = geometry->getMaterialAppearance();
            if ((ShapeAppearance.getSize() == 1)
                && (ShapeAppearance[0] == defaultMaterial || ShapeAppearance[0] == materialAppearance)
                && (material != defaultMaterial)) {
                ShapeAppearance.setValue(material);
            }
            materialAppearance = material;
        }
    }

    ViewProviderDragger::updateData(prop);
}

SoPickedPointList ViewProviderGeometryObject::getPickedPoints(
    const SbVec2s& pos,
    const View3DInventorViewer& viewer,
    bool pickAll
) const
{
    auto root = new SoSeparator;
    root->ref();
    root->addChild(viewer.getHeadlight());
    root->addChild(viewer.getSoRenderManager()->getCamera());
    root->addChild(getRoot());

    SoRayPickAction rp(viewer.getSoRenderManager()->getViewportRegion());
    rp.setPickAll(pickAll);
    rp.setRadius(viewer.getPickRadius());
    rp.setPoint(pos);
    rp.apply(root);
    root->unref();

    // returns a copy of the list
    return rp.getPickedPointList();
}

SoPickedPoint* ViewProviderGeometryObject::getPickedPoint(
    const SbVec2s& pos,
    const View3DInventorViewer& viewer
) const
{
    auto root = new SoSeparator;
    root->ref();
    root->addChild(viewer.getHeadlight());
    root->addChild(viewer.getSoRenderManager()->getCamera());
    root->addChild(getRoot());

    SoRayPickAction rp(viewer.getSoRenderManager()->getViewportRegion());
    rp.setPoint(pos);
    rp.setRadius(viewer.getPickRadius());
    rp.apply(root);
    root->unref();

    // returns a copy of the point
    SoPickedPoint* pick = rp.getPickedPoint();
    // return (pick ? pick->copy() : 0); // needs the same instance of CRT under MS Windows
    return (pick ? new SoPickedPoint(*pick) : nullptr);
}

unsigned long ViewProviderGeometryObject::getBoundColor() const
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );
    // white (255,255,255)
    unsigned long bbcol = hGrp->GetUnsigned("BoundingBoxColor", 4294967295UL);
    return bbcol;
}

void ViewProviderGeometryObject::setCoinAppearance(const App::Material& source)
{
    // Coin3D backend
    pcShapeMaterial->ambientColor
        .setValue(source.ambientColor.r, source.ambientColor.g, source.ambientColor.b);
    pcShapeMaterial->diffuseColor
        .setValue(source.diffuseColor.r, source.diffuseColor.g, source.diffuseColor.b);
    pcShapeMaterial->specularColor
        .setValue(source.specularColor.r, source.specularColor.g, source.specularColor.b);
    pcShapeMaterial->emissiveColor
        .setValue(source.emissiveColor.r, source.emissiveColor.g, source.emissiveColor.b);
    pcShapeMaterial->shininess.setValue(source.shininess);
    pcShapeMaterial->transparency.setValue(source.transparency);

    // 同步到抽象层节点 / Sync to abstraction layer node
    syncMaterialToRenderNode(source);
}

namespace
{
float getBoundBoxFontSize()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );
    return hGrp->GetFloat("BoundingBoxFontSize", 10.0);
}
}  // namespace

void ViewProviderGeometryObject::showBoundingBox(bool show)
{
    if (!pcBoundSwitch && show) {
        unsigned long bbcol = getBoundColor();
        float red {};
        float green {};
        float blue {};
        red = ((bbcol >> 24) & 0xff) / 255.0F;
        green = ((bbcol >> 16) & 0xff) / 255.0F;
        blue = ((bbcol >> 8) & 0xff) / 255.0F;

        pcBoundSwitch = new SoSwitch();
        pcBoundSwitch->setName("BoundSwitch");
        auto pBoundingSep = new SoSeparator();
        auto lineStyle = new SoDrawStyle;
        lineStyle->lineWidth = 2.0F;
        pBoundingSep->addChild(lineStyle);

        pcBoundColor->rgb.setValue(red, green, blue);
        pBoundingSep->addChild(pcBoundColor);
        auto font = new SoFont();
        font->size.setValue(getBoundBoxFontSize());
        pBoundingSep->addChild(font);

        pBoundingSep->addChild(new SoResetTransform());
        pBoundingSep->addChild(pcBoundingBox);
        pcBoundingBox->coordsOn.setValue(false);
        pcBoundingBox->dimensionsOn.setValue(true);

        // add to the highlight node
        pcBoundSwitch->addChild(pBoundingSep);
        pcRoot->addChild(pcBoundSwitch);
    }

    if (pcBoundSwitch) {
        pcBoundSwitch->whichChild = (show ? 0 : -1);
    }
}

void ViewProviderGeometryObject::setSelectable(bool selectable)
{
    // Coin3D backend: search and update SoFCSelection nodes
    SoSearchAction sa;
    sa.setInterest(SoSearchAction::ALL);
    sa.setSearchingAll(true);
    sa.setType(Gui::SoFCSelection::getClassTypeId());
    sa.apply(pcRoot);

    SoPathList& pathList = sa.getPaths();

    for (int i = 0; i < pathList.getLength(); i++) {
        auto selNode = dynamic_cast<SoFCSelection*>(pathList[i]->getTail());
        if (selectable) {
            if (selNode) {
                selNode->selectionMode = SoFCSelection::SEL_ON;
                selNode->preselectionMode = SoFCSelection::AUTO;
            }
        }
        else {
            if (selNode) {
                selNode->selectionMode = SoFCSelection::SEL_OFF;
                selNode->preselectionMode = SoFCSelection::OFF;
                selNode->selected = SoFCSelection::NOTSELECTED;
            }
        }
    }

    pickStyle->style.setValue(selectable ? SoPickStyle::SHAPE : SoPickStyle::UNPICKABLE);

    // 同步到抽象层节点 / Sync to abstraction layer node
    syncPickStyleToRenderNode(selectable);
}

PyObject* ViewProviderGeometryObject::getPyObject()
{
    if (!pyViewObject) {
        pyViewObject = new ViewProviderGeometryObjectPy(this);
    }
    pyViewObject->IncRef();
    return pyViewObject;
}

void ViewProviderGeometryObject::handleChangedPropertyName(
    Base::XMLReader& reader,
    const char* TypeName,
    const char* PropName
)
{
    if (strcmp(PropName, "ShapeColor") == 0
        && strcmp(TypeName, App::PropertyColor::getClassTypeId().getName()) == 0) {
        App::PropertyColor prop;
        prop.Restore(reader);
        ShapeAppearance.setDiffuseColor(prop.getValue());
    }
    else if (strcmp(PropName, "ShapeMaterial") == 0
             && strcmp(TypeName, App::PropertyMaterial::getClassTypeId().getName()) == 0) {
        App::PropertyMaterial prop;
        prop.Restore(reader);
        ShapeAppearance.setValue(prop.getValue());
    }
    else {
        ViewProviderDragger::handleChangedPropertyName(reader, TypeName, PropName);
    }
}

//---------------------------------------------------------------------------
// 渲染抽象层集成 / Rendering Abstraction Layer Integration
//---------------------------------------------------------------------------

void ViewProviderGeometryObject::initRenderNodes()
{
    // TEMPORARILY DISABLED: Render abstraction layer initialization
    // This was causing crashes when loading documents
    // TODO: Re-enable once render abstraction is fully stable
#if 0
    // 检查是否已初始化（幂等性）/ Check if already initialized (idempotent)
    if (m_renderMaterial) {
        return;
    }

    // 首先调用基类初始化 / First call base class init
    ViewProviderDragger::initRenderNodes();

    // 获取节点工厂 / Get node factory
    auto* factory = getNodeFactory();
    if (!factory) {
        return;
    }

    // 创建抽象层材质节点 / Create abstraction layer material node
    m_renderMaterial = factory->createMaterial();
    if (m_renderMaterial) {
        m_renderMaterial->setName("ShapeMaterial");
        // 同步初始材质 / Sync initial material
        App::Material mat = App::Material::getDefaultAppearance();
        syncMaterialToRenderNode(mat);
    }

    // 创建抽象层拾取样式节点 / Create abstraction layer pick style node
    m_renderPickStyle = factory->createPickStyle();
    if (m_renderPickStyle) {
        m_renderPickStyle->setName("PickStyle");
        syncPickStyleToRenderNode(true);
    }

    // TODO: 创建边界框节点（需要 RenderBoundingBox 类型）
    // TODO: Create bounding box node (needs RenderBoundingBox type)
#endif
}

void ViewProviderGeometryObject::syncMaterialToRenderNode(const App::Material& mat)
{
    if (!m_renderMaterial) {
        return;
    }

    // 尝试使用 Coin3D 特定接口（当前唯一实现）
    // Try using Coin3D-specific interface (currently the only implementation)
#ifdef RENDER_HAS_COIN3D_BACKEND
    if (auto* coin3dMat = dynamic_cast<Render::Coin3DMaterial*>(m_renderMaterial.get())) {
        // 使用 Coin3DMaterial 提供的 float 参数版本
        // Use the float-parameter versions provided by Coin3DMaterial
        coin3dMat->setAmbientColor(mat.ambientColor.r, mat.ambientColor.g, mat.ambientColor.b);
        coin3dMat->setDiffuseColor(mat.diffuseColor.r, mat.diffuseColor.g, mat.diffuseColor.b);
        coin3dMat->setSpecularColor(mat.specularColor.r, mat.specularColor.g, mat.specularColor.b);
        coin3dMat->setEmissiveColor(mat.emissiveColor.r, mat.emissiveColor.g, mat.emissiveColor.b);
        coin3dMat->setShininess(mat.shininess);
        coin3dMat->setTransparency(mat.transparency);
        return;
    }
#endif

    // TODO: 添加 OsgVerse 后端支持
    // TODO: Add OsgVerse backend support
#ifdef RENDER_HAS_OSGVERSE_BACKEND
    // if (auto* osgMat = dynamic_cast<Render::OsgVerseMaterial*>(m_renderMaterial.get())) {
    //     // Set OsgVerse material properties
    // }
#endif
}

void ViewProviderGeometryObject::syncPickStyleToRenderNode(bool selectable)
{
    if (!m_renderPickStyle) {
        return;
    }

    // TODO: 当 RenderPickStyle 类型实现后，在这里设置拾取样式
    // TODO: When RenderPickStyle type is implemented, set pick style here
    //
    // auto* pickNode = dynamic_cast<Render::RenderPickStyle*>(m_renderPickStyle.get());
    // if (pickNode) {
    //     pickNode->setStyle(selectable ? PickStyle::Shape : PickStyle::Unpickable);
    // }

    (void)selectable;  // 避免未使用警告 / Avoid unused warning
}
