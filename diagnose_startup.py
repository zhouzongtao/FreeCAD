# -*- coding: utf-8 -*-
"""
Diagnose FreeCAD startup and BackendRegistry

Run this in FreeCAD Python console
"""

print("=" * 60)
print("FreeCAD Startup Diagnostics")
print("=" * 60)

# Check 1: Basic imports
print("\n[1] Testing basic imports...")
try:
    import FreeCAD
    print(f"  OK: FreeCAD version {FreeCAD.Version()}")
except Exception as e:
    print(f"  ERROR: {e}")

try:
    import Gui
    print("  OK: Gui module imported")
except Exception as e:
    print(f"  ERROR: {e}")

# Check 2: Check Gui module attributes
print("\n[2] Checking Gui module attributes...")
try:
    import Gui
    attrs = dir(Gui)
    if 'BackendRegistry' in attrs:
        print("  OK: BackendRegistry found in Gui module")
    else:
        print("  ERROR: BackendRegistry NOT found in Gui module")
        print(f"  Available attributes: {[a for a in attrs if not a.startswith('_')][:10]}...")
except Exception as e:
    print(f"  ERROR: {e}")

# Check 3: Try to import BackendRegistry
print("\n[3] Trying to import BackendRegistry...")
try:
    from Gui import BackendRegistry
    print("  OK: BackendRegistry imported")
    print(f"  Type: {type(BackendRegistry)}")
    print(f"  Methods: {dir(BackendRegistry)[:5]}...")
except AttributeError as e:
    print(f"  ERROR: {e}")
    print("  BackendRegistry module not found in Gui")
except Exception as e:
    print(f"  ERROR: {e}")
    import traceback
    traceback.print_exc()

# Check 4: Check if modules can be imported
print("\n[4] Testing module imports...")
try:
    import CoinGui
    print("  OK: CoinGui imported")
except Exception as e:
    print(f"  ERROR: CoinGui - {e}")

try:
    import OsgVerseGui
    print("  OK: OsgVerseGui imported")
except Exception as e:
    print(f"  ERROR: OsgVerseGui - {e}")

print("\n" + "=" * 60)
print("Diagnostics complete")
print("=" * 60)
