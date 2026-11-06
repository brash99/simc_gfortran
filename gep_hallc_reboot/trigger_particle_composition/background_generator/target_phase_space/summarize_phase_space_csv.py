#!/usr/bin/env python3
"""Summarize particle species in generic correlated NPS phase-space CSVs."""

from __future__ import annotations

import argparse
import csv
from collections import Counter
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("inputs", type=Path, nargs="+")
    args = parser.parse_args()
    for path in args.inputs:
        species = Counter()
        energy = Counter()
        maximum = Counter()
        events = 0
        with path.open(newline="") as stream:
            for _ in range(4):
                next(stream)
            for row in csv.reader(stream):
                events += 1
                count = int(row[2])
                for index in range(count):
                    offset = 3 + 8 * index
                    pdg = int(row[offset])
                    total = float(row[offset + 1])
                    species[pdg] += 1
                    energy[pdg] += total
                    maximum[pdg] = max(maximum[pdg], total)
        details = " ".join(
            f"{pdg}:{species[pdg]}(sum={energy[pdg]:.1f},max={maximum[pdg]:.2f})"
            for pdg in sorted(species)
        )
        print(f"{path.name} events={events} {details}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
