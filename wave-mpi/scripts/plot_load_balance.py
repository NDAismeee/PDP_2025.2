#!/usr/bin/env python3
from pathlib import Path

TIMING_DIR = Path(__file__).resolve().parent.parent / "results" / "summaries"
OUTPUT_PATH = Path(__file__).resolve().parent.parent / "results" / "figures" / "load_balance.png"


def main() -> None:
    print(f"plot_load_balance.py: read {TIMING_DIR}, write {OUTPUT_PATH}")


if __name__ == "__main__":
    main()
