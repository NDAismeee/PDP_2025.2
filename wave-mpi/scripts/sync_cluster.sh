#!/usr/bin/env bash

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

rsync -az \
    --delete \
    --exclude build \
    --exclude .git \
    "$PROJECT_DIR/" \
    slave1:~/wave-mpi/

rsync -az \
    --delete \
    --exclude build \
    --exclude .git \
    "$PROJECT_DIR/" \
    slave2:~/wave-mpi/

echo "Cluster synchronization completed."
