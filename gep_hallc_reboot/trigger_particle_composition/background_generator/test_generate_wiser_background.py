#!/usr/bin/env python3
"""Unit tests for the Wiser low-Q2 background generator."""

import math
import unittest

import numpy as np

from generate_wiser_background import (
    HADRON_SCHEMA,
    PI0_SCHEMA,
    decay_pi0,
    face_hit,
    wiser_sigma,
)


class WiserTests(unittest.TestCase):
    def test_reference_points_are_stable(self):
        # Values independently recorded from JeffersonLab evgen_inclusive's
        # wiser_pion.h with rad_len=0.05.
        p = np.array([1.0, 3.0, 5.0])
        th = np.radians(np.array([10.0, 16.57, 25.0]))
        rl = np.full(3, 0.05)
        expected_plus = np.array([34842.1465783, 206.54705251, 0.0])
        expected_minus = np.array([18683.8364202, 138.049098527, 0.0])
        self.assertTrue(np.allclose(wiser_sigma(8.456, p, th, rl, 0),
                                    expected_plus, rtol=3.0e-4, atol=1.0e-12))
        self.assertTrue(np.allclose(wiser_sigma(8.456, p, th, rl, 1),
                                    expected_minus, rtol=3.0e-4, atol=1.0e-12))

    def test_forbidden_kinematics_return_zero(self):
        value = wiser_sigma(1.0, np.array([2.0]), np.array([0.2]),
                            np.array([0.05]), 1)
        self.assertEqual(value[0], 0.0)

    def test_pi0_decay_conserves_energy_and_direction(self):
        rng = np.random.default_rng(3)
        parent = np.array([[0.0, 1.0, 3.0], [0.2, -0.4, 2.0]])
        e1, d1, e2, d2 = decay_pi0(rng, parent)
        self.assertTrue(np.allclose(e1[:, None] * d1 + e2[:, None] * d2, parent))
        self.assertTrue(np.allclose(np.linalg.norm(d1, axis=1), 1.0))
        self.assertTrue(np.allclose(np.linalg.norm(d2, axis=1), 1.0))

    def test_face_hit(self):
        angle = math.radians(16.57)
        central = np.array([[0.0, math.sin(angle), math.cos(angle)]])
        self.assertTrue(face_hit(central, np.array([0.0]), angle, 357.0,
                                 61.5, 73.8)[0])

    def test_pi0_schema_matches_geant4_contract(self):
        self.assertEqual(len(PI0_SCHEMA.split(",")), 18)
        self.assertEqual(PI0_SCHEMA.split(",")[6], "gamma1_E_GeV")
        self.assertEqual(PI0_SCHEMA.split(",")[10], "gamma2_E_GeV")

    def test_hadron_schema_matches_geant4_contract(self):
        self.assertEqual(len(HADRON_SCHEMA.split(",")), 11)
        self.assertEqual(HADRON_SCHEMA.split(",")[6], "pdg_id")
        self.assertEqual(HADRON_SCHEMA.split(",")[7], "primary_E_GeV")


if __name__ == "__main__":
    unittest.main()
