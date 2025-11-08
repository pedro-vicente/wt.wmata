#!/bin/bash
# Download WMATA Rail GTFS static data and extract shapes
# Usage: ./get.sh YOUR_API_KEY
set -e 

if [ $# -eq 0 ]; then
    echo "Usage: $0 YOUR_WMATA_API_KEY"
    exit 1
fi

API_KEY="$1"
GTFS_URL="https://api.wmata.com/gtfs/rail-gtfs-static.zip"
OUTPUT_DIR="gtfs"
ZIP_FILE="gtfs.zip"

mkdir -p "$OUTPUT_DIR"
curl -# -L -H "api_key: ${API_KEY}" -o "${OUTPUT_DIR}/${ZIP_FILE}" "${GTFS_URL}"
unzip -q -o "${OUTPUT_DIR}/${ZIP_FILE}" -d "${OUTPUT_DIR}"
