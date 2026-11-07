@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
set "MAP_FILE=%PROJECT_DIR%\build\pico2_2.elf.map"
set "HTML_FILE=%PROJECT_DIR%\build\firmware_size.html"

echo ========================================================
echo   Generating Pico 2 Memory Footprint Visual Report...
echo ========================================================

python "%SCRIPT_DIR%pico_size_report.py" --map "%MAP_FILE%" --html "%HTML_FILE%"

if exist "%HTML_FILE%" (
    echo.
    echo Opening visual report in web browser...
    start "" "%HTML_FILE%"
) else (
    echo [ERROR] Failed to generate "%HTML_FILE%"
)

echo.
pause
