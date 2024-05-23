@rem Setup variables

@set ARCH=x86
@if "%PROCESSOR_ARCHITECTURE%" == "AMD64" (
    @set ARCH=x86_64

    @rem Check for Windows 10 2004
    @for /f "tokens=6 delims=.[] " %%i in ('ver') do @(
        @if %%i lss 19041 @set ARCH=x86_64_old
    )
) else (
    @if "%PROCESSOR_ARCHITECTURE%" == "ARM64" @set ARCH=ARM64
)

@set BASENAME=fortfw
@set FILENAME=%BASENAME%.sys

@set SRCDIR=%~dp0..\%ARCH%
@set SRCPATH=%SRCDIR%\%FILENAME%

@set DSTDIR=%SystemRoot%\System32\drivers
@set DSTPATH=%DSTDIR%\%FILENAME%

@set DRIVERSVC=%BASENAME%
@set DISPNAME=Fort Firewall Driver

@set FORTSVC=FortFirewallSvc
