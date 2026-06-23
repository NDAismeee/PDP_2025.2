#!/usr/bin/env python3
"""
Plot per-rank timing (load balance) for each MPI configuration.

Reads rank_timing_*.csv files from results/summaries/ and saves
load_balance.png to results/figures/.
"""

import csv
import re
import sys
from pathlib import Path
from typing import Any, Dict, List, Tuple

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import numpy as np
except ImportError:
    print("matplotlib and numpy are required. Install with: pip install matplotlib numpy", file=sys.stderr)
    sys.exit(1)

ROOT = Path(__file__).resolve().parent.parent
SUMMARIES_DIR = ROOT / "results" / "summaries"
OUTPUT_PATH = ROOT / "results" / "figures" / "load_balance.png"

RANK_FILE_RE = re.compile(r"rank_timing_N(\d+)_P(\d+)\.csv")


def read_rank_timing(path: Path) -> List[Dict[str, Any]]:
    rows: List[Dict[str, Any]] = []
    with open(path, newline="") as fh:
        reader = csv.DictReader(fh)
        for row in reader:
            rows.append(
                {
                    "rank": int(row["rank"]),
                    "hostname": row["hostname"].strip(),
                    "local_start": int(row["local_start"]),
                    "local_count": int(row["local_count"]),
                    "compute_x": float(row["compute_x"]),
                    "communication_x": float(row["communication_x"]),
                    "compute_y": float(row["compute_y"]),
                    "communication_y": float(row["communication_y"]),
                    "total_time": float(row["total_time"]),
                }
            )
    return rows


def main() -> None:
    rank_files = sorted(SUMMARIES_DIR.glob("rank_timing_*.csv"))
    if not rank_files:
        print(
            f"Error: No rank_timing_*.csv files found in {SUMMARIES_DIR}. "
            "Run benchmark first: python scripts/run_experiments.py",
            file=sys.stderr,
        )
        sys.exit(1)

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)

    # Parse each file to get (N, P) and rows
    file_data: List[Tuple[int, int, List[Dict[str, Any]]]] = []
    for f in rank_files:
        m = RANK_FILE_RE.match(f.name)
        if not m:
            continue
        n = int(m.group(1))
        p = int(m.group(2))
        rows = read_rank_timing(f)
        if rows:
            file_data.append((n, p, rows))

    if not file_data:
        print("No valid rank timing data found.", file=sys.stderr)
        sys.exit(1)

    # Sort by N then P
    file_data.sort(key=lambda x: (x[0], x[1]))

    # Create one subplot per configuration
    n_configs = len(file_data)
    cols = min(3, n_configs)
    rows_count = (n_configs + cols - 1) // cols

    fig, axes = plt.subplots(
        rows_count, cols,
        figsize=(5 * cols, 4 * rows_count),
        squeeze=False,
    )
    fig.suptitle("Per-Rank Timing Distribution (Load Balance)", fontsize=14)

    for idx, (n, p, data) in enumerate(file_data):
        row = idx // cols
        col = idx % cols
        ax = axes[row][col]

        ranks = [d["rank"] for d in data]
        totals = [d["total_time"] for d in data]
        computes = [d["compute_x"] + d["compute_y"] for d in data]
        comms = [d["communication_x"] + d["communication_y"] for d in data]

        x = np.arange(len(ranks))
        width = 0.3

        ax.bar(x - width, totals, width, label="Total", alpha=0.9)
        ax.bar(x, computes, width, label="Compute", alpha=0.7)
        ax.bar(x + width, comms, width, label="Comm", alpha=0.7)

        ax.set_title(f"N={n} P={p}")
        ax.set_xlabel("Rank")
        ax.set_ylabel("Time (s)")
        ax.set_xticks(x)
        ax.set_xticklabels([str(r) for r in ranks])
        ax.legend(fontsize="small")
        ax.grid(axis="y", alpha=0.3)

        # Annotate load imbalance
        if totals:
            avg = sum(totals) / len(totals)
            max_t = max(totals)
            imbalance = (max_t - avg) / avg * 100.0 if avg > 0.0 else 0.0
            ax.text(
                0.98, 0.95,
                f"Imbalance: {imbalance:.1f}%",
                transform=ax.transAxes,
                ha="right",
                va="top",
                fontsize=8,
                bbox=dict(boxstyle="round", alpha=0.3, facecolor="white"),
            )

    # Hide unused subplots
    for idx in range(n_configs, rows_count * cols):
        row = idx // cols
        col = idx % cols
        axes[row][col].set_visible(False)

    plt.tight_layout()
    fig.savefig(str(OUTPUT_PATH), dpi=150)
    plt.close(fig)
    print(f"Load balance plot saved to {OUTPUT_PATH}")


if __name__ == "__main__":
    main()