@cd %~dp0
@echo off

@set CAB_PATH=..\driver-cab\fortfw*.cab

@call sign-env-certum.bat

signtool.exe sign /n "%CRT_NAME%" /fd sha256 /tr %TS_URL% %CAB_PATH%
