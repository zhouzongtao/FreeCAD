# SPDX-License-Identifier: LGPL-2.1-or-later
"""
OsgVerseGui Module Initialization

This module provides an alternative rendering backend using OpenSceneGraph
and OsgVerse for FreeCAD's 3D view.
"""

# Import the C++ module to trigger registration
import OsgVerseGui

# Module is now initialized and backend should be registered
print("OsgVerseGui: Python Init.py executed")


def runTests():
    """Run all OsgVerseGui unit tests"""
    import unittest

    # Import test modules
    from OsgVerseGui import TestRenderAbstractionLayer
    from OsgVerseGui import TestOsgVerseBackend

    # Create combined test suite
    suite = unittest.TestSuite()
    suite.addTests(TestRenderAbstractionLayer.suite())
    suite.addTests(TestOsgVerseBackend.suite())

    # Run tests
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)

    return result.wasSuccessful()
