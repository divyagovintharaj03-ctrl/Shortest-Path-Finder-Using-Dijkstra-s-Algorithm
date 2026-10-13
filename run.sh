#!/usr/bin/env bash
set -e

echo "========================================================="
echo "  Starting Shortest Path Finder (Linux / macOS)"
echo "========================================================="

if [ ! -f "pathfinder" ]; then
    echo "pathfinder binary not found. Building now..."
    make all
fi

echo "Starting C HTTP Engine on http://localhost:8080 ..."
echo "Open your web browser to: http://localhost:8080"
echo "Press Ctrl+C to stop."
echo ""
./pathfinder --serve 8080
