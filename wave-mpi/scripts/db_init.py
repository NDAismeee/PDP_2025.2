#!/usr/bin/env python3
"""
Initialize the PostgreSQL database for Wave-MPI benchmark results.

Usage:
    python scripts/db_init.py                  # create tables
    python scripts/db_init.py --drop           # drop then recreate all tables

Requires: psycopg2  (pip install psycopg2-binary)
Connection defaults (override via environment variables):
    PGHOST=localhost  PGPORT=5432  PGDATABASE=wave_mpi
    PGUSER=wave_user  PGPASSWORD=wave_pass
"""

import os
import sys
from pathlib import Path

try:
    import psycopg2
except ImportError:
    print("psycopg2 is required. Install: pip install psycopg2-binary", file=sys.stderr)
    sys.exit(1)

ROOT = Path(__file__).resolve().parent.parent
SCHEMA_PATH = ROOT / "db" / "schema.sql"

DB_CONFIG = {
    "host": os.environ.get("PGHOST", "localhost"),
    "port": os.environ.get("PGPORT", "5432"),
    "dbname": os.environ.get("PGDATABASE", "wave_mpi"),
    "user": os.environ.get("PGUSER", "wave_user"),
    "password": os.environ.get("PGPASSWORD", "wave_pass"),
}


def get_connection():
    return psycopg2.connect(**DB_CONFIG)


def main() -> None:
    drop_first = "--drop" in sys.argv

    if not SCHEMA_PATH.exists():
        print(f"Error: schema not found at {SCHEMA_PATH}", file=sys.stderr)
        sys.exit(1)

    schema_sql = SCHEMA_PATH.read_text()

    try:
        conn = get_connection()
        conn.autocommit = True
        cur = conn.cursor()

        if drop_first:
            print("Dropping existing tables ...")
            cur.execute(
                "DROP TABLE IF EXISTS rank_timing, speedup_summary, benchmark_summary CASCADE;"
            )
            print("Tables dropped.")

        print(f"Executing schema: {SCHEMA_PATH}")
        cur.execute(schema_sql)
        print("Schema applied successfully.")

        cur.close()
        conn.close()
    except psycopg2.OperationalError as e:
        print(f"Cannot connect to PostgreSQL: {e}", file=sys.stderr)
        print("Make sure the database is running:", file=sys.stderr)
        print("  docker compose up -d", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()