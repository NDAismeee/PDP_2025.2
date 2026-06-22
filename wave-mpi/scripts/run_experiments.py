#!/usr/bin/env python3
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = ROOT / "build"

GRID_SIZES = [127, 255, 511, 1023]
PROCESS_COUNTS = [1, 2, 4, 8]
COMMUNICATION_MODES = ["blocking", "nonblocking"]
REPEAT_COUNT = 5


def main() -> None:
    print("run_experiments.py: pending full benchmark automation")
    print(f"project_root={ROOT}")
    print(f"planned_grid_sizes={GRID_SIZES}")
    print(f"planned_process_counts={PROCESS_COUNTS}")
    print(f"planned_modes={COMMUNICATION_MODES}")
    print(f"repeat_count={REPEAT_COUNT}")


if __name__ == "__main__":
    main()
