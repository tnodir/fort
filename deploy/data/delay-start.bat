@rem Delayed start of the Fort Firewall client

@cd "%~dp0"
@echo off

@set INSTALLER_PATH=%1
@shift

@rem Wait for Installer's file deletion
@for /l %%i in (0,1,3) do (
    timeout /t 1 > NUL
    @if not exist %INSTALLER_PATH% goto EXIT
)

:EXIT
start FortFirewall.exe %*
