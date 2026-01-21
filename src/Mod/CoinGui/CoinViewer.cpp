// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#ifndef _PreComp_
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoCamera.h>
# include <Inventor/nodes/SoPerspectiveCamera.h>
#endif

#include "CoinViewer.h"
#include <Base/Console.h>
#include <Gui/ViewProvider.h>
#include <Gui/Quarter/QuarterWidget.h>
#include <App/DocumentObject.h>

using namespace CoinGui;
using SIM::Coin3D::Quarter::QuarterWidget;

CoinViewer::CoinViewer(QWidget* parent)
    : _viewer(nullptr)
    , _sceneRoot(nullptr)
    , _navigationStyle("Inventor")
{
    Base::Console().message("CoinViewer: Creating viewer\n");
    
    // Create scene root
    _sceneRoot = new SoSeparator();
    _sceneRoot->ref();
    
    // Create Quarter viewer widget
    _viewer = new QuarterWidget(parent);
    _viewer->setSceneGraph(_sceneRoot);
    _viewer->setBackgroundColor(QColor(0, 0, 0));
    _viewer->show();
    
    Base::Console().message("CoinViewer: Viewer created successfully\n");
}

CoinViewer::~CoinViewer()
{
    Base::Console().message("CoinViewer: Destroying viewer\n");
    
    // Clean up view provider nodes
    for (auto& pair : _vpNodes) {
        if (pair.second) {
            pair.second->unref();
        }
    }
    _vpNodes.clear();
    
    // Clean up scene
    if (_sceneRoot) {
        _sceneRoot->unref();
        _sceneRoot = nullptr;
    }
    
    // Delete viewer widget
    if (_viewer) {
        delete _viewer;
        _viewer = nullptr;
    }
    
    Base::Console().message("CoinViewer: Viewer destroyed\n");
}

std::string CoinViewer::getBackendName() const
{
    return "Coin3D";
}

QWidget* CoinViewer::getWidget()
{
    return _viewer;
}

void CoinViewer::addViewProvider(Gui::ViewProvider* vp)
{
    if (!vp) {
        Base::Console().warning("CoinViewer: Cannot add null ViewProvider\n");
        return;
    }
    
    Base::Console().message("CoinViewer: Adding ViewProvider\n");
    
    // TODO: Get Coin3D scene graph from ViewProvider
    // For now, create a placeholder node
    SoSeparator* vpNode = new SoSeparator();
    vpNode->ref();
    
    _vpNodes[vp] = vpNode;
    _sceneRoot->addChild(vpNode);
    
    render();
}

void CoinViewer::removeViewProvider(Gui::ViewProvider* vp)
{
    if (!vp) {
        return;
    }
    
    auto it = _vpNodes.find(vp);
    if (it != _vpNodes.end()) {
        Base::Console().message("CoinViewer: Removing ViewProvider\n");
        
        _sceneRoot->removeChild(it->second);
        it->second->unref();
        _vpNodes.erase(it);
        
        render();
    }
}

void CoinViewer::updateViewProvider(Gui::ViewProvider* vp)
{
    if (!vp) {
        return;
    }
    
    auto it = _vpNodes.find(vp);
    if (it != _vpNodes.end()) {
        Base::Console().message("CoinViewer: Updating ViewProvider\n");
        // TODO: Update the scene graph
        render();
    }
}

void CoinViewer::clearScene()
{
    Base::Console().message("CoinViewer: Clearing scene\n");
    
    // Remove all view provider nodes
    for (auto& pair : _vpNodes) {
        _sceneRoot->removeChild(pair.second);
        pair.second->unref();
    }
    _vpNodes.clear();
    
    render();
}

void CoinViewer::render()
{
    if (_viewer) {
        _viewer->redraw();
    }
}

void CoinViewer::setBackgroundColor(const QColor& color)
{
    if (_viewer) {
        _viewer->setBackgroundColor(color);
        render();
    }
}

void CoinViewer::setAntiAliasing(bool enable)
{
    // TODO: Implement anti-aliasing control
    Base::Console().message("CoinViewer: Anti-aliasing %s\n", enable ? "enabled" : "disabled");
}

void CoinViewer::viewAll()
{
    if (_viewer) {
        _viewer->viewAll();
    }
}

void CoinViewer::setCamera(const float position[3], 
                          const float orientation[4],
                          const float up[3])
{
    if (!_viewer) {
        return;
    }
    
    SoCamera* camera = _viewer->getSoRenderManager()->getCamera();
    if (camera) {
        camera->position.setValue(position[0], position[1], position[2]);
        camera->orientation.setValue(orientation[0], orientation[1], orientation[2], orientation[3]);
        render();
    }
}

void CoinViewer::getCamera(float position[3], 
                          float orientation[4],
                          float up[3]) const
{
    if (!_viewer) {
        return;
    }
    
    SoCamera* camera = _viewer->getSoRenderManager()->getCamera();
    if (camera) {
        const SbVec3f& pos = camera->position.getValue();
        position[0] = pos[0];
        position[1] = pos[1];
        position[2] = pos[2];
        
        const SbRotation& rot = camera->orientation.getValue();
        float x, y, z, w;
        rot.getValue(x, y, z, w);
        orientation[0] = x;
        orientation[1] = y;
        orientation[2] = z;
        orientation[3] = w;
        
        // TODO: Get up vector
        up[0] = 0.0f;
        up[1] = 1.0f;
        up[2] = 0.0f;
    }
}

std::vector<App::DocumentObject*> CoinViewer::getSelection() const
{
    // TODO: Implement selection retrieval
    return std::vector<App::DocumentObject*>();
}

void CoinViewer::setSelection(const std::vector<App::DocumentObject*>& objects)
{
    // TODO: Implement selection
    Base::Console().message("CoinViewer: Setting selection (%zu objects)\n", objects.size());
}

void CoinViewer::clearSelection()
{
    // TODO: Implement selection clearing
    Base::Console().message("CoinViewer: Clearing selection\n");
}

void CoinViewer::setNavigationStyle(const std::string& style)
{
    _navigationStyle = style;
    Base::Console().message("CoinViewer: Navigation style set to '%s'\n", style.c_str());
    // TODO: Actually change the navigation style
}

std::string CoinViewer::getNavigationStyle() const
{
    return _navigationStyle;
}

bool CoinViewer::supportsFeature(const std::string& feature) const
{
    // Coin3D supports most standard features
    if (feature == "transparency") return true;
    if (feature == "selection") return true;
    if (feature == "navigation") return true;
    if (feature == "camera") return true;
    if (feature == "lighting") return true;
    
    return false;
}

std::string CoinViewer::getVersion() const
{
    return "Coin3D 4.0+";
}
