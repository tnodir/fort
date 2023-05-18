@setlocal

@cd %~dp0
@echo off

@set CHECK_WIN10=
@if not exist ".\build\driver\x86\" (
  @set CHECK_WIN10=Y
)

@set INNO_PATH=D:\Utils\Dev\InnoSetup5\ISCC.exe

"%INNO_PATH%" FortFirewall.iss /DCHECK_WIN10=%CHECK_WIN10%
