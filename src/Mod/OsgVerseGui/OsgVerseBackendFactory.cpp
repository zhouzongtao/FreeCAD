// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#include "OsgVerseBackendFactory.h"
#include "OsgVerseViewer.h"
#include <Base/Console.h>

using namespace OsgVerseGui;

OsgVerseBackendFactory::OsgVerseBackendFactory()
{
    Base::Console().message("OsgVerseBackendFactory: Created\n");
}

OsgVerseBackendFactory::~OsgVerseBackendFactory()
{
    Base::Console().message("OsgVerseBackendFactory: Destroyed\n");
}

std::string OsgVerseBackendFactory::getName() const
{
    return "OsgVerse";
}

std::string OsgVerseBackendFactory::getDescription() const
{
    return "OsgVerse rendering backend using OpenSceneGraph";
}

Gui::IViewer3D* OsgVerseBackendFactory::createViewer(QWidget* parent)
{
    Base::Console().message("OsgVerseBackendFactory: Creating viewer\n");
    
    try {
        return new OsgVerseViewer(parent);
    }
    catch (const std::exception& e) {
        Base::Console().error("OsgVerseBackendFactory: Failed to create viewer: %s\n", e.what());
        return nullptr;
    }
    catch (...) {
        Base::Console().error("OsgVerseBackendFactory: Failed to create viewer (unknown exception)\n");
        return nullptr;
    }
}

void OsgVerseBackendFactory::destroyViewer(Gui::IViewer3D* viewer)
{
    if (viewer) {
        Base::Console().message("OsgVerseBackendFactory: Destroying viewer\n");
        delete viewer;
    }
}

bool OsgVerseBackendFactory::isAvailable() const
{
    // Check if OSG is available
    // For now, always return true since we're compiled with OSG
    return true;
}

std::string OsgVerseBackendFactory::getVersion() const
{
    return "OsgVerse + OSG 3.6+";
}

int OsgVerseBackendFactory::getPriority() const
{
    // Lower priority than Coin3D (which is 10)
    // OsgVerse is optional/experimental
    return 5;
}
