-- Wave-MPI Benchmark Database Schema
-- PostgreSQL 16
-- Auto-created on container startup via docker-compose

-- Benchmark run summary: one row per experiment configuration
CREATE TABLE IF NOT EXISTS benchmark_summary (
    run_id          INTEGER PRIMARY KEY,
    timestamp       TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    grid_size       INTEGER NOT NULL,
    time_steps      INTEGER NOT NULL DEFAULT 50,
    processes       INTEGER NOT NULL,
    machines        INTEGER NOT NULL DEFAULT 1,
    solver          TEXT NOT NULL DEFAULT 'thomas',
    communication   TEXT NOT NULL DEFAULT 'blocking',
    total_time      DOUBLE PRECISION NOT NULL,
    compute_x       DOUBLE PRECISION NOT NULL DEFAULT 0.0,
    communication_x DOUBLE PRECISION NOT NULL DEFAULT 0.0,
    compute_y       DOUBLE PRECISION NOT NULL DEFAULT 0.0,
    communication_y DOUBLE PRECISION NOT NULL DEFAULT 0.0,
    waiting_time    DOUBLE PRECISION NOT NULL DEFAULT 0.0,
    max_error       DOUBLE PRECISION,
    l2_error        DOUBLE PRECISION,
    speedup         DOUBLE PRECISION,
    efficiency      DOUBLE PRECISION,
    status          TEXT NOT NULL DEFAULT 'unknown'
);

-- Per-rank timing: one row per MPI rank per run
CREATE TABLE IF NOT EXISTS rank_timing (
    id              SERIAL PRIMARY KEY,
    run_id          INTEGER NOT NULL REFERENCES benchmark_summary(run_id),
    rank            INTEGER NOT NULL,
    hostname        TEXT NOT NULL DEFAULT 'unknown',
    local_start     INTEGER NOT NULL DEFAULT 0,
    local_count     INTEGER NOT NULL DEFAULT 0,
    compute_x       DOUBLE PRECISION NOT NULL DEFAULT 0.0,
    communication_x DOUBLE PRECISION NOT NULL DEFAULT 0.0,
    compute_y       DOUBLE PRECISION NOT NULL DEFAULT 0.0,
    communication_y DOUBLE PRECISION NOT NULL DEFAULT 0.0,
    total_time      DOUBLE PRECISION NOT NULL DEFAULT 0.0
);

-- Speedup metrics precomputed per grid_size & process_count
CREATE TABLE IF NOT EXISTS speedup_summary (
    id              SERIAL PRIMARY KEY,
    grid_size       INTEGER NOT NULL,
    processes       INTEGER NOT NULL,
    communication   TEXT NOT NULL,
    serial_time     DOUBLE PRECISION NOT NULL,
    parallel_time   DOUBLE PRECISION NOT NULL,
    speedup         DOUBLE PRECISION NOT NULL,
    efficiency      DOUBLE PRECISION NOT NULL,
    UNIQUE (grid_size, processes, communication)
);

-- Indexes for common queries
CREATE INDEX IF NOT EXISTS idx_benchmark_grid ON benchmark_summary(grid_size);
CREATE INDEX IF NOT EXISTS idx_benchmark_procs ON benchmark_summary(processes);
CREATE INDEX IF NOT EXISTS idx_benchmark_status ON benchmark_summary(status);
CREATE INDEX IF NOT EXISTS idx_rank_timing_run ON rank_timing(run_id);