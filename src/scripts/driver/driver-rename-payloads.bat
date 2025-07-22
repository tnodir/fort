@echo off

@set ROOT=%~dp0..\..\..

@set SYS_NAME=fortfw.sys
@set DL_SYS_NAME=fortfwdl.sys

ren "%ROOT%\build-driver-win7\Win32\%DL_SYS_NAME%" "%SYS_NAME%"
ren "%ROOT%\build-driver-win7\x64\%DL_SYS_NAME%" "%SYS_NAME%"
ren "%ROOT%\build-driver-win10\ARM64\%DL_SYS_NAME%" "%SYS_NAME%"
ren "%ROOT%\build-driver-win10\x64\%DL_SYS_NAME%" "%SYS_NAME%"

