#!/usr/bin/bash

set -euo pipefail

PORT=/dev/ttyUSB0
INP_FILE=${1:-}

if [ "${INP_FILE}" == "" ]; then
    echo "Usage:"
    echo "  ./simul.sh code_file"
    echo
    echo "You have to update the variable PORT defined at the"
    echo "beginning of this script. At the moment it is:"
    echo "${PORT}"
    echo
    echo "code_file contains lines of timings in the form"
    echo "low_duration,high_duration"
    echo "For example:"
    echo "0,10000"
    echo "300,500"
    echo "300,500"
    echo "500,300"
    echo "300,5000"
    exit 1
fi

./am2 -R -t 4 simul.ino -u
stty -F "${PORT}" -hupcl -echo 115200
cat "${PORT}" &

sleep 1

cat "${INP_FILE}" > "${PORT}"
echo "." > "${PORT}"

sleep 1

# From
#   https://unix.stackexchange.com/questions/43527/kill-all-background-jobs
kill $(jobs -p)

exit 0
