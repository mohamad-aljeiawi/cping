@echo off

:: Kill existing process
echo Killing existing process...
adb shell su -c "kill -9 $(pidof cping_memory_delta_force)"

:: Build using ndk-build
echo Building with NDK...
call "C:/Users/mobil/AppData/Local/Android/Sdk/ndk/29.0.13113456/ndk-build"
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b 1
)

:: Push executable
echo Pushing cping_memory_delta_force to device...
adb push "C:/Users/mobil/Desktop/project/android/cping/cping-memory-delta-force/libs/arm64-v8a/cping_memory_delta_force" "/data/local/tmp/"
if %ERRORLEVEL% NEQ 0 (
    echo Push failed!
    pause
    exit /b 1
)

:: Set permissions
echo Setting permissions...
@REM adb shell su -c "mv /data/local/tmp/cping_memory /data/data/com.cping.jo/"
adb shell su -c "chmod 755 /data/local/tmp/cping_memory_delta_force"

:: Start PUBG Mobile
echo Starting PUBG Mobile...
adb shell monkey -p com.garena.game.df -c android.intent.category.LAUNCHER 1
@REM timeout /t 5 > nul

:: Run cping_memory_delta_force
echo Starting cping_memory_delta_force...
adb shell su -c "/data/local/tmp/cping_memory_delta_force"

echo.
echo Press any key to exit...
pause
