#!/bin/bash

# Check if the file is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <csv_file>"
    exit 1
fi

# CSV file path
CSV_FILE="$1"

# Check if the file exists
if [ ! -f "$CSV_FILE" ]; then
    echo "Error: File '$CSV_FILE' not found!"
    exit 1
fi

# Read the CSV file line by line
while IFS=, read -r token vote; do
    # Skip the header line if present
    if [ "$token" == "token" ]; then
        continue
    fi

    # Trim spaces if there are any
    token=$(echo "$token" | xargs)
    vote=$(echo "$vote" | xargs)

    # Make sure both token and vote are not empty
    if [ -n "$token" ] && [ -n "$vote" ]; then
        # Print or process the token and vote as arguments
        # Example: Call your program with the arguments -vote and -token
        echo "Processing token: $token with vote: $vote"
        ./your_program -vote "$vote" -token "$token"
    else
        echo "Warning: Skipping invalid line: $token,$vote"
    fi
done < "$CSV_FILE"

echo "CSV processing completed."
