import sys
import traceback

print("=" * 80)
print("Testing FreeCADGui Import")
print("=" * 80)

try:
    print("\n1. Importing FreeCAD...")
    import FreeCAD
    print("   SUCCESS: FreeCAD imported")
except Exception as e:
    print(f"   FAILED: {e}")
    traceback.print_exc()
    sys.exit(1)

try:
    print("\n2. Importing FreeCADGui...")
    import FreeCADGui
    print("   SUCCESS: FreeCADGui imported")
except Exception as e:
    print(f"   FAILED: {e}")
    traceback.print_exc()
    sys.exit(1)

print("\n" + "=" * 80)
print("All imports successful!")
print("=" * 80)
