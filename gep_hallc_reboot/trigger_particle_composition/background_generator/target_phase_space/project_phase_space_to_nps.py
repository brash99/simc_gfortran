#!/usr/bin/env python3
"""Project target-exit phase space onto the rectangular NPS front face."""

from __future__ import annotations

import argparse
import csv
import json
import math
from array import array
from collections import defaultdict
from pathlib import Path

import ROOT


ELECTRON_CHARGE_C = 1.602176634e-19
C_CM_PER_NS = 29.9792458


def scalar(tree, branch: str):
    tree.GetEntry(0)
    return getattr(tree, branch)


def make_branch(tree, name: str, code: str):
    storage = array(code, [0])
    leaf = {"q": "L", "i": "I", "d": "D"}[code]
    tree.Branch(name, storage, f"{name}/{leaf}")
    return storage


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("input_root", type=Path)
    parser.add_argument("output_root", type=Path)
    parser.add_argument("--angle-deg", type=float, required=True)
    parser.add_argument("--distance-m", type=float, required=True)
    parser.add_argument("--current-uA", type=float, required=True)
    parser.add_argument("--width-cm", type=float, default=61.5)
    parser.add_argument("--height-cm", type=float, default=73.8)
    parser.add_argument("--threshold-csv", type=Path)
    parser.add_argument("--summary-json", type=Path)
    args = parser.parse_args()

    source = ROOT.TFile.Open(str(args.input_root), "READ")
    if not source or source.IsZombie():
        raise RuntimeError(f"cannot open {args.input_root}")
    phase = source.Get("phase_space")
    info = source.Get("run_info")
    if not phase or not info or info.GetEntries() != 1:
        raise RuntimeError("input lacks phase_space or one-entry run_info tree")
    completed_events = int(scalar(info, "completed_events"))
    beam_energy = float(scalar(info, "beam_total_energy_GeV"))
    production_cut = float(scalar(info, "secondary_kinetic_cut_GeV"))
    normalization_mode = (int(scalar(info, "normalization_mode"))
                          if info.GetBranch("normalization_mode") else 0)
    if completed_events <= 0:
        raise RuntimeError("completed_events must be positive")

    theta = math.radians(args.angle_deg)
    distance_cm = 100.0 * args.distance_m
    # Canonical Hall coordinates used by the SIMC/Wiser interfaces:
    # x points vertically down, y points toward beam-left, z downstream.
    normal = (0.0, math.sin(theta), math.cos(theta))
    local_x_axis = (0.0, math.cos(theta), -math.sin(theta))
    center = tuple(distance_cm * value for value in normal)
    half_width = 0.5 * args.width_cm
    half_height = 0.5 * args.height_cm

    output = ROOT.TFile.Open(str(args.output_root), "RECREATE")
    if not output or output.IsZombie():
        raise RuntimeError(f"cannot create {args.output_root}")
    hits = ROOT.TTree("nps_hits", "Target phase-space rays intersecting NPS")
    hit_fields = {
        "event_id": make_branch(hits, "event_id", "q"),
        "track_id": make_branch(hits, "track_id", "i"),
        "parent_id": make_branch(hits, "parent_id", "i"),
        "pdg_id": make_branch(hits, "pdg_id", "i"),
        "nps_x_cm": make_branch(hits, "nps_x_cm", "d"),
        "nps_y_cm": make_branch(hits, "nps_y_cm", "d"),
        "path_cm": make_branch(hits, "path_cm", "d"),
        "arrival_time_ns": make_branch(hits, "arrival_time_ns", "d"),
        "kinetic_GeV": make_branch(hits, "kinetic_GeV", "d"),
        "total_GeV": make_branch(hits, "total_GeV", "d"),
    }

    event_accumulator = defaultdict(
        lambda: {"n": 0, "n_em": 0, "sum": 0.0, "sum_em": 0.0,
                 "max": 0.0, "earliest": math.inf, "weight_Hz": None}
    )
    species = defaultdict(int)
    projected_forward = 0
    face_hits = 0
    face_particle_weight_hz = 0.0

    for row in phase:
        px, py, pz = float(row.px_GeV), float(row.py_GeV), float(row.pz_GeV)
        momentum = math.sqrt(px * px + py * py + pz * pz)
        if momentum <= 0.0:
            continue
        ux, uy, uz = px / momentum, py / momentum, pz / momentum
        denominator = ux * normal[0] + uy * normal[1] + uz * normal[2]
        if denominator <= 0.0:
            continue
        rx, ry, rz = float(row.x_cm), float(row.y_cm), float(row.z_cm)
        numerator = ((center[0] - rx) * normal[0] +
                     (center[1] - ry) * normal[1] +
                     (center[2] - rz) * normal[2])
        path = numerator / denominator
        if path <= 0.0:
            continue
        projected_forward += 1
        dx = rx + path * ux - center[0]
        dy = ry + path * uy - center[1]
        dz = rz + path * uz - center[2]
        nps_x = dx * local_x_axis[0] + dy * local_x_axis[1] + dz * local_x_axis[2]
        nps_y = dx
        if abs(nps_x) > half_width or abs(nps_y) > half_height:
            continue
        face_hits += 1
        track_weight = float(row.weight)
        if normalization_mode == 1:
            face_particle_weight_hz += track_weight
        total_energy = float(row.total_GeV)
        beta = momentum / total_energy if total_energy > 0.0 else 0.0
        arrival = float(row.time_ns)
        if beta > 0.0:
            arrival += path / (beta * C_CM_PER_NS)
        values = {
            "event_id": int(row.event_id), "track_id": int(row.track_id),
            "parent_id": int(row.parent_id), "pdg_id": int(row.pdg_id),
            "nps_x_cm": nps_x, "nps_y_cm": nps_y, "path_cm": path,
            "arrival_time_ns": arrival, "kinetic_GeV": float(row.kinetic_GeV),
            "total_GeV": total_energy,
        }
        for name, value in values.items():
            hit_fields[name][0] = value
        hits.Fill()
        pdg = int(row.pdg_id)
        species[pdg] += 1
        acc = event_accumulator[int(row.event_id)]
        if acc["weight_Hz"] is None:
            acc["weight_Hz"] = track_weight
        elif normalization_mode == 1 and not math.isclose(
                acc["weight_Hz"], track_weight, rel_tol=1.0e-10, abs_tol=1.0e-12):
            raise RuntimeError("particles in one source event have unequal Hz weights")
        acc["n"] += 1
        acc["sum"] += total_energy
        acc["max"] = max(acc["max"], total_energy)
        acc["earliest"] = min(acc["earliest"], arrival)
        if abs(pdg) == 11 or pdg == 22:
            acc["n_em"] += 1
            acc["sum_em"] += total_energy

    events = ROOT.TTree("nps_events", "Incident-electron events intersecting NPS")
    event_fields = {
        "event_id": make_branch(events, "event_id", "q"),
        "n_particles": make_branch(events, "n_particles", "i"),
        "n_em_particles": make_branch(events, "n_em_particles", "i"),
        "sum_total_GeV": make_branch(events, "sum_total_GeV", "d"),
        "sum_em_GeV": make_branch(events, "sum_em_GeV", "d"),
        "max_particle_GeV": make_branch(events, "max_particle_GeV", "d"),
        "earliest_time_ns": make_branch(events, "earliest_time_ns", "d"),
        "event_weight_Hz": make_branch(events, "event_weight_Hz", "d"),
    }
    for event_id in sorted(event_accumulator):
        acc = event_accumulator[event_id]
        values = {
            "event_id": event_id, "n_particles": acc["n"],
            "n_em_particles": acc["n_em"], "sum_total_GeV": acc["sum"],
            "sum_em_GeV": acc["sum_em"], "max_particle_GeV": acc["max"],
            "earliest_time_ns": acc["earliest"],
            "event_weight_Hz": (acc["weight_Hz"]
                                if normalization_mode == 1 else 0.0),
        }
        for name, value in values.items():
            event_fields[name][0] = value
        events.Fill()

    electron_flux_hz = args.current_uA * 1.0e-6 / ELECTRON_CHARGE_C
    if normalization_mode == 1:
        event_rate_hz = sum(acc["weight_Hz"] for acc in event_accumulator.values())
        particle_rate_hz = face_particle_weight_hz
    else:
        event_rate_hz = electron_flux_hz * len(event_accumulator) / completed_events
        particle_rate_hz = electron_flux_hz * face_hits / completed_events
    thresholds = sorted(set(
        [production_cut + 0.1 * i for i in range(
            int(round((beam_energy - production_cut) / 0.1)) + 1)]
        + [3.387, 3.629, 3.870]
    ))
    threshold_rows = []
    for threshold in thresholds:
        accepted = sum(acc["sum_em"] >= threshold
                       for acc in event_accumulator.values())
        probability = accepted / completed_events
        rate_hz = (sum(acc["weight_Hz"] for acc in event_accumulator.values()
                       if acc["sum_em"] >= threshold)
                   if normalization_mode == 1 else electron_flux_hz * probability)
        threshold_rows.append({
            "threshold_GeV": threshold,
            "accepted_events": accepted,
            "sample_fraction": probability,
            "rate_Hz": rate_hz,
        })

    metadata = ROOT.TTree("projection_info", "NPS geometric projection metadata")
    metadata_values = {
        "completed_events": ("q", completed_events),
        "phase_space_rows": ("q", int(phase.GetEntries())),
        "forward_plane_intersections": ("q", projected_forward),
        "face_particle_hits": ("q", face_hits),
        "face_event_hits": ("q", len(event_accumulator)),
        "angle_deg": ("d", args.angle_deg), "distance_m": ("d", args.distance_m),
        "width_cm": ("d", args.width_cm), "height_cm": ("d", args.height_cm),
        "current_uA": ("d", args.current_uA),
        "beam_total_energy_GeV": ("d", beam_energy),
        "production_cut_GeV": ("d", production_cut),
        "normalization_mode": ("i", normalization_mode),
        "particle_rate_Hz": ("d", particle_rate_hz),
        "event_rate_Hz": ("d", event_rate_hz),
    }
    metadata_storage = {}
    for name, (code, value) in metadata_values.items():
        metadata_storage[name] = make_branch(metadata, name, code)
        metadata_storage[name][0] = value
    metadata.Fill()
    output.cd()
    hits.Write()
    events.Write()
    metadata.Write()
    output.Close()

    csv_path = args.threshold_csv or args.output_root.with_suffix(".thresholds.csv")
    with csv_path.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=threshold_rows[0].keys())
        writer.writeheader()
        writer.writerows(threshold_rows)
    summary = {
        "input": str(args.input_root), "completed_incident_electrons": completed_events,
        "beam_total_energy_GeV": beam_energy, "production_cut_GeV": production_cut,
        "normalization_mode": ("absolute_Hz_event_weights"
                               if normalization_mode == 1 else "per_incident_electron"),
        "nps_angle_deg": args.angle_deg, "nps_distance_m": args.distance_m,
        "nps_width_cm": args.width_cm, "nps_height_cm": args.height_cm,
        "current_uA": args.current_uA, "phase_space_rows": int(phase.GetEntries()),
        "forward_plane_intersections": projected_forward,
        "face_particle_hits": face_hits, "face_event_hits": len(event_accumulator),
        "face_particle_rate_Hz": particle_rate_hz, "face_event_rate_Hz": event_rate_hz,
        "pdg_face_counts": dict(sorted(species.items())),
        "coordinate_convention": {
            "nps_x": "Hall horizontal scattering plane; positive toward larger polar angle",
            "nps_y": "Hall vertical coordinate, using Hall +x (downward)",
        },
        "important_limitation": "Geometric incident-energy projection; not calorimeter deposited energy or trigger response.",
    }
    source.Close()
    json_path = args.summary_json or args.output_root.with_suffix(".summary.json")
    json_path.write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
