@rem Install driver

@set ARCH=32
@if defined PROGRAMFILES(X86) @set ARCH=64

@set BASENAME=fortfw
@set FILENAME=%BASENAME%%ARCH%.sys
@set SRCPATH=%~dp0..\%FILENAME%
@set DSTPATH=%SystemRoot%\System32\drivers\%BASENAME%.sys

@set DRIVERSVC=%BASENAME%
@set DISPNAME=Fort Firewall Driver


@rem Copy driver to system storage
@if exist "%DSTPATH%" (
    @echo Error: Driver already installed. Uninstall it first
    @set RCODE=1
    @goto EXIT
)

copy "%SRCPATH%" "%DSTPATH%"
@if ERRORLEVEL 1 (
    @echo Error: Cannot copy driver to system
    @set RCODE=%ERRORLEVEL%
    @goto EXIT
)


@rem Create the driver service
sc create %DRIVERSVC% binPath= "%DSTPATH%" type= kernel start= auto depend= BFE DisplayName= "%DISPNAME%"
@if ERRORLEVEL 1 (
    @echo Error: Cannot create a service
    @set RCODE=%ERRORLEVEL%
    @goto EXIT
)

@rem Start the driver service
sc start %DRIVERSVC%
@if ERRORLEVEL 1 (
    @echo Error: Cannot start the service
    @set RCODE=%ERRORLEVEL%
    @goto EXIT
)


@set RCODE=0
@goto EXIT

:EXIT
@echo End execution... Error Code = %RCODE%
@if %RCODE% neq 0 (
    @pause
)
@exit /b %RCODE%
