#!/usr/bin/env python3
"""Create a radiated-elastic SHMS electron-singles input for run 17193."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "infiles" / "xem2_run17193_elastic.inp"
OUTPUT = ROOT / "infiles" / "xem2_run17193_shms_elastic_singles.inp"


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
        "; XEM2 run 17193 radiated-elastic SHMS electron-singles sample.\n"
        "; spect_mode=-2 removes HMS transport and coincidence acceptance.\n",
    )
    for old, new in (
        ("  ngen = -1000000", "  ngen = -2000000"),
        ("  random_seed = 1719301", "  random_seed = 1719331"),
        ("  SPedge%p%delta%min = -15.0", "  SPedge%p%delta%min = -100.0"),
        ("  SPedge%p%delta%max = 15.0", "  SPedge%p%delta%max = 100.0"),
        ("  SPedge%p%yptar%min = -90.0", "  SPedge%p%yptar%min = -1000.0"),
        ("  SPedge%p%yptar%max = 90.0", "  SPedge%p%yptar%max = 1000.0"),
        ("  SPedge%p%xptar%min = -50.0", "  SPedge%p%xptar%min = -1000.0"),
        ("  SPedge%p%xptar%max = 50.0", "  SPedge%p%xptar%max = 1000.0"),
        ("  spect_mode = 0", "  spect_mode = -2"),
    ):
        text = replace_once(text, old, new)
    OUTPUT.write_text(text)
    print(OUTPUT)


if __name__ == "__main__":
    main()
