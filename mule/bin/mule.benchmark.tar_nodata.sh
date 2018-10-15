#! /usr/bin/env bash

if [[ -z "$1" ]]; then
	echo ""
	echo "Create a tarball of one benchmkark directory (with job directories as subfolders)"
	echo "All large output files (*.csv) will be excluded"
	echo ""
	echo "Usage:"
	echo "	$0 [benchmark_folder_name]"
	echo ""
fi


BENCHDIR="$(realpath $1)"


TARBALL="$BENCHDIR.tar.xz"

if [[ -e "$TARBALL" ]]; then
	echo "Error: File $TARBALL already exists"
	exit 1
fi

echo "Creating tarball $TARBALL"
tar cJvf "$TARBALL" $BENCHDIR --exclude=*csv

