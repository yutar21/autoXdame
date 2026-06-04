@echo off
setlocal

echo ====================================
echo   PICO MACRO - AUTO BUILD SCRIPT
echo ====================================
echo.

REM Set Pico SDK path
set "PICO_SDK_PATH=C:\Program Files\Raspberry Pi\Pico SDK v1.5.1\pico-sdk"

REM Check if SDK exists
if not exist "%PICO_SDK_PATH%" (
    echo [ERROR] Pico SDK not found at: %PICO_SDK_PATH%
    echo Please update PICO_SDK_PATH in this script
    pause
    exit /b 1
)

echo [INFO] Using Pico SDK: %PICO_SDK_PATH%
echo.

REM Navigate to project directory
cd /d "%~dp0"

REM Clean old build
if exist "build" (
    echo [INFO] Cleaning old build directory...
    rmdir /s /q build
)

REM Create new build directory
echo [INFO] Creating build directory...
mkdir build
cd build

REM Detect build tool
where ninja >nul 2>&1
if %ERRORLEVEL% equ 0 (
    set "BUILD_TOOL=Ninja"
    set "BUILD_CMD=ninja"
) else (
    where nmake >nul 2>&1
    if %ERRORLEVEL% equ 0 (
        set "BUILD_TOOL=NMake Makefiles"
        set "BUILD_CMD=nmake"
    ) else (
        echo [ERROR] Neither Ninja nor NMake found!
        echo Please run this script from:
        echo   - Developer Command Prompt for Pico (has Ninja^)
        echo   - Developer Command Prompt for VS (has NMake^)
        pause
        exit /b 1
    )
)

echo [INFO] Using build tool: %BUILD_TOOL%
echo.

REM Run CMake
echo [INFO] Running CMake...
cmake -G "%BUILD_TOOL%" "%~dp0"
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo [INFO] Building firmware...
%BUILD_CMD%
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo ====================================
echo   BUILD SUCCESS!
echo ====================================
echo.
echo Output files:
dir /b pico_macro.*
echo.
echo File to flash: pico_macro.uf2
echo Location: %~dp0build\pico_macro.uf2
echo.

pause
