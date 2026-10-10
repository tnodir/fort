@rem Make the installers of all the targets and their GnuPG signatures: win7 (x86), win10 (x64), win10-arm64

@call "%~dp0build-win7.bat"
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@call "%~dp0build-win10.bat"
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@call "%~dp0build-win10-arm64.bat"
