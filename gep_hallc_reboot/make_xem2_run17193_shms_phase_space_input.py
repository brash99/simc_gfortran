#!/usr/bin/env python3
"""Create a uniform SHMS phase-space transport sample for run 17193."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "infiles" / "xem2_run17193_elastic.inp"
OUTPUT = ROOT / "infiles" / "xem2_run17193_shms_phase_space.inp"


def replace_once(text: str, old: str, new: str) -> str:
    if text.count(old) != 1:
        raise ValueError(f"expected exactly one occurrence of {old!r}")
    return text.replace(old, new)


def main() -> None:
    text = SOURCE.read_text()
    text = text.replace(
        "; XEM2 run 17193 elastic ep closure sample.\n"
        "; Measured/replay anchors: Ebeam=10.551 GeV, SHMS e-=4.935 GeV/c at\n"
        "; 26.0 deg, HMS p=6.476 GeV/c at 19.52 deg, 10 cm LH2 target.\n",
        "; XEM2 run 17193 uniform electron-arm phase-space transport sample.\n"
        "; The SHMS target-variable generation limits match the elastic input;\n"
        "; spect_mode=-2 removes the HMS detector acceptance.\n",
    )
    for old, new in (
        ("  ngen = -1000000", "  ngen = -5000000"),
        ("  doing_phsp = 0", "  doing_phsp = 1"),
        ("  random_seed = 1719301", "  random_seed = 1719321"),
        ("  spect_mode = 0", "  spect_mode = -2"),
        ("  using_rad = 1", "  using_rad = 0"),
    ):
        text = replace_once(text, old, new)
    OUTPUT.write_text(text)
    print(OUTPUT)


if __name__ == "__main__":
    main()
