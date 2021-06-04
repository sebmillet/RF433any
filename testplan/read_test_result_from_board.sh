#!/usr/bin/bash

# read_test_result_from_board.sh

# Copyright 2021 Sébastien Millet

# Read the test result from the board on PORT specified as argument, typically
# /dev/ttyUSB0.
# The test result is supposed enclosed between
#   ----- BEGIN TEST -----
# and
#   ----- END TEST -----
# lines.

set -euo pipefail

# Trick to read line by line in bash:
#   https://stackoverflow.com/questions/10929453/read-a-file-line-by-line-assigning-the-value-to-a-variable

REC=0

PORT=${1:-}

if [ -z "${PORT}" ] || [ -n "${2-}" ]; then
    echo "Usage:"
    echo "  read_test_result_from_board.sh PORT"
    echo "For example:"
    echo "  read_test_result_from_board.sh /dev/ttyUSB0"
    exit 1
fi

while IFS= read -r l; do
    if [ "${l}" == "----- END TEST -----" ]; then
        exit 0
    fi
    if [ $REC -eq 1 ]; then
        echo "${l}"
    fi
    if [ "${l}" == "----- BEGIN TEST -----" ]; then
        REC=1
    fi
done < "${PORT}"

exit 1
