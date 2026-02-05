@echo off
echo Self-Leveling Platform - Test Mode GUI
echo ========================================
echo.

REM Check if Python is available
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python not found in PATH
    echo Please install Python from https://python.org
    pause
    exit /b 1
)

REM Check if pyserial is installed
python -c "import serial" >nul 2>&1
if errorlevel 1 (
    echo Installing pyserial...
    pip install pyserial
    echo.
)

REM Run the GUI
echo Starting GUI...
python "%~dp0test_mode_gui.py"

if errorlevel 1 (
    echo.
    echo GUI exited with error. Press any key to close.
    pause >nul
)
