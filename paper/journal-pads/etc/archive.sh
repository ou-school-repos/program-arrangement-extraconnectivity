#!/bin/bash -e

# for debugging
# set -x

PROJECT_ROOT="$(git rev-parse --show-toplevel)"
NESTED_DIR="paper/journal-pads"
cd "$PROJECT_ROOT/$NESTED_DIR"

# User supplied directory path (otherwise default to PROJECT_ROOT)
if [ -z "$1" ]; then
	QUERY_PATH="$PROJECT_ROOT/$NESTED_DIR"
else
	QUERY_PATH="$(realpath "$1")"
fi

echo $QUERY_PATH

# Input variables
PNG_DPI="${dpi:-72}"

XOPP_FILES=$(
	find "$QUERY_PATH" -name \*.xopp |
		grep -v "\\.autosave" | grep -v "\\.archive" | grep -v "\\.junk-dupes" | grep -v "/exam[1-2]/"
)
test "$XOPP_FILES"

# Perform archiving operations
for f in $XOPP_FILES; do
	echo "$f"
	continue
	fmoddate=$(stat -c "%Y" $f)
	fbase="$(basename $f .xopp)"

	# cd to file's directory
	cd "$(dirname "$f")"

	# Create PDF and DJVU (binaries)
	xournalpp "$f" -p "$fbase.pdf"
	# pdf2djvu "$fbase.pdf" -o "$fbase.djvu"

	touch -d @$fmoddate "$fbase.pdf"
	# touch -d @$fmoddate "$fbase.djvu"

	# Move binaries to folders in out/*/
	mkdir -p out/pdf/
	# mkdir -p out/djvu/
	mv "$fbase.pdf" out/pdf/
	# mv "$fbase.djvu" out/djvu/

	# Create PNG binaries
	xournalpp "$f" --export-png-dpi=${PNG_DPI} -i "$fbase".png
	mkdir -p out/png/
	touch -d @$fmoddate "$fbase"*.png
	mv "$fbase"*.png out/png/

	# Extract XML from xopp (natively gzipped)
	mkdir -p out/xml/
	cp -p $fbase.xopp out/xml/$fbase.xml.gz
	gzip -d -f out/xml/$fbase.xml.gz
	touch -d @$fmoddate out/xml/$fbase.xml

	# cd back to original directory, for good measure
	cd "$PROJECT_ROOT/$NESTED_DIR"
done
