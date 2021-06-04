#!/usr/bin/bash

# tt.sh

# Execute test plan of RF433any library
# Requires an Arduino plugged on PC

# Accepts one optional argument, the test number to execute.
# Without argument, runs all tests.

# Copyright 2021 Sébastien Millet
# 
# `RF433any' is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
# 
# `RF433any' is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this program. If not, see
# <https://www.gnu.org/licenses>.

set -euo pipefail

PORT=/dev/ttyUSB0

PASSED=0
FAILED=0

cd ..

START=1
STOP=5
if [ -n "${1:-}" ]; then
    START="$1";
    STOP="$1";
fi

for ((i=START; i<=STOP; i++)); do

    echo "== ROUND $i"

    testplan/test/am testplan/test/test.ino -u --stty -t "${i}"

    sleep 2

    if [ "${i}" -le 2 ]; then
        cd testplan/track
    elif [ "${i}" -le 4 ]; then
        cd testplan/decoder
    elif [ "${i}" -le 5 ]; then
        cd testplan/user
    else

        echo "Unknown testplan number, aborted."
        exit 99
    fi

    for d in [0-9][0-9]; do
        inpfile=$(ls "${d}"/code*)
        tmpout="${d}/tmpout${i}.txt"
        expfile="${d}/expect${i}.txt"
        ../exectest.sh "${inpfile}" "${tmpout}" "${PORT}"
        echo -n "$i:${d}"
        if cmp "${expfile}" "${tmpout}" > /dev/null 2> /dev/null; then
            PASSED=$((PASSED + 1))
            echo "    test ok"
        else
            FAILED=$((FAILED + 1))
            echo " ** TEST KO, actual output differs from expected"
        fi
    done

    cd ../..

done

echo "------"
echo "PASSED: ${PASSED}"
echo "FAILED: ${FAILED}"

if [ "${FAILED}" -eq 0 ]; then
    echo "OK"
else
    echo
    echo "**************"
    echo "***** KO *****"
    echo "**************"
    exit 1
fi
