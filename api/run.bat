@echo off
REM Log Processor API - Windows Startup Script

echo.
echo ========================================
echo  Log Processor FastAPI Server
echo ========================================
echo.

REM Check if Python is installed
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python not found. Please install Python 3.8+
    pause
    exit /b 1
)

REM Install dependencies
echo [1/2] Installing dependencies...
python -m pip install -q -r requirements.txt
if errorlevel 1 (
    echo ERROR: Failed to install dependencies
    pause
    exit /b 1
)

echo [2/2] Starting FastAPI server...
echo.
echo Server will be available at: http://127.0.0.1:8000
echo API documentation at: http://127.0.0.1:8000/docs
echo.
echo Press Ctrl+C to stop the server.
echo.

python -m uvicorn app:app --reload --port 8000 --host 127.0.0.1

pause
