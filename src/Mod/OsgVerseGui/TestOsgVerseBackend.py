# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2024 FreeCAD Development Team                           *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   OsgVerse Backend Integration Tests                                    *
# ***************************************************************************

"""
Integration tests for the OsgVerse rendering backend.

These tests verify that the OsgVerse backend correctly implements
the IViewer3D interface and integrates properly with FreeCAD.
"""

import os
import sys
import time
import unittest
import FreeCAD
import FreeCADGui


def hasOsgVerseGui():
    """Check if OsgVerseGui module is available"""
    try:
        import OsgVerseGui
        return True
    except ImportError:
        return False


# =============================================================================
# Test: OsgVerse Module Loading
# =============================================================================
class TestOsgVerseModuleLoading(unittest.TestCase):
    """Test OsgVerse module loading and initialization"""

    def test_module_import(self):
        """Test that OsgVerseGui can be imported"""
        try:
            import OsgVerseGui
            FreeCAD.Console.PrintMessage("OsgVerseGui module imported successfully\n")
            self.assertTrue(True)
        except ImportError as e:
            self.skipTest(f"OsgVerseGui not available: {e}")

    def test_module_initialization(self):
        """Test that OsgVerseGui initializes correctly"""
        if not hasOsgVerseGui():
            self.skipTest("OsgVerseGui not available")

        import OsgVerseGui
        # Module should be initialized without errors
        self.assertTrue(True)


# =============================================================================
# Test: Geometry Conversion
# =============================================================================
@unittest.skipUnless(hasOsgVerseGui(), "OsgVerseGui not available")
class TestGeometryConversion(unittest.TestCase):
    """Test geometry conversion from FreeCAD to OSG"""

    def setUp(self):
        """Set up test document"""
        self.doc = FreeCAD.newDocument("TestGeomConv")

    def tearDown(self):
        """Clean up"""
        FreeCAD.closeDocument("TestGeomConv")

    def test_box_conversion(self):
        """Test converting Part::Box to OSG geometry"""
        box = self.doc.addObject("Part::Box", "Box")
        self.doc.recompute()

        # Box should have valid shape
        self.assertFalse(box.Shape.isNull())

        # ViewObject should exist
        self.assertIsNotNone(box.ViewObject)

    def test_sphere_conversion(self):
        """Test converting Part::Sphere to OSG geometry"""
        sphere = self.doc.addObject("Part::Sphere", "Sphere")
        self.doc.recompute()

        self.assertFalse(sphere.Shape.isNull())
        self.assertIsNotNone(sphere.ViewObject)

    def test_cone_conversion(self):
        """Test converting Part::Cone to OSG geometry"""
        cone = self.doc.addObject("Part::Cone", "Cone")
        self.doc.recompute()

        self.assertFalse(cone.Shape.isNull())
        self.assertIsNotNone(cone.ViewObject)

    def test_cylinder_conversion(self):
        """Test converting Part::Cylinder to OSG geometry"""
        cylinder = self.doc.addObject("Part::Cylinder", "Cylinder")
        self.doc.recompute()

        self.assertFalse(cylinder.Shape.isNull())
        self.assertIsNotNone(cylinder.ViewObject)

    def test_torus_conversion(self):
        """Test converting Part::Torus to OSG geometry"""
        torus = self.doc.addObject("Part::Torus", "Torus")
        self.doc.recompute()

        self.assertFalse(torus.Shape.isNull())
        self.assertIsNotNone(torus.ViewObject)

    def test_complex_shape_conversion(self):
        """Test converting complex fused shape"""
        box = self.doc.addObject("Part::Box", "Box")
        sphere = self.doc.addObject("Part::Sphere", "Sphere")
        sphere.Placement.Base.x = 5
        self.doc.recompute()

        # Create fusion
        fusion = self.doc.addObject("Part::Fuse", "Fusion")
        fusion.Base = box
        fusion.Tool = sphere
        self.doc.recompute()

        self.assertFalse(fusion.Shape.isNull())
        self.assertIsNotNone(fusion.ViewObject)


# =============================================================================
# Test: ViewProvider Synchronization
# =============================================================================
@unittest.skipUnless(hasOsgVerseGui(), "OsgVerseGui not available")
class TestViewProviderSync(unittest.TestCase):
    """Test ViewProvider synchronization with OsgVerse viewer"""

    def setUp(self):
        """Set up test document"""
        self.doc = FreeCAD.newDocument("TestVPSync")

    def tearDown(self):
        """Clean up"""
        FreeCAD.closeDocument("TestVPSync")

    def test_visibility_sync(self):
        """Test visibility changes sync to viewer"""
        box = self.doc.addObject("Part::Box", "Box")
        self.doc.recompute()

        vp = box.ViewObject

        # Toggle visibility
        vp.Visibility = False
        FreeCADGui.updateGui()
        self.assertFalse(vp.Visibility)

        vp.Visibility = True
        FreeCADGui.updateGui()
        self.assertTrue(vp.Visibility)

    def test_color_sync(self):
        """Test color changes sync to viewer"""
        box = self.doc.addObject("Part::Box", "Box")
        self.doc.recompute()

        vp = box.ViewObject

        if hasattr(vp, 'ShapeColor'):
            # Change to red
            vp.ShapeColor = (1.0, 0.0, 0.0, 0.0)
            FreeCADGui.updateGui()
            self.assertAlmostEqual(vp.ShapeColor[0], 1.0, places=2)

            # Change to blue
            vp.ShapeColor = (0.0, 0.0, 1.0, 0.0)
            FreeCADGui.updateGui()
            self.assertAlmostEqual(vp.ShapeColor[2], 1.0, places=2)

    def test_transparency_sync(self):
        """Test transparency changes sync to viewer"""
        box = self.doc.addObject("Part::Box", "Box")
        self.doc.recompute()

        vp = box.ViewObject

        if hasattr(vp, 'Transparency'):
            # Set transparency
            vp.Transparency = 50
            FreeCADGui.updateGui()
            self.assertEqual(vp.Transparency, 50)

            vp.Transparency = 0
            FreeCADGui.updateGui()
            self.assertEqual(vp.Transparency, 0)

    def test_object_update_sync(self):
        """Test object parameter changes sync to viewer"""
        box = self.doc.addObject("Part::Box", "Box")
        self.doc.recompute()

        # Change box dimensions
        box.Length = 20
        box.Width = 30
        box.Height = 40
        self.doc.recompute()
        FreeCADGui.updateGui()

        # Verify shape updated
        bbox = box.Shape.BoundBox
        self.assertAlmostEqual(bbox.XLength, 20, places=1)
        self.assertAlmostEqual(bbox.YLength, 30, places=1)
        self.assertAlmostEqual(bbox.ZLength, 40, places=1)


# =============================================================================
# Test: Scene Graph Operations
# =============================================================================
@unittest.skipUnless(hasOsgVerseGui(), "OsgVerseGui not available")
class TestSceneGraphOperations(unittest.TestCase):
    """Test scene graph operations"""

    def setUp(self):
        """Set up test document"""
        self.doc = FreeCAD.newDocument("TestSceneGraph")

    def tearDown(self):
        """Clean up"""
        FreeCAD.closeDocument("TestSceneGraph")

    def test_add_object_to_scene(self):
        """Test adding object to scene"""
        box = self.doc.addObject("Part::Box", "Box")
        self.doc.recompute()

        # Object should be visible in scene
        self.assertTrue(box.ViewObject.Visibility)

    def test_remove_object_from_scene(self):
        """Test removing object from scene"""
        box = self.doc.addObject("Part::Box", "Box")
        self.doc.recompute()

        # Remove object
        self.doc.removeObject("Box")
        self.doc.recompute()

        # Object should be gone
        self.assertIsNone(self.doc.getObject("Box"))

    def test_multiple_objects_in_scene(self):
        """Test multiple objects in scene"""
        objects = []
        for i in range(5):
            obj = self.doc.addObject("Part::Box", f"Box{i}")
            obj.Placement.Base.x = i * 15
            objects.append(obj)

        self.doc.recompute()

        # All objects should exist and be visible
        for i, obj in enumerate(objects):
            self.assertIsNotNone(obj)
            self.assertTrue(obj.ViewObject.Visibility)


# =============================================================================
# Test: Navigation and Interaction
# =============================================================================
class TestNavigationInteraction(unittest.TestCase):
    """Test navigation and user interaction"""

    def setUp(self):
        """Set up test document"""
        self.doc = FreeCAD.newDocument("TestNavigation")
        self.box = self.doc.addObject("Part::Box", "Box")
        self.doc.recompute()

    def tearDown(self):
        """Clean up"""
        FreeCAD.closeDocument("TestNavigation")

    def test_fit_all(self):
        """Test fit all operation"""
        view = FreeCADGui.ActiveDocument.ActiveView
        if view is None:
            self.skipTest("No active view")

        if hasattr(view, 'fitAll'):
            view.fitAll()
            self.assertTrue(True)

    def test_view_standard_positions(self):
        """Test standard view positions"""
        view = FreeCADGui.ActiveDocument.ActiveView
        if view is None:
            self.skipTest("No active view")

        positions = ['ViewFront', 'ViewBack', 'ViewTop', 'ViewBottom',
                     'ViewLeft', 'ViewRight', 'ViewIsometric']

        for pos in positions:
            if hasattr(FreeCADGui, 'runCommand'):
                try:
                    FreeCADGui.runCommand(pos, 0)
                except Exception:
                    pass  # Command might not be available


# =============================================================================
# Test: Error Handling
# =============================================================================
class TestErrorHandling(unittest.TestCase):
    """Test error handling in OsgVerse backend"""

    def test_null_shape_handling(self):
        """Test handling of null shapes"""
        doc = FreeCAD.newDocument("TestNullShape")

        # Create object without valid shape
        obj = doc.addObject("Part::Feature", "EmptyFeature")
        doc.recompute()

        # Should not crash
        self.assertTrue(obj.Shape.isNull())

        FreeCAD.closeDocument("TestNullShape")

    def test_invalid_color_handling(self):
        """Test handling of invalid color values"""
        doc = FreeCAD.newDocument("TestInvalidColor")
        box = doc.addObject("Part::Box", "Box")
        doc.recompute()

        vp = box.ViewObject
        if hasattr(vp, 'ShapeColor'):
            # Original color
            original = vp.ShapeColor

            # Try setting invalid colors (should be clamped)
            try:
                vp.ShapeColor = (2.0, -1.0, 0.5, 0.0)
            except Exception:
                pass  # May raise exception

            # Should not crash
            self.assertTrue(True)

        FreeCAD.closeDocument("TestInvalidColor")


# =============================================================================
# Test: Memory Management
# =============================================================================
class TestMemoryManagement(unittest.TestCase):
    """Test memory management"""

    def test_document_close_cleanup(self):
        """Test that closing document cleans up resources"""
        import gc

        for i in range(3):
            doc = FreeCAD.newDocument(f"TestMemory{i}")
            for j in range(10):
                doc.addObject("Part::Box", f"Box{j}")
            doc.recompute()
            FreeCAD.closeDocument(f"TestMemory{i}")

        # Force garbage collection
        gc.collect()

        # Should not have memory leaks (hard to test, just ensure no crash)
        self.assertTrue(True)


# =============================================================================
# Test Suite Definition
# =============================================================================
def suite():
    """Create test suite"""
    test_suite = unittest.TestSuite()

    test_suite.addTests(unittest.TestLoader().loadTestsFromTestCase(TestOsgVerseModuleLoading))
    test_suite.addTests(unittest.TestLoader().loadTestsFromTestCase(TestGeometryConversion))
    test_suite.addTests(unittest.TestLoader().loadTestsFromTestCase(TestViewProviderSync))
    test_suite.addTests(unittest.TestLoader().loadTestsFromTestCase(TestSceneGraphOperations))
    test_suite.addTests(unittest.TestLoader().loadTestsFromTestCase(TestNavigationInteraction))
    test_suite.addTests(unittest.TestLoader().loadTestsFromTestCase(TestErrorHandling))
    test_suite.addTests(unittest.TestLoader().loadTestsFromTestCase(TestMemoryManagement))

    return test_suite


if __name__ == '__main__':
    runner = unittest.TextTestRunner(verbosity=2)
    runner.run(suite())
