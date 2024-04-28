@rem Install driver

@set ARCH=x86
@if "%PROCESSOR_ARCHITECTURE%" == "AMD64" @set ARCH=x86_64
@if "%PROCESSOR_ARCHITECTURE%" == "ARM64" @set ARCH=ARM64

@set BASENAME=fortfw
@set FILENAME=%BASENAME%.sys
@set SRCPATH=%~dp0..\%ARCH%\%FILENAME%
@set DSTPATH=%SystemRoot%\System32\drivers\%FILENAME%

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
sc create %DRIVERSVC% binPath= "%DSTPATH%" type= kernel start= auto ^
	group= "NetworkProvider" depend= BFE DisplayName= "%DISPNAME%"
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
@echo End execution... Result Code = %RCODE%
@if %RCODE% neq 0 if "%1" neq "/SILENT" (
    @echo ARGS: %*
    @pause
)
@exit /b %RCODE%
