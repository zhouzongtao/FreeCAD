# Quick inline test for BackendRegistry
# Copy and paste this directly into FreeCAD Python console

print("=" * 60)
print("Testing BackendRegistry")
print("=" * 60)

# Step 1: Import Gui
print("\nStep 1: Import Gui module...")
try:
    import Gui
    print("OK: Gui module imported")
except ImportError as e:
    print(f"ERROR: {e}")
    
# Step 2: Try to access BackendRegistry
print("\nStep 2: Access BackendRegistry...")
try:
    from Gui import BackendRegistry
    print("OK: BackendRegistry imported")
except ImportError as e:
    print(f"ERROR: BackendRegistry not available: {e}")
    print("Note: BackendRegistry Python bindings need to be compiled")
    print("FreeCADGui needs to be recompiled with BackendRegistryPy.cpp")
    
# Step 3: Import OsgVerseGui
print("\nStep 3: Import OsgVerseGui...")
try:
    import OsgVerseGui
    print("OK: OsgVerseGui module imported")
except ImportError as e:
    print(f"ERROR: {e}")
    import sys
    print(f"Python path: {sys.path[:3]}")
    
# Step 4: Import CoinGui
print("\nStep 4: Import CoinGui...")
try:
    import CoinGui
    print("OK: CoinGui module imported")
except ImportError as e:
    print(f"ERROR: {e}")

print("\n" + "=" * 60)
print("Test complete")
print("=" * 60)
