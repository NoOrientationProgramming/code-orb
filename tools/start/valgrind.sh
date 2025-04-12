#!/bin/sh

dHere="$(pwd)"
dTool="$dHere/$(dirname $0)"
dTarget="$dTool/../build-native"
dRelHereToTarget="$(realpath --relative-to=$dHere $dTarget)"
dRelTargetToTool="$(realpath --relative-to=$dTarget $dTool)"

#	--gen-suppressions=all \

cd "${dRelHereToTarget}" && \
ninja && \

valgrind \
	--leak-check=full \
	--show-leak-kinds=all \
	--suppressions=${dRelTargetToTool}/valgrind_suppressions.txt \
./codeorb \
	$@

