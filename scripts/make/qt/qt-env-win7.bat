@rem Set up the console's environment to build the static Qt of win7 (x86) with the EWDK (Old)
@rem Usage: call it in a console, then run the Qt's configure script there

@call "%~dp0..\config.bat"

@call "%~dp0..\env-win7.bat"
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@set "PATH=%CMAKE_WIN7_HOME%\bin;%PERL_HOME%\bin;%PATH%"
