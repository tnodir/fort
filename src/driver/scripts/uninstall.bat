@rem Uninstall driver

@set CERTNAME=FortFirewallTestCert

@set BASENAME=fortfw
@set DSTPATH=%SystemRoot%\System32\drivers\%BASENAME%.sys


@rem Remove the service
sc stop %BASENAME%
sc delete %BASENAME%
@if ERRORLEVEL 1 (
    @echo Error: Cannot delete the service
    @set RCODE=%ERRORLEVEL%
    @goto EXIT
)


@rem Remove driver from system storage
Del "%DSTPATH%"


@rem Remove driver from system storage
CertMgr /del /c /n "%CERTNAME%" -s -r localMachine Root


@set RCODE=0
@goto EXIT

:EXIT
@echo End execution... Error Code = %RCODE%
@exit /b %RCODE%
