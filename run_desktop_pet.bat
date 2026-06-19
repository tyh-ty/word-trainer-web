@echo off
setlocal
cd /d "%~dp0desktop-pet"
if not exist "build\DesktopPet.exe" (
  cmake -S . -B build
  if errorlevel 1 goto failed
  cmake --build build
  if errorlevel 1 goto failed
)
start "" "%~dp0desktop-pet\build\DesktopPet.exe"
exit /b 0

:failed
echo DesktopPet build failed.
pause
exit /b 1
