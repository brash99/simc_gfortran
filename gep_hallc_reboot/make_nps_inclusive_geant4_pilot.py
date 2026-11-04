#!/usr/bin/env python3
"""Convert inclusive-generator events to strict GEANT4 hybrid pilot samples."""

from __future__ import annotations

import argparse
import csv
import math
from pathlib import Path


FIELDS = (
    "event_id,weight,vertex_x_cm,vertex_y_cm,vertex_z_cm,vertex_beam_E_GeV,"
    "vertex_e_E_GeV,vertex_e_px_GeV,vertex_e_py_GeV,vertex_e_pz_GeV,"
    "rad_tail,rad1_GeV,rad2_GeV,rad3_GeV,target_eloss_GeV,target_teff_X0,"
    "primary_x_cm,primary_y_cm,primary_z_cm,primary_E_GeV,primary_px_GeV,"
    "primary_py_GeV,primary_pz_GeV"
)
ELECTRON_MASS_GEV = 0.00051099895


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("run", type=int)
    parser.add_argument("beam_energy_GeV", type=float)
    parser.add_argument("--stride", type=int, default=8)
    parser.add_argument("--maximum-events", type=int, default=50000)
    parser.add_argument(
        "--minimum-energy-GeV",
        type=float,
        default=0.0,
        help="Discard events below this energy before writing the pilot sample",
    )
    parser.add_argument("--input", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.stride <= 0 or args.maximum_events <= 0:
        parser.error("stride and maximum-events must be positive")

    input_path = args.input or Path(f"/tmp/nps_run{args.run}_inclusive_1m.csv")
    output_path = args.output or Path(
        f"/tmp/nps_run{args.run}_inclusive_geant4_pilot.hybrid.csv"
    )
    selected = 0
    visited = 0
    with input_path.open(newline="") as source, output_path.open("w", newline="") as target:
        reader = csv.DictReader(source)
        target.write("# simc_geant4_hybrid_v1\n")
        target.write("# energies_and_momenta_GeV positions_cm directions_Hall_xyz\n")
        target.write("# Hall_axes: x_down y_left z_downstream\n")
        target.write(FIELDS + "\n")
        for visited, row in enumerate(reader, start=1):
            if (visited - 1) % args.stride != 0:
                continue
            event_id = int(row["event"])
            weight = float(row["weight_Hz"])
            vertex_x = float(row["vertex_x_cm"])
            vertex_y = float(row["vertex_y_cm"])
            vertex_z = float(row["vertex_z_cm"])
            energy = float(row["electron_energy_GeV"])
            if energy < args.minimum_energy_GeV:
                continue
            px = float(row["px_GeV"])
            py = float(row["py_GeV"])
            pz = float(row["pz_GeV"])
            input_momentum = math.sqrt(px * px + py * py + pz * pz)
            physical_momentum = math.sqrt(
                max(0.0, energy * energy - ELECTRON_MASS_GEV * ELECTRON_MASS_GEV)
            )
            momentum_scale = physical_momentum / input_momentum
            px *= momentum_scale
            py *= momentum_scale
            pz *= momentum_scale
            values = (
                event_id,
                weight,
                vertex_x,
                vertex_y,
                vertex_z,
                args.beam_energy_GeV,
                energy,
                px,
                py,
                pz,
                0,
                0.0,
                0.0,
                0.0,
                0.0,
                0.0,
                vertex_x,
                vertex_y,
                vertex_z,
                energy,
                px,
                py,
                pz,
            )
            target.write(",".join(str(value) for value in values) + "\n")
            selected += 1
            if selected >= args.maximum_events:
                break
    print(
        f"run {args.run}: selected {selected} events through input row {visited}; "
        f"wrote {output_path}"
    )


if __name__ == "__main__":
    main()
