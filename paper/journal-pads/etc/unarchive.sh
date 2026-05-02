#!/bin/bash -e

# for debugging
set -x

PROJECT_ROOT="$(git rev-parse --show-toplevel)"
cd "$PROJECT_ROOT"

# User supplied directory path (otherwise default to PROJECT_ROOT)
if [ -z "$1" ]; then
	QUERY_PATH="$PROJECT_ROOT"
else
	QUERY_PATH="$(realpath "$1")"
fi

# assume old exam archive folders are up to date
XML_FILES=$(find "$QUERY_PATH" -name *.xml | grep -v "\\.junk-dupes" | grep -v "/exam[1-2]")
test "$XML_FILES"

# Perform archiving operations
for f in $XML_FILES; do
	echo "$f"
	fmoddate=$(stat -c "%Y" $f)
	fbase="$(basename $f .xml)"

	# cd to file's directory
	cd "$(dirname "$f")"

	# Create .gz binary (with same mod-date as .xopp file)
	gzip -kf "$f"
	touch -d @$fmoddate "$f.gz"

	# Rename extension (.xopp is just .gz in disguise)
	mv "$f.gz" "$fbase.xopp"

	# We are currently in out/xml, so...
	# Move back to grandparent directory (and prompt user to confirm any overwrites)
	mv -i "$fbase.xopp" ../..

	# cd back to original directory, for good measure
	cd "$PROJECT_ROOT"
done
