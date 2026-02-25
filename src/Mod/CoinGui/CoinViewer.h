// SPDX-License-Identifier: LGPL-2.1-or-later
/**
 * CoinViewer.h - Coin3D implementation of IViewer3D
 */

#ifndef COINGUI_COINVIEWER_H
#define COINGUI_COINVIEWER_H

#include <Gui/View3D/Interfaces/IViewer3D.h>
#include <map>

// Forward declarations
namespace SIM { namespace Coin3D { namespace Quarter { class QuarterWidget; }}}
class SoSeparator;
class SoCamera;

namespace Gui {
    class ViewProvider;
}

namespace CoinGui {

/**
 * @brief Coin3D implementation of the 3D viewer interface
 * 
 * This class wraps the existing Coin3D viewer functionality and
 * provides it through the IViewer3D interface.
 */
class CoinViewer : public Gui::IViewer3D {
public:
    explicit CoinViewer(QWidget* parent = nullptr);
    ~CoinViewer() override;
    
    // IViewer3D interface implementation
    std::string getBackendName() const override;
    QWidget* getWidget() override;
    
    // Scene Management
    void addViewProvider(Gui::ViewProvider* vp) override;
    void removeViewProvider(Gui::ViewProvider* vp) override;
    void updateViewProvider(Gui::ViewProvider* vp) override;
    void clearScene() override;
    
    // Rendering Control
    void render() override;
    void setBackgroundColor(const QColor& color) override;
    void setAntiAliasing(bool enable) override;
    
    // Camera Control
    void viewAll() override;
    void setCamera(const float position[3], 
                  const float orientation[4],
                  const float up[3]) override;
    void getCamera(float position[3], 
                  float orientation[4],
                  float up[3]) const override;
    
    // Selection
    std::vector<App::DocumentObject*> getSelection() const override;
    void setSelection(const std::vector<App::DocumentObject*>& objects) override;
    void clearSelection() override;
    
    // Interaction
    void setNavigationStyle(const std::string& style) override;
    std::string getNavigationStyle() const override;
    
    // Capabilities
    bool supportsFeature(const std::string& feature) const override;
    std::string getVersion() const override;
    
    // Access to internal Coin3D viewer (for compatibility)
    SIM::Coin3D::Quarter::QuarterWidget* getQuarterWidget() const { return _viewer; }
    SoSeparator* getSceneGraph() const { return _sceneRoot; }
    
private:
    SIM::Coin3D::Quarter::QuarterWidget* _viewer;
    SoSeparator* _sceneRoot;
    std::map<Gui::ViewProvider*, SoSeparator*> _vpNodes;
    std::string _navigationStyle;
};

} // namespace CoinGui

#endif // COINGUI_COINVIEWER_H
