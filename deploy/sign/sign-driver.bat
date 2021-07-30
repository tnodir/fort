@cd %~dp0
@echo off

@rem ARCH: 64, 32
@set ARCH=%1

@set DRV_PATH=..\build\driver\fortfw%ARCH%.sys

@call sign-env.bat

signtool.exe sign /ac "%CRT_PATH%" /n "%CRT_NAME%" /fd sha1 /t http://time.certum.pl/ %DRV_PATH%
signtool.exe sign /as /ac "%CRT_PATH%" /n "%CRT_NAME%" /fd sha256 /tr http://time.certum.pl/ %DRV_PATH%
