#!/usr/bin/env python3
from pathlib import Path

SUMMARY_PATH = Path(__file__).resolve().parent.parent / "results" / "summaries" / "benchmark_summary.csv"
OUTPUT_PATH = Path(__file__).resolve().parent.parent / "results" / "figures" / "runtime.png"


def main() -> None:
    print(f"plot_runtime.py: read {SUMMARY_PATH}, write {OUTPUT_PATH}")


if __name__ == "__main__":
    main()
