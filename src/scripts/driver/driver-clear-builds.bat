@echo off

@set ROOT=%~dp0..\..\..

@set SYS_MASK=fortfw.*

del "%ROOT%\build-driver-win7\Win32\%SYS_MASK%"
del "%ROOT%\build-driver-win7\x64\%SYS_MASK%"
del "%ROOT%\build-driver-win10\ARM64\%SYS_MASK%"
del "%ROOT%\build-driver-win10\x64\%SYS_MASK%"

