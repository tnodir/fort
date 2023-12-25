@rem Execute the command

@cd %~dp0
@echo off

set "SystemPath=%SystemRoot%\System32"
if defined PROGRAMFILES(X86) (
    if exist %SystemRoot%\Sysnative\* set "SystemPath=%SystemRoot%\Sysnative"
)

start "" /MIN /W %SystemPath%\cmd.exe /C %1
