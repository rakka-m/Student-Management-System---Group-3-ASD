@echo off
setlocal
cd /d "%~dp0"

where g++ >nul 2>nul
if not errorlevel 1 goto :gcc

where cl >nul 2>nul
if not errorlevel 1 goto :msvc

echo.
echo No C++ compiler was found on PATH.
echo.
echo Install one of the following, then run this file again:
echo   - MinGW-w64 (g++)  https://winlibs.com/
echo   - Visual Studio Build Tools with the "Desktop development with C++" workload
echo.
echo You can also compile manually:
echo   g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 -static student_management.cpp -o student_management.exe
echo.
pause
exit /b 1

:gcc
echo Building with g++...
g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 -static student_management.cpp -o student_management.exe
if errorlevel 1 goto :failed
echo Build successful: student_management.exe
exit /b 0

:msvc
echo Building with MSVC...
cl /std:c++17 /EHsc /W4 /Fe:student_management.exe student_management.cpp
if errorlevel 1 goto :failed
echo Build successful: student_management.exe
exit /b 0

:failed
echo.
echo Build failed. Read the compiler messages above.
pause
exit /b 1
