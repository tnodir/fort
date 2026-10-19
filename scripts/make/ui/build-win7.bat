@rem Build build-win7\ui_bin\FortFirewall.exe (x86) with the EWDK (Old) and the static Qt of win7
@rem Usage: build-win7.bat [make args], e.g. "build-win7.bat clean"

@setlocal

@call "%~dp0..\config.bat"

@call "%~dp0..\env-win7.bat"
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@set "BUILD_NAME=win7"
@set "QMAKE=%QT_STATIC_WIN7%\bin\qmake.exe"
@set QMAKE_ARGS=-spec win32-msvc "CONFIG-=qml_debug"

@call "%~dp0build-ui.bat" %*
