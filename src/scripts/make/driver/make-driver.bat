@rem Build the driver with the EWDK: make-driver.bat <EWDK: Old|New> <NAME> <OUT>
@rem e.g. make-driver.bat New win10-64 win10\x64
@rem -> src\driver\msvcbuild-win10-64.bat -> build-driver-win10\x64\fortfw.sys
@rem msvcbuild.bat's exit code is of its rd/del after MSBuild, not of MSBuild,
@rem so the result is checked by the created fortfw.sys.

@setlocal

@set EWDK=%1
@set NAME=%2
@set OUT=%3

@call "%~dp0..\config.bat"

@set "EWDK_DIR=%EWDK_NEW%"
@if /i "%EWDK%"=="Old" set "EWDK_DIR=%EWDK_OLD%"

@set "SYS_PATH=%FORT_ROOT%\build-driver-%OUT%\fortfw.sys"

@call "%EWDK_DIR%\BuildEnv\SetupBuildEnv.cmd" amd64 >nul
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@del /q "%SYS_PATH%" 2>nul

@call "%FORT_ROOT%\src\driver\msvcbuild-%NAME%.bat"

@if not exist "%SYS_PATH%" (
    @echo Error: "%SYS_PATH%" is not built
    @exit /b 1
)

@exit /b 0
