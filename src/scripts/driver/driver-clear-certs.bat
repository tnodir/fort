@echo off

@set ROOT=%~dp0..\..\..

@set CLEAR_CERT_PATH=%ROOT%\deploy\sign\clear-certs.bat

@set SYS_NAME=fortfw.sys

CALL "%CLEAR_CERT_PATH%" "%ROOT%\build-driver-win7\Win32\%SYS_NAME%"
CALL "%CLEAR_CERT_PATH%" "%ROOT%\build-driver-win7\x64\%SYS_NAME%"
CALL "%CLEAR_CERT_PATH%" "%ROOT%\build-driver-win10\ARM64\%SYS_NAME%"
CALL "%CLEAR_CERT_PATH%" "%ROOT%\build-driver-win10\x64\%SYS_NAME%"

