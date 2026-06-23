#!/usr/bin/env python3
"""
Plot speedup and efficiency vs process count for each grid size.

Reads benchmark_summary.csv and saves speedup.png to results/figures/.
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
OUTPUT_PATH = ROOT / "results" / "figures" / "speedup.png"


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
                    "speedup": float(row["speedup"]),
                    "efficiency": float(row["efficiency"]),
                    "total_time": float(row["total_time"]),
                }
            )
    return rows


def main() -> None:
    rows = read_summary(SUMMARY_PATH)
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)

    # Group by grid_size -> dict of process -> data
    grid_sizes = sorted(set(r["grid_size"] for r in rows))
    comm_modes = sorted(set(r["communication"] for r in rows))

    fig, axes = plt.subplots(
        len(comm_modes), 2,
        figsize=(14, 5 * len(comm_modes)),
        squeeze=False,
    )
    fig.suptitle("Speedup and Efficiency Analysis", fontsize=14)

    for ci, comm in enumerate(comm_modes):
        data_speedup: Dict[int, Dict[int, float]] = defaultdict(dict)
        data_efficiency: Dict[int, Dict[int, float]] = defaultdict(dict)

        for r in rows:
            if r["communication"] != comm:
                continue
            data_speedup[r["grid_size"]][r["processes"]] = r["speedup"]
            data_efficiency[r["grid_size"]][r["processes"]] = r["efficiency"]

        # Speedup subplot
        ax_speedup = axes[ci][0]
        for gs in grid_sizes:
            procs = sorted(data_speedup[gs].keys())
            values = [data_speedup[gs][p] for p in procs]
            ax_speedup.plot(procs, values, marker="o", label=f"N={gs}")
        # Ideal speedup line
        all_procs = sorted(set(p for d in data_speedup.values() for p in d.keys()))
        if all_procs:
            ax_speedup.plot(all_procs, all_procs, "k--", alpha=0.3, label="Ideal")
        ax_speedup.set_title(f"Speedup – {comm}")
        ax_speedup.set_xlabel("Processes P")
        ax_speedup.set_ylabel("Speedup S(P)")
        ax_speedup.legend(fontsize="small")
        ax_speedup.grid(alpha=0.3)

        # Efficiency subplot
        ax_eff = axes[ci][1]
        for gs in grid_sizes:
            procs = sorted(data_efficiency[gs].keys())
            values = [data_efficiency[gs][p] for p in procs]
            ax_eff.plot(procs, values, marker="s", label=f"N={gs}")
        ax_eff.axhline(y=1.0, color="k", linestyle="--", alpha=0.3, label="Ideal")
        ax_eff.set_title(f"Efficiency – {comm}")
        ax_eff.set_xlabel("Processes P")
        ax_eff.set_ylabel("Efficiency E(P)")
        ax_eff.legend(fontsize="small")
        ax_eff.grid(alpha=0.3)

    plt.tight_layout()
    fig.savefig(str(OUTPUT_PATH), dpi=150)
    plt.close(fig)
    print(f"Speedup plot saved to {OUTPUT_PATH}")


if __name__ == "__main__":
    main()