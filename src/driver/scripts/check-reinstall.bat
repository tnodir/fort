@rem Check existing driver file's timestamp or reinstall it

@call "%~dp0setup-vars.bat"


@rem TODO: COMPAT: Remove after v4.1.0: The driver was started automatically before v3.21.0
@sc config %DRIVERSVC% start= demand >NUL

@if exist "%DSTPATH%" (
    xcopy /L /D /Y "%SRCPATH%" "%DSTPATH%" | findstr /B /C:"0 " >NUL && goto EXIT
)

"%~dp0reinstall.bat" %*

:EXIT
