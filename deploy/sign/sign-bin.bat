@cd %~dp0
@echo off

@set BIN_PATH=.\*.bin

@call sign-env-sectigo.bat

signtool.exe sign /n "%CRT_NAME%" /fd SHA256 /td SHA256 /tr %TS_URL% %BIN_PATH%
