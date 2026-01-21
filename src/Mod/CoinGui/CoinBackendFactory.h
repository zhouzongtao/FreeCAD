// SPDX-License-Identifier: LGPL-2.1-or-later
/**
 * CoinBackendFactory.h - Factory for Coin3D rendering backend
 */

#ifndef COINGUI_COINBACKENDFACTORY_H
#define COINGUI_COINBACKENDFACTORY_H

#include <Gui/View3D/Interfaces/IBackendFactory.h>

namespace CoinGui {

/**
 * @brief Factory for creating Coin3D viewers
 * 
 * This factory creates instances of the Coin3D rendering backend.
 */
class CoinBackendFactory : public Gui::IBackendFactory {
public:
    CoinBackendFactory();
    ~CoinBackendFactory() override;
    
    // IBackendFactory interface
    std::string getName() const override;
    std::string getDescription() const override;
    Gui::IViewer3D* createViewer(QWidget* parent = nullptr) override;
    void destroyViewer(Gui::IViewer3D* viewer) override;
    bool isAvailable() const override;
    std::string getVersion() const override;
    int getPriority() const override;
};

} // namespace CoinGui

#endif // COINGUI_COINBACKENDFACTORY_H
