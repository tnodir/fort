@cd %~dp0
@echo off

@set OUT_PATH=..\out\*.exe

@call sign-env-certum.bat

signtool.exe sign /n "%CRT_NAME%" /fd SHA256 /td SHA256 /tr %TS_URL% %OUT_PATH%
