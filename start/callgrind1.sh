#!/bin/sh

dHere="$(pwd)"
dTool="$dHere/$(dirname $0)"
dTarget="$dTool/../build-native"
dRelHereToTarget="$(realpath --relative-to=$dHere $dTarget)"

cd "${dRelHereToTarget}" && \
rm -f ./callgrind.out.* && \
ninja && \

valgrind \
	--tool=callgrind \
./gw-dbg-swt \
	--verbosity 0

