#!/usr/bin/env python3
"""Generate low-Q2 inclusive pion backgrounds with the Wiser fit.

The Wiser parameterization is an effective electron-induced inclusive-hadron
cross section: bremsstrahlung production is integrated over photon energy and
the supplied radiator thickness.  This program samples pion momentum and
solid angle, propagates charged pions to the NPS face, and decays pi0 parents
to two photons.  Cross sections are in nb/(GeV sr); output rates are in Hz.
"""

from __future__ import annotations

import argparse
import csv
import math
from dataclasses import dataclass
from pathlib import Path

import numpy as np

MP = 0.9382720813
MPI = 0.13957039
MPI0 = 0.1349768
ME = 0.00051099895
ALPHA = 1.0 / 137.035999084
AVOGADRO = 6.02214076e23
E_CHARGE = 1.602176634e-19
NB_TO_CM2 = 1.0e-33
LH2_X0_G_CM2 = 63.04

# Wiser fit parameters for pi+ and pi-, copied from JeffersonLab's
# evgen_inclusive/wiser_pion.h.  Indices 0 and 1 are pi+ and pi-.
A1 = np.array([566.0, 486.0])
A2 = np.array([829.0, 115.0])
A3 = np.array([1.79, 1.77])
A4 = np.array([2.10, 2.18])
A5 = np.array([-5.49, -5.23])
A6 = np.array([-1.73, -1.82])
GL_X, GL_W = np.polynomial.legendre.leggauss(24)
PI0_SCHEMA = (
    "event_id,weight,vertex_x_cm,vertex_y_cm,vertex_z_cm,beam_E_GeV,"
    "gamma1_E_GeV,gamma1_px_GeV,gamma1_py_GeV,gamma1_pz_GeV,"
    "gamma2_E_GeV,gamma2_px_GeV,gamma2_py_GeV,gamma2_pz_GeV,"
    "proton_E_GeV,proton_px_GeV,proton_py_GeV,proton_pz_GeV"
)
HADRON_SCHEMA = (
    "event_id,weight,vertex_x_cm,vertex_y_cm,vertex_z_cm,beam_E_GeV,"
    "pdg_id,primary_E_GeV,primary_px_GeV,primary_py_GeV,primary_pz_GeV"
)


def wiser_sigma(ebeam: float, momentum: np.ndarray, theta: np.ndarray,
                rad_len: np.ndarray, pion_type: int) -> np.ndarray:
    """Return Wiser d2sigma/(dp dOmega) in nb/(GeV sr).

    ``rad_len`` is the effective radiator in radiation lengths: internal
    radiation plus 4/3 times the external material before the vertex.
    """
    p = np.asarray(momentum, dtype=float)
    th = np.asarray(theta, dtype=float)
    rl = np.asarray(rad_len, dtype=float)
    mass = MPI
    residual_mass = MP if pion_type == 0 else MP + MPI
    ef = np.sqrt(p * p + mass * mass)
    pt = p * np.sin(th)
    mt = np.sqrt(pt * pt + mass * mass)
    u = residual_mass ** 2 - mass * mass - MP * MP + 2.0 * MP * ef
    denom = 2.0 * (MP - ef + p * np.cos(th))
    eg_min = np.divide(u, denom, out=np.full_like(p, np.inf), where=denom > 0.0)
    valid = (p > 0.0) & (eg_min > 0.0) & (eg_min < ebeam) & (rl > 0.0)
    result = np.zeros_like(p)
    if not np.any(valid):
        return result

    pv, efv, ptv, mtv = p[valid], ef[valid], pt[valid], mt[valid]
    lo = eg_min[valid]
    half = 0.5 * (ebeam - lo)
    mid = 0.5 * (ebeam + lo)
    eg = mid[:, None] + half[:, None] * GL_X[None, :]
    s = MP * MP + 2.0 * MP * eg
    beta_cm = eg / (eg + MP)
    gamma_cm = (eg + MP) / np.sqrt(s)
    pl_cm = gamma_cm * (pv[:, None] * np.cos(th[valid])[:, None] -
                        beta_cm * efv[:, None])
    p_cm = np.sqrt(pl_cm * pl_cm + ptv[:, None] ** 2)
    pcm_arg = (s + (residual_mass ** 2 - mass ** 2) ** 2 / s -
               2.0 * (residual_mass ** 2 + mass ** 2))
    p_cm_max = 0.5 * np.sqrt(np.maximum(0.0, pcm_arg))
    x_r = np.divide(p_cm, p_cm_max, out=np.full_like(p_cm, np.inf),
                    where=p_cm_max > 0.0)
    base = np.maximum(0.0, 1.0 - x_r + A3[pion_type] ** 2 / s)
    integrand = ((A1[pion_type] + A2[pion_type] / np.sqrt(s)) *
                 base ** A4[pion_type] / eg)
    integral = half * np.sum(GL_W[None, :] * integrand, axis=1)
    sig_e = (integral * np.exp(A5[pion_type] * mtv) *
             np.exp(A6[pion_type] * ptv * ptv / efv))
    result[valid] = pv * pv / efv * sig_e * rl[valid] * 1000.0
    return result


def face_hit(direction: np.ndarray, vertex_z: np.ndarray, angle: float,
             distance: float, width: float, height: float) -> np.ndarray:
    """Return mask for rays intersecting the rectangular NPS face."""
    normal = np.array([0.0, math.sin(angle), math.cos(angle)])
    horizontal = np.array([0.0, math.cos(angle), -math.sin(angle)])
    denom = direction @ normal
    numer = distance - vertex_z * normal[2]
    ray_t = np.divide(numer, denom, out=np.full_like(numer, -1.0),
                      where=denom > 0.0)
    hit = direction * ray_t[:, None]
    hit[:, 2] += vertex_z
    relative = hit - distance * normal
    return ((ray_t > 0.0) & (np.abs(relative @ horizontal) <= width / 2.0) &
            (np.abs(relative[:, 0]) <= height / 2.0))


def decay_pi0(rng: np.random.Generator, parent_p: np.ndarray) -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    """Isotropically decay pi0 four-vectors; return E1,n1,E2,n2."""
    n = len(parent_p)
    parent_e = np.sqrt(np.sum(parent_p * parent_p, axis=1) + MPI0 * MPI0)
    c = rng.uniform(-1.0, 1.0, n)
    ph = rng.uniform(0.0, 2.0 * math.pi, n)
    s = np.sqrt(1.0 - c * c)
    rest = 0.5 * MPI0 * np.column_stack((s * np.cos(ph), s * np.sin(ph), c))
    beta = parent_p / parent_e[:, None]
    beta2 = np.sum(beta * beta, axis=1)
    gamma = parent_e / MPI0
    dot = np.sum(beta * rest, axis=1)
    factor = np.divide((gamma - 1.0) * dot, beta2,
                       out=np.zeros_like(dot), where=beta2 > 0.0) + gamma * MPI0 / 2.0
    p1 = rest + factor[:, None] * beta
    e1 = gamma * (MPI0 / 2.0 + dot)
    p2 = parent_p - p1
    e2 = parent_e - e1
    return e1, p1 / e1[:, None], e2, p2 / e2[:, None]


@dataclass
class Accumulator:
    total: float = 0.0
    squares: float = 0.0

    def add(self, values: np.ndarray) -> None:
        self.total += float(np.sum(values))
        self.squares += float(np.sum(values * values))

    def result(self, trials: int) -> tuple[float, float]:
        variance = trials / (trials - 1.0) * max(0.0, self.squares - self.total ** 2 / trials)
        return self.total, math.sqrt(variance)


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser()
    p.add_argument("--trials", type=int, default=2_000_000)
    p.add_argument("--chunk-size", type=int, default=100_000)
    p.add_argument("--seed", type=int, default=6887)
    p.add_argument("--beam-energy", type=float, default=8.456)
    p.add_argument("--current-ua", type=float, default=29.41712856)
    p.add_argument("--target-length-cm", type=float, default=10.0)
    p.add_argument("--target-density", type=float, default=0.0708)
    p.add_argument("--nps-angle-deg", type=float, default=16.57)
    p.add_argument("--nps-distance-cm", type=float, default=357.0)
    p.add_argument("--nps-width-cm", type=float, default=61.5)
    p.add_argument("--nps-height-cm", type=float, default=73.8)
    p.add_argument("--theta-max-deg", type=float, default=60.0)
    p.add_argument("--pi0-scale", choices=("isospin", "evgen"), default="isospin",
                   help="pi0=(pi+ + pi-)/2, or evgen_inclusive's 2*(sum)")
    p.add_argument("--pi0-output", type=Path,
                   help="write accepted two-photon events in simc_geant4_pi0_v1 format")
    p.add_argument("--pim-output", type=Path,
                   help="write NPS-directed pi- events in simc_geant4_hadron_v1 format")
    p.add_argument("--pim-min-energy-GeV", type=float, default=0.0,
                   help="minimum total pi- energy written for GEANT4 transport")
    return p.parse_args()


def main() -> None:
    a = parse_args()
    if a.trials < 2 or a.chunk_size < 1 or not (0.0 < a.theta_max_deg <= 180.0):
        raise SystemExit("invalid trial, chunk, or angular range")
    rng = np.random.default_rng(a.seed)
    angle = math.radians(a.nps_angle_deg)
    cos_min = math.cos(math.radians(a.theta_max_deg))
    phase = a.beam_energy * (1.0 - cos_min) * 2.0 * math.pi
    electron_flux = a.current_ua * 1.0e-6 / E_CHARGE
    areal_protons = a.target_density * a.target_length_cm * AVOGADRO
    rate_norm = electron_flux * areal_protons * NB_TO_CM2 * phase / a.trials
    internal_rad = 2.0 * math.log(a.beam_energy / ME) * ALPHA / math.pi

    pim = Accumulator()
    pi0_any = Accumulator()
    thresholds = (3.387, 3.629, 3.870)
    pi0_threshold = {threshold: Accumulator() for threshold in thresholds}

    output_stream = None
    output_writer = None
    output_events = 0
    if a.pi0_output:
        a.pi0_output.parent.mkdir(parents=True, exist_ok=True)
        output_stream = a.pi0_output.open("w", newline="")
        output_stream.write("# simc_geant4_pi0_v1\n")
        output_stream.write("# energies_and_momenta_GeV positions_cm directions_Hall_xyz\n")
        output_stream.write("# Hall_axes: x_down y_left z_downstream\n")
        output_stream.write(PI0_SCHEMA + "\n")
        # The strict GEANT4 reader requires the final numeric token to end at
        # the line boundary; csv.writer's platform-independent CRLF default
        # would leave a trailing carriage return in that token.
        output_writer = csv.writer(output_stream, lineterminator="\n")

    pim_stream = None
    pim_writer = None
    pim_output_events = 0
    if a.pim_output:
        a.pim_output.parent.mkdir(parents=True, exist_ok=True)
        pim_stream = a.pim_output.open("w", newline="")
        pim_stream.write("# simc_geant4_hadron_v1\n")
        pim_stream.write("# energies_and_momenta_GeV positions_cm directions_Hall_xyz\n")
        pim_stream.write("# Hall_axes: x_down y_left z_downstream\n")
        pim_stream.write(HADRON_SCHEMA + "\n")
        pim_writer = csv.writer(pim_stream, lineterminator="\n")

    done = 0
    while done < a.trials:
        n = min(a.chunk_size, a.trials - done)
        momentum = rng.uniform(0.0, a.beam_energy, n)
        costh = rng.uniform(cos_min, 1.0, n)
        theta = np.arccos(costh)
        phi = rng.uniform(0.0, 2.0 * math.pi, n)
        vertex_z = rng.uniform(-a.target_length_cm / 2.0,
                               a.target_length_cm / 2.0, n)
        sinth = np.sqrt(1.0 - costh * costh)
        direction = np.column_stack((sinth * np.sin(phi),
                                     sinth * np.cos(phi), costh))
        parent_p = momentum[:, None] * direction
        upstream = a.target_density * (vertex_z + a.target_length_cm / 2.0)
        rad_len = internal_rad + (4.0 / 3.0) * upstream / LH2_X0_G_CM2
        sig_plus = wiser_sigma(a.beam_energy, momentum, theta, rad_len, 0)
        sig_minus = wiser_sigma(a.beam_energy, momentum, theta, rad_len, 1)

        direct_hit = face_hit(direction, vertex_z, angle, a.nps_distance_cm,
                              a.nps_width_cm, a.nps_height_cm)
        pim_weight = rate_norm * sig_minus
        pim.add(pim_weight * direct_hit)
        if pim_writer is not None:
            pion_energy = np.sqrt(momentum * momentum + MPI * MPI)
            selected_pim = direct_hit & (pion_energy >= a.pim_min_energy_GeV)
            for index in np.flatnonzero(selected_pim):
                pim_writer.writerow((
                    done + index + 1, pim_weight[index], 0.0, 0.0,
                    vertex_z[index], a.beam_energy, -211, pion_energy[index],
                    *parent_p[index],
                ))
                pim_output_events += 1

        pi0_factor = 0.5 if a.pi0_scale == "isospin" else 2.0
        pi0_weight = rate_norm * pi0_factor * (sig_plus + sig_minus)
        e1, d1, e2, d2 = decay_pi0(rng, parent_p)
        h1 = face_hit(d1, vertex_z, angle, a.nps_distance_cm,
                      a.nps_width_cm, a.nps_height_cm)
        h2 = face_hit(d2, vertex_z, angle, a.nps_distance_cm,
                      a.nps_width_cm, a.nps_height_cm)
        incident = h1 * e1 + h2 * e2
        accepted = h1 | h2
        pi0_any.add(pi0_weight * accepted)
        for threshold in thresholds:
            pi0_threshold[threshold].add(pi0_weight * (incident > threshold))
        if output_writer is not None:
            p1 = e1[:, None] * d1
            p2 = e2[:, None] * d2
            for index in np.flatnonzero(accepted):
                output_writer.writerow((
                    done + index + 1, pi0_weight[index], 0.0, 0.0,
                    vertex_z[index], a.beam_energy, e1[index], *p1[index],
                    e2[index], *p2[index], MP, 0.0, 0.0, 0.0,
                ))
                output_events += 1
        done += n

    if output_stream is not None:
        output_stream.close()
    if pim_stream is not None:
        pim_stream.close()

    print(f"Wiser low-Q2 background: run-6887 conditions, {a.trials} trials")
    print(f"theta integration: 0--{a.theta_max_deg:g} deg")
    print(f"effective radiator: internal {internal_rad:.6f} + vertex-dependent external")
    print(f"pi0 convention: {a.pi0_scale}")
    value, error = pim.result(a.trials)
    print(f"pi- parents intersecting NPS [Hz]: {value:.6g} +/- {error:.3g}")
    value, error = pi0_any.result(a.trials)
    print(f"pi0: at least one decay photon on NPS [Hz]: {value:.6g} +/- {error:.3g}")
    for threshold in thresholds:
        value, error = pi0_threshold[threshold].result(a.trials)
        print(f"pi0 incident photon energy > {threshold:.3f} GeV [Hz]: {value:.6g} +/- {error:.3g}")
    if a.pi0_output:
        print(f"pi0 GEANT4 events: {output_events} written to {a.pi0_output}")
    if a.pim_output:
        print(f"pi- GEANT4 events: {pim_output_events} written to {a.pim_output}")


if __name__ == "__main__":
    main()
