@cd %~dp0
@echo off

@set TARGET_PATH=build

rd /Q /S %TARGET_PATH%
md %TARGET_PATH%

powershell.exe -executionpolicy remotesigned -file setup-deployment.ps1 %TARGET_PATH% %*
