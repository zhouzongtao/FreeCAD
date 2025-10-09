from Base.Metadata import export, constmethod
from .DrawingFeature import DrawingFeature
from App.Vector import Vector

@export(
    Twin="Line",
    TwinPointer="Line",
    Include="Mod/Drawing/App/DrawingFeature.h", 
    FatherInclude="Mod/Drawing/App/DrawingFeaturePy.h",
    Namespace="Drawing",
    FatherNamespace="Drawing"
)
class Line(DrawingFeature):
    """
    Line drawing object
    
    Creates a straight line between two points or from a start point
    with specified length and angle.
    
    Author: FreeCAD Project
    Licence: LGPL
    """
    
    # Line specific properties
    Length: float = ...
    Angle: float = ...
    
    def calculateGeometry(self) -> None:
        """Calculate line geometry based on start point, length and angle"""
        ...
    
    @constmethod
    def getDirection(self) -> Vector:
        """Get the direction vector of the line"""
        ...
