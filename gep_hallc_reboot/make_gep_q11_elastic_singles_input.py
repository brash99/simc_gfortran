#!/usr/bin/env python3
"""Derive the GEp Q2=11 electron-singles SIMC input from the matched baseline."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "infiles" / "gep_q11_nps_30cm_3m_10M.inp"
OUTPUT = ROOT / "infiles" / "gep_q11_nps_elastic_singles.inp"


def replace_once(text: str, old: str, new: str) -> str:
    if text.count(old) != 1:
        raise ValueError(f"expected exactly one occurrence of {old!r}")
    return text.replace(old, new)


def main() -> None:
    text = SOURCE.read_text()
    text = text.replace(
        "; High-statistics GEp-IV Q2=11 elastic sample for direct comparison with\n"
        "; xem2_run17193_nps_30cm_3m_10M.inp.  The target, NPS geometry, acceptance,\n"
        "; and radiative convention are held fixed while the central kinematics are\n"
        "; changed to Andrew's original proposed setting.\n",
        "; GEp-IV Q2=11 radiated-elastic electron-singles sample.\n"
        "; spect_mode=-2 disables proton-arm transport so no HMS coincidence\n"
        "; acceptance is imposed; the electron arm is the full 3 m NPS face.\n",
    )
    replacements = (
        ("  ngen = -10000000", "  ngen = -2000000"),
        ("  random_seed = 11001184", "  random_seed = 11001191"),
        ("  SPedge%e%delta%min = -15.0", "  SPedge%e%delta%min = -85.0"),
        ("  SPedge%e%delta%max = 15.0", "  SPedge%e%delta%max = 5.0"),
        ("  SPedge%p%delta%min = -15.0", "  SPedge%p%delta%min = -100.0"),
        ("  SPedge%p%delta%max = 15.0", "  SPedge%p%delta%max = 100.0"),
        ("  SPedge%p%yptar%min = -90.0", "  SPedge%p%yptar%min = -1000.0"),
        ("  SPedge%p%yptar%max = 90.0", "  SPedge%p%yptar%max = 1000.0"),
        ("  SPedge%p%xptar%min = -50.0", "  SPedge%p%xptar%min = -1000.0"),
        ("  SPedge%p%xptar%max = 50.0", "  SPedge%p%xptar%max = 1000.0"),
        ("  spect_mode = 0", "  spect_mode = -2"),
        ("  Nntu = 1", "  Nntu = 0"),
        ("  hybrid_export = 0", "  hybrid_export = 1"),
    )
    for old, new in replacements:
        text = replace_once(text, old, new)
    OUTPUT.write_text(text)
    print(OUTPUT)


if __name__ == "__main__":
    main()
