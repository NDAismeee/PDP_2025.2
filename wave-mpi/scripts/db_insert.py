#!/usr/bin/env python3
"""
Insert benchmark results from CSV files into PostgreSQL.

Reads benchmark_summary.csv and rank_timing_*.csv from results/summaries/
and pushes all rows to the PostgreSQL database via psycopg2.

Usage:
    python scripts/db_insert.py                          # insert all CSVs
    python scripts/db_insert.py --run-id 5               # insert single run

Requires: psycopg2  (pip install psycopg2-binary)
Requires the database to be running: docker compose up -d
"""

import csv
import os
import re
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional

try:
    import psycopg2
except ImportError:
    print("psycopg2 is required. Install: pip install psycopg2-binary", file=sys.stderr)
    sys.exit(1)

ROOT = Path(__file__).resolve().parent.parent
SUMMARIES_DIR = ROOT / "results" / "summaries"
SUMMARY_CSV = SUMMARIES_DIR / "benchmark_summary.csv"

DB_CONFIG = {
    "host": os.environ.get("PGHOST", "localhost"),
    "port": os.environ.get("PGPORT", "5432"),
    "dbname": os.environ.get("PGDATABASE", "wave_mpi"),
    "user": os.environ.get("PGUSER", "wave_user"),
    "password": os.environ.get("PGPASSWORD", "wave_pass"),
}

RANK_FILE_RE = re.compile(r"rank_timing_N(\d+)_P(\d+)\.csv")

SUMMARY_COLS = [
    "run_id", "timestamp", "grid_size", "time_steps", "processes", "machines",
    "solver", "communication", "total_time", "compute_x", "communication_x",
    "compute_y", "communication_y", "waiting_time", "max_error", "l2_error",
    "speedup", "efficiency", "status",
]

RANK_TIMING_COLS = [
    "run_id", "rank", "hostname", "local_start", "local_count",
    "compute_x", "communication_x", "compute_y", "communication_y", "total_time",
]


def get_connection():
    return psycopg2.connect(**DB_CONFIG)


def read_summary_csv(path: Path, target_run_id: Optional[int] = None) -> List[Dict[str, str]]:
    if not path.exists():
        print(f"Warning: {path} not found. Skipping.", file=sys.stderr)
        return []

    rows: List[Dict[str, str]] = []
    with open(path, newline="") as fh:
        reader = csv.DictReader(fh)
        for row in reader:
            rid = int(row.get("run_id", -1))
            if target_run_id is not None and rid != target_run_id:
                continue
            rows.append(row)
    return rows


def insert_summary(cur, row: Dict[str, str]):
    values = tuple(row.get(col, "") for col in SUMMARY_COLS)
    placeholders = ", ".join(["%s"] * len(values))
    cols = ", ".join(SUMMARY_COLS)
    sql = f"""
        INSERT INTO benchmark_summary ({cols})
        VALUES ({placeholders})
        ON CONFLICT (run_id) DO UPDATE SET
            timestamp    = EXCLUDED.timestamp,
            grid_size    = EXCLUDED.grid_size,
            time_steps   = EXCLUDED.time_steps,
            processes    = EXCLUDED.processes,
            machines     = EXCLUDED.machines,
            solver       = EXCLUDED.solver,
            communication = EXCLUDED.communication,
            total_time   = EXCLUDED.total_time,
            compute_x    = EXCLUDED.compute_x,
            communication_x = EXCLUDED.communication_x,
            compute_y    = EXCLUDED.compute_y,
            communication_y = EXCLUDED.communication_y,
            waiting_time = EXCLUDED.waiting_time,
            max_error    = EXCLUDED.max_error,
            l2_error     = EXCLUDED.l2_error,
            speedup      = EXCLUDED.speedup,
            efficiency   = EXCLUDED.efficiency,
            status       = EXCLUDED.status
    """
    cur.execute(sql, values)


def insert_rank_timing(cur, row: Dict[str, str]):
    values = tuple(row.get(col, "") for col in RANK_TIMING_COLS)
    placeholders = ", ".join(["%s"] * len(values))
    cols = ", ".join(RANK_TIMING_COLS)
    sql = f"INSERT INTO rank_timing ({cols}) VALUES ({placeholders})"
    cur.execute(sql, values)


def main() -> None:
    target_run_id: Optional[int] = None
    args = sys.argv[1:]
    for i, arg in enumerate(args):
        if arg == "--run-id" and i + 1 < len(args):
            target_run_id = int(args[i + 1])

    try:
        conn = get_connection()
        cur = conn.cursor()
    except psycopg2.OperationalError as e:
        print(f"Cannot connect to PostgreSQL: {e}", file=sys.stderr)
        print("Make sure the database is running:", file=sys.stderr)
        print("  docker compose up -d", file=sys.stderr)
        sys.exit(1)

    # Insert summary rows
    summary_rows = read_summary_csv(SUMMARY_CSV, target_run_id)
    for row in summary_rows:
        try:
            insert_summary(cur, row)
        except Exception as e:
            print(f"Failed to insert run_id={row.get('run_id')}: {e}", file=sys.stderr)

    # Insert per-rank timing
    rank_files = sorted(SUMMARIES_DIR.glob("rank_timing_*.csv"))
    for rf in rank_files:
        with open(rf, newline="") as fh:
            reader = csv.DictReader(fh)
            for row in reader:
                rid = int(row.get("run_id", -1))
                if target_run_id is not None and rid != target_run_id:
                    continue
                try:
                    insert_rank_timing(cur, row)
                except Exception as e:
                    print(f"Failed to insert rank_timing from {rf.name}: {e}", file=sys.stderr)

    conn.commit()
    cur.close()
    conn.close()

    print(f"Database insert complete. {len(summary_rows)} summary rows inserted.")
    print(f"Connect to psql: docker compose exec postgres psql -U wave_user -d wave_mpi")


if __name__ == "__main__":
    main()