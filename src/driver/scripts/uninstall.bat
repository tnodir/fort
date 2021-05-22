@rem Uninstall driver

@set BASENAME=fortfw
@set DSTPATH=%SystemRoot%\System32\drivers\%BASENAME%.sys

@set DRIVERSVC=%BASENAME%
@set FORTSVC=FortFirewallSvc


@rem Stop and delete the FortFirewall service
@sc stop %FORTSVC%
@sc delete %FORTSVC%


@rem Stop the driver service
sc stop %DRIVERSVC%

@rem Remove the driver service
sc delete %DRIVERSVC%
@if ERRORLEVEL 1 (
    @echo Error: Cannot delete the service
    @set RCODE=%ERRORLEVEL%
    @rem @goto EXIT
)


@rem Remove driver from system storage
Del "%DSTPATH%"


@set RCODE=0
@goto EXIT

:EXIT
@echo End execution... Error Code = %RCODE%
@exit /b %RCODE%
