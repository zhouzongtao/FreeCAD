// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#include "OsgVerseViewer.h"
#include "OsgVerseWidget.h"
#include "GeometryConverter.h"
#include <Base/Console.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <App/DocumentObject.h>

// Part module - we can include this because OsgVerseGui links Part!
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>

// OSG includes
#include <osgViewer/Viewer>
#include <osg/ShapeDrawable>
#include <osg/Material>

using namespace OsgVerseGui;

OsgVerseViewer::OsgVerseViewer(QWidget* parent)
    : _widget(nullptr)
    , _sceneRoot(nullptr)
    , _navigationStyle("Trackball")
{
    Base::Console().message("OsgVerseViewer: Constructor START\n");
    
    try {
        Base::Console().message("OsgVerseViewer: Creating OsgVerseWidget...\n");
        
        // Create Qt OpenGL widget
        _widget = new OsgVerseWidget(parent);
        
        Base::Console().message("OsgVerseViewer: OsgVerseWidget created\n");
        
        // Get OSG viewer from widget
        osgViewer::Viewer* viewer = _widget->getViewer();
        
        if (viewer) {
            Base::Console().message("OsgVerseViewer: Got viewer from widget (valid)\n");
        } else {
            Base::Console().warning("OsgVerseViewer: Got viewer from widget (NULL!)\n");
        }
        
        // Create scene root
        _sceneRoot = new osg::Group();
        
        Base::Console().message("OsgVerseViewer: Scene root created\n");
        
        // Set scene data
        if (viewer) {
            viewer->setSceneData(_sceneRoot.get());
            Base::Console().message("OsgVerseViewer: Scene data set\n");
        } else {
            Base::Console().warning("OsgVerseViewer: Viewer is null!\n");
        }
        
        Base::Console().message("OsgVerseViewer: Constructor END - SUCCESS\n");
    }
    catch (const std::exception& e) {
        Base::Console().error("OsgVerseViewer: Exception in constructor: %s\n", e.what());
        throw;
    }
    catch (...) {
        Base::Console().error("OsgVerseViewer: Unknown exception in constructor\n");
        throw;
    }
}

OsgVerseViewer::~OsgVerseViewer()
{
    Base::Console().message("OsgVerseViewer: Destroying viewer\n");
    
    // Clean up view provider nodes
    _vpNodes.clear();
    
    // Clean up scene
    _sceneRoot = nullptr;
    
    // Delete widget (which will delete the viewer)
    if (_widget) {
        delete _widget;
        _widget = nullptr;
    }
    
    Base::Console().message("OsgVerseViewer: Viewer destroyed\n");
}

std::string OsgVerseViewer::getBackendName() const
{
    return "OsgVerse";
}

QWidget* OsgVerseViewer::getWidget()
{
    return _widget;
}

void OsgVerseViewer::addViewProvider(Gui::ViewProvider* vp)
{
    if (!vp) {
        Base::Console().warning("OsgVerseViewer: Cannot add null ViewProvider\n");
        return;
    }
    
    Base::Console().message("OsgVerseViewer: Adding ViewProvider\n");
    
    // Create scene node for this ViewProvider
    osg::ref_ptr<osg::Node> node = createNodeForViewProvider(vp);
    
    if (!node) {
        Base::Console().warning("OsgVerseViewer: Failed to create node, using placeholder\n");
        node = createPlaceholderSphere();
    }
    
    // Store and add to scene
    _vpNodes[vp] = node;
    _sceneRoot->addChild(node.get());
    
    render();
}

void OsgVerseViewer::removeViewProvider(Gui::ViewProvider* vp)
{
    if (!vp) {
        return;
    }
    
    auto it = _vpNodes.find(vp);
    if (it != _vpNodes.end()) {
        Base::Console().message("OsgVerseViewer: Removing ViewProvider\n");
        
        _sceneRoot->removeChild(it->second.get());
        _vpNodes.erase(it);
        
        render();
    }
}

void OsgVerseViewer::updateViewProvider(Gui::ViewProvider* vp)
{
    if (!vp) {
        return;
    }
    
    auto it = _vpNodes.find(vp);
    if (it != _vpNodes.end()) {
        Base::Console().message("OsgVerseViewer: Updating ViewProvider\n");
        
        // Remove old node
        _sceneRoot->removeChild(it->second.get());
        
        // Create new node
        osg::ref_ptr<osg::Node> node = createNodeForViewProvider(vp);
        if (!node) {
            node = createPlaceholderSphere();
        }
        
        // Update and add
        it->second = node;
        _sceneRoot->addChild(node.get());
        
        render();
    }
}

void OsgVerseViewer::clearScene()
{
    Base::Console().message("OsgVerseViewer: Clearing scene\n");
    
    // Remove all view provider nodes
    for (auto& pair : _vpNodes) {
        _sceneRoot->removeChild(pair.second.get());
    }
    _vpNodes.clear();
    
    render();
}

void OsgVerseViewer::render()
{
    if (_widget) {
        _widget->update(); // Triggers paintGL()
    }
}

void OsgVerseViewer::setBackgroundColor(const QColor& color)
{
    if (_widget) {
        osgViewer::Viewer* viewer = _widget->getViewer();
        if (viewer) {
            osg::Vec4 bgColor(
                color.redF(),
                color.greenF(),
                color.blueF(),
                1.0f
            );
            viewer->getCamera()->setClearColor(bgColor);
            render();
        }
    }
}

void OsgVerseViewer::setAntiAliasing(bool enable)
{
    Base::Console().message("OsgVerseViewer: Anti-aliasing %s\n", enable ? "enabled" : "disabled");
    // TODO: Implement anti-aliasing control
}

void OsgVerseViewer::viewAll()
{
    if (_widget) {
        osgViewer::Viewer* viewer = _widget->getViewer();
        if (viewer) {
            viewer->home();
        }
    }
}

void OsgVerseViewer::setCamera(const float position[3], 
                               const float orientation[4],
                               const float up[3])
{
    // TODO: Implement camera control
    Base::Console().message("OsgVerseViewer: Setting camera\n");
}

void OsgVerseViewer::getCamera(float position[3], 
                               float orientation[4],
                               float up[3]) const
{
    // TODO: Implement camera query
}

std::vector<App::DocumentObject*> OsgVerseViewer::getSelection() const
{
    // TODO: Implement selection retrieval
    return std::vector<App::DocumentObject*>();
}

void OsgVerseViewer::setSelection(const std::vector<App::DocumentObject*>& objects)
{
    Base::Console().message("OsgVerseViewer: Setting selection (%zu objects)\n", objects.size());
    // TODO: Implement selection
}

void OsgVerseViewer::clearSelection()
{
    Base::Console().message("OsgVerseViewer: Clearing selection\n");
    // TODO: Implement selection clearing
}

void OsgVerseViewer::setNavigationStyle(const std::string& style)
{
    _navigationStyle = style;
    Base::Console().message("OsgVerseViewer: Navigation style set to '%s'\n", style.c_str());
    // TODO: Actually change the navigation style
}

std::string OsgVerseViewer::getNavigationStyle() const
{
    return _navigationStyle;
}

bool OsgVerseViewer::supportsFeature(const std::string& feature) const
{
    // OsgVerse supports most standard features
    if (feature == "transparency") return true;
    if (feature == "selection") return true;
    if (feature == "navigation") return true;
    if (feature == "camera") return true;
    if (feature == "lighting") return true;
    if (feature == "shadows") return true;  // OsgVerse advantage
    
    return false;
}

std::string OsgVerseViewer::getVersion() const
{
    return "OsgVerse + OSG 3.6+";
}

//===========================================================================
// Private methods
//===========================================================================

osg::ref_ptr<osg::Node> OsgVerseViewer::createNodeForViewProvider(Gui::ViewProvider* vp)
{
    if (!vp) {
        return nullptr;
    }
    
    // Check if this is a ViewProviderDocumentObject
    auto* vpDoc = dynamic_cast<Gui::ViewProviderDocumentObject*>(vp);
    if (!vpDoc) {
        Base::Console().message("OsgVerseViewer: ViewProvider is not a ViewProviderDocumentObject\n");
        return nullptr;
    }
    
    App::DocumentObject* obj = vpDoc->getObject();
    if (!obj) {
        Base::Console().warning("OsgVerseViewer: ViewProvider has no object\n");
        return nullptr;
    }
    
    // Check if this is a Part::Feature
    if (!obj->isDerivedFrom(Part::Feature::getClassTypeId())) {
        Base::Console().message("OsgVerseViewer: Object is not a Part::Feature\n");
        return nullptr;
    }
    
    // ✅ KEY ADVANTAGE: We can directly call Part::Feature::getTopoShape()
    // because OsgVerseGui links the Part module!
    try {
        Base::Console().message("OsgVerseViewer: Extracting TopoDS_Shape from Part::Feature\n");
        
        Part::TopoShape topoShape = Part::Feature::getTopoShape(
            obj,
            Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
        );
        
        const TopoDS_Shape& shape = topoShape.getShape();
        
        if (shape.IsNull()) {
            Base::Console().warning("OsgVerseViewer: Shape is null\n");
            return nullptr;
        }
        
        Base::Console().message("OsgVerseViewer: Converting TopoDS_Shape to OSG geometry\n");
        
        // Convert using GeometryConverter
        GeometryConverter::ConversionOptions options;
        options.deflection = 0.1;
        options.angle = 0.5;
        options.computeNormals = true;
        
        GeometryConverter::ConversionStats stats;
        osg::ref_ptr<osg::Geode> geode = GeometryConverter::convertShape(shape, options, &stats);
        
        if (!geode) {
            Base::Console().error("OsgVerseViewer: GeometryConverter failed\n");
            return nullptr;
        }
        
        Base::Console().message("OsgVerseViewer: Conversion successful - %d vertices, %d triangles\n",
                               stats.vertexCount, stats.triangleCount);
        
        // Apply material
        applyMaterial(geode.get(), QColor(200, 200, 200));
        
        return geode;
    }
    catch (const Standard_Failure& e) {
        Base::Console().error("OsgVerseViewer: OCCT exception: %s\n", e.GetMessageString());
        return nullptr;
    }
    catch (const std::exception& e) {
        Base::Console().error("OsgVerseViewer: Exception: %s\n", e.what());
        return nullptr;
    }
    catch (...) {
        Base::Console().error("OsgVerseViewer: Unknown exception\n");
        return nullptr;
    }
}

osg::ref_ptr<osg::Node> OsgVerseViewer::createPlaceholderSphere()
{
    Base::Console().message("OsgVerseViewer: Creating placeholder sphere\n");
    
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    
    // Create a red sphere
    osg::ref_ptr<osg::Sphere> sphere = new osg::Sphere(osg::Vec3(0, 0, 0), 1.0f);
    osg::ref_ptr<osg::ShapeDrawable> drawable = new osg::ShapeDrawable(sphere.get());
    drawable->setColor(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));  // Red
    
    geode->addDrawable(drawable.get());
    
    return geode;
}

void OsgVerseViewer::applyMaterial(osg::Node* node, const QColor& color)
{
    if (!node) {
        return;
    }
    
    osg::ref_ptr<osg::StateSet> stateSet = node->getOrCreateStateSet();
    osg::ref_ptr<osg::Material> material = new osg::Material();
    
    osg::Vec4 diffuse(color.redF(), color.greenF(), color.blueF(), 1.0f);
    material->setDiffuse(osg::Material::FRONT_AND_BACK, diffuse);
    material->setAmbient(osg::Material::FRONT_AND_BACK, diffuse * 0.3f);
    material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(0.5f, 0.5f, 0.5f, 1.0f));
    material->setShininess(osg::Material::FRONT_AND_BACK, 32.0f);
    
    stateSet->setAttributeAndModes(material.get(), osg::StateAttribute::ON);
}
