from Base.Metadata import export, constmethod
from App.DocumentObject import DocumentObject
from App.Vector import Vector
from typing import List, Optional

@export(
    Twin="Feature",
    TwinPointer="Feature", 
    Include="Mod/Drawing/App/DrawingFeature.h",
    FatherInclude="App/DocumentObjectPy.h",
    Namespace="Drawing",
    FatherNamespace="App"
)
class DrawingFeature(DocumentObject):
    """
    Base class for all drawing objects in the Drawing workbench
    
    This class provides common properties and methods for 2D drawing objects
    such as lines, circles, rectangles, and text.
    
    Author: FreeCAD Project
    Licence: LGPL
    """
    
    # Common properties
    StartPoint: Vector = ...
    EndPoint: Vector = ...
    LineWidth: float = ...
    LineColor: tuple = ...
    LineStyle: str = ...
    Construction: bool = ...
    
    @constmethod
    def getViewProviderName(self) -> str:
        """Get the name of the associated view provider"""
        ...
    
    def recompute(self) -> bool:
        """Recompute the drawing object geometry"""
        ...
