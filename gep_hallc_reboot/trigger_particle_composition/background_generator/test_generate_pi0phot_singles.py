#!/usr/bin/env python3
"""Numerical unit checks for the inclusive pi0 photoproduction generator."""

import importlib.util
import math
import unittest
from pathlib import Path

import numpy as np

MODULE_PATH = Path(__file__).with_name("generate_pi0phot_singles.py")
SPEC = importlib.util.spec_from_file_location("pi0phot_singles", MODULE_PATH)
GEN = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GEN)


class Pi0PhotSinglesTest(unittest.TestCase):
    def test_cross_section_matches_simc_reference(self):
        # Reference recorded by tests/test_pi0phot.f for the Q11 endpoint.
        value = GEN.dsdt_nb_gev2(np.array([10.700]), np.array([-0.1436]), 7.0)[0]
        self.assertAlmostEqual(value, 0.01158894, places=7)

    def test_face_intersection(self):
        angle = math.radians(16.57)
        central = np.array([[0.0, math.sin(angle), math.cos(angle)]])
        hit, x, y = GEN.photon_face_hit(central, np.array([0.0]), angle,
                                        357.0, 61.5, 73.8)
        self.assertTrue(hit[0])
        self.assertAlmostEqual(x[0], 0.0, places=11)
        self.assertAlmostEqual(y[0], 0.0, places=11)

    def test_cross_section_variations_are_positive(self):
        energy = np.array([4.0, 5.0, 8.456])
        angle = np.array([-0.5, 0.0, 0.5])
        for exponent in (6.0, 7.0, 8.0):
            self.assertTrue(np.all(GEN.dsdt_nb_gev2(energy, angle, exponent) > 0.0))


if __name__ == "__main__":
    unittest.main()
