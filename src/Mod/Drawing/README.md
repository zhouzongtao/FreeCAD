# Drawing Workbench

A complete C++ implementation of a 2D drawing workbench for FreeCAD, demonstrating the use of AST/YAPTU tools for Python binding generation.

## Overview

The Drawing workbench provides 2D drawing functionality similar to the Draft workbench but implemented entirely in C++ to showcase:

- Modern C++ FreeCAD module development
- AST/YAPTU Python binding generation
- Interactive mouse input handling
- Custom ViewProvider implementation
- Proper separation of App/Gui layers

## Features

### Drawing Objects
- **Line**: Create lines with start/end points, length, and angle
- **Circle**: Create circles and arcs with center, radius, and angle range  
- **Rectangle**: Create rectangles with width, height, and optional rounded corners
- **Polygon**: Create polygons with multiple points or regular polygons
- **Text**: Add text annotations with font, size, and positioning
- **Dimension**: Create measurement dimensions between objects

### Interactive Tools
- Mouse coordinate picking
- Grid snapping
- Real-time preview during creation
- Coordinate input dialog
- Context-sensitive prompts

### Display Features
- Customizable line styles (solid, dashed, dotted, dash-dot)
- Adjustable line width and colors
- Point markers for control points
- Construction geometry support

## Architecture

### C++ Core (App layer)
```
Drawing/App/
├── DrawingFeature.h/.cpp     # Base feature class and derived objects
├── DrawingFeature.pyi        # Python binding interface definition  
├── Line.pyi                  # Line object Python interface
├── Circle.pyi                # Circle object Python interface
└── CMakeLists.txt            # Build configuration with AST/YAPTU
```

### GUI Components (Gui layer)  
```
Drawing/Gui/
├── ViewProviderDrawing.h/.cpp # Visual representation classes
├── Command.h/.cpp            # Interactive drawing commands
├── Workbench.h/.cpp          # Workbench definition
└── CMakeLists.txt            # GUI build configuration
```

### Python Integration
```
Drawing/
├── Init.py                   # Module initialization
├── InitGui.py                # GUI workbench registration
└── drawing.dox               # Documentation
```

## Building

The Drawing workbench uses AST/YAPTU tools for automatic Python binding generation:

1. **Interface Definition**: Python .pyi files define the binding interface
2. **AST Parsing**: Python AST parser extracts class and method information
3. **Code Generation**: YAPTU template engine generates C++ binding code
4. **Compilation**: Standard C++ compilation of generated and hand-written code

### Build Commands

```bash
# Configure build
cmake -B build -S .

# Build the Drawing workbench
cmake --build build --target Drawing DrawingGui

# The AST/YAPTU tools will automatically:
# 1. Parse DrawingFeature.pyi, Line.pyi, Circle.pyi
# 2. Generate DrawingFeaturePy.h/.cpp, LinePy.h/.cpp, CirclePy.h/.cpp  
# 3. Compile the complete workbench
```

## Usage

### Python API

```python
import FreeCAD as App
import Drawing

# Create drawing objects programmatically
doc = App.newDocument()

# Create a line
line = Drawing.makeLine(App.Vector(0,0,0), App.Vector(10,5,0))

# Create a circle  
circle = Drawing.makeCircle(App.Vector(0,0,0), 5.0)

# Create a rectangle
rect = Drawing.makeRectangle(App.Vector(-5,-5,0), 10.0, 8.0)

doc.recompute()
```

### Interactive Usage

1. Switch to Drawing workbench
2. Use toolbar tools for interactive drawing:
   - **Line tool**: Click two points to create a line
   - **Circle tool**: Click center, then click for radius
   - **Rectangle tool**: Click two diagonal corners
   - **Text tool**: Click position and enter text
   - **Dimension tool**: Click two points and dimension line position

## Development Notes

This workbench demonstrates several important FreeCAD development concepts:

### AST/YAPTU Usage
- Shows how to define Python interfaces in .pyi files
- Demonstrates automatic binding generation
- Illustrates the integration with CMake build system

### C++ Architecture
- Proper inheritance from FreeCAD base classes
- Property system usage
- Signal/slot integration
- Exception handling

### GUI Development
- Custom ViewProvider implementation
- Interactive command development
- Mouse event handling
- 3D scene graph manipulation

## Learning Value

This workbench serves as a comprehensive example for developers wanting to:

1. **Learn C++ FreeCAD development**
2. **Understand AST/YAPTU binding generation**
3. **Implement interactive mouse tools**
4. **Create custom ViewProviders**
5. **Structure complex FreeCAD modules**

## License

LGPL v2.1 or later
