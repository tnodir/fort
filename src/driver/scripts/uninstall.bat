@rem Uninstall driver

@call "%~dp0setup-vars.bat"


@rem Stop and delete the FortFirewall service
@sc stop %FORTSVC%
@sc delete %FORTSVC%


@rem Stop the driver service
sc stop %DRIVERSVC%

@rem Remove the driver service
sc delete %DRIVERSVC%
@if ERRORLEVEL 1 (
    @echo Error: Cannot delete a service: %DRIVERSVC%
    @set RCODE=%ERRORLEVEL%
    @rem @goto EXIT
)


@rem Remove driver from system storage
@if exist "%DSTPATH%" (
    Del "%DSTPATH%"
)


@set RCODE=0
@goto EXIT

:EXIT
@echo End execution... Result Code = %RCODE%
@exit /b %RCODE%
