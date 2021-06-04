#!/usr/bin/bash

set -euo pipefail

INPFILE=${1:-}
OUTFILE=${2:-}
PORT=${3:-}

if [ -z "${PORT}" ] || [ -n "${4:-}" ]; then
    echo "Usage:"
    echo "  exectest.sh INPFILE OUTFILE PORT"
    echo "Example:"
    echo "  exectest.sh codes.txt results.txt /dev/ttyUSB0"
    exit 1
fi

../read_test_result_from_board.sh "${PORT}" > "${OUTFILE}" &

sleep 0.2

cat "${INPFILE}" > "${PORT}"
echo "." > "${PORT}"

wait

