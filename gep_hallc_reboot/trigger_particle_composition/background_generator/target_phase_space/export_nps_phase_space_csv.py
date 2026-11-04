#!/usr/bin/env python3
"""Export weighted target-exit ROOT rows as correlated NPS input events."""

from __future__ import annotations

import argparse
import csv
from collections import defaultdict
from pathlib import Path

import ROOT


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("input_root", type=Path)
    parser.add_argument("output_csv", type=Path)
    parser.add_argument("manifest_csv", type=Path)
    args = parser.parse_args()

    source = ROOT.TFile.Open(str(args.input_root), "READ")
    tree = source.Get("phase_space") if source else None
    if not source or source.IsZombie() or not tree:
        raise RuntimeError(f"cannot read phase_space from {args.input_root}")
    grouped = defaultdict(list)
    weights = {}
    for row in tree:
        event_id = int(row.event_id)
        weight = float(row.weight)
        if event_id in weights and abs(weights[event_id] - weight) > max(
                1.0e-12, 1.0e-10 * abs(weight)):
            raise RuntimeError(f"event {event_id} has inconsistent weights")
        weights[event_id] = weight
        grouped[event_id].append((
            int(row.pdg_id), float(row.total_GeV),
            float(row.x_cm), float(row.y_cm), float(row.z_cm),
            float(row.px_GeV), float(row.py_GeV), float(row.pz_GeV),
        ))
    particle_count = int(tree.GetEntries())
    source.Close()

    args.output_csv.parent.mkdir(parents=True, exist_ok=True)
    with args.output_csv.open("w", newline="") as stream:
        stream.write("# gep_nps_phase_space_v1\n")
        stream.write("# energies_and_momenta_GeV positions_cm directions_Hall_xyz\n")
        stream.write("# Hall_axes: x_down y_left z_downstream\n")
        stream.write("event_id,weight_Hz,n_particles,particle_[pdg,total_E_GeV,x_cm,y_cm,z_cm,px_GeV,py_GeV,pz_GeV]*N\n")
        writer = csv.writer(stream, lineterminator="\n")
        for output_index, event_id in enumerate(sorted(grouped)):
            particles = grouped[event_id]
            flattened = [value for particle in particles for value in particle]
            writer.writerow((event_id, weights[event_id], len(particles), *flattened))

    with args.manifest_csv.open("w", newline="") as stream:
        writer = csv.writer(stream, lineterminator="\n")
        writer.writerow(("entry", "source_event_id", "event_weight_Hz", "n_particles"))
        for entry, event_id in enumerate(sorted(grouped)):
            writer.writerow((entry, event_id, weights[event_id], len(grouped[event_id])))
    print(f"wrote {len(grouped)} correlated events and {particle_count} particles")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
