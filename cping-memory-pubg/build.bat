@echo off
setlocal EnableDelayedExpansion

::============================================================================
::  CPING Memory Game - Build & Deploy Script
::----------------------------------------------------------------------------
::  What this script does (in order):
::    1. Verifies your environment (NDK_PATH, adb, Android.mk, device, root)
::    2. Reads the module name from jni\Android.mk (LOCAL_MODULE)
::    3. Kills any running instance of the module on the device
::    4. Builds the native binary with ndk-build
::    5. Pushes the compiled binary to /data/local/tmp on the device
::    6. Sets executable permissions
::    7. Launches Game (com.tencent.ig, com.vng.pubgmobile, com.pubg.krmobile, com.rekoo.pubgm, com.pubg.imobile)
::    8. Runs the binary as root
::
::  Requirements:
::    - NDK_PATH env var pointing to your Android NDK install
::    - adb in PATH and a USB-connected device with USB debugging on
::    - Rooted device (su required)
::============================================================================

:: ---- Pretty separators -------------------------------------------------------
set "SEP=----------------------------------------------------------------------"
set "STEP=0"

:: ---- Resolve script paths ----------------------------------------------------
set "SCRIPT_DIR=%~dp0"
set "ANDROID_MK=%SCRIPT_DIR%jni\Android.mk"
set "OUTPUT_DIR=%SCRIPT_DIR%libs\arm64-v8a"
set "DEVICE_DIR=/data/local/tmp"
set "PUBG_PACKAGE=com.tencent.ig"

echo %SEP%
echo  CPING Memory Game - Build ^& Deploy
echo  Script dir : %SCRIPT_DIR%
echo %SEP%
echo.

::============================================================================
:: STEP 1: Environment checks
::============================================================================
call :step "Checking environment"

:: 1a. NDK_PATH set?
if not defined NDK_PATH (
    call :fail "NDK_PATH is not set." ^
        "Set a system env var NDK_PATH pointing to your NDK folder," ^
        "e.g. C:\Users\you\AppData\Local\Android\Sdk\ndk\30.0.xxxxxx"
)
echo   [OK] NDK_PATH = %NDK_PATH%

:: 1b. ndk-build present in NDK_PATH?
if not exist "%NDK_PATH%\ndk-build.cmd" if not exist "%NDK_PATH%\ndk-build" (
    call :fail "ndk-build not found inside NDK_PATH." ^
        "Make sure NDK_PATH points to the NDK root (where ndk-build lives),"^
        "not to a parent folder."
)
echo   [OK] ndk-build found

:: 1c. adb available?
where adb >nul 2>&1
if errorlevel 1 (
    call :fail "adb is not in PATH." ^
        "Install Android Platform-Tools and add them to PATH," ^
        "or add %%ANDROID_HOME%%\platform-tools to PATH."
)
echo   [OK] adb available

:: 1d. Android.mk exists?
if not exist "%ANDROID_MK%" (
    call :fail "Cannot find Android.mk." ^
        "Expected at: %ANDROID_MK%" ^
        "Run this script from the project root."
)
echo   [OK] Android.mk found

echo.

::============================================================================
:: STEP 2: Read module name from Android.mk
::============================================================================
call :step "Reading module name from Android.mk"

set "MODULE_NAME="
for /f "tokens=2 delims==" %%A in ('findstr /R /C:"^[ 	]*LOCAL_MODULE[ 	]*:=" "%ANDROID_MK%"') do (
    if not defined MODULE_NAME set "MODULE_NAME=%%A"
)

call :trim MODULE_NAME

if not defined MODULE_NAME (
    call :fail "Failed to parse LOCAL_MODULE from Android.mk." ^
        "Expected a line like: LOCAL_MODULE := your_module_name"
)
echo   [OK] Module: [%MODULE_NAME%]
echo.

::============================================================================
:: STEP 3: Device checks
::============================================================================
call :step "Checking device connection"

:: 3a. Any device connected?
set "DEVICE_COUNT=0"
for /f "skip=1 tokens=1,2" %%D in ('adb devices') do (
    if /i "%%E"=="device" set /a DEVICE_COUNT+=1
)
if %DEVICE_COUNT% EQU 0 (
    call :fail "No authorized device detected." ^
        "Connect your device via USB, enable USB debugging," ^
        "and accept the RSA prompt on the device. Run 'adb devices' to verify."
)
if %DEVICE_COUNT% GTR 1 (
    echo   [WARN] %DEVICE_COUNT% devices connected - adb will pick one.
) else (
    echo   [OK] 1 device connected
)

:: 3b. Root available?
for /f "delims=" %%R in ('adb shell su -c "id -u" 2^>nul') do set "ROOT_UID=%%R"
set "ROOT_UID=%ROOT_UID: =%"
if not "%ROOT_UID%"=="0" (
    call :fail "Root (su) not available on device." ^
        "This script needs a rooted device. Got uid='%ROOT_UID%' (expected 0)." ^
        "Grant root to shell via your root manager (Magisk/KernelSU/etc)."
)
echo   [OK] Root access confirmed
echo.

::============================================================================
:: STEP 4: Kill running instance
::============================================================================
call :step "Killing existing %MODULE_NAME% process (if any)"
adb shell su -c "pkill -9 %MODULE_NAME% 2>/dev/null; exit 0" >nul 2>&1
echo   [OK] Done
echo.

::============================================================================
:: STEP 5: Build with NDK
::============================================================================
call :step "Building %MODULE_NAME% with ndk-build"
echo   NDK    : %NDK_PATH%
echo   Source : %SCRIPT_DIR%jni
echo.

pushd "%SCRIPT_DIR%" >nul
call "%NDK_PATH%\ndk-build"
set "BUILD_RC=%ERRORLEVEL%"
popd >nul

if %BUILD_RC% NEQ 0 (
    call :fail "ndk-build returned exit code %BUILD_RC%." ^
        "Scroll up to see the compiler error and fix it."
)

set "BIN_PATH=%OUTPUT_DIR%\%MODULE_NAME%"
if not exist "%BIN_PATH%" (
    call :fail "Build reported success but the binary is missing." ^
        "Expected: %BIN_PATH%" ^
        "Check Android.mk - is BUILD_EXECUTABLE set and TARGET_ARCH_ABI=arm64-v8a?"
)
echo   [OK] Built: %BIN_PATH%
echo.

::============================================================================
:: STEP 6: Push binary to device
::============================================================================
call :step "Pushing binary to %DEVICE_DIR%"
adb push "%BIN_PATH%" "%DEVICE_DIR%/"
if %ERRORLEVEL% NEQ 0 (
    call :fail "adb push failed." ^
        "Check that %DEVICE_DIR% is writable and the device is still connected."
)
echo   [OK] Pushed
echo.

::============================================================================
:: STEP 7: Set executable permissions
::============================================================================
call :step "Setting permissions (chmod 755)"
adb shell su -c "chmod 755 %DEVICE_DIR%/%MODULE_NAME%"
if %ERRORLEVEL% NEQ 0 (
    call :fail "Failed to chmod the binary on device."
)
echo   [OK] Done
echo.

::============================================================================
:: STEP 8: Launch PUBG Mobile
::============================================================================
call :step "Launching PUBG Mobile (%PUBG_PACKAGE%)"

adb shell pm path %PUBG_PACKAGE% >nul 2>&1
if errorlevel 1 (
    echo   [WARN] %PUBG_PACKAGE% is not installed - skipping launch.
) else (
    adb shell monkey -p %PUBG_PACKAGE% -c android.intent.category.LAUNCHER 1 >nul
    if errorlevel 1 (
        echo   [WARN] Failed to launch %PUBG_PACKAGE% - continuing anyway.
    ) else (
        echo   [OK] Launched
    )
)
echo.

::============================================================================
:: STEP 9: Run the binary
::============================================================================
call :step "Running %MODULE_NAME% on device"
echo %SEP%
adb shell su -c "%DEVICE_DIR%/%MODULE_NAME%"
echo %SEP%
echo.

echo   [DONE] All steps completed.
echo.
pause
endlocal
exit /b 0


::============================================================================
:: Helper: print a step header.    Usage:  call :step "Description"
::============================================================================
:step
set /a STEP+=1
echo [STEP !STEP!] %~1
goto :eof

::============================================================================
:: Helper: trim leading/trailing spaces, tabs and CR from a variable.
::   Usage:  call :trim VAR_NAME
::============================================================================
:trim
setlocal EnableDelayedExpansion
set "s=!%~1!"
:_trim_lead
if not defined s goto :_trim_done
if "!s:~0,1!"==" "  set "s=!s:~1!" & goto :_trim_lead
if "!s:~0,1!"=="	" set "s=!s:~1!" & goto :_trim_lead
:_trim_tail
if not defined s goto :_trim_done
if "!s:~-1!"==" "  set "s=!s:~0,-1!" & goto :_trim_tail
if "!s:~-1!"=="	" set "s=!s:~0,-1!" & goto :_trim_tail
rem strip a trailing carriage return that may sneak in from CRLF parsing
for /f "delims=" %%C in ("!s!") do set "s=%%C"
:_trim_done
endlocal & set "%~1=%s%"
goto :eof

::============================================================================
:: Helper: print a fatal error and exit.
::   Usage: call :fail "Main message" "Hint line 1" "Hint line 2" ...
::============================================================================
:fail
echo.
echo %SEP%
echo  ERROR: %~1
shift
:fail_loop
if "%~1"=="" goto :fail_done
echo         %~1
shift
goto :fail_loop
:fail_done
echo %SEP%
echo.
pause
endlocal
exit /b 1
