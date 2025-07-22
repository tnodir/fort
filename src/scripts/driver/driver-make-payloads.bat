@echo off

@set ROOT=%~dp0..\..\..

@set DP_PATH=%ROOT%\build-win10\driver_payload\DriverPayload.exe

@set KEY_PATH=%ROOT%\build-tmp\fort.rsa

@set SYS_NAME=fortfw.sys
@set DL_SYS_NAME=fortfwdl.sys

"%DP_PATH%" --input "%ROOT%\build-driver-loader-win7\Win32\%SYS_NAME%" --output "%ROOT%\build-driver-win7\Win32\%DL_SYS_NAME%" --payload "%ROOT%\build-driver-win7\Win32\%SYS_NAME%" --secret "%KEY_PATH%"
"%DP_PATH%" --input "%ROOT%\build-driver-loader-win7\x64\%SYS_NAME%" --output "%ROOT%\build-driver-win7\x64\%DL_SYS_NAME%" --payload "%ROOT%\build-driver-win7\x64\%SYS_NAME%" --secret "%KEY_PATH%"

"%DP_PATH%" --input "%ROOT%\build-driver-loader-win10\ARM64\%SYS_NAME%" --output "%ROOT%\build-driver-win10\ARM64\%DL_SYS_NAME%" --payload "%ROOT%\build-driver-win10\ARM64\%SYS_NAME%" --secret "%KEY_PATH%"
"%DP_PATH%" --input "%ROOT%\build-driver-loader-win10\x64\%SYS_NAME%" --output "%ROOT%\build-driver-win10\x64\%DL_SYS_NAME%" --payload "%ROOT%\build-driver-win10\x64\%SYS_NAME%" --secret "%KEY_PATH%"

