@rem Uninstall driver

@set DISPNAME=Fort Firewall

@set BASENAME=fortfw
@set DSTPATH=%SystemRoot%\System32\drivers\%BASENAME%.sys


@rem Close the "Services" list window
taskkill /F /IM mmc.exe

@rem Stop the service
net stop %BASENAME%

@rem Remove the service
sc delete %BASENAME%
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
