@rem Re-install driver

@cd "%~dp0"
@echo off

%COMSPEC% /C "%~dp0uninstall.bat"

"%~dp0install.bat"
