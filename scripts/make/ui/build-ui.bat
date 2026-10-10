@rem Build build-<BUILD_NAME>: re-run qmake and make
@rem The caller calls config.bat, sets up the compiler's environment and BUILD_NAME, QMAKE, QMAKE_ARGS
@rem Usage: build-ui.bat [make args]

@setlocal

@set "BUILD_DIR=%FORT_ROOT%\build-%BUILD_NAME%"

@if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
@cd /d "%BUILD_DIR%"
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

"%QMAKE%" -r -o Makefile "%FORT_ROOT%\src\FortFirewall.pro" %QMAKE_ARGS%
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@rem Parallel build: jom, else nmake with cl's /MP
@if exist "%JOM_PATH%" (
    "%JOM_PATH%" %*
) else (
    set "CL=/MP"
    nmake /nologo %*
)
