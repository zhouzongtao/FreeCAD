// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#ifndef _PreComp_
# include <Inventor/C/basic.h>
#endif

#include "CoinBackendFactory.h"
#include "CoinViewer.h"
#include <Base/Console.h>

using namespace CoinGui;

CoinBackendFactory::CoinBackendFactory()
{
    Base::Console().message("CoinBackendFactory: Initialized\n");
}

CoinBackendFactory::~CoinBackendFactory()
{
    Base::Console().message("CoinBackendFactory: Destroyed\n");
}

std::string CoinBackendFactory::getName() const
{
    return "Coin3D";
}

std::string CoinBackendFactory::getDescription() const
{
    return "Coin3D rendering backend (default, stable)";
}

Gui::IViewer3D* CoinBackendFactory::createViewer(QWidget* parent)
{
    try {
        Base::Console().message("CoinBackendFactory: Creating Coin3D viewer\n");
        return new CoinViewer(parent);
    }
    catch (const std::exception& e) {
        Base::Console().error("CoinBackendFactory: Failed to create viewer: %s\n", e.what());
        return nullptr;
    }
}

void CoinBackendFactory::destroyViewer(Gui::IViewer3D* viewer)
{
    if (viewer) {
        Base::Console().message("CoinBackendFactory: Destroying viewer\n");
        delete viewer;
    }
}

bool CoinBackendFactory::isAvailable() const
{
    // Check if Coin3D is available
    // This is always true if we compiled with Coin3D support
    return true;
}

std::string CoinBackendFactory::getVersion() const
{
    // Get Coin3D version
    // Note: coin_get_version() requires Coin3D headers
    return "Coin3D 4.0+";
}

int CoinBackendFactory::getPriority() const
{
    // Coin3D is the default backend, give it highest priority
    return 10;
}
