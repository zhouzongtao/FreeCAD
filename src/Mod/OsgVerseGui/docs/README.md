# FreeCAD Render Abstraction Layer Documentation

This documentation covers the FreeCAD Render Abstraction Layer, which provides a unified interface for multiple 3D rendering backends.

## Overview

The Render Abstraction Layer allows FreeCAD to support multiple rendering backends through a common interface (`IViewer3D`). Currently supported backends:

- **Coin3D** (Default) - The traditional FreeCAD rendering backend using Open Inventor
- **OsgVerse** - Modern OpenSceneGraph-based rendering backend

## Documentation Contents

1. [IViewer3D API Reference](API_Reference.md) - Complete interface documentation
2. [Backend Implementation Guide](Backend_Implementation_Guide.md) - How to create new backends
3. [ViewProvider Integration](ViewProvider_Integration.md) - ViewProvider system integration
4. [Geometry Converter Guide](Geometry_Converter_Guide.md) - OCCT to rendering format conversion
5. [Performance Optimization](Performance_Optimization.md) - Best practices and tips
6. [Architecture Overview](Architecture_Overview.md) - System design and structure

## Quick Start

### Using OsgVerse Backend

```python
import FreeCAD
import FreeCADGui
import OsgVerseGui

# OsgVerse backend is automatically registered on import
# To create a view with OsgVerse:
# (This is handled automatically by FreeCAD's view creation system)
```

### Running Tests

```bash
# Run headless tests
FreeCADCmd -c "
import sys
sys.path.insert(0, 'Mod/OsgVerseGui')
import TestHeadless
import unittest
unittest.TextTestRunner(verbosity=2).run(TestHeadless.suite())
"
```

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      FreeCAD GUI                             │
├─────────────────────────────────────────────────────────────┤
│                    View3DInventor                            │
│              (Document View Container)                       │
├─────────────────────────────────────────────────────────────┤
│                     IViewer3D                                │
│              (Abstract Viewer Interface)                     │
├──────────────────────┬──────────────────────────────────────┤
│     CoinViewer       │         OsgVerseViewer               │
│   (Coin3D Backend)   │       (OsgVerse Backend)             │
├──────────────────────┼──────────────────────────────────────┤
│  View3DInventorViewer│      OsgVerseWidget                  │
│  (Existing Coin3D)   │    (Qt OpenGL Widget)                │
└──────────────────────┴──────────────────────────────────────┘
```

## License

LGPL-2.1-or-later
