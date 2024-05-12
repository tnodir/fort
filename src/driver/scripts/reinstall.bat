@rem Re-install driver

@cd "%~dp0"
@echo off

%ComSpec% /C "%~dp0uninstall.bat"

"%~dp0install.bat" %*
