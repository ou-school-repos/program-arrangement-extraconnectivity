#!/bin/bash -e

set -e

# for debugging
# set -x

PROJECT_ROOT="$(git rev-parse --show-toplevel)"
NESTED_DIR="paper/journal-pads"
JOURNAL_ROOT="$PROJECT_ROOT/$NESTED_DIR"
cd "$JOURNAL_ROOT"

# User supplied directory path (otherwise default to journal-pads root)
if [ -z "$1" ]; then
	QUERY_PATH="$JOURNAL_ROOT"
else
	QUERY_PATH="$(realpath "$1")"
fi

echo "$QUERY_PATH"

mapfile -d '' XML_FILES < <(
	find "$QUERY_PATH" \
		-type f -path '*/out/xml/*.xml' \
		! -path '*/out/xml/out/*' \
		! -path '*/.junk-dupes/*' \
		! -path '*/exam[1-2]/*' \
		-print0
)

if [ "${#XML_FILES[@]}" -eq 0 ]; then
	echo "No archived XML files found under $QUERY_PATH" >&2
	exit 1
fi

# Perform archiving operations
for f in "${XML_FILES[@]}"; do
	echo "$f"
	fmoddate=$(stat -c "%Y" "$f")
	fbase="$(basename "$f" .xml)"
	xml_dir="$(dirname "$f")"
	out_dir="$(dirname "$xml_dir")"
	dest_dir="$(dirname "$out_dir")"

	if [ "$(basename "$xml_dir")" != "xml" ] || [ "$(basename "$out_dir")" != "out" ]; then
		echo "Skipping non-archive XML path: $f" >&2
		continue
	fi

	# cd to file's directory
	cd "$xml_dir"

	# Create .xopp binary beside the source note's out/ directory.
	# .xopp is just gzip-compressed XML.
	dest="$dest_dir/$fbase.xopp"
	tmp="$(mktemp --tmpdir="$dest_dir" ".$fbase.xopp.XXXXXX")"
	gzip -c "$f" >"$tmp"
	touch -d "@$fmoddate" "$tmp"

	if [ -e "$dest" ] && [ "${overwrite:-ask}" != "1" ]; then
		if [ -t 0 ]; then
			read -r -p "overwrite '$dest'? [y/N] " answer
			case "$answer" in
			y | Y | yes | YES)
				mv -f "$tmp" "$dest"
				;;
			*)
				rm -f "$tmp"
				echo "Skipped existing file: $dest" >&2
				;;
			esac
		else
			rm -f "$tmp"
			echo "Destination exists; set overwrite=1 to replace: $dest" >&2
			continue
		fi
	else
		mv -f "$tmp" "$dest"
	fi

	# cd back to original directory, for good measure
	cd "$JOURNAL_ROOT"
done
