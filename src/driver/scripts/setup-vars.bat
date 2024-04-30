@rem Setup variables

@set ARCH=x86
@if "%PROCESSOR_ARCHITECTURE%" == "AMD64" @set ARCH=x86_64
@if "%PROCESSOR_ARCHITECTURE%" == "ARM64" @set ARCH=ARM64

@set BASENAME=fortfw
@set FILENAME=%BASENAME%.sys

@set SRCPATH=%~dp0..\%ARCH%\%FILENAME%
@set DSTPATH=%SystemRoot%\System32\drivers\%FILENAME%

@set DRIVERSVC=%BASENAME%
@set DISPNAME=Fort Firewall Driver

@set FORTSVC=FortFirewallSvc
