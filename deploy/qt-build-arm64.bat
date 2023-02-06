@rem Open "Visual Studio .NET Command Prompt for ARM64" to run this script

@setlocal

@cd %~dp0
@echo off

@call ./qt-build.bat ^
	-platform win32-arm64-msvc TARGET_CONFIGURE_ARGS=-release ^
	-qt-host-path "%~dp0build-qt\static"
