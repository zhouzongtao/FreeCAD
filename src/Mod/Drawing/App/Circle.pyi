from Base.Metadata import export, constmethod
from .DrawingFeature import DrawingFeature
from App.Vector import Vector

@export(
    Twin="Circle",
    TwinPointer="Circle",
    Include="Mod/Drawing/App/DrawingFeature.h",
    FatherInclude="Mod/Drawing/App/DrawingFeaturePy.h", 
    Namespace="Drawing",
    FatherNamespace="Drawing"
)
class Circle(DrawingFeature):
    """
    Circle drawing object
    
    Creates a circle or circular arc with specified center, radius,
    and optional start/end angles.
    
    Author: FreeCAD Project
    Licence: LGPL
    """
    
    # Circle specific properties
    Center: Vector = ...
    Radius: float = ...
    FirstAngle: float = ...
    LastAngle: float = ...
    
    def calculateGeometry(self) -> None:
        """Calculate circle geometry"""
        ...
    
    @constmethod
    def isFullCircle(self) -> bool:
        """Check if this is a full circle (360 degrees)"""
        ...
