#!/usr/bin/env python3
"""Split correlated NPS phase-space and its manifest into aligned chunks."""

from __future__ import annotations

import argparse
import csv
import math
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("phase_space", type=Path)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("output_stem", type=Path)
    parser.add_argument("--chunks", type=int, default=4)
    args = parser.parse_args()
    if args.chunks < 1:
        raise ValueError("--chunks must be positive")

    with args.phase_space.open(newline="") as stream:
        comments = [next(stream) for _ in range(3)]
        phase_header = next(stream)
        phase_rows = list(csv.reader(stream))
    with args.manifest.open(newline="") as stream:
        manifest_reader = csv.reader(stream)
        manifest_header = next(manifest_reader)
        manifest_rows = list(manifest_reader)
    if len(phase_rows) != len(manifest_rows):
        raise RuntimeError("phase-space and manifest row counts differ")

    chunk_size = math.ceil(len(phase_rows) / args.chunks)
    for chunk in range(args.chunks):
        begin = chunk * chunk_size
        end = min(len(phase_rows), begin + chunk_size)
        phase_path = Path(f"{args.output_stem}_chunk{chunk}.csv")
        manifest_path = Path(f"{args.output_stem}_chunk{chunk}_manifest.csv")
        phase_path.parent.mkdir(parents=True, exist_ok=True)
        with phase_path.open("w", newline="") as stream:
            stream.writelines(comments)
            stream.write(phase_header)
            writer = csv.writer(stream, lineterminator="\n")
            for entry, row in enumerate(phase_rows[begin:end]):
                row[0] = str(entry)
                writer.writerow(row)
        with manifest_path.open("w", newline="") as stream:
            writer = csv.writer(stream, lineterminator="\n")
            writer.writerow(manifest_header)
            for entry, row in enumerate(manifest_rows[begin:end]):
                row[0] = str(entry)
                writer.writerow(row)
        print(f"chunk {chunk}: entries={end - begin} range=[{begin},{end})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
