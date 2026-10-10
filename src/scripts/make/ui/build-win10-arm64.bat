@rem Build build-win10-arm64\ui_bin\FortFirewall.exe (ARM64) with VS 2026 (x64 host) and the static Qt of arm64
@rem (its host tools are of the win10 one)
@rem Usage: build-win10-arm64.bat [make args], e.g. "build-win10-arm64.bat clean"

@setlocal

@call "%~dp0..\config.bat"

@call "%VCVARS_DIR%\vcvarsamd64_arm64.bat" >nul
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@set "BUILD_NAME=win10-arm64"
@set "QMAKE=%QT_STATIC_WIN10%\bin\qmake6.exe"
@set QMAKE_ARGS=-qtconf "%QT_STATIC_ARM64%\bin\target_qt.conf" -spec win32-arm64-msvc "CONFIG-=qml_debug"

@call "%~dp0build-ui.bat" %*
