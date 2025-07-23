@echo off

CALL %~dp0driver-clear-certs.bat
CALL %~dp0driver-make-payloads.bat
CALL %~dp0driver-clear-builds.bat
CALL %~dp0driver-rename-payloads.bat

