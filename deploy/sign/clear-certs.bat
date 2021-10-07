@cd %~dp0
@echo off

@set BIN_PATH=%1

signtool.exe remove /s %BIN_PATH%
