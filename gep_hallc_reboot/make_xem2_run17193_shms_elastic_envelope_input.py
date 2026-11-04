#!/usr/bin/env python3
"""Create a hard-envelope elastic sample for validating the SHMS map."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "infiles" / "xem2_run17193_shms_elastic_singles.inp"
OUTPUT = ROOT / "infiles" / "xem2_run17193_shms_elastic_envelope.inp"


def replace_once(text: str, old: str, new: str) -> str:
    if text.count(old) != 1:
        raise ValueError(f"expected exactly one occurrence of {old!r}")
    return text.replace(old, new)


def main() -> None:
    text = SOURCE.read_text()
    replacements = (
        ("; XEM2 run 17193 radiated-elastic SHMS electron-singles sample.\n"
         "; spect_mode=-2 removes HMS transport and coincidence acceptance.\n",
         "; XEM2 run 17193 radiated-elastic hard-envelope sample.\n"
         "; The projected plane spans the SHMS generation limits and omits\n"
         "; SHMS transport so that the phase-space map can be validated.\n"),
        ("  random_seed = 1719331", "  random_seed = 1719341"),
        ("  electron_arm = 5", "  electron_arm = 8"),
        ("  drift_to_cal = 0.0", "  drift_to_cal = 300.0"),
        ("  calo_half_width = 0.0", "  calo_half_width = 27.0732"),
        ("  calo_half_height = 0.0", "  calo_half_height = 15.0125"),
        ("  hybrid_export = 0", "  hybrid_export = 1"),
    )
    for old, new in replacements:
        text = replace_once(text, old, new)
    OUTPUT.write_text(text)
    print(OUTPUT)


if __name__ == "__main__":
    main()
