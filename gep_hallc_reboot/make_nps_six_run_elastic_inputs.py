#!/usr/bin/env python3
"""Create SIMC radiated-elastic electron-singles inputs for six NPS runs."""

from __future__ import annotations

import math
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PROTON_MASS_GEV = 0.9382720813
RUNS = {
    6158: (6.3700, 21.17),
    6165: (6.3700, 19.87),
    6168: (6.3700, 18.56),
    2855: (10.5394, 16.58),
    5183: (10.5394, 15.57),
    3888: (10.5394, 14.76),
}


TEMPLATE = """; Run-{run} radiated-elastic electron singles directed at the NPS face.
; The proton arm is generated kinematically but spect_mode=-2 disables its
; detector Monte Carlo, so no HMS coincidence acceptance is imposed.
; Elastic solution at the NPS face center: E'e={eprime:.6f} GeV,
; p_p={proton_momentum:.6f} GeV/c, theta_p={proton_angle:.6f} deg.

begin parm experiment
  ngen = -1000000
  EXPER%charge = 1.0
  doing_phsp = 0
  doing_kaon = 0
  doing_pion = 0
  doing_decay = 0
  ctau = 780.4
  random_seed = {seed}
end parm experiment

begin parm kinematics_main
  Ebeam = {beam_energy_MeV:.3f}
  dEbeam = 0.05
  electron_arm = 8
  hadron_arm = 1
  spec%e%P = {electron_momentum_MeV:.3f}
  spec%e%theta = {electron_angle:.5f}
  spec%p%P = {proton_momentum_MeV:.3f}
  spec%p%theta = {proton_angle:.6f}
end parm kinematics_main

begin parm target
  targ%A = 1.0
  targ%Z = 1.0
  targ%mass_amu = 1.007276
  targ%mrec_amu = 0.0
  targ%rho = 0.0723
  targ%thick = 723.0
  targ%angle = 0.0
  targ%abundancy = 100.0
  targ%can = 3
end parm target

begin parm debug
  debug(1) = 0
  debug(2) = 0
  debug(3) = 0
  debug(4) = 0
  debug(5) = 0
end parm debug

begin parm e_arm_accept
  SPedge%e%delta%min = -85.0
  SPedge%e%delta%max = 5.0
  SPedge%e%yptar%min = -500.0
  SPedge%e%yptar%max = 500.0
  SPedge%e%xptar%min = -500.0
  SPedge%e%xptar%max = 500.0
end parm e_arm_accept

begin parm p_arm_accept
  SPedge%p%delta%min = -100.0
  SPedge%p%delta%max = 100.0
  SPedge%p%yptar%min = -1000.0
  SPedge%p%yptar%max = 1000.0
  SPedge%p%xptar%min = -1000.0
  SPedge%p%xptar%max = 1000.0
end parm p_arm_accept

begin parm beamandtargetinfo
  gen%xwid = 0.008868
  gen%ywid = 0.004235
  targ%fr_pattern = 1.0
  targ%fr1 = 0.1
  targ%fr2 = 0.1
  targ%xoffset = 0.0
  targ%yoffset = 0.0
  targ%zoffset = 0.0
end parm beamandtergetinfo

begin parm spect_offset
  spec%e%offset%x = 0.0
  spec%e%offset%y = 0.0
  spec%e%offset%z = 0.0
  spec%e%offset%xptar = 0.0
  spec%e%offset%yptar = 0.0
  spec%p%offset%x = 0.0
  spec%p%offset%y = 0.0
  spec%p%offset%z = 0.0
  spec%p%offset%xptar = 0.0
  spec%p%offset%yptar = 0.0
end parm spect_offset

begin parm simulate
  hard_cuts = 0
  using_rad = 1
  use_expon = 0
  one_tail = 0
  intcor_mode = 1
  spect_mode = -2
  cuts%Em%min = 0.0
  cuts%Em%max = 0.0
  using_Eloss = 1
  correct_Eloss = 0
  correct_raster = 1
  mc_smear = 1
  deForest_flag = 0
  rad_flag = 0
  extrad_flag = 2
  lambda(1) = 0.0
  lambda(2) = 0.0
  lambda(3) = 0.0
  Nntu = 0
  using_Coulomb = 1
  dE_edge_test = 0.0
  use_offshell_rad = 1
  Egamma_gen_max = 0.0
  drift_to_cal = 950.0
  calo_half_width = 30.75
  calo_half_height = 36.9
  hybrid_export = 1
end parm simulate
"""


def elastic_solution(beam_energy: float, electron_angle_deg: float) -> tuple[float, float, float]:
    theta = math.radians(electron_angle_deg)
    eprime = beam_energy / (
        1.0 + 2.0 * beam_energy / PROTON_MASS_GEV * math.sin(theta / 2.0) ** 2
    )
    proton_momentum = math.sqrt(
        beam_energy**2
        + eprime**2
        - 2.0 * beam_energy * eprime * math.cos(theta)
    )
    proton_angle = math.degrees(
        math.atan2(
            eprime * math.sin(theta),
            beam_energy - eprime * math.cos(theta),
        )
    )
    return eprime, proton_momentum, proton_angle


def main() -> None:
    for run, (beam_energy, electron_angle) in RUNS.items():
        eprime, proton_momentum, proton_angle = elastic_solution(
            beam_energy, electron_angle
        )
        text = TEMPLATE.format(
            run=run,
            seed=run * 100 + 17,
            beam_energy_MeV=1000.0 * beam_energy,
            electron_momentum_MeV=1000.0 * eprime,
            electron_angle=electron_angle,
            proton_momentum_MeV=1000.0 * proton_momentum,
            proton_momentum=proton_momentum,
            proton_angle=proton_angle,
            eprime=eprime,
        )
        output = ROOT / "infiles" / f"nps_run{run}_elastic_singles.inp"
        output.write_text(text)
        print(f"wrote {output.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
