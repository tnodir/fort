@rem Make the loader of a built driver: make-loader.bat <PLAT> <ARCH>
@rem e.g. make-loader.bat win10 x64 -> build-driver-win10\x64\fortfw.sys
@rem (build-driver-loader-win10\x64\fortfw.sys with the payload of build-driver-win10\x64\fortfw.sys)

@setlocal

@set PLAT=%1
@set ARCH=%2

@call "%~dp0..\config.bat"

@set "DP_PATH=%FORT_ROOT%\build-win10\driver_payload\DriverPayload.exe"

@set "DRV_DIR=%FORT_ROOT%\build-driver-%PLAT%\%ARCH%"
@set "LOADER_DIR=%FORT_ROOT%\build-driver-loader-%PLAT%\%ARCH%"

@set SYS_NAME=fortfw.sys
@set DL_SYS_NAME=fortfwdl.sys

@rem signtool.exe of the EWDK for clear-certs.bat
@call "%EWDK_NEW%\BuildEnv\SetupBuildEnv.cmd" amd64 >nul
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@call "%FORT_ROOT%\deploy\sign\clear-certs.bat" "%DRV_DIR%\%SYS_NAME%"
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@del /q "%DRV_DIR%\%DL_SYS_NAME%" 2>nul

"%DP_PATH%" --input "%LOADER_DIR%\%SYS_NAME%" --output "%DRV_DIR%\%DL_SYS_NAME%" --payload "%DRV_DIR%\%SYS_NAME%" --secret "%FORT_RSA_KEY%"
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@if not exist "%DRV_DIR%\%DL_SYS_NAME%" (
    @echo Error: "%DRV_DIR%\%DL_SYS_NAME%" is not created
    @exit /b 1
)

@rem Clear the driver's build, except the loader
@del /q "%DRV_DIR%\fortfw.*"

ren "%DRV_DIR%\%DL_SYS_NAME%" "%SYS_NAME%"
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%
