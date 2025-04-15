#!/bin/sh

if [ -z "$1" ]; then
	echo "usage: $0 <file>"
	exit 1
fi

cppcheck \
	--language=c++ \
	--std=c++11 \
	--enable=all \
	--check-level=exhaustive \
	-D dPackageName="\"CodeOrb\"" \
	--suppress=ctuOneDefinitionRuleViolation \
	--suppress=unusedFunction \
	--suppress=missingOverride \
	--suppress=cstyleCast \
	--suppress=variableScope \
	--suppress=missingInclude \
	--suppress=missingIncludeSystem \
	--suppress=noExplicitConstructor \
	--suppress=uselessAssignmentPtrArg \
	--suppress=constParameterCallback \
	--suppress=unmatchedSuppression \
	--suppress=checkersReport \
	$1

