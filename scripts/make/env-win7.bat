@rem Set up the environment of the win7 (x86) builds with the EWDK (Old)
@rem The caller calls config.bat; it may add its tools to PATH after it

@rem PATH anew: only the system's folders and the build tools
@set "PATH=%SystemRoot%\System32;%SystemRoot%;%SystemRoot%\System32\Wbem;%SystemRoot%\System32\WindowsPowerShell\v1.0"

@call "%EWDK_OLD%\BuildEnv\SetupBuildEnv.cmd" x86 >nul
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@rem The EWDK sets up VS with "-winsdk=none": add the Windows SDK (incl. UCRT) paths
@set "SDK_INC=%WindowsSdkDir%Include\%Version_Number%"
@set "SDK_LIB=%WindowsSdkDir%Lib\%Version_Number%"
@set "VC_DIR=%VCToolsInstallDir%"
@set "INCLUDE=%VC_DIR%include;%SDK_INC%\ucrt;%SDK_INC%\shared;%SDK_INC%\um;%SDK_INC%\winrt"
@set "LIB=%VC_DIR%lib\x86;%SDK_LIB%\ucrt\x86;%SDK_LIB%\um\x86"
@set "PATH=%WindowsSdkVerBinPath%;%PATH%"
