@rem Check existing driver file's timestamp or reinstall it

@call "%~dp0setup-vars.bat"


@if exist "%DSTPATH%" (
    xcopy /L /D /Y "%SRCPATH%" "%DSTPATH%" | findstr /B /C:"0 " >NUL && goto EXIT
)

"%~dp0reinstall.bat" %*

:EXIT
