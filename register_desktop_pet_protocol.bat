@echo off
setlocal
set "ROOT=%~dp0"
set "TARGET=%ROOT%run_desktop_pet.bat"
set "ICON=%ROOT%desktop-pet\build\DesktopPet.exe"

reg add "HKCU\Software\Classes\wordtrainerpet" /ve /d "URL:Word Trainer Desktop Pet" /f >nul
if errorlevel 1 goto failed
reg add "HKCU\Software\Classes\wordtrainerpet" /v "URL Protocol" /d "" /f >nul
if errorlevel 1 goto failed
reg add "HKCU\Software\Classes\wordtrainerpet\DefaultIcon" /ve /d "\"%ICON%\"" /f >nul
if errorlevel 1 goto failed
reg add "HKCU\Software\Classes\wordtrainerpet\shell\open\command" /ve /d "\"%TARGET%\" \"%%1\"" /f >nul
if errorlevel 1 goto failed

echo Registered wordtrainerpet:// for this project.
echo Open web\index.html or the GitHub Pages site to launch the C++ desktop pet from the web page.
exit /b 0

:failed
echo Failed to register wordtrainerpet://.
exit /b 1
