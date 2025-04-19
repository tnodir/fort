@cd %~dp0
@echo off

@set APP_PATH=..\build\FortFirewall.exe

@call sign-env-certum.bat

signtool.exe sign /n "%CRT_NAME%" /fd SHA256 /td SHA256 /tr %TS_URL% %APP_PATH%
