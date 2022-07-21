@cd %~dp0
@echo off

@rem ARCH: x86, x86_64
@set ARCH=%1

@set DRV_PATH=..\build\driver\%ARCH%\fortfw.sys

@call sign-env-sectigo.bat

signtool.exe sign /ac "%CRT_PATH%" /n "%CRT_NAME%" /fd sha256 /tr %TS_URL% %DRV_PATH%
