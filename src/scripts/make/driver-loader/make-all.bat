@rem Make the loaders of all the drivers: win7 (x86, x64), win10 (x64, arm64)

@call "%~dp0build-win7-32.bat"
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@call "%~dp0build-win7-64.bat"
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@call "%~dp0build-win10-64.bat"
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@call "%~dp0build-win10-arm64.bat"
