#!/usr/bin/env python3
"""Add SIMC's collinear final-state photon to an elastic hybrid event file."""

from __future__ import annotations

import argparse
import csv
import math
from pathlib import Path


BASE_FIELDS = [
    "event_id",
    "weight",
    "vertex_x_cm",
    "vertex_y_cm",
    "vertex_z_cm",
    "vertex_beam_E_GeV",
    "vertex_e_E_GeV",
    "vertex_e_px_GeV",
    "vertex_e_py_GeV",
    "vertex_e_pz_GeV",
    "rad_tail",
    "rad1_GeV",
    "rad2_GeV",
    "rad3_GeV",
    "target_eloss_GeV",
    "target_teff_X0",
    "primary_x_cm",
    "primary_y_cm",
    "primary_z_cm",
    "primary_E_GeV",
    "primary_px_GeV",
    "primary_py_GeV",
    "primary_pz_GeV",
]
FSR_FIELDS = ["fsr_E_GeV", "fsr_px_GeV", "fsr_py_GeV", "fsr_pz_GeV"]


def main() -> None:
    parser = argparse.ArgumentParser(
        description=(
            "Convert simc_geant4_hybrid_v1 events to the versioned mixed "
            "electron-plus-final-state-photon schema."
        )
    )
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    event_count = 0
    photon_count = 0
    photon_energy_sum = 0.0
    with args.input.open(newline="") as source, args.output.open("w", newline="") as target:
        rows = (line for line in source if not line.startswith("#"))
        reader = csv.DictReader(rows)
        if reader.fieldnames != BASE_FIELDS:
            raise ValueError(
                "Input does not have the exact simc_geant4_hybrid_v1 schema: "
                f"{reader.fieldnames}"
            )

        target.write("# simc_geant4_electron_photon_v1\n")
        target.write("# energies_and_momenta_GeV positions_cm directions_Hall_xyz\n")
        target.write("# Hall_axes: x_down y_left z_downstream\n")
        writer = csv.DictWriter(
            target, fieldnames=BASE_FIELDS + FSR_FIELDS, lineterminator="\n"
        )
        writer.writeheader()

        for row in reader:
            fsr_energy = float(row["rad2_GeV"]) if int(row["rad_tail"]) == 2 else 0.0
            px = float(row["primary_px_GeV"])
            py = float(row["primary_py_GeV"])
            pz = float(row["primary_pz_GeV"])
            momentum = math.sqrt(px * px + py * py + pz * pz)
            if momentum <= 0.0:
                raise ValueError(f"Event {row['event_id']} has zero primary momentum")
            scale = fsr_energy / momentum
            row.update(
                {
                    "fsr_E_GeV": f"{fsr_energy:.17g}",
                    "fsr_px_GeV": f"{px * scale:.17g}",
                    "fsr_py_GeV": f"{py * scale:.17g}",
                    "fsr_pz_GeV": f"{pz * scale:.17g}",
                }
            )
            writer.writerow(row)
            event_count += 1
            if fsr_energy > 0.0:
                photon_count += 1
                photon_energy_sum += fsr_energy

    print(f"events={event_count}")
    print(f"events_with_fsr={photon_count}")
    print(f"summed_fsr_energy_GeV={photon_energy_sum:.9f}")
    print(f"output={args.output}")


if __name__ == "__main__":
    main()
