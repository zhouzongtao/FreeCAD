// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef OSGVERSEGUI_VIEWER_H
#define OSGVERSEGUI_VIEWER_H

#include "PreCompiled.h"
#include <Gui/View3D/Interfaces/IViewer3D.h>
#include <osg/ref_ptr>
#include <osg/Group>
#include <map>

// Forward declarations
namespace osgViewer {
    class Viewer;
}
class QWidget;

namespace OsgVerseGui {
    class OsgVerseWidget;
}

namespace OsgVerseGui {

/**
 * @brief OsgVerse implementation of IViewer3D
 * 
 * This viewer uses OpenSceneGraph (OSG) and OsgVerse for 3D rendering.
 * It can directly access Part::Feature::getTopoShape() since OsgVerseGui
 * is an independent module that links the Part module.
 */
class OsgVerseGuiExport OsgVerseViewer : public Gui::IViewer3D {
public:
    explicit OsgVerseViewer(QWidget* parent = nullptr);
    virtual ~OsgVerseViewer();
    
    // IViewer3D interface - Basic info
    std::string getBackendName() const override;
    QWidget* getWidget() override;
    
    // IViewer3D interface - Scene management
    void addViewProvider(Gui::ViewProvider* vp) override;
    void removeViewProvider(Gui::ViewProvider* vp) override;
    void updateViewProvider(Gui::ViewProvider* vp) override;
    void clearScene() override;
    
    // IViewer3D interface - Rendering
    void render() override;
    void setBackgroundColor(const QColor& color) override;
    void setAntiAliasing(bool enable) override;
    
    // IViewer3D interface - Camera
    void viewAll() override;
    void setCamera(const float position[3], 
                   const float orientation[4],
                   const float up[3]) override;
    void getCamera(float position[3], 
                   float orientation[4],
                   float up[3]) const override;
    
    // IViewer3D interface - Selection
    std::vector<App::DocumentObject*> getSelection() const override;
    void setSelection(const std::vector<App::DocumentObject*>& objects) override;
    void clearSelection() override;
    
    // IViewer3D interface - Interaction
    void setNavigationStyle(const std::string& style) override;
    std::string getNavigationStyle() const override;
    
    // IViewer3D interface - Capabilities
    bool supportsFeature(const std::string& feature) const override;
    std::string getVersion() const override;

private:
    /**
     * @brief Create a scene node for a ViewProvider
     * 
     * This method extracts the TopoDS_Shape from the Part::Feature object
     * and converts it to OSG geometry using GeometryConverter.
     * 
     * @param vp ViewProvider to create node for
     * @return OSG node, or nullptr if failed
     */
    osg::ref_ptr<osg::Node> createNodeForViewProvider(Gui::ViewProvider* vp);
    
    /**
     * @brief Create a placeholder sphere for objects without geometry
     * 
     * @return OSG node containing a red sphere
     */
    osg::ref_ptr<osg::Node> createPlaceholderSphere();
    
    /**
     * @brief Apply material to a node
     * 
     * @param node Node to apply material to
     * @param color Material color
     */
    void applyMaterial(osg::Node* node, const QColor& color);

private:
    OsgVerseWidget* _widget;                              ///< Qt OpenGL widget
    osg::ref_ptr<osg::Group> _sceneRoot;                  ///< Scene root node
    std::map<Gui::ViewProvider*, osg::ref_ptr<osg::Node>> _vpNodes; ///< ViewProvider to node mapping
    std::string _navigationStyle;                         ///< Current navigation style
};

} // namespace OsgVerseGui

#endif // OSGVERSEGUI_VIEWER_H
