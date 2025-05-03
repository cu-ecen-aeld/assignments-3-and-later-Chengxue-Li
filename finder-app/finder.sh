#!/bin/sh
# if args is less than 2, return 1
if [ "$#" -lt 2 ]; then
    exit 1
fi

# if the 1st argument is not a directory, return 1
if [ ! -d "$1" ]; then
    exit 1
fi

# iterate through the files in the directory. count all lines that contain the string
file_count=0
line_count=0

for file in "$1"/*; do
    if [ -f "$file" ]; then
        matches=$(grep -c "$2" "$file")
        line_count=$((line_count + $matches))
        file_count=$((file_count + 1))
    fi
done

echo "The number of files are $file_count and the number of matching lines are $line_count"