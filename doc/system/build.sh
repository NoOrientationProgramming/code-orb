#!/bin/sh

if [ ! -z "$1" ]; then
	p="$1"
	echo "Building $p"
	plantuml -tpng $p
	plantuml -tsvg $p
	# apt-get install librsvg2-bin
	bName="$(basename ${p%.*})"
	# rsvg-convert -f pdf -o ${bName}.pdf ${bName}.svg
	echo "Building $p: Done"
	exit 0
fi

for p in *.pu; do
	echo "Building $p"
	plantuml -tpng $p
	plantuml -tsvg $p
	echo "Building $p: Done"
done

