@rem Make the installer of a target and its GnuPG signature: make-installer.bat <CONFIG> <FILE_SUFFIX>
@rem e.g. make-installer.bat win10 -windows10-x86_64
@rem -> deploy\out\FortFirewall-<version>-windows10-x86_64.exe and its .exe.sig

@setlocal

@set CONFIG=%1
@set FILE_SUFFIX=%2

@call "%~dp0..\config.bat"

@set "DEPLOY_DIR=%FORT_ROOT%\deploy"
@set "OUT_DIR=%DEPLOY_DIR%\out"

@call "%DEPLOY_DIR%\setup-deployment.bat" -Config %CONFIG%
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@call "%DEPLOY_DIR%\create-installer.bat"
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@rem The created installer is the latest one of the suffix
@set INSTALLER_PATH=
@for /f "delims=" %%F in ('dir /b /o-d "%OUT_DIR%\FortFirewall-*%FILE_SUFFIX%.exe"') do @(
    @if not defined INSTALLER_PATH @set "INSTALLER_PATH=%OUT_DIR%\%%F"
)

@if not defined INSTALLER_PATH (
    @echo Error: "%OUT_DIR%\FortFirewall-*%FILE_SUFFIX%.exe" is not created
    @exit /b 1
)

"%GPG_PATH%" --batch --yes --pinentry-mode loopback --passphrase-file "%GPG_PHRASE%" --local-user %GPG_KEY% --digest-algo SHA512 --detach-sign "%INSTALLER_PATH%"
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

@if not exist "%INSTALLER_PATH%.sig" (
    @echo Error: "%INSTALLER_PATH%.sig" is not created
    @exit /b 1
)
