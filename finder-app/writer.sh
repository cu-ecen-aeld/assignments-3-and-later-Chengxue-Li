# if args is less than 2, return 1
if [ "$#" -lt 2 ]; then
    exit 1
fi

# Create the directory for $1 if it doesn't exist
mkdir -p "$(dirname "$1")"

# Write the string $2 to the file $1
if ! echo "$2" > "$1"; then
    exit 1
fi
