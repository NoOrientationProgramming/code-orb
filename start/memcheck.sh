#!/bin/sh

dHere="$(pwd)"
dTool="$dHere/$(dirname $0)"
dTarget="$dTool/../build-native"
dRelHereToTarget="$(realpath --relative-to=$dHere $dTarget)"

cd "${dRelHereToTarget}" && \
ninja && \

valgrind \
	--tool=memcheck \
	--track-origins=yes \
./gw-dbg-swt \
	--port-telnet 4000 \
	--port-ssh 4001 \
	$@

