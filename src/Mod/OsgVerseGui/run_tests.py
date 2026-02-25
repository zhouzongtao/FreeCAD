#!/usr/bin/env python
# SPDX-License-Identifier: LGPL-2.1-or-later

"""
OsgVerseGui Test Runner

Run this script to execute all unit tests for the rendering abstraction layer.

Usage:
    FreeCADCmd run_tests.py              # Run all tests
    FreeCADCmd run_tests.py -v           # Run with verbose output
    FreeCADCmd run_tests.py --quick      # Run only quick tests
"""

import sys
import unittest
import time


def run_all_tests(verbose=2):
    """Run all tests and return results"""
    import FreeCAD
    import FreeCADGui

    FreeCAD.Console.PrintMessage("=" * 70 + "\n")
    FreeCAD.Console.PrintMessage("OsgVerseGui Render Abstraction Layer - Test Suite\n")
    FreeCAD.Console.PrintMessage("=" * 70 + "\n\n")

    start_time = time.time()

    # Import test modules
    try:
        from OsgVerseGui import TestRenderAbstractionLayer
        from OsgVerseGui import TestOsgVerseBackend
    except ImportError as e:
        FreeCAD.Console.PrintError(f"Failed to import test modules: {e}\n")
        return False

    # Create test suite
    suite = unittest.TestSuite()
    suite.addTests(TestRenderAbstractionLayer.suite())
    suite.addTests(TestOsgVerseBackend.suite())

    # Run tests
    runner = unittest.TextTestRunner(verbosity=verbose)
    result = runner.run(suite)

    elapsed = time.time() - start_time

    # Print summary
    FreeCAD.Console.PrintMessage("\n" + "=" * 70 + "\n")
    FreeCAD.Console.PrintMessage("Test Summary\n")
    FreeCAD.Console.PrintMessage("=" * 70 + "\n")
    FreeCAD.Console.PrintMessage(f"Tests run: {result.testsRun}\n")
    FreeCAD.Console.PrintMessage(f"Failures: {len(result.failures)}\n")
    FreeCAD.Console.PrintMessage(f"Errors: {len(result.errors)}\n")
    FreeCAD.Console.PrintMessage(f"Skipped: {len(result.skipped)}\n")
    FreeCAD.Console.PrintMessage(f"Time: {elapsed:.2f}s\n")
    FreeCAD.Console.PrintMessage("=" * 70 + "\n")

    if result.wasSuccessful():
        FreeCAD.Console.PrintMessage("\n✓ All tests passed!\n")
    else:
        FreeCAD.Console.PrintError("\n✗ Some tests failed!\n")

    return result.wasSuccessful()


def run_quick_tests(verbose=2):
    """Run only quick tests (skip performance benchmarks)"""
    import FreeCAD
    import FreeCADGui

    FreeCAD.Console.PrintMessage("Running quick tests (skipping benchmarks)...\n\n")

    try:
        from OsgVerseGui import TestRenderAbstractionLayer
        from OsgVerseGui import TestOsgVerseBackend
    except ImportError as e:
        FreeCAD.Console.PrintError(f"Failed to import test modules: {e}\n")
        return False

    # Create suite excluding performance tests
    suite = unittest.TestSuite()

    # Add only non-performance tests
    loader = unittest.TestLoader()

    for test_class in [
        TestRenderAbstractionLayer.TestOsgVerseGuiImport,
        TestRenderAbstractionLayer.TestBackendRegistration,
        TestRenderAbstractionLayer.TestIViewer3DInterface,
        TestRenderAbstractionLayer.TestViewProviderManagement,
        TestRenderAbstractionLayer.TestPickingOperations,
        TestRenderAbstractionLayer.TestDocumentOperations,
        TestOsgVerseBackend.TestOsgVerseModuleLoading,
        TestOsgVerseBackend.TestGeometryConversion,
        TestOsgVerseBackend.TestViewProviderSync,
        TestOsgVerseBackend.TestErrorHandling,
    ]:
        suite.addTests(loader.loadTestsFromTestCase(test_class))

    runner = unittest.TextTestRunner(verbosity=verbose)
    result = runner.run(suite)

    return result.wasSuccessful()


if __name__ == '__main__':
    # Parse arguments
    verbose = 2
    quick = False

    for arg in sys.argv[1:]:
        if arg == '-v' or arg == '--verbose':
            verbose = 3
        elif arg == '-q' or arg == '--quiet':
            verbose = 1
        elif arg == '--quick':
            quick = True

    # Run tests
    if quick:
        success = run_quick_tests(verbose)
    else:
        success = run_all_tests(verbose)

    # Exit with appropriate code
    sys.exit(0 if success else 1)
