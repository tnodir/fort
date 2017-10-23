@rem Install driver

@set DISPNAME=Fort Firewall
@set CERTNAME=FortFirewallTestCert
@set CERTPATH=%~dp0%CERTNAME%.cer

@set ARCH=32
@if defined PROGRAMFILES(X86) @set ARCH=64

@set BASENAME=fortfw
@set FILENAME=%BASENAME%%ARCH%.sys
@set SRCPATH=%~dp0..\%FILENAME%
@set DSTPATH=%SystemRoot%\System32\drivers\%BASENAME%.sys


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


@rem Create testing certificate
@if exist "%CERTPATH%" (
    @goto CERT_ADDED
)

MakeCert -r -pe -ss "%DISPNAME%" -n "CN=%CERTNAME%" "%CERTPATH%"
@if ERRORLEVEL 1 (
    @echo Error: Cannot create certificate
    @set RCODE=%ERRORLEVEL%
    @goto EXIT
)

CertMgr /add "%CERTPATH%" /s /r localMachine root
@if ERRORLEVEL 1 (
    @echo Error: Cannot add certificate to store
    @set RCODE=%ERRORLEVEL%
    @goto EXIT
)

Del "%CERTPATH%"

:CERT_ADDED


@rem Sign the driver
SignTool sign /a /v /s "%DISPNAME%" /n "%CERTNAME%" "%DSTPATH%"
@if ERRORLEVEL 1 (
    @echo Error: Cannot sign the driver
    @set RCODE=%ERRORLEVEL%
    @goto EXIT
)


@rem Create service
sc create %BASENAME% binPath= "%DSTPATH%" type= kernel start= auto depend= BFE DisplayName= "%DISPNAME%"
@if ERRORLEVEL 1 (
    @echo Error: Cannot create a service
    @set RCODE=%ERRORLEVEL%
    @goto EXIT
)

sc start %BASENAME%
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
