import sys

print("Testing OsgVerse Backend")
print("=" * 60)

import FreeCADGui

print("\n1. Checking for getCurrentRenderBackend...")
if hasattr(FreeCADGui, 'getCurrentRenderBackend'):
    print("   FOUND")
    backend = FreeCADGui.getCurrentRenderBackend()
    print("   Current backend ID:", backend)
else:
    print("   NOT FOUND")
    print("\n2. Available FreeCADGui attributes:")
    for attr in sorted(dir(FreeCADGui)):
        if not attr.startswith('_'):
            print("   -", attr)
