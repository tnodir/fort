@cd %~dp0
@echo off

@set TARGET_PATH=build

md %TARGET_PATH%
cd %TARGET_PATH%
del /Q /F qt*.* FortFirewall.exe
rd /Q /S imports plugins i18n
cd ..

powershell.exe -executionpolicy remotesigned -file setup-deployment.ps1 %TARGET_PATH% %*

xcopy /Q /S /Y qt %TARGET_PATH%\
