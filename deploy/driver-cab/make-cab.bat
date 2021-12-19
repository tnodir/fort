@cd %~dp0
@echo off

@set PLAT=%1
@set CONFIG=%2

@set DRV_PATH=%~dp0..\..\build-driver-loader-%CONFIG%\%PLAT%\fortfw.sys

@call ../sign/clear-certs.bat %DRV_PATH%

@cd %~dp0
makecab.exe /f fortfw.ddf /D PLAT=%PLAT% /D CONFIG=%CONFIG%

@del /Q setup.*
