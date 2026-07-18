#!/bin/bash -e

set -e

# for debugging
# set -x

PROJECT_ROOT="$(git rev-parse --show-toplevel)"
NESTED_DIR="paper/journal-pads"
JOURNAL_ROOT="$PROJECT_ROOT/$NESTED_DIR"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$JOURNAL_ROOT"

# User supplied directory path (otherwise default to journal-pads root)
if [ -z "$1" ]; then
	QUERY_PATH="$JOURNAL_ROOT"
else
	QUERY_PATH="$(realpath "$1")"
fi

echo "$QUERY_PATH"

# Input variables
PNG_DPI="${dpi:-72}"

FIND_TMP="$(mktemp)"
trap 'rm -f "$FIND_TMP"' EXIT
if ! find "$QUERY_PATH" \
	\( -type d -name out -o -type d -name .git \) -prune -o \
	-type f -name '*.xopp' \
	! -path '*.autosave*' \
	! -path '*.archive*' \
	! -path '*/.junk-dupes/*' \
	! -path '*/exam[1-2]/*' \
	-print0 >"$FIND_TMP"; then
	echo "Error: find failed while searching $QUERY_PATH" >&2
	exit 1
fi
mapfile -d '' XOPP_FILES <"$FIND_TMP"

if [ "${#XOPP_FILES[@]}" -eq 0 ]; then
	echo "No source .xopp files found under $QUERY_PATH" >&2
	exit 1
fi

# Perform archiving operations
for f in "${XOPP_FILES[@]}"; do
	echo "$f"
	fmoddate=$(stat -c "%Y" "$f")
	fbase="$(basename "$f" .xopp)"

	# cd to file's directory
	cd "$(dirname "$f")"

	# Create PDF and DJVU (binaries)
	xournalpp "$f" -p "$fbase.pdf"
	touch -d @$fmoddate "$fbase.pdf"
	"$SCRIPT_DIR/pdfdet" "$fbase.pdf"

	if ! "$SCRIPT_DIR/pdf2djvudet" "$fbase.pdf"; then
		echo "Warning: failed to create $fbase.djvu; continuing without DjVu output" >&2
		rm -f "$fbase.djvu" "out/djvu/$fbase.djvu"
	fi

	# Move binaries to folders in out/*/
	mkdir -p out/pdf/
	mv "$fbase.pdf" out/pdf/
	if [ -e "$fbase.djvu" ]; then
		mkdir -p out/djvu/
		mv "$fbase.djvu" out/djvu/
	fi

	# Create PNG binaries
	xournalpp "$f" --export-png-dpi=${PNG_DPI} -i "$fbase".png
	mkdir -p out/png/
	touch -d @$fmoddate "$fbase"*.png
	mv "$fbase"*.png out/png/

	# Extract XML from xopp (natively gzipped)
	mkdir -p out/xml/
	cp -p "$fbase.xopp" "out/xml/$fbase.xml.gz"
	gzip -d -f "out/xml/$fbase.xml.gz"
	touch -d "@$fmoddate" "out/xml/$fbase.xml"

	# cd back to original directory, for good measure
	cd "$JOURNAL_ROOT"
done
