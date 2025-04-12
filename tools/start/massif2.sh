#!/bin/sh

dHere="$(pwd)"
dTool="$dHere/$(dirname $0)"
dTarget="$dTool/../../build-native"
dRelHereToTarget="$(realpath --relative-to=$dHere $dTarget)"

file="$(ls -1 ${dRelHereToTarget}/massif.out.* | sort | tail -n 1)"
if [ -n "$1" ]; then
	file="$1"
fi

massif-visualizer "$file" > /dev/null 2>&1 &

