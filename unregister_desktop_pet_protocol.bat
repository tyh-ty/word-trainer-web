@echo off
reg delete "HKCU\Software\Classes\wordtrainerpet" /f >nul 2>nul
echo Unregistered wordtrainerpet://.
