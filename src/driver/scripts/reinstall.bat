@rem Re-install driver

@cd %~dp0
@echo off

%COMSPEC% /C uninstall.bat

install.bat
