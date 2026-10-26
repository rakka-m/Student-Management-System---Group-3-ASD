@echo off
setlocal
cd /d "%~dp0"

if not exist student_management.exe (
    call build.bat
    if errorlevel 1 exit /b 1
)

student_management.exe
