#!/usr/bin/bash

set -euo pipefail

cd track

for d in [0-9][0-9]; do
    rm -f "${d}"/tmpout*.txt
done

cd ../decoder

for d in [0-9][0-9]; do
    rm -f "${d}"/tmpout*.txt
done

cd ../user

for d in [0-9][0-9]; do
    rm -f "${d}"/tmpout*.txt
done

