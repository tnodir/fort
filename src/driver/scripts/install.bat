@rem Install driver

@call "%~dp0setup-vars.bat"


@rem Copy driver to system storage
@if exist "%DSTPATH%" (
    @echo Error: Driver already installed. Uninstall it first: "%DSTPATH%"
    @set RCODE=1
    @goto EXIT
)


@if not exist "%SystemRoot%\System32\robocopy.exe" @goto USE_COPY

robocopy "%SRCDIR%" "%DSTDIR%" "%FILENAME%" /R:0 >NUL
@if ERRORLEVEL 2 (
    @echo Error: Cannot copy driver to system: "%SRCDIR%" "%DSTDIR%" "%FILENAME%"
    @set RCODE=%ERRORLEVEL%
    @goto EXIT
)

@goto END_USE_COPY

:USE_COPY

copy "%SRCPATH%" "%DSTPATH%"
@if ERRORLEVEL 1 (
    @echo Error: Cannot copy driver to system: "%SRCPATH%" "%DSTPATH%"
    @set RCODE=%ERRORLEVEL%
    @goto EXIT
)

:END_USE_COPY


@rem Create the driver service
sc create %DRIVERSVC% binPath= "%DSTPATH%" type= kernel start= auto ^
	group= "NetworkProvider" depend= BFE DisplayName= "%DISPNAME%"
@if ERRORLEVEL 1 (
    @echo Error: Cannot create a service: %DRIVERSVC% "%DSTPATH%"
    @set RCODE=%ERRORLEVEL%
    @goto EXIT
)

@rem Start the driver service
sc start %DRIVERSVC%
@if ERRORLEVEL 1 (
    @echo Error: Cannot start a service: %DRIVERSVC%
    @set RCODE=%ERRORLEVEL%
    @goto EXIT
)


@set RCODE=0
@goto EXIT

:EXIT
@echo End execution... Result Code = %RCODE%
@if %RCODE% neq 0 if not "%1" == "/SILENT" (
    @pause
)
@exit /b %RCODE%
