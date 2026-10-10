@rem Build build-win7\ui_bin\FortFirewall.exe (x86) with the EWDK (New: VS 2022) and the static Qt of win7
@rem Usage: build-win7.bat [make args], e.g. "build-win7.bat clean"

@setlocal

@call "%~dp0..\config.bat"

@call "%EWDK_NEW%\BuildEnv\SetupBuildEnv.cmd" x86 >nul
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@rem The EWDK sets up VS with "-winsdk=none": add the Windows SDK (incl. UCRT) paths
@set "SDK_INC=%WindowsSdkDir%Include\%Version_Number%"
@set "SDK_LIB=%WindowsSdkDir%Lib\%Version_Number%"
@set "VC_DIR=%VCToolsInstallDir%"
@set "INCLUDE=%VC_DIR%include;%SDK_INC%\ucrt;%SDK_INC%\shared;%SDK_INC%\um;%SDK_INC%\winrt"
@set "LIB=%VC_DIR%lib\x86;%SDK_LIB%\ucrt\x86;%SDK_LIB%\um\x86"
@set "PATH=%WindowsSdkVerBinPath%;%PATH%"

@set "BUILD_NAME=win7"
@set "QMAKE=%QT_STATIC_WIN7%\bin\qmake.exe"
@set QMAKE_ARGS=-spec win32-msvc "CONFIG-=qml_debug"

@call "%~dp0build-ui.bat" %*
