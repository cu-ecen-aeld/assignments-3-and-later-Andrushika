#!/bin/sh

if [ $# -lt 2 ]; then
    echo "Error: Two arguments required: writefile and writestr"
    exit 1
fi

writefile=$1
writestr=$2

mkdir -p "$(dirname "$writefile")"

echo "$writestr" > "$writefile"
if [ $? -ne 0 ]; then
    echo "Error: Could not create file ${writefile}"
    exit 1
fi
