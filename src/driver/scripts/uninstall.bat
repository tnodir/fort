@rem Uninstall driver

@set DISPNAME=Fort Firewall
@set CERTNAME=FortFirewallTestCert

@set BASENAME=fortfw
@set DSTPATH=%SystemRoot%\System32\drivers\%BASENAME%.sys


@rem Remove the service
sc stop %BASENAME%
sc delete %BASENAME%
@if ERRORLEVEL 1 (
    @echo Error: Cannot delete the service
    @set RCODE=%ERRORLEVEL%
    @rem @goto EXIT
)


@rem Remove driver from system storage
Del "%DSTPATH%"


@rem Remove driver from system storage
"%~dp0CertMgr" /del /c /n "%CERTNAME%" -s -r localMachine Root
"%~dp0CertMgr" /del /c /n "%CERTNAME%" -s "%DISPNAME%"


@set RCODE=0
@goto EXIT

:EXIT
@echo End execution... Error Code = %RCODE%
@exit /b %RCODE%
