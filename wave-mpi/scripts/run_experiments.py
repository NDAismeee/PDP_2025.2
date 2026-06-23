#!/usr/bin/env python3
"""
Automated benchmark script for the wave-mpi project.

Runs every (grid_size, process_count, communication_mode) combination
REPEAT_COUNT times, parses stdout for key=value output, and writes
aggregated summary rows (median timings) to benchmark_summary.csv.
"""

import csv
import os
import re
import statistics
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Optional, Tuple

ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = ROOT / "build"
RAW_DIR = ROOT / "results" / "raw"
SUMMARIES_DIR = ROOT / "results" / "summaries"
SUMMARY_CSV = SUMMARIES_DIR / "benchmark_summary.csv"

SERIAL_BIN = BUILD_DIR / "wave_serial"
MPI_BIN = BUILD_DIR / "wave_mpi"

GRID_SIZES = [127, 255, 511, 1023]
PROCESS_COUNTS = [1, 2, 4, 8]
COMMUNICATION_MODES = ["blocking", "nonblocking"]
REPEAT_COUNT = 5
SOLVER = "thomas"

KEY_VALUE_RE = re.compile(r"^(\w[\w_]*)=(.*)$")


def parse_output(stdout: str) -> Dict[str, str]:
    """Parse key=value pairs from program stdout."""
    result: Dict[str, str] = {}
    for line in stdout.strip().splitlines():
        line = line.strip()
        if not line:
            continue
        m = KEY_VALUE_RE.match(line)
        if m:
            result[m.group(1)] = m.group(2)
    return result


def run_once(
    args: List[str], mpirun: bool = False, np: int = 1, hostfile: Optional[Path] = None
) -> subprocess.CompletedProcess:
    """Run a single execution of wave_serial or wave_mpi."""
    cmd: List[str] = []
    if mpirun:
        cmd = ["mpirun", "-np", str(np)]
        if hostfile and hostfile.exists():
            cmd += ["--hostfile", str(hostfile)]
        cmd += [str(MPI_BIN)]
        cmd += args
    else:
        cmd = [str(SERIAL_BIN)] + args

    return subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        cwd=str(ROOT),
    )


def extract_float(data: Dict[str, str], key: str) -> float:
    try:
        return float(data[key])
    except (KeyError, ValueError):
        return 0.0


def median_of(values: List[float]) -> float:
    if not values:
        return 0.0
    return statistics.median(values)


def write_summary_header_once():
    """Write CSV header if the file doesn't exist yet."""
    if SUMMARY_CSV.exists():
        return
    SUMMARY_CSV.parent.mkdir(parents=True, exist_ok=True)
    with open(SUMMARY_CSV, "w", newline="") as fh:
        writer = csv.writer(fh)
        writer.writerow(
            [
                "run_id",
                "timestamp",
                "grid_size",
                "time_steps",
                "processes",
                "machines",
                "solver",
                "communication",
                "total_time",
                "compute_x",
                "communication_x",
                "compute_y",
                "communication_y",
                "waiting_time",
                "max_error",
                "l2_error",
                "speedup",
                "efficiency",
                "status",
            ]
        )


def main() -> None:
    if not SERIAL_BIN.exists():
        print(
            f"Error: {SERIAL_BIN} not found. Build the project first: cmake --build build -j",
            file=sys.stderr,
        )
        sys.exit(1)
    if not MPI_BIN.exists():
        print(
            f"Error: {MPI_BIN} not found. Build the project first: cmake --build build -j",
            file=sys.stderr,
        )
        sys.exit(1)

    RAW_DIR.mkdir(parents=True, exist_ok=True)
    SUMMARIES_DIR.mkdir(parents=True, exist_ok=True)
    write_summary_header_once()

    hostfile = ROOT / "hosts"

    run_id = 0

    # Pre-compute serial reference times for each grid size
    serial_times: Dict[int, float] = {}
    for grid_size in GRID_SIZES:
        serial_times_list: List[float] = []
        ref_path = RAW_DIR / f"serial_N{grid_size}.csv"
        for _ in range(REPEAT_COUNT):
            proc = run_once(
                [
                    "--grid-size",
                    str(grid_size),
                    "--time-steps",
                    "50",
                    "--solver",
                    SOLVER,
                    "--output",
                    str(ref_path),
                ],
                mpirun=False,
            )
            data = parse_output(proc.stdout)
            total = extract_float(data, "runtime_seconds")
            if total > 0.0:
                serial_times_list.append(total)
        if serial_times_list:
            serial_times[grid_size] = median_of(serial_times_list)
            print(
                f"serial N={grid_size}: median total = {serial_times[grid_size]:.6f} s"
            )
        else:
            print(f"WARNING: serial N={grid_size} failed all runs", file=sys.stderr)

    # Benchmark MPI configurations
    for grid_size in GRID_SIZES:
        ref_path = RAW_DIR / f"serial_N{grid_size}.csv"
        serial_time = serial_times.get(grid_size, 0.0)

        for np in PROCESS_COUNTS:
            for comm_mode in COMMUNICATION_MODES:
                total_list: List[float] = []
                compute_x_list: List[float] = []
                communication_x_list: List[float] = []
                compute_y_list: List[float] = []
                communication_y_list: List[float] = []
                max_error_list: List[float] = []
                l2_error_list: List[float] = []
                status_list: List[str] = []

                for _ in range(REPEAT_COUNT):
                    args = [
                        "--grid-size",
                        str(grid_size),
                        "--time-steps",
                        "50",
                        "--solver",
                        SOLVER,
                        "--communication",
                        comm_mode,
                        "--reference",
                        str(ref_path),
                        "--output",
                        str(RAW_DIR / f"mpi_N{grid_size}_P{np}_{comm_mode}.csv"),
                    ]

                    proc = run_once(args, mpirun=True, np=np, hostfile=hostfile)
                    data = parse_output(proc.stdout)

                    total = extract_float(data, "total_seconds")
                    if total > 0.0:
                        total_list.append(total)
                        compute_x_list.append(extract_float(data, "compute_x_seconds"))
                        communication_x_list.append(
                            extract_float(data, "communication_x_seconds")
                        )
                        compute_y_list.append(extract_float(data, "compute_y_seconds"))
                        communication_y_list.append(
                            extract_float(data, "communication_y_seconds")
                        )
                        max_err = extract_float(data, "max_absolute_error")
                        l2 = extract_float(data, "l2_error")
                        if max_err > 0.0 or l2 > 0.0:
                            max_error_list.append(max_err)
                            l2_error_list.append(l2)
                        status_list.append(data.get("status", "unknown"))
                    else:
                        status_list.append("failed")

                if not total_list:
                    print(
                        f"SKIP N={grid_size} P={np} {comm_mode}: all runs failed",
                        file=sys.stderr,
                    )
                    continue

                median_total = median_of(total_list)
                median_compute_x = median_of(compute_x_list)
                median_comm_x = median_of(communication_x_list)
                median_compute_y = median_of(compute_y_list)
                median_comm_y = median_of(communication_y_list)
                median_max_error = median_of(max_error_list) if max_error_list else 0.0
                median_l2_error = median_of(l2_error_list) if l2_error_list else 0.0

                # Speedup & efficiency
                speedup = serial_time / median_total if serial_time > 0.0 else 0.0
                efficiency = speedup / np if np > 0 else 0.0

                status = "success" if "failed" not in status_list else "failed"

                timestamp = datetime.now(timezone.utc).isoformat()

                with open(SUMMARY_CSV, "a", newline="") as fh:
                    writer = csv.writer(fh)
                    writer.writerow(
                        [
                            run_id,
                            timestamp,
                            grid_size,
                            50,
                            np,
                            1,  # machines – update when using --machine-count
                            SOLVER,
                            comm_mode,
                            f"{median_total:.6f}",
                            f"{median_compute_x:.6f}",
                            f"{median_comm_x:.6f}",
                            f"{median_compute_y:.6f}",
                            f"{median_comm_y:.6f}",
                            "0.0",
                            f"{median_max_error:.12e}",
                            f"{median_l2_error:.12e}",
                            f"{speedup:.4f}",
                            f"{efficiency:.4f}",
                            status,
                        ]
                    )

                print(
                    f"run_id={run_id} N={grid_size} P={np} {comm_mode} "
                    f"total={median_total:.4f}s speedup={speedup:.2f}x "
                    f"efficiency={efficiency:.3f}"
                )

                run_id += 1

    print(f"\nBenchmark complete.  {run_id} configurations written to {SUMMARY_CSV}")


if __name__ == "__main__":
    main()