@cd %~dp0
@echo off

@set APP_PATH=..\build\FortFirewall.exe

@call sign-env.bat

signtool.exe sign /ac "%CRT_PATH%" /n "%CRT_NAME%" /fd sha256 /tr http://time.certum.pl/ %APP_PATH%
