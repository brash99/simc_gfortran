#!/usr/bin/env python3
"""Export weighted target-exit ROOT rows as correlated NPS input events."""

from __future__ import annotations

import argparse
import csv
import math
from collections import defaultdict
from pathlib import Path

import ROOT


ELECTRON_CHARGE_C = 1.602176634e-19


def scalar(tree, branch: str):
    tree.GetEntry(0)
    return getattr(tree, branch)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("input_root", type=Path)
    parser.add_argument("output_csv", type=Path)
    parser.add_argument("manifest_csv", type=Path)
    parser.add_argument("--current-uA", type=float)
    parser.add_argument(
        "--selected-groups-root", type=Path,
        help="Optional projected ROOT file; retain complete correlated groups "
             "having at least one straight-line NPS-face intersection")
    args = parser.parse_args()

    source = ROOT.TFile.Open(str(args.input_root), "READ")
    tree = source.Get("phase_space") if source else None
    info = source.Get("run_info") if source else None
    if (not source or source.IsZombie() or not tree or not info or
            info.GetEntries() != 1):
        raise RuntimeError(f"cannot read phase_space/run_info from {args.input_root}")
    normalization_mode = (int(scalar(info, "normalization_mode"))
                          if info.GetBranch("normalization_mode") else 0)
    completed_events = int(scalar(info, "completed_events"))
    if normalization_mode in (0, 2) and args.current_uA is None:
        raise RuntimeError("--current-uA is required for per-electron samples")
    electron_flux_hz = ((args.current_uA or 0.0) * 1.0e-6 /
                        ELECTRON_CHARGE_C)
    selected_keys = None
    selection_source = None
    if args.selected_groups_root:
        selection_source = ROOT.TFile.Open(str(args.selected_groups_root), "READ")
        selected_tree = (selection_source.Get("nps_events")
                         if selection_source else None)
        if not selection_source or selection_source.IsZombie() or not selected_tree:
            raise RuntimeError("cannot read nps_events selection tree")
        selected_keys = {
            (int(row.event_id), int(row.electronuclear_group))
            for row in selected_tree
        }
    grouped = defaultdict(list)
    weights = {}
    for row in tree:
        event_id = int(row.event_id)
        if normalization_mode == 2:
            if not bool(row.electronuclear_lineage):
                continue
            group_id = int(row.electronuclear_group)
            key = (event_id, group_id)
            if selected_keys is not None and key not in selected_keys:
                continue
            weight = (electron_flux_hz * float(row.weight) /
                      completed_events)
        elif normalization_mode == 1:
            group_id = -1
            key = (event_id, group_id)
            weight = float(row.weight)
        else:
            group_id = -1
            key = (event_id, group_id)
            weight = electron_flux_hz / completed_events
        if key in weights and not math.isclose(
                weights[key], weight, rel_tol=1.0e-10, abs_tol=1.0e-300):
            raise RuntimeError(f"correlated group {key} has inconsistent weights")
        weights[key] = weight
        grouped[key].append((
            int(row.pdg_id), float(row.total_GeV),
            float(row.x_cm), float(row.y_cm), float(row.z_cm),
            float(row.px_GeV), float(row.py_GeV), float(row.pz_GeV),
        ))
    particle_count = sum(len(particles) for particles in grouped.values())
    source.Close()
    if selection_source:
        selection_source.Close()

    args.output_csv.parent.mkdir(parents=True, exist_ok=True)
    with args.output_csv.open("w", newline="") as stream:
        stream.write("# gep_nps_phase_space_v1\n")
        stream.write("# energies_and_momenta_GeV positions_cm directions_Hall_xyz\n")
        stream.write("# Hall_axes: x_down y_left z_downstream\n")
        stream.write("event_id,weight_Hz,n_particles,particle_[pdg,total_E_GeV,x_cm,y_cm,z_cm,px_GeV,py_GeV,pz_GeV]*N\n")
        writer = csv.writer(stream, lineterminator="\n")
        for output_index, key in enumerate(sorted(grouped)):
            particles = grouped[key]
            flattened = [value for particle in particles for value in particle]
            writer.writerow((output_index, weights[key], len(particles), *flattened))

    with args.manifest_csv.open("w", newline="") as stream:
        writer = csv.writer(stream, lineterminator="\n")
        writer.writerow(("entry", "source_event_id", "electronuclear_group",
                         "event_weight_Hz", "n_particles"))
        for entry, key in enumerate(sorted(grouped)):
            event_id, group_id = key
            writer.writerow((entry, event_id, group_id, weights[key],
                             len(grouped[key])))
    print(f"wrote {len(grouped)} correlated events and {particle_count} particles")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
