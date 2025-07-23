@cd %~dp0
@echo off

@set BIN_PATH=%1

signtool.exe remove /s /v %BIN_PATH%
