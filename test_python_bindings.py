import sys

print("=" * 80)
print("Checking FreeCADGui Python Bindings")
print("=" * 80)

import FreeCADGui

print("\nAvailable attributes in FreeCADGui:")
attrs = [attr for attr in dir(FreeCADGui) if not attr.startswith('_')]
for attr in sorted(attrs):
    print(f"  - {attr}")

print("\nLooking for RenderManager functions:")
render_funcs = [
    'getCurrentRenderBackend',
    'switchRenderBackend',
    'isRenderBackendAvailable',
    'getRendererInfo',
    'getRenderStats',
    'resetRenderStats'
]

for func in render_funcs:
    if hasattr(FreeCADGui, func):
        print(f"  ✓ {func}")
    else:
        print(f"  ✗ {func} - NOT FOUND")

print("\n" + "=" * 80)
