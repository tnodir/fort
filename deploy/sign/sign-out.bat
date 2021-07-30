@cd %~dp0
@echo off

@set OUT_PATH=..\out\*.exe

@call sign-env.bat

signtool.exe sign /ac "%CRT_PATH%" /n "%CRT_NAME%" /fd sha256 /tr http://time.certum.pl/ %OUT_PATH%
