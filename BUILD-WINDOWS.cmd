@echo off
setlocal
cd /d "%~dp0"
where cmake >nul 2>&1
if errorlevel 1 goto missing
where git >nul 2>&1
if errorlevel 1 goto missing
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 goto failed
cmake --build build --config Release --target ReelGlue_VST3 ReelGlueTests --parallel 3
if errorlevel 1 goto failed
ctest --test-dir build -C Release --output-on-failure
if errorlevel 1 goto failed
set "ATR102_PROJECT_ROOT=%CD%"
if not exist out mkdir out
powershell -NoProfile -Command "$root = $env:ATR102_PROJECT_ROOT; Compress-Archive -Path (Join-Path $root 'build\ReelGlue_artefacts\Release\VST3\ATR-102.vst3') -DestinationPath (Join-Path $root 'out\ATR-102-Windows-x64-VST3.zip') -Force"
if errorlevel 1 goto failed
echo.
echo Build complete. Windows VST3 ZIP:
echo %CD%\out\ATR-102-Windows-x64-VST3.zip
echo Copy the complete ATR-102.vst3 directory into C:\Program Files\Common Files\VST3
echo This is a development build. See docs\DSP_NOTES.md for remaining work.
pause
exit /b 0
:missing
echo Install Git, CMake 3.22+ and Visual Studio 2022 with Desktop development with C++.
echo Include the MSVC v143 x64 toolchain and Windows SDK.
goto failed
:failed
echo Build failed. Read the error above; no successful Windows build is being claimed.
pause
exit /b 1
