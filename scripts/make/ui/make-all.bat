@rem Build all the UI trees: win7 (x86), win10 (x64), win10-arm64
@rem Usage: make-all.bat [make args], e.g. "make-all.bat clean"

@call "%~dp0build-win7.bat" %*
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@call "%~dp0build-win10.bat" %*
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@call "%~dp0build-win10-arm64.bat" %*
