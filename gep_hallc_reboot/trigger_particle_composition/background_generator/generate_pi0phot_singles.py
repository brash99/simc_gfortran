#!/usr/bin/env python3
"""Generate absolutely normalized gamma p -> pi0 p NPS singles.

This is the inclusive-NPS counterpart of SIMC's doing_pi0phot channel.  It
does not require or transport an HMS proton.  The output contains events for
which at least one pi0 decay photon intersects the rectangular NPS face.
Energies and momenta are in GeV, positions in cm, and event weights in Hz.
"""

from __future__ import annotations

import argparse
import csv
import math
from pathlib import Path

import numpy as np

MP = 0.9382720813
MPI0 = 0.1349768
AVOGADRO = 6.02214076e23
E_CHARGE = 1.602176634e-19
NB_TO_CM2 = 1.0e-33
LH2_X0_G_CM2 = 63.04

C4 = np.array([-0.785, -0.654, -0.508, -0.356, -0.203, -0.055, 0.025,
               0.084, 0.140, 0.211, 0.255, 0.329, 0.435, 0.529, 0.644])
V4 = np.array([24.1, 22.5, 8.06, 4.25, 6.57, 6.78, 5.97, 3.87, 4.62,
               1.83, 4.93, 15.9, 40.1, 102.9, 184.2])
C5 = np.array([-0.751, -0.626, -0.490, -0.304, -0.167, -0.035, 0.048,
               0.166, 0.238, 0.386, 0.431, 0.562, 0.671, 0.743])
V5 = np.array([8.4, 4.46, 1.78, 2.45, 1.87, 1.61, 1.42, 1.91, 1.71,
               2.78, 3.56, 27.5, 132.6, 179.6])


def dsdt_nb_gev2(egamma: np.ndarray, costh: np.ndarray,
                  exponent: float) -> np.ndarray:
    """Anderson wide-angle model, matching physics_pi0phot.f."""
    x4 = np.exp(np.interp(costh, C4, np.log(V4)))
    x5 = np.exp(np.interp(costh, C5, np.log(V5)))
    s = MP * MP + 2.0 * MP * egamma
    s4 = MP * MP + 2.0 * MP * 4.0
    s5 = MP * MP + 2.0 * MP * 5.0
    base = np.sqrt(x4 * (s / s4) ** -7.0 * x5 * (s / s5) ** -7.0)
    return base * (s / s5) ** (-(exponent - 7.0))


def photon_face_hit(direction: np.ndarray, vertex_z: np.ndarray, angle: float,
                    distance: float, width: float, height: float):
    """Intersect rays with NPS; return hit mask and local face coordinates."""
    normal = np.array([0.0, math.sin(angle), math.cos(angle)])
    horizontal = np.array([0.0, math.cos(angle), -math.sin(angle)])
    denom = direction @ normal
    numer = distance - vertex_z * normal[2]
    ray_t = np.divide(numer, denom, out=np.full_like(numer, -1.0),
                      where=denom > 0.0)
    hit = direction * ray_t[:, None]
    hit[:, 2] += vertex_z
    relative = hit - distance * normal
    face_x = relative @ horizontal
    face_y = relative[:, 0]
    accepted = ((ray_t > 0.0) & (np.abs(face_x) <= width / 2.0) &
                (np.abs(face_y) <= height / 2.0))
    return accepted, face_x, face_y


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--trials", type=int, default=1_000_000)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--seed", type=int, default=6887)
    parser.add_argument("--exponent", type=float, default=7.0)
    parser.add_argument("--beam-energy", type=float, default=8.456)
    parser.add_argument("--current-ua", type=float, default=29.41712856)
    parser.add_argument("--target-length-cm", type=float, default=10.0)
    parser.add_argument("--target-density", type=float, default=0.0708)
    parser.add_argument("--nps-angle-deg", type=float, default=16.57)
    parser.add_argument("--nps-distance-cm", type=float, default=357.0)
    parser.add_argument("--nps-width-cm", type=float, default=61.5)
    parser.add_argument("--nps-height-cm", type=float, default=73.8)
    parser.add_argument("--egamma-min", type=float, default=0.5)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if args.trials <= 0 or not (0.0 < args.egamma_min < args.beam_energy):
        raise SystemExit("invalid trial count or photon-energy range")

    rng = np.random.default_rng(args.seed)
    n = args.trials
    egmax = args.beam_energy * (1.0 - 1.0e-6)
    egamma = rng.uniform(args.egamma_min, egmax, n)
    costh = rng.uniform(C5[0], C4[-1], n)
    phi = rng.uniform(0.0, 2.0 * math.pi, n)
    vertex_z = rng.uniform(-args.target_length_cm / 2.0,
                           args.target_length_cm / 2.0, n)

    s = MP * MP + 2.0 * MP * egamma
    root_s = np.sqrt(s)
    epi_cm = (s + MPI0 * MPI0 - MP * MP) / (2.0 * root_s)
    ppi_cm = np.sqrt(np.maximum(0.0, epi_cm * epi_cm - MPI0 * MPI0))
    sinth = np.sqrt(np.maximum(0.0, 1.0 - costh * costh))
    beta_cm = egamma / (egamma + MP)
    gamma_cm = (egamma + MP) / root_s
    pion = np.empty((n, 4))  # E, px, py, pz in Hall coordinates
    pion[:, 0] = gamma_cm * (epi_cm + beta_cm * ppi_cm * costh)
    pion[:, 1] = ppi_cm * sinth * np.cos(phi)
    pion[:, 2] = ppi_cm * sinth * np.sin(phi)
    pion[:, 3] = gamma_cm * (ppi_cm * costh + beta_cm * epi_cm)

    # Isotropic pi0 decay in its rest frame, followed by a vector boost.
    decay_costh = rng.uniform(-1.0, 1.0, n)
    decay_phi = rng.uniform(0.0, 2.0 * math.pi, n)
    decay_sinth = np.sqrt(1.0 - decay_costh * decay_costh)
    rest_p = 0.5 * MPI0 * np.column_stack((decay_sinth * np.cos(decay_phi),
                                           decay_sinth * np.sin(decay_phi),
                                           decay_costh))
    beta_vec = pion[:, 1:4] / pion[:, 0, None]
    beta2 = np.sum(beta_vec * beta_vec, axis=1)
    gamma_pi = pion[:, 0] / MPI0
    dot = np.sum(beta_vec * rest_p, axis=1)
    boost_factor = ((gamma_pi - 1.0) * dot / beta2 + gamma_pi * MPI0 / 2.0)
    g1_p = rest_p + boost_factor[:, None] * beta_vec
    g1_e = gamma_pi * (MPI0 / 2.0 + dot)
    g2_p = pion[:, 1:4] - g1_p
    g2_e = pion[:, 0] - g1_e
    g1_dir = g1_p / g1_e[:, None]
    g2_dir = g2_p / g2_e[:, None]

    angle = math.radians(args.nps_angle_deg)
    hit1, x1, y1 = photon_face_hit(g1_dir, vertex_z, angle,
                                   args.nps_distance_cm, args.nps_width_cm,
                                   args.nps_height_cm)
    hit2, x2, y2 = photon_face_hit(g2_dir, vertex_z, angle,
                                   args.nps_distance_cm, args.nps_width_cm,
                                   args.nps_height_cm)

    kg_cm = (s - MP * MP) / (2.0 * root_s)
    dsdo_nb_sr = dsdt_nb_gev2(egamma, costh, args.exponent) * kg_cm * ppi_cm / math.pi
    upstream_g_cm2 = args.target_density * (vertex_z + args.target_length_cm / 2.0)
    radlen = upstream_g_cm2 / LH2_X0_G_CM2
    y = egamma / args.beam_energy
    flux_per_gev = radlen / egamma * (4.0 / 3.0 - 4.0 * y / 3.0 + y * y)
    electron_flux = args.current_ua * 1.0e-6 / E_CHARGE
    areal_protons = args.target_density * args.target_length_cm * AVOGADRO
    phase_volume = ((egmax - args.egamma_min) * (C4[-1] - C5[0]) *
                    2.0 * math.pi)
    weight = (electron_flux * areal_protons * NB_TO_CM2 * phase_volume / n *
              flux_per_gev * dsdo_nb_sr)

    selected = hit1 | hit2
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fields = ["event", "weight_Hz", "vertex_z_cm", "egamma_GeV",
              "costh_cm", "phi_cm_rad", "gamma1_hit", "gamma1_energy_GeV",
              "gamma1_px_GeV", "gamma1_py_GeV", "gamma1_pz_GeV",
              "gamma1_face_x_cm", "gamma1_face_y_cm", "gamma2_hit",
              "gamma2_energy_GeV", "gamma2_px_GeV", "gamma2_py_GeV",
              "gamma2_pz_GeV", "gamma2_face_x_cm", "gamma2_face_y_cm"]
    with args.output.open("w", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(fields)
        for i in np.flatnonzero(selected):
            writer.writerow((i + 1, weight[i], vertex_z[i], egamma[i], costh[i],
                             phi[i], int(hit1[i]), g1_e[i], *g1_p[i], x1[i], y1[i],
                             int(hit2[i]), g2_e[i], *g2_p[i], x2[i], y2[i]))

    incident_energy = hit1 * g1_e + hit2 * g2_e
    def estimate(mask: np.ndarray) -> tuple[float, float]:
        values = weight * mask
        total = float(values.sum())
        # Each stored weight already contains 1/N.  This is the standard
        # error of the N-trial Monte Carlo integral, including zero trials.
        variance = n / (n - 1.0) * max(0.0, float(np.sum(values * values)) - total * total / n)
        return total, math.sqrt(variance)

    print(f"Trials: {n}")
    print(f"Accepted parent events: {np.count_nonzero(selected)}")
    total, error = estimate(selected)
    print(f"At least one photon on NPS [Hz]: {total:.6g} +/- {error:.3g}")
    for threshold in (3.387, 3.629, 3.870):
        total, error = estimate(incident_energy > threshold)
        print(f"Incident photon energy > {threshold:.3f} GeV [Hz]: "
              f"{total:.6g} +/- {error:.3g}")
    print(f"Output: {args.output}")


if __name__ == "__main__":
    main()
