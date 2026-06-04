@echo off
setlocal

echo ====================================
echo   BUILD WINDOWS APP - BOTH VERSIONS
echo ====================================
echo.

cd /d "%~dp0"

REM Check for cl.exe (Visual Studio)
where cl.exe >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] cl.exe not found!
    echo [INFO] Please run this from "Developer Command Prompt for VS"
    echo.
    echo Or open "x64 Native Tools Command Prompt for VS 2022"
    echo from Start Menu
    pause
    exit /b 1
)

echo [INFO] Building ORIGINAL version (Keyboard Hook)...
cl.exe send_q_cpp.cpp ^
  /link setupapi.lib hid.lib user32.lib ^
  /out:send_q_hook.exe

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed for Hook version!
    pause
    exit /b 1
)
echo [OK] send_q_hook.exe created
echo.

echo [INFO] Building OPTIMIZED version (Raw Input)...
cl.exe send_q_rawinput.cpp ^
  /link setupapi.lib hid.lib user32.lib ^
  /out:send_q_rawinput.exe

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed for Raw Input version!
    pause
    exit /b 1
)
echo [OK] send_q_rawinput.exe created
echo.

echo ====================================
echo   BUILD SUCCESS!
echo ====================================
echo.
echo Output files:
echo   - send_q_hook.exe      (Original, ~1-2ms latency)
echo   - send_q_rawinput.exe  (Optimized, ~0.2-0.5ms latency)
echo.
echo Recommendation: Use send_q_rawinput.exe for lowest latency
echo.

REM Clean up intermediate files
del *.obj >nul 2>&1

pause
