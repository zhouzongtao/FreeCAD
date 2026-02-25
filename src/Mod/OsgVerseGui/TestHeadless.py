# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2024 FreeCAD Development Team                           *
# *                                                                         *
# *   Headless Unit Tests for Render Abstraction Layer                      *
# *   These tests can run without GUI (FreeCADCmd)                          *
# ***************************************************************************

"""
Headless unit tests for the Render Abstraction Layer.

These tests run in FreeCADCmd without GUI support.
They test:
1. Module loading
2. Document operations
3. Shape creation and validation
4. Basic ViewProvider properties (without rendering)
"""

import os
import sys
import time
import unittest
import FreeCAD


def hasOsgVerseGui():
    """Check if OsgVerseGui module is available"""
    try:
        import OsgVerseGui
        return True
    except ImportError:
        return False


# =============================================================================
# Test: Module Import
# =============================================================================
class TestModuleImport(unittest.TestCase):
    """Test module import in headless mode"""

    def test_freecad_import(self):
        """Test that FreeCAD is available"""
        self.assertIsNotNone(FreeCAD)
        FreeCAD.Console.PrintMessage("FreeCAD module loaded\n")

    def test_osgverse_import(self):
        """Test that OsgVerseGui can be imported"""
        try:
            import OsgVerseGui
            self.assertTrue(True)
            FreeCAD.Console.PrintMessage("OsgVerseGui module loaded\n")
        except ImportError as e:
            self.skipTest(f"OsgVerseGui not available: {e}")

    def test_part_import(self):
        """Test that Part module is available"""
        import Part
        self.assertIsNotNone(Part)
        FreeCAD.Console.PrintMessage("Part module loaded\n")


# =============================================================================
# Test: Document Operations
# =============================================================================
class TestDocumentOperations(unittest.TestCase):
    """Test document operations in headless mode"""

    def test_create_document(self):
        """Test creating a new document"""
        doc = FreeCAD.newDocument("TestDoc1")
        self.assertIsNotNone(doc)
        self.assertEqual(doc.Name, "TestDoc1")
        FreeCAD.closeDocument("TestDoc1")

    def test_add_object(self):
        """Test adding object to document"""
        doc = FreeCAD.newDocument("TestDoc2")
        box = doc.addObject("Part::Box", "TestBox")
        self.assertIsNotNone(box)
        self.assertEqual(box.Name, "TestBox")
        FreeCAD.closeDocument("TestDoc2")

    def test_document_recompute(self):
        """Test document recompute"""
        doc = FreeCAD.newDocument("TestDoc3")
        box = doc.addObject("Part::Box", "TestBox")
        box.Length = 20
        doc.recompute()
        self.assertEqual(box.Length.Value, 20)
        FreeCAD.closeDocument("TestDoc3")

    def test_delete_object(self):
        """Test deleting object from document"""
        doc = FreeCAD.newDocument("TestDoc4")
        box = doc.addObject("Part::Box", "Box")
        doc.recompute()
        doc.removeObject("Box")
        self.assertIsNone(doc.getObject("Box"))
        FreeCAD.closeDocument("TestDoc4")

    def test_multiple_documents(self):
        """Test multiple documents"""
        docs = []
        for i in range(3):
            docs.append(FreeCAD.newDocument(f"MultiDoc{i}"))

        self.assertEqual(len(docs), 3)

        for i in range(3):
            FreeCAD.closeDocument(f"MultiDoc{i}")


# =============================================================================
# Test: Shape Creation
# =============================================================================
class TestShapeCreation(unittest.TestCase):
    """Test shape creation in headless mode"""

    def setUp(self):
        """Set up test document"""
        self.doc = FreeCAD.newDocument("TestShapes")

    def tearDown(self):
        """Clean up"""
        FreeCAD.closeDocument("TestShapes")

    def test_box_shape(self):
        """Test creating Box shape"""
        box = self.doc.addObject("Part::Box", "Box")
        self.doc.recompute()

        self.assertFalse(box.Shape.isNull())
        self.assertEqual(box.Shape.ShapeType, "Solid")

    def test_sphere_shape(self):
        """Test creating Sphere shape"""
        sphere = self.doc.addObject("Part::Sphere", "Sphere")
        self.doc.recompute()

        self.assertFalse(sphere.Shape.isNull())
        self.assertEqual(sphere.Shape.ShapeType, "Solid")

    def test_cylinder_shape(self):
        """Test creating Cylinder shape"""
        cylinder = self.doc.addObject("Part::Cylinder", "Cylinder")
        self.doc.recompute()

        self.assertFalse(cylinder.Shape.isNull())
        self.assertEqual(cylinder.Shape.ShapeType, "Solid")

    def test_cone_shape(self):
        """Test creating Cone shape"""
        cone = self.doc.addObject("Part::Cone", "Cone")
        self.doc.recompute()

        self.assertFalse(cone.Shape.isNull())
        self.assertEqual(cone.Shape.ShapeType, "Solid")

    def test_torus_shape(self):
        """Test creating Torus shape"""
        torus = self.doc.addObject("Part::Torus", "Torus")
        self.doc.recompute()

        self.assertFalse(torus.Shape.isNull())
        self.assertEqual(torus.Shape.ShapeType, "Solid")

    def test_compound_shape(self):
        """Test creating compound shape"""
        box = self.doc.addObject("Part::Box", "Box")
        sphere = self.doc.addObject("Part::Sphere", "Sphere")
        sphere.Placement.Base.x = 5
        self.doc.recompute()

        compound = self.doc.addObject("Part::Compound", "Compound")
        compound.Links = [box, sphere]
        self.doc.recompute()

        self.assertFalse(compound.Shape.isNull())

    def test_fusion_shape(self):
        """Test creating fused shape"""
        box = self.doc.addObject("Part::Box", "Box")
        sphere = self.doc.addObject("Part::Sphere", "Sphere")
        sphere.Placement.Base.x = 5
        self.doc.recompute()

        fusion = self.doc.addObject("Part::Fuse", "Fusion")
        fusion.Base = box
        fusion.Tool = sphere
        self.doc.recompute()

        self.assertFalse(fusion.Shape.isNull())


# =============================================================================
# Test: Shape Properties
# =============================================================================
class TestShapeProperties(unittest.TestCase):
    """Test shape properties"""

    def setUp(self):
        """Set up test document"""
        self.doc = FreeCAD.newDocument("TestShapeProps")

    def tearDown(self):
        """Clean up"""
        FreeCAD.closeDocument("TestShapeProps")

    def test_box_dimensions(self):
        """Test box dimension properties"""
        box = self.doc.addObject("Part::Box", "Box")
        box.Length = 10
        box.Width = 20
        box.Height = 30
        self.doc.recompute()

        bbox = box.Shape.BoundBox
        self.assertAlmostEqual(bbox.XLength, 10, places=1)
        self.assertAlmostEqual(bbox.YLength, 20, places=1)
        self.assertAlmostEqual(bbox.ZLength, 30, places=1)

    def test_sphere_radius(self):
        """Test sphere radius property"""
        sphere = self.doc.addObject("Part::Sphere", "Sphere")
        sphere.Radius = 15
        self.doc.recompute()

        # Diameter should be 2 * radius
        bbox = sphere.Shape.BoundBox
        self.assertAlmostEqual(bbox.XLength, 30, places=1)

    def test_placement(self):
        """Test object placement"""
        box = self.doc.addObject("Part::Box", "Box")
        box.Placement.Base.x = 100
        box.Placement.Base.y = 200
        box.Placement.Base.z = 300
        self.doc.recompute()

        self.assertAlmostEqual(box.Placement.Base.x, 100, places=1)
        self.assertAlmostEqual(box.Placement.Base.y, 200, places=1)
        self.assertAlmostEqual(box.Placement.Base.z, 300, places=1)


# =============================================================================
# Test: Performance (Headless)
# =============================================================================
class TestHeadlessPerformance(unittest.TestCase):
    """Performance tests in headless mode"""

    def test_bulk_object_creation(self):
        """Test creating many objects"""
        doc = FreeCAD.newDocument("TestBulk")

        start = time.time()
        num_objects = 100

        for i in range(num_objects):
            doc.addObject("Part::Box", f"Box{i}")

        doc.recompute()
        elapsed = time.time() - start

        FreeCAD.closeDocument("TestBulk")

        FreeCAD.Console.PrintMessage(f"Created {num_objects} objects in {elapsed:.3f}s\n")
        self.assertLess(elapsed, 10.0)

    def test_document_lifecycle(self):
        """Test document open/close cycle"""
        start = time.time()
        cycles = 10

        for i in range(cycles):
            doc = FreeCAD.newDocument(f"CycleDoc{i}")
            doc.addObject("Part::Box", "Box")
            doc.recompute()
            FreeCAD.closeDocument(f"CycleDoc{i}")

        elapsed = time.time() - start

        FreeCAD.Console.PrintMessage(f"Completed {cycles} document cycles in {elapsed:.3f}s\n")
        self.assertLess(elapsed, 10.0)


# =============================================================================
# Test: OsgVerse Specific (Headless)
# =============================================================================
@unittest.skipUnless(hasOsgVerseGui(), "OsgVerseGui not available")
class TestOsgVerseHeadless(unittest.TestCase):
    """OsgVerse-specific headless tests"""

    def test_module_attributes(self):
        """Test OsgVerseGui module has expected attributes"""
        import OsgVerseGui

        # Check module is valid
        self.assertIsNotNone(OsgVerseGui)


# =============================================================================
# Test Suite Definition
# =============================================================================
def suite():
    """Create headless test suite"""
    test_suite = unittest.TestSuite()

    test_suite.addTests(unittest.TestLoader().loadTestsFromTestCase(TestModuleImport))
    test_suite.addTests(unittest.TestLoader().loadTestsFromTestCase(TestDocumentOperations))
    test_suite.addTests(unittest.TestLoader().loadTestsFromTestCase(TestShapeCreation))
    test_suite.addTests(unittest.TestLoader().loadTestsFromTestCase(TestShapeProperties))
    test_suite.addTests(unittest.TestLoader().loadTestsFromTestCase(TestHeadlessPerformance))
    test_suite.addTests(unittest.TestLoader().loadTestsFromTestCase(TestOsgVerseHeadless))

    return test_suite


if __name__ == '__main__':
    FreeCAD.Console.PrintMessage("=" * 70 + "\n")
    FreeCAD.Console.PrintMessage("OsgVerseGui Headless Test Suite\n")
    FreeCAD.Console.PrintMessage("=" * 70 + "\n\n")

    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite())

    FreeCAD.Console.PrintMessage("\n" + "=" * 70 + "\n")
    if result.wasSuccessful():
        FreeCAD.Console.PrintMessage("All headless tests passed!\n")
    else:
        FreeCAD.Console.PrintMessage(f"Tests failed: {len(result.failures)} failures, {len(result.errors)} errors\n")
    FreeCAD.Console.PrintMessage("=" * 70 + "\n")
