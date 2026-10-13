@echo off
echo =========================================================
echo   Compiling Shortest Path Finder (C Core Backend)
echo =========================================================

REM Detect GCC or Clang
where gcc >nul 2>nul
if %errorlevel% neq 0 (
    echo GCC not in current PATH. Adding local LLVM-MinGW if present...
    set "PATH=%LOCALAPPDATA%\Microsoft\WinGet\Packages\MartinStorsjo.LLVM-MinGW.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\llvm-mingw-20260908-ucrt-x86_64\bin;%PATH%"
)

echo [1/2] Compiling main application (pathfinder.exe)...
gcc -Wall -Wextra -O2 -Ibackend/include backend/src/graph.c backend/src/minheap.c backend/src/dijkstra.c backend/src/storage.c backend/src/server.c backend/src/main.c -lws2_32 -o pathfinder.exe
if %errorlevel% neq 0 (
    echo Error: Failed to compile pathfinder.exe
    exit /b %errorlevel%
)
echo [1/2] pathfinder.exe compiled successfully!

echo [2/2] Compiling automated test suite (test_pathfinder.exe)...
gcc -Wall -Wextra -O2 -Ibackend/include backend/src/graph.c backend/src/minheap.c backend/src/dijkstra.c backend/src/storage.c backend/tests/test_pathfinder.c -o test_pathfinder.exe
if %errorlevel% neq 0 (
    echo Error: Failed to compile test_pathfinder.exe
    exit /b %errorlevel%
)
echo [2/2] test_pathfinder.exe compiled successfully!

echo =========================================================
echo   Build Complete! Running unit tests now...
echo =========================================================
test_pathfinder.exe
