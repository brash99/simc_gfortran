#!/usr/bin/env python3
"""Plot the current GEp-IV proposal trigger-rate envelope."""

from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


ROOT = Path(__file__).resolve().parent
THRESHOLD = np.array([3.4, 3.6, 3.9])
ELASTIC = np.array([2.248, 2.234, 2.182])
PI0_BASELINE = np.array([0.467, 0.377, 0.245])

# NPS rates are the run-1918 EDTM ambient estimate and accepted-physics
# cross-check, scaled to 75 uA and 30 cm. HMS rates use the run-2875
# positive-polarity projection. All accidentals assume a 20 ns full window.
NPS_AMBIENT = np.array([89.3e3, 74.1e3, 59.3e3])
NPS_UPPER = np.array([323.107e3, 263.0e3, 195.0e3])
WINDOW = 20e-9
HMS = {
    "S1X AND S1Y": (7.430e3, "#1f77b4", "o"),
    "S1/S2 3-of-4": (0.929e3, "#d95f02", "s"),
}


def main() -> None:
    plt.rcParams.update(
        {
            "font.size": 11,
            "axes.titlesize": 13,
            "axes.labelsize": 12,
            "legend.fontsize": 10,
            "figure.dpi": 160,
        }
    )
    fig, axes = plt.subplots(1, 2, figsize=(11.5, 4.6), constrained_layout=True)

    for label, (hms_rate, color, marker) in HMS.items():
        central_acc = NPS_AMBIENT * hms_rate * WINDOW
        upper_acc = NPS_UPPER * hms_rate * WINDOW
        central_total = central_acc + ELASTIC + PI0_BASELINE
        upper_total = upper_acc + ELASTIC + PI0_BASELINE

        axes[0].plot(
            THRESHOLD,
            central_acc,
            color=color,
            marker=marker,
            linewidth=2.2,
            markersize=7,
            label=label,
        )
        axes[1].fill_between(
            THRESHOLD,
            central_total,
            upper_total,
            color=color,
            alpha=0.17,
        )
        axes[1].plot(
            THRESHOLD,
            central_total,
            color=color,
            marker=marker,
            linewidth=2.2,
            markersize=7,
            label=f"{label}: central",
        )
        axes[1].plot(
            THRESHOLD,
            upper_total,
            color=color,
            linestyle="--",
            linewidth=1.5,
            label=f"{label}: conservative",
        )

    axes[0].plot(
        THRESHOLD,
        ELASTIC,
        color="#2ca02c",
        marker="^",
        linewidth=1.8,
        label="elastic signal",
    )
    axes[0].plot(
        THRESHOLD,
        PI0_BASELINE,
        color="#9467bd",
        marker="D",
        linewidth=1.8,
        label=r"baseline $\gamma p\rightarrow\pi^0p$",
    )

    axes[0].set_title("Components of the central trigger budget")
    axes[0].set_ylabel("Rate (Hz)")
    axes[0].legend(frameon=False)
    axes[1].set_title("Total-rate commissioning envelope")
    axes[1].set_ylabel("Total Level-1 rate (Hz)")
    axes[1].legend(frameon=False, loc="upper right")

    for ax in axes:
        ax.set_xlabel("Offline-equivalent NPS threshold (GeV)")
        ax.set_xticks(THRESHOLD)
        ax.set_xlim(3.34, 3.96)
        ax.set_ylim(bottom=0)
        ax.grid(True, alpha=0.25)

    fig.suptitle(
        "GEp-IV NPS x HMS trigger projection: 75 μA, 30 cm LH$_2$, 20 ns",
        fontsize=15,
    )
    fig.text(
        0.5,
        -0.02,
        "E12-13-010 run 1918 anchors the 3 m production spectrum; an E12-13-010-period elastic calibration (run 2875) anchors positive-polarity HMS rates. "
        "Shaded bands use the trigger-correlated NPS spectrum as a conservative cross-check.",
        ha="center",
        fontsize=9,
    )

    output = ROOT / "plots" / "pac_trigger_budget"
    fig.savefig(output.with_suffix(".png"), bbox_inches="tight")
    fig.savefig(output.with_suffix(".pdf"), bbox_inches="tight")


if __name__ == "__main__":
    main()
