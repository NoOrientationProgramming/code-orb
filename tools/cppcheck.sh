#!/bin/sh

find \
	. \
	-type f \
	-name "*.cpp" \
	! -path "./deps/ProcessingCore/targets/stm32/SystemDebugging.*" \
	! -path "./deps/tclap_loc/*" \
| xargs cppcheck \
	--language=c++ \
	--std=c++11 \
	$@

