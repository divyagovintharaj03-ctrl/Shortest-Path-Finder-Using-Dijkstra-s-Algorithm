@echo off
echo =========================================================
echo   Starting Shortest Path Finder
echo =========================================================

REM Detect GCC or Clang if binary needs building
if not exist pathfinder.exe (
    echo pathfinder.exe not found. Building now...
    call build.bat
)

echo Starting C HTTP Engine on http://localhost:8080 ...
echo Open your web browser to: http://localhost:8080
echo Press Ctrl+C to stop.
echo.
pathfinder.exe --serve 8080
