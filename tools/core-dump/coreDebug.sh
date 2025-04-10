#!/bin/sh

fileExe="../../build-native/codeorb"

if [ -n "$1" ]; then
	fileCore="$1"
else
	fileCore="$(find report/*.core | tail -1)"
fi

echo
echo "##########################"
echo "#### Using core: $fileCore"
echo

gdb -q "$fileExe" "$fileCore"

# ---------------------

# gdb -q --args ./tgs ...

# (gdb) run
# (gdb) gcore output.core
# (gdb) tui enable
# (gdb) up
# (gdb) down
# (gdb) bt
# (gdb) f 9
# (gdb) info variables
# (gdb) info locals
# (gdb) info args
# (gdb) p <var>

# readelf -a output.core

