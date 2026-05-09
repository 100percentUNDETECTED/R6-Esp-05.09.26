@echo off
cd /d "%~dp0"
echo.
echo   ===========================
echo    R6-External-ESP Driver Loader
echo   ===========================
echo.

net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [!] Run as Administrator!
    pause
    exit /b 1
)

:: Check if drivers exist in current folder
if not exist "signeddrv.sys" (
    echo [!] signeddrv.sys not found in %~dp0
    pause
    exit /b 1
)

:: --- Memory Driver (WinNotify) ---
echo [*] Loading WinNotify driver...
sc stop WinNotify >nul 2>&1
sc delete WinNotify >nul 2>&1
sc create WinNotify type=kernel binPath="%~dp0signeddrv.sys"
sc start WinNotify
if %errorlevel% equ 0 (
    echo [+] WinNotify loaded!
) else (
    echo [!] WinNotify failed to start.
)
echo.

echo [+] Done! Run R6-External-ESP.exe now.
echo.
pause
