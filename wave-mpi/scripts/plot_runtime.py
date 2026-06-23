#!/usr/bin/env python3
"""
Plot runtime breakdown: total, compute, and communication vs grid size.

Reads benchmark_summary.csv and saves runtime.png to results/figures/.
"""

import csv
import sys
from collections import defaultdict
from pathlib import Path
from typing import Any, Dict, List, Tuple

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
except ImportError:
    print("matplotlib is required. Install with: pip install matplotlib", file=sys.stderr)
    sys.exit(1)

ROOT = Path(__file__).resolve().parent.parent
SUMMARY_PATH = ROOT / "results" / "summaries" / "benchmark_summary.csv"
OUTPUT_PATH = ROOT / "results" / "figures" / "runtime.png"


def read_summary(path: Path) -> List[Dict[str, Any]]:
    if not path.exists():
        print(f"Error: {path} not found. Run benchmark first: python scripts/run_experiments.py", file=sys.stderr)
        sys.exit(1)

    rows: List[Dict[str, Any]] = []
    with open(path, newline="") as fh:
        reader = csv.DictReader(fh)
        for row in reader:
            if row.get("status", "").strip() != "success":
                continue
            rows.append(
                {
                    "grid_size": int(row["grid_size"]),
                    "processes": int(row["processes"]),
                    "communication": row["communication"].strip(),
                    "total_time": float(row["total_time"]),
                    "compute_x": float(row["compute_x"]),
                    "communication_x": float(row["communication_x"]),
                    "compute_y": float(row["compute_y"]),
                    "communication_y": float(row["communication_y"]),
                }
            )
    return rows


def main() -> None:
    rows = read_summary(SUMMARY_PATH)
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)

    # Group by (communication, processes) -> list of (grid_size, total_time, ...)
    grouped: Dict[Tuple[str, int], Dict[int, Dict[str, float]]] = defaultdict(
        lambda: defaultdict(lambda: {"total": 0.0, "compute": 0.0, "comm": 0.0})
    )

    for r in rows:
        key = (r["communication"], r["processes"])
        gs = r["grid_size"]
        grouped[key][gs]["total"] = r["total_time"]
        grouped[key][gs]["compute"] = r["compute_x"] + r["compute_y"]
        grouped[key][gs]["comm"] = r["communication_x"] + r["communication_y"]

    if not grouped:
        print("No successful benchmark data found.", file=sys.stderr)
        sys.exit(1)

    # Use the first (communication, processes) key to determine available grid sizes
    first_key = next(iter(grouped))
    grid_sizes = sorted(grouped[first_key].keys())

    # Create subplots: one per process count, comparing blocking vs nonblocking
    process_counts = sorted(set(p for _, p in grouped.keys()))

    fig, axes = plt.subplots(
        len(process_counts), 1,
        figsize=(10, 5 * len(process_counts)),
        squeeze=False,
    )
    fig.suptitle("Runtime Breakdown: Total vs Compute vs Communication", fontsize=14)

    for idx, np in enumerate(process_counts):
        ax = axes[idx][0]
        x = range(len(grid_sizes))
        width = 0.12
        labels = [str(gs) for gs in grid_sizes]

        for j, comm in enumerate(["blocking", "nonblocking"]):
            key = (comm, np)
            if key not in grouped:
                continue
            data = grouped[key]
            totals = [data[gs]["total"] for gs in grid_sizes]
            computes = [data[gs]["compute"] for gs in grid_sizes]
            comms = [data[gs]["comm"] for gs in grid_sizes]

            offset = j * 3 * width
            ax.bar([xi + offset for xi in x], totals, width, label=f"{comm} total", alpha=0.9)
            ax.bar([xi + offset + width for xi in x], computes, width, label=f"{comm} compute", alpha=0.7)
            ax.bar([xi + offset + 2 * width for xi in x], comms, width, label=f"{comm} comm", alpha=0.7)

        ax.set_title(f"P = {np}")
        ax.set_xticks([xi + 1.5 * width for xi in x])
        ax.set_xticklabels(labels)
        ax.set_xlabel("Grid size N")
        ax.set_ylabel("Time (s)")
        ax.legend(fontsize="small", ncol=2)
        ax.grid(axis="y", alpha=0.3)

    plt.tight_layout()
    fig.savefig(str(OUTPUT_PATH), dpi=150)
    plt.close(fig)
    print(f"Runtime plot saved to {OUTPUT_PATH}")


if __name__ == "__main__":
    main()