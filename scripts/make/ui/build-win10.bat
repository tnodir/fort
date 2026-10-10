@rem Build build-win10\ui_bin\FortFirewall.exe (x64; with DriverPayload and tests) with VS 2026 and the static Qt of win10
@rem Usage: build-win10.bat [make args], e.g. "build-win10.bat clean"

@setlocal

@call "%~dp0..\config.bat"

@call "%VCVARS_DIR%\vcvars64.bat" >nul
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@set "BUILD_NAME=win10"
@set "QMAKE=%QT_STATIC_WIN10%\bin\qmake.exe"
@set QMAKE_ARGS=-spec win32-msvc "CONFIG-=qml_debug" "CONFIG+=driver_payload" "CONFIG+=tests"

@call "%~dp0build-ui.bat" %*
